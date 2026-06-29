#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void app_main(void)
{
    while(1) {
        ESP_LOGI("TEST_GW", "Le Gateway compile correctement !");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
