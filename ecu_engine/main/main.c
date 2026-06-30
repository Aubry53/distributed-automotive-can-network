#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_random.h"
#include "can_protocol.h"

#define CAN_TX_IO      22
#define CAN_RX_IO      21

static const char *TAG = "CAN_ECU";

void init_can(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_IO, CAN_RX_IO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
}

// Tâche 1 : Simulation du moteur avec consommation et panne thermique
void engine_simulation_task(void *pvParameters) {
    can_engine_data_t current_data = { .rpm = 850, .temperature = 25, .fuel_level = 100 };
    twai_message_t tx_msg = { .identifier = CAN_ID_ENGINE_DATA, .data_length_code = sizeof(can_engine_data_t) };
    uint32_t loop_counter = 0;

    while (1) {
        loop_counter++;
        
        // 1. Oscillation des RPM
        current_data.rpm += (esp_random() % 30) - 14;
        if (current_data.rpm < 800) current_data.rpm = 800;

        // 2. Simulation d'une consommation de carburant (baisse toutes les 10 secondes)
        if (loop_counter % 50 == 0 && current_data.fuel_level > 0) {
            current_data.fuel_level--;
        }

        // 3. Simulation d'une panne : après 30 secondes, surchauffe critique à 115°C
        if (loop_counter > 150) { 
            current_data.temperature = 115; 
        } else if (current_data.temperature < 90) {
            current_data.temperature += 1; // Chauffe normale au départ
        }

        memcpy(tx_msg.data, &current_data, tx_msg.data_length_code);
        twai_transmit(&tx_msg, pdMS_TO_TICKS(50));

        vTaskDelay(pdMS_TO_TICKS(200)); // 5Hz
    }
}

// Tâche 2 : Heartbeat cyclique (Battement de cœur du système)
void heartbeat_task(void *pvParameters) {
    can_heartbeat_t hbeat = { .status_mask = 0x01, .counter = 0 }; // 0x01 = Engine Running
    twai_message_t tx_msg = { .identifier = CAN_ID_HEARTBEAT, .data_length_code = sizeof(can_heartbeat_t) };

    while (1) {
        hbeat.counter++;
        
        // Si le moteur surchauffe, on lève le bit d'erreur 1 (Alerte Temp) dans le masque de statut
        if (hbeat.counter > 30) {
            hbeat.status_mask |= 0x02; // Injection d'erreur dans le protocole
        }

        memcpy(tx_msg.data, &hbeat, tx_msg.data_length_code);
        twai_transmit(&tx_msg, pdMS_TO_TICKS(50));
        
        ESP_LOGI(TAG, "[CAN TX ➔ HEARTBEAT] Sent status: 0x%02X | Count: %u", hbeat.status_mask, hbeat.counter);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1Hz (Toutes les secondes)
    }
}

void app_main(void) {
    init_can();
    // Utilisation de FreeRTOS pour lancer deux tâches concurrentes en parallèle
    xTaskCreate(engine_simulation_task, "EngineSim", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "Heartbeat", 4096, NULL, 5, NULL);
}

