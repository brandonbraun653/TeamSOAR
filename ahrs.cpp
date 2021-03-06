#include "ahrs.hpp"

/* C/C++ Includes */
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/* Thor Includes */
#include "Thor/include/thor.h"
#include "Thor/include/spi.h"
#include "Thor/include/gpio.h"
#include "Thor/include/exceptions.h"

/* Boost Includes */
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "ahrs.hpp"
#include "LSM9DS1.hpp"
#include "config.hpp"
#include "dataTypes.hpp"
#include "threading.hpp"

/* Madgwick Filter */
#include "madgwick.hpp"

/* Kalman Filter */
#include "kalman/SquareRootUnscentedKalmanFilter.hpp"
#include "IMUModel.hpp"



namespace SOAR_AHRS
{
	typedef float T;
	typedef IMU::State<T> State;
	typedef IMU::Control<T> Control;
	typedef IMU::Measurement<T> Measurement;
	typedef IMU::SystemModel<T> SystemModel;
	typedef IMU::MeasurementModel<T> MeasurementModel;

	const int updateRate_mS = (1.0 / SENSOR_UPDATE_FREQ_HZ) * 1000.0;
	const int magMaxUpdateRate_mS = (1.0 / LSM9DS1_M_MAX_BW) * 1000.0;

	const float accelUncertainty = 0.8f;
	const float gyroUncertainty = 1.05f;

