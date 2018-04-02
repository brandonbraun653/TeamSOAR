/* C/C++ Includes */
#include <stdint.h>
#include <stdlib.h>

/* Thor Includes */
#include "Thor/include/thor.h"
#include "Thor/include/gpio.h"
#include "Thor/include/exti.h"
#include "Thor/include/interrupt.h"

/* Boost Includes */
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "dataTypes.hpp"
#include "threading.hpp"
#include "led.hpp"

using namespace ThorDef::GPIO;
GPIOClass_sPtr greenLed;

namespace SOAR_LED
{
	void parseTaskNotification(uint32_t notification)
	{
		if (notification == (LED_GREEN | LED_STATIC_ON))
			greenLed->write(HIGH);
	}

	void ledTask(void* argument)
	{
		#ifdef DEBUG
		volatile UBaseType_t stackHighWaterMark_LEDSTATUS = 0;
		#endif

		greenLed = boost::make_shared<GPIOClass>(GPIOA, PIN_5, ULTRA_SPD, NOALTERNATE);
		
		/* Set up the GPIO Led */
		greenLed->mode(OUTPUT_PP);
		greenLed->write(LOW);


		/* Tell init task that this thread's initialization is done and ok to run.
		* Wait for init task to resume operation. */
		xTaskSendMessage(INIT_TASK, 1u);
		vTaskSuspend(NULL);
		taskYIELD();

		TickType_t lastTimeWoken = xTaskGetTickCount();
		for (;;)
		{
			#ifdef DEBUG
			stackHighWaterMark_LEDSTATUS = uxTaskGetStackHighWaterMark(NULL);
			#endif

			/* Check for some new notifications before continuing */
			parseTaskNotification(ulTaskNotifyTake(pdTRUE, 0));

			greenLed->write(HIGH);
			vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(150));
			greenLed->write(LOW);
			vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(1000));
		}
	}
}