#include "App.h"
/**
 * create task parameters
 */
#define CREATE_STACK_SIZE 2048
#define CREATE_TASK_PRIO 1
static void create_task(void *pvParameters);
/**
 * communication task
 */
#define COM_STACK_SIZE 2048
#define COM_TASK_PRIO 2
static void com_task(void *pvParameters);
void app_start()
{
    /**
     * Initialize configuration
     */
    esp_log_level_set("*", ESP_LOG_NONE);  // 禁用所有模块的日志
    uart_init(9600);
    wifi_init();
    mqtt_init();
    /**
     * create task
     */
    xTaskCreate(
        create_task,       // 任务函数
        "create_task",     // 任务名称
        CREATE_STACK_SIZE, // 栈大小（单位：word，不是字节）
        NULL,              // 参数
        CREATE_TASK_PRIO,  // 优先级
        NULL              // 任务句柄
    );
}
static void create_task(void *pvParameters)
{
    // 创建任务
    xTaskCreate(
        com_task,       // 任务函数
        "com_task",     // 任务名称
        COM_STACK_SIZE, // 栈大小（单位：word，不是字节）
        NULL,           // 参数
        COM_TASK_PRIO,  // 优先级
        NULL           // TCB 控制块
    );
    // 删除当前任务
    vTaskDelete(NULL);
}
static void com_task(void *pvParameters)
{
    while (1)
    {
        send_mqtt_data();                      // 发送数据
    }
}