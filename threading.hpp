#pragma once
#ifndef SOAR_THREADING_HPP
#define SOAR_THREADING_HPP

/* Boost Includes */
#include <boost/container/vector.hpp>

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */


/*----------------------------------
* Queues
*----------------------------------*/
extern QueueHandle_t qIMU;
extern QueueHandle_t qAHRS;		
extern QueueHandle_t qSERIAL;


/*----------------------------------
* Semaphores and Mutexes
*----------------------------------*/
extern SemaphoreHandle_t ahrsBufferMutex;



enum TaskIndex
{
	INIT_TASK,
	LED_STATUS_TASK,
	AHRS_TASK,
	SERIAL_TASK,
	TOTAL_TASK_SIZE
};
extern boost::container::vector<void*> TaskHandle;

/* Allows sending a notification message to any task from anywhere */
extern BaseType_t xTaskSendMessage(const TaskIndex, const uint32_t);
extern BaseType_t xTaskSendMessageFromISR(const TaskIndex, const uint32_t);

#endif