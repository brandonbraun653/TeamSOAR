/** 
Author: Brandon Braun 3/29/18
Email: brandonbraun653@gmail.com

Short Description:
	This software provides an attitude heading and reference system (AHRS) implementation for the purpose
	of instrumenting a custom flight simulator cockpit. An STM32F4 microcontroller is paired with an 
	LSTM9DS1 9-DOF inertial measurement unit (IMU) to gather the data, which is outputted to a computer
	over a serial port.

Long Description:
************TODO******************

-----------------------------------------------------------------------------------------------------------------
ENVIRONMENT:

	1. Visual Studio 2017 Community Edition (Required)
		a. This is the main IDE used. Do NOT use the Arduino IDE. It's trash.
		b. Download Link: https://www.visualstudio.com/downloads/

	2. VisualGDB (Required)
		a. Provides the libraries needed for core STM32 interface
		b. It also is a nice plugin to Visual Studio that makes developing for this platform easy
		c. Download Link: http://visualgdb.com/download/ 

	3. Git (Required)
		a. Enables you to pull and commit code from Github
		b. Download Link: https://git-scm.com/downloads
		c. Git setup instructions: https://help.github.com/articles/set-up-git/

	4. ST-Link Utility (Optional)
		a. Lets you upgrade firmware for the on-board programmer and a whole host of other goodies
		b. Download Link: http://www.st.com/en/development-tools/stsw-link004.html

	5. Visual Assist (Optional)
		a. A nice little add-on to Visual Studio
		b. Download Link: https://www.wholetomato.com/

	6. STM32CubeMX (Optional)
		a. This software provides a GUI interface to generate initialization code for the STM32
		b. Download Link: http://www.st.com/en/development-tools/stm32cubemx.html

		
DEPENDENCIES:

	1. STM32F4 Cube HAL
		a. Supplied by VisualGDB
		b. Contains the core low level drivers for the STM32F4 family of devices
		c. Project -> VisualGDB Project Properties -> Embedded Frameworks (Make sure the option is checked & hit apply)

	2. Fast Semihosting and Embedded Profiler
		a. Supplied by VisualGDB
		b. Lets the user debug code and get various specs in real time from the MCU
		c. Project -> VisualGDB Project Properties -> Embedded Frameworks (Make sure the option is checked & hit apply)

	3. FreeRTOS
		a. This is like the MCU operating system. It allows scheduling of tasks and other neat things to run programs asynchronously.
		b. Download Link: https://www.freertos.org/a00104.html
		c. Alternate: https://aws.amazon.com/freertos/getting-started/ 

	4. BOOST
		a. A handy C++ library that adds a lot of nice features
		b. Download Link: http://www.boost.org/users/download/
	
	5. Eigen
		a. This provides matrix math implementations for C++
		b. https://github.com/eigenteam/eigen-git-mirror

	4. Thor_STM32
		a. Primary driver for peripherals that build on top of the HAL library
		b. https://github.com/brandonbraun653/Thor_STM32

*/

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"

/* Thor Includes */
#include "Thor/include/thor.h"

#ifdef DEBUG
#include "SysprogsProfiler.h"
#endif

/* Thread Task Includes */
#include "config.hpp"
#include "threading.hpp"
#include "ahrs.hpp"
#include "coms.hpp"
#include "led.hpp"


void init(void* parameter);

int main(void)
{
	HAL_Init();	/* Initializes STM32 Cube Stuff */
	ThorInit();	/* Initializes custom things for Thor, like the MCU and peripheral clocks */

	
	/*	Useful for several of the debugging functionalities like the
		Chronometer and Performance Profiler in VGDB */
	#ifdef DEBUG
	InitializeSamplingProfiler();
	InitializeInstrumentingProfiler();
	#endif 

	xTaskCreate(init, "init", 500, NULL, 1, &TaskHandle[INIT_TASK]);
	vTaskStartScheduler();

	/* We will never reach here as the scheduler should have taken over */
	for (;;)
	{
	}
}

void init(void* parameter)
{
	/* Initialize the pointers to all task handles (except for INIT_TASK). This
	* prevents errors in calling xTaskSendMessage should a task no longer exist.
	* */
	for (int task = 1; task < TOTAL_TASK_SIZE; task++)
		TaskHandle[task] = (TaskHandle_t)0;

	volatile BaseType_t error = pdPASS;
	TickType_t lastTimeWoken = xTaskGetTickCount();

	/* Create a task and then wait until its initialization sequence has finished before continuing.
	* The tasks MUST be created in this order:
	* 1. LedStatus
	* 2. Serial
	* 3. AHRS
	* */

	error = xTaskCreate(SOAR_LED::ledTask, "ledTask", 350, NULL, STATUS_LEDS_PRIORITY, &TaskHandle[LED_STATUS_TASK]);
	while (!ulTaskNotifyTake(pdTRUE, 0))
		vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(10));

	error = xTaskCreate(SOAR_SERIAL::serialTask, "serialTask", 1000, NULL, STATUS_LEDS_PRIORITY, &TaskHandle[SERIAL_TASK]);
	while (!ulTaskNotifyTake(pdTRUE, 0))
		vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(10));

	error = xTaskCreate(SOAR_AHRS::ahrsTask, "ahrsTask", 8000, NULL, AHRS_UPDATE_PRIORITY, &TaskHandle[AHRS_TASK]);
	while (!ulTaskNotifyTake(pdTRUE, 0))
		vTaskDelayUntil(&lastTimeWoken, pdMS_TO_TICKS(10));


	#ifdef DEBUG
	volatile TaskHandle_t hAhrs = TaskHandle[AHRS_TASK];
	volatile TaskHandle_t hLed = TaskHandle[LED_STATUS_TASK];
	volatile TaskHandle_t hSerial = TaskHandle[SERIAL_TASK];
	volatile size_t bytesRemaining = xPortGetFreeHeapSize();
	#endif


	if (error == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
	{
		/* If you hit this point, one of the above tasks tried to allocate more heap space
		* than was available. Run in a Debug build and check the bytesRemaining variable. */
		for (;;) {}
	}


	/* Resume all tasks in the correct order */
	vTaskResume(TaskHandle[LED_STATUS_TASK]);
	vTaskResume(TaskHandle[SERIAL_TASK]);
	vTaskResume(TaskHandle[AHRS_TASK]);
	

	/* Ensure a clean deletion of the task upon exit */
	TaskHandle[INIT_TASK] = (void*)0;	//Deletes our personal log of this task's existence
	vTaskDelete(NULL);					//Deletes the kernel's log of this task's existence
}