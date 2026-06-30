#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_random.h"
#include "can_protocol.h"

#define CAN_TX_IO      22  // Pin di trasmissione collegato al Transceiver
#define CAN_RX_IO      21  // Pin di ricezione collegato al Transceiver

static const char *TAG = "CAN_ECU";

void init_can(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_IO, CAN_RX_IO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Velocità: 500 kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI (CAN) installato con successo sulla ECU.");
    }
    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Bus CAN avviato sulla ECU.");
    }
}

void can_tx_task(void *pvParameters) {
    // Inizializzazione della struttura dati con valori di partenza realistici
    can_engine_data_t current_data = {
        .rpm = 850,
        .temperature = 25,
        .fuel_level = 100
    };

    // Configurazione della struttura del messaggio CAN hardware di Espressif
    twai_message_t tx_msg;
    tx_msg.identifier = CAN_ID_ENGINE_DATA;
    tx_msg.extd = 0;              // 0 = Standard ID (11 bit)
    tx_msg.rtr = 0;               // 0 = Data Frame (non una richiesta remota)
    tx_msg.data_length_code = sizeof(can_engine_data_t); // Lunghezza: 4 byte

    while (1) {
        // Simulazione dinamica: i giri oscillano, la temperatura sale lentamente
        current_data.rpm += (esp_random() % 30) - 14; 
        if (current_data.rpm < 800) current_data.rpm = 800;
        if (current_data.rpm > 4500) current_data.rpm = 4500;

        if (current_data.temperature < 90) {
            current_data.temperature += 1; // Il motore si scalda fino a 90°C
        }

        // Copia la struttura dati simulata nel buffer del messaggio CAN hardware
        memcpy(tx_msg.data, &current_data, tx_msg.data_length_code);

        // Invia il pacchetto sul bus CAN fisico
        esp_err_t err = twai_transmit(&tx_msg, pdMS_TO_TICKS(100));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "[CAN TX] Messaggio inviato: RPM=%u Temp=%d Fuel=%u", 
                     current_data.rpm, current_data.temperature, current_data.fuel_level);
        } else {
            ESP_LOGE(TAG, "Errore trasmissione CAN: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // Frequenza di invio: 5Hz (ogni 200ms)
    }
}

void app_main(void) {
    init_can();
    // Creazione della task FreeRTOS di simulazione e trasmissione
    xTaskCreate(can_tx_task, "CanTxTask", 4096, NULL, 5, NULL);
}
