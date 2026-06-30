#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

// identificatori di messaggi CAN (11-bit Standard ID)
#define CAN_ID_ENGINE_DATA  0x100  // Données dynamiques du moteur
#define CAN_ID_HEARTBEAT    0x104  // Statut système et erreurs

// Struttura del messaggio del motore (dimensione: 4 byte)
typedef struct __attribute__((packed)) {
    uint16_t rpm;          // Giri al minuto (da 0 a 5000)
    int8_t   temperature;  // Temperatura in °C (da -40 a 125)
    uint8_t  fuel_level;   // Livello del carburante in % (da 0 a 100)
} can_engine_data_t;

// Struttura del messaggio Battito cardiaco (dimensione : 2 ottetti)
typedef struct __attribute__((packed)) {
    uint8_t status_mask;  // Bit 0: Motor ON/OFF, Bit 1: Alert Temp, Bit 2: Errore del sensore
    uint8_t counter;      // Contatore di invio ciclico (da 0 a 255)
} can_heartbeat_t;

#endif // CAN_PROTOCOL_H