	void ahrsTask(void* argument)
	{
		#ifdef DEBUG
		volatile float pitch;
		volatile float roll;
		volatile float yaw;
		volatile float ax;
		volatile float ay;
		volatile float az;
		volatile float gx;
		volatile float gy;
		volatile float gz;
		volatile float mx;
		volatile float my;
		volatile float mz;
		volatile UBaseType_t stackHighWaterMark_AHRS = 0;
		volatile size_t bytesRemaining = xPortGetFreeHeapSize();
		#endif

		AHRSData_t ahrsData;					/* Output struct to push to the motor controller thread */


		/*----------------------------------
		* Initialize the UKF
		*----------------------------------*/
		State x, x_ukf;
		Control u;
		SystemModel sys;
		MeasurementModel om;
		Measurement meas;

		Eigen::Matrix<T, 6, 6> R;
		Eigen::Matrix<T, 6, 6> processNoise;

		Kalman::SquareRootUnscentedKalmanFilter<State> ukf(0.5f, 3.0f, 0.0f);

		x.setZero();
		u.setZero();
		R.setZero();

		T cnst = 5.00e-4f;
		T dnst = 3.33e-4f;
		T enst = 5.00e-4f;

		processNoise <<
			cnst, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, dnst, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, enst, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, cnst, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, dnst, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, enst;

		sys.setCovariance(processNoise);

		R(0, 0) = accelUncertainty * accelUncertainty;
		R(1, 1) = accelUncertainty * accelUncertainty;
		R(2, 2) = accelUncertainty * accelUncertainty;
		R(3, 3) = gyroUncertainty * gyroUncertainty;
		R(4, 4) = gyroUncertainty * gyroUncertainty;
		R(5, 5) = gyroUncertainty * gyroUncertainty;

		om.setCovariance(R);

		ukf.init(x);


		/*----------------------------------
		* Initialize the IMU
		*----------------------------------*/
		GPIOClass_sPtr lsm_ss_xg = boost::make_shared<GPIOClass>(GPIOC, PIN_4, ULTRA_SPD, NOALTERNATE);
		GPIOClass_sPtr lsm_ss_m = boost::make_shared<GPIOClass>(GPIOC, PIN_3, ULTRA_SPD, NOALTERNATE);
		SPIClass_sPtr lsm_spi = spi2;

		LSM9DS1 imu(lsm_spi, lsm_ss_xg, lsm_ss_m);

		/* Force halt of the device if the IMU cannot be reached */
		if (imu.begin() == 0)
			BasicErrorHandler("The IMU WHO_AM_I registers did not return valid readings");

		//TODO: Switch this out to print over the serial port

		imu.calibrate(true); /* "true" forces an automatic software subtraction of the calculated bias from all further data */
		imu.calibrateMag(true); /* "true" writes the offest into the mag sensor hardware for automatic subtraction in results */

		int count = 0;


		/*----------------------------------
		* Initialize the Madgwick Filter
		*----------------------------------*/
		float beta = 10.0;
		Eigen::Vector3f accel_raw, gyro_raw, mag_raw;
		Eigen::Vector3f accel_filtered, gyro_filtered, mag_filtered, eulerDeg;

		MadgwickFilter ahrs((AHRS_UPDATE_RATE_MULTIPLIER * SENSOR_UPDATE_FREQ_HZ), beta);

		float dt = (1.0f / SENSOR_UPDATE_FREQ_HZ);
		float tau_accel = 0.01f;
		float alpha_lp_accel = dt / tau_accel;


		/* Tell init task that this thread's initialization is done and ok to run.
		* Wait for init task to resume operation. */
		xTaskSendMessage(INIT_TASK, 1u);
		vTaskSuspend(NULL);
		taskYIELD();


		TickType_t lastTimeWoken = xTaskGetTickCount();
		for (;;)
		{
			#ifdef DEBUG
			stackHighWaterMark_AHRS = uxTaskGetStackHighWaterMark(NULL);
			bytesRemaining = xPortGetFreeHeapSize();
			#endif

			/*----------------------------
			* Sensor Reading
			*---------------------------*/
			/* Update Accel & Gyro Data at whatever frequency set by user. Max bandwidth on
			* chip is 952Hz which will saturate FreeRTOS if sampled that often.*/
			//taskENTER_CRITICAL();
			imu.readAccel();
			imu.readGyro();
			

			/* Update Mag Data at a max frequency set by LSM9DS1_M_MAX_BW (~75Hz) */
			#if (SENSOR_UPDATE_FREQ_HZ > LSM9DS1_M_MAX_BW)
			if (count > magMaxUpdateRate_mS)
			{
				imu.readMag();
				imu.calcMag();
				count = 0;
			}
			else
				count += updateRate_mS;
			#else
			imu.readMag();
			imu.calcMag();
			#endif
			//taskEXIT_CRITICAL();

			/* Convert raw data from chip into meaningful data */
			imu.calcAccel(); imu.calcGyro();

// 			#ifdef DEBUG
// 			ax = imu.aRaw[0];
// 			ay = imu.aRaw[1];
// 			az = imu.aRaw[2];
// 
// 			gx = imu.gRaw[0];
// 			gy = imu.gRaw[1];
// 			gz = imu.gRaw[2];
// 
// 			mx = imu.mRaw[0];
// 			my = imu.mRaw[1];
// 			mz = imu.mRaw[2];
// 			#endif

			accel_raw << imu.aRaw[0], imu.aRaw[1], imu.aRaw[2];
			gyro_raw << imu.gRaw[0], imu.gRaw[1], imu.gRaw[2];
			mag_raw <<
				-imu.mRaw[0],  //Align mag x with accel x
				-imu.mRaw[1],  //Align mag y with accel y
				-imu.mRaw[2];  //from LSM9DS1, mag z is opposite direction of accel z


			/*----------------------------
			* UKF Algorithm
			*---------------------------*/
			//Simulate the system
			x = sys.f(x, u);

			//Predict state for current time step 
			x_ukf = ukf.predict(sys);

			//Take a measurement given system state
			meas << accel_raw, gyro_raw;

			//Update the state equation given measurement
			x_ukf = ukf.update(om, meas);


			accel_filtered << x_ukf.ax(), x_ukf.ay(), x_ukf.az();
			gyro_filtered << x_ukf.gx(), x_ukf.gy(), x_ukf.gz();
			mag_filtered = mag_raw;


			/*----------------------------
			* AHRS Algorithm
			*---------------------------*/
			/* The Madgwick filter needs to run between 3-5 times as fast IMU measurements
			* to achieve decent convergence to a stable value. This thread only runs when
			* new data has arrived from the IMU, so frequency multiplication is as simple
			* as looping 3-5 times here. */
			for (int i = 0; i < AHRS_UPDATE_RATE_MULTIPLIER; i++)
				ahrs.update(accel_filtered, gyro_filtered, mag_filtered);

			ahrs.getEulerDeg(eulerDeg);
			ahrsData(eulerDeg, accel_filtered, gyro_filtered, mag_filtered);

			#ifdef DEBUG
			pitch = eulerDeg(0);
			roll = eulerDeg(1);
			yaw = eulerDeg(2);

			ax = accel_filtered(0);
			ay = accel_filtered(1);
			az = accel_filtered(2);

			gx = gyro_filtered(0);
			gy = gyro_filtered(1);
			gz = gyro_filtered(2);

			mx = mag_filtered(0);
			my = mag_filtered(1);
			mz = mag_filtered(2);
			#endif

			/* Send data over to the Serial thread*/
			if (xSemaphoreTake(ahrsBufferMutex, 0) == pdPASS)
			{
				xQueueOverwrite(qAHRS, &ahrsData);
				xSemaphoreGive(ahrsBufferMutex);
			}

			vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(updateRate_mS));
		}
	}
}

