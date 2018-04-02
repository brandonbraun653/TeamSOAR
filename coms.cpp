#include "coms.hpp"

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "config.hpp"
#include "threading.hpp"

namespace SOAR_SERIAL
{
	const int updateRate_mS = (1.0 / CONSOLE_UPDATE_FREQ_HZ) * 1000.0;

	void serialTask(void* argument)
	{



		/* Tell init task that this thread's initialization is done and ok to run.
		* Wait for init task to resume operation. */
		xTaskSendMessage(INIT_TASK, 1u);
		vTaskSuspend(NULL);
		taskYIELD();


		TickType_t lastTimeWoken = xTaskGetTickCount();
		for (;;)
		{

			vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(updateRate_mS));
		}
	}
}
