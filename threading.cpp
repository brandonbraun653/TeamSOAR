#include "threading.hpp"

QueueHandle_t qAHRS = xQueueCreate(1, sizeof(AHRSData_t));
SemaphoreHandle_t ahrsBufferMutex = xSemaphoreCreateMutex();

boost::container::vector<void*> TaskHandle(TOTAL_TASK_SIZE);


BaseType_t xTaskSendMessage(TaskIndex idx, uint32_t msg)
{
	if (TaskHandle[idx])
		return xTaskNotify(TaskHandle[idx], msg, eSetValueWithOverwrite);
	else
		return pdFAIL;
}

BaseType_t xTaskSendMessageFromISR(TaskIndex idx, uint32_t msg)
{
	if (TaskHandle[idx])
		return xTaskNotifyFromISR(TaskHandle[idx], msg, eSetValueWithOverwrite, NULL);
	else
		return pdFAIL;
}
