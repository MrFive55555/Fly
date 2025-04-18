#include "Com.h"
char json_str[512]; // 用于存储 JSON 字符串
/**
 * uart configuration
 */
#define BUF_SIZE (1024)
/**
 * wifi configuration
 */
#define MAXIMUM_RETRY 1000
static const char *TAG = "wifi station";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static int s_retry_num = 0;
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
/**
 * mqtt configuration
 */
typedef struct
{
    char client_id[128]; /* 客户端ID */
    char username[128];  /* 客户端用户名 */
    char password[64];   /* 连接密码 */
} mqtt_params_t;
static mqtt_params_t mqtt_params = {
    .client_id = {0},
    .username = {0},
    .password = {0},
};
int g_publish_flag = 0;                                    /* 发布成功标志位 */
static esp_mqtt_client_handle_t client = NULL;             /* MQTT客户端句柄 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static void hmac_sha1(const char *key, const char *msg, char *out_hex);
static void generate_mqtt_params(char *client_id, char *username, char *password);
// static const char *TAG = "MQTT";
/**
 * uart initialization
 */
void uart_init(uint32_t baud_rate)
{
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0);
    // ESP_LOGI("UART", "UART initialized!");
}
/**
 * connect wifi by event handler
 */
void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS},
    };
    /* Setting a password implies station will connect to all security modes including WEP/WPA.
     * However these modes are deprecated and not advisable to be used. Incase your Access point
     * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (strlen((char *)wifi_config.sta.password))
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    // ESP_LOGI(TAG, "wifi_init_sta finished.");
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        // ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
        //          WIFI_SSID, WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        // ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
        //          WIFI_SSID, WIFI_PASS);
    }
    else
    {
        // ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
// connect wifi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            // ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        //  ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        // ESP_LOGI(TAG, "got ip:%s",
        //          ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
/**
 * connect mqtt by event handler
 */
void mqtt_init(void)
{
    generate_mqtt_params(mqtt_params.client_id, mqtt_params.username, mqtt_params.password);
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = HOST_NAME,                    /* MQTT地址 */
        .port = HOST_PORT,                    /* MQTT端口号 */
        .client_id = mqtt_params.client_id,   /* 设备名称 */
        .username = mqtt_params.username,     /* 产品ID */
        .password = mqtt_params.password,     /* 计算出来的密码 */
        .transport = MQTT_TRANSPORT_OVER_TCP, /* TCP模式 */
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
// mqtt event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    // int msg_id;
    char json_buf[256] = {0};
    int len;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        // ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // msg_id = esp_mqtt_client_subscribe(client, DEVICE_SUBSCRIBE, 0);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        esp_mqtt_client_subscribe(client, DEVICE_SUBSCRIBE, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        // ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, DEVICE_PUBLISH, "ESP8266 has onlined", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        esp_mqtt_client_publish(client, DEVICE_PUBLISH, "ESP8266 has onlined", 0, 0, 0);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        /* printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data); */
        // 提取 JSON 字符串
        len = event->data_len < sizeof(json_buf) - 1 ? event->data_len : sizeof(json_buf) - 1;
        memcpy(json_buf, event->data, len);
        json_buf[len] = '\0';
        // 解析 JSON 指令
        cJSON *root = cJSON_Parse(json_buf);
        if (root)
        {
            char uart_cmd[64];

            // 只提取第一个字段
            cJSON *item = root->child;
            if (item && cJSON_IsNumber(item))
            {
                // 构造 UART 命令，如 $CMTAK1\r\n
                snprintf(uart_cmd, sizeof(uart_cmd), "$CM%s%d\r\n", item->string, item->valueint);
                uart_write_bytes(UART_NUM_0, uart_cmd, strlen(uart_cmd));
            }

            cJSON_Delete(root);
        }
        else
        {
            // ESP_LOGW("MQTT", "JSON parse failed.");
        }
        break;
    case MQTT_EVENT_ERROR:
        // ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        // ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
void send_mqtt_data(void)
{
    uint8_t data[256] = {0};
    int len = uart_read_bytes(UART_NUM_0, data, sizeof(data) - 1, pdMS_TO_TICKS(100));

    if (len <= 0)
        return;

    data[len] = '\0'; // null终结，确保字符串安全

    cJSON *root = cJSON_CreateObject();

    char *fb_start = strstr((char *)data, "$FB"); // head of frame
    if (fb_start)
    {
        fb_start += 3;                           // 跳过 "$FB"
        char *fb_end = strchr(fb_start, "\r\n"); // 查找反馈结束 blank

        if (fb_end)
        {
            // 提取 FB 部分
            char fb_value[64] = {0};
            strncpy(fb_value, fb_start, fb_end - fb_start);
            fb_value[fb_end - fb_start] = '\0';
            cJSON_AddStringToObject(root, "FB", fb_value);

            // 提取 other 部分（从逗号之后到末尾）
            char *other_data = fb_end + 1;
            char *end = strstr(other_data, "\r\n");
            if (end)
                *end = '\0'; // 去除结尾

            cJSON_AddStringToObject(root, "other", other_data);
        }
        else
        {
            // 没有逗号，整个都是 FB 值
            char *end = strstr(fb_start, "\r\n");
            if (end)
                *end = '\0';
            cJSON_AddStringToObject(root, "FB", fb_start);
        }
    }
    else
    {
        // 没有 FB 字段，默认全打包为 other
        cJSON_AddStringToObject(root, "other", (char *)data);
    }

    char *out = cJSON_PrintUnformatted(root);
    snprintf(json_str, sizeof(json_str), "%s", out);
    esp_mqtt_client_publish(client, DEVICE_PUBLISH, json_str, 0, 0, 0);

    cJSON_Delete(root);
    free(out);
}

static void hmac_sha1(const char *key, const char *msg, char *out_hex)
{
    unsigned char hash[20]; // SHA1 输出 20 字节
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1); // 1 = HMAC
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)msg, strlen(msg));
    mbedtls_md_hmac_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    // 转成十六进制字符串
    for (int i = 0; i < 20; ++i)
        sprintf(out_hex + i * 2, "%02x", hash[i]);
}
// 生成阿里云 MQTT 参数
static void generate_mqtt_params(char *client_id, char *username, char *password)
{
    // 组装待签名字符串
    char sign_content[128];
    sprintf(sign_content, "clientId%sdeviceName%sproductKey%s",
            DEVICE_NAME, DEVICE_NAME, PRODUCT_KEY);

    // HMAC-SHA1 签名结果
    hmac_sha1(DEVICE_SECRET, sign_content, password);

    // clientId：可以固定写死，带 securemode/signmethod
    sprintf(client_id, "%s|securemode=3,signmethod=hmacsha1|", DEVICE_NAME);

    // username：格式为 deviceName&productKey
    sprintf(username, "%s&%s", DEVICE_NAME, PRODUCT_KEY);
}
