/* C/C++ Includes */
#include <stdint.h>
#include <string>
#include <memory>
#include <cstdio>

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* HAL Includes */
#include "stm32f4xx_hal.h"

/* Thor Includes */
#include "Thor\include\uart.h"


/* Project Includes */
#include "config.hpp"
#include "threading.hpp"
#include "coms.hpp"

#define WRITE_RAW true

namespace SOAR_SERIAL
{
	const int updateRate_mS = (1.0 / CONSOLE_UPDATE_FREQ_HZ) * 1000.0;

	/* Input data from the AHRS algorithm */
	AHRSData_t ahrs;	

	/* Formatter to output as std::string...much easier to use */
	template<typename ... Args>
	std::string stringFormat(const std::string& format, Args ... args)
	{
		size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; //Extra space for '\0'

		std::unique_ptr<char[]> buf(new char[size]);

		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); //Exclude the '\0'
	}

	#define MAX_PRECISION	(10)
	static const double rounders[MAX_PRECISION + 1] =
	{
		0.5,				// 0
		0.05,				// 1
		0.005,				// 2
		0.0005,				// 3
		0.00005,			// 4
		0.000005,			// 5
		0.0000005,			// 6
		0.00000005,			// 7
		0.000000005,		// 8
		0.0000000005,		// 9
		0.00000000005		// 10
	};

	char * ftoa(double f, char * buf, int precision)
	{
		char * ptr = buf;
		char * p = ptr;
		char * p1;
		char c;
		long intPart;

		// check precision bounds
		if (precision > MAX_PRECISION)
			precision = MAX_PRECISION;

		// sign stuff
		if (f < 0)
		{
			f = -f;
			*ptr++ = '-';
		}

		if (precision < 0)  // negative precision == automatic precision guess
		{
			if (f < 1.0) precision = 6;
			else if (f < 10.0) precision = 5;
			else if (f < 100.0) precision = 4;
			else if (f < 1000.0) precision = 3;
			else if (f < 10000.0) precision = 2;
			else if (f < 100000.0) precision = 1;
			else precision = 0;
		}

		// round value according the precision
		if (precision)
			f += rounders[precision];

		// integer part...
		intPart = f;
		f -= intPart;

		if (!intPart)
			*ptr++ = '0';
		else
		{
			// save start pointer
			p = ptr;

			// convert (reverse order)
			while (intPart)
			{
				*p++ = '0' + intPart % 10;
				intPart /= 10;
			}

			// save end pos
			p1 = p;

			// reverse result
			while (p > ptr)
			{
				c = *--p;
				*p = *ptr;
				*ptr++ = c;
			}

			// restore end pos
			ptr = p1;
		}

		// decimal part
		if (precision)
		{
			// place decimal point
			*ptr++ = '.';

			// convert
			while (precision--)
			{
				f *= 10.0;
				c = f;
				*ptr++ = '0' + c;
				f -= c;
			}
		}

		// terminating zero
		*ptr = 0;

		return buf;
	}
	
	void serialTask(void* argument)
	{
		uart2->begin(921600);


		std::string data = "hello friend\n";

		std::string gx, gy, gz;
		std::string ax, ay, az;
		std::string mx, my, mz;
		std::string pitch, roll, yaw;

		const int precision = 2;
		char *buff = new char(100);

		/* Tell init task that this thread's initialization is done and ok to run.
		* Wait for init task to resume operation. */
		xTaskSendMessage(INIT_TASK, 1u);
		vTaskSuspend(NULL);
		taskYIELD();

		TickType_t lastTimeWoken = xTaskGetTickCount();
		for (;;)
		{
			/* Check for an update from the AHRS thread. This will always pull the latest information. */
			if (xSemaphoreTake(ahrsBufferMutex, 0) == pdPASS)
			{
				xQueueReceive(qAHRS, &ahrs, 0);
				xSemaphoreGive(ahrsBufferMutex);
			}

			if (WRITE_RAW)
			{
				/* Simple csv style data */
				gx = ftoa(ahrs.gx(), buff, precision);
				gy = ftoa(ahrs.gy(), buff, precision);
				gz = ftoa(ahrs.gz(), buff, precision);

				ax = ftoa(ahrs.ax(), buff, precision);
				ay = ftoa(ahrs.ay(), buff, precision);
				az = ftoa(ahrs.az(), buff, precision);

				pitch = ftoa(ahrs.pitch(), buff, precision);
				roll = ftoa(ahrs.roll(), buff, precision);
				yaw = ftoa(ahrs.yaw(), buff, precision);
				

				std::string line = 
					pitch + ',' + roll + ',' + yaw + ',' + \
					ax + ',' + ay + ',' + az + ',' + \
					gx + ',' + gy + ',' + gz + "\r\n";
				uart2->write(line);
			}
			else
			{
				/* Pretty Print data to the terminal */
			}

			vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(updateRate_mS));
		}
	}



}
