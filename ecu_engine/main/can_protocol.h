#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

// Identificatori dei messaggi CAN (ID standard a 11 bit)
#define CAN_ID_ENGINE_DATA  0x100  // Dati dinamici del motore
#define CAN_ID_HEARTBEAT    0x104  // Stato del sistema ed errori

// Struttura del messaggio del motore (dimensione: 4 byte)
typedef struct __attribute__((packed)) {
    uint16_t rpm;          // Giri al minuto (0 a 5000)
    int8_t   temperature;  // Temperatura in °C (-40 a 125)
    uint8_t  fuel_level;   // Livello carburante in % (0 a 100)
} can_engine_data_t;

// Struttura del messaggio Battito cardiaco (Dimensione : 2 ottetti)
typedef struct __attribute__((packed)) {
    uint8_t status_mask;  // Bit 0: Motore ON/OFF, Bit 1: Allerta Temp, Bit 2: Errore Sensore
    uint8_t counter;      // Contatore ciclico di invio (0 a 255)
} can_heartbeat_t;

#endif // CAN_PROTOCOL_H
