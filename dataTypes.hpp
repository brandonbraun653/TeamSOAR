#pragma once
#ifndef SOAR_DATATYPES_HPP
#define SOAR_DATATYPES_HPP

/* C/C++ Includes */
#include <stdint.h>
#include <stdlib.h>

/* Eigen Includes */
#include <Eigen/Eigen>


enum LEDInstructions
{
	//Select which led to apply actions to
	LED_BLUE = (1U << 31),
	LED_YELLOW = (1U << 30),
	LED_RED = (1U << 29),
	LED_GREEN = (1U << 28),

	//Available actions for each led
	LED_STATIC_ON = (1U << 1),
	LED_STATIC_OFF = (1U << 2),
	LED_FLASH_SLOW = (1U << 3),
	LED_FLASH_MED = (1U << 4),
	LED_FLASH_FAST = (1U << 5)
};


struct AHRSData_t
{
	AHRSData_t()
	{
		eulerAngles.setZero();
		accel.setZero();
		gyro.setZero();
		mag.setZero();
	}

	void operator()(Eigen::Vector3f euler_deg, Eigen::Vector3f acceleration_ms2,
		Eigen::Vector3f gyroscope_dps, Eigen::Vector3f magnetometer_g)
	{
		eulerAngles = euler_deg;
		accel = acceleration_ms2;
		gyro = gyroscope_dps;
		mag = magnetometer_g;
	}

	Eigen::Vector3f eulerAngles;	/* [PITCH, ROLL, YAW] (deg) */
	Eigen::Vector3f accel;			/* [X, Y, Z] (m/s^2) */
	Eigen::Vector3f gyro;			/* [X, Y, Z] (dps) */
	Eigen::Vector3f mag;			/* [X, Y, Z] (gauss) */


	const float& pitch() { return this->eulerAngles(0); }
	const float& roll() { return this->eulerAngles(1); }
	const float& yaw() { return this->eulerAngles(2); }

	const float& ax() { return this->accel(0); }
	const float& ay() { return this->accel(1); }
	const float& az() { return this->accel(2); }

	const float& gx() { return this->gyro(0); }
	const float& gy() { return this->gyro(1); }
	const float& gz() { return this->gyro(2); }

	const float& mx() { return this->mag(0); }
	const float& my() { return this->mag(1); }
	const float& mz() { return this->mag(2); }
};

#endif 