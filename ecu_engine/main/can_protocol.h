#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

// Identificateurs des messages CAN (11-bit Standard ID)
#define CAN_ID_ENGINE_DATA  0x100  // Dati dinamici del motore
#define CAN_ID_HEARTBEAT    0x104  // Stato del sistema ed errori

// Structure du message Moteur (Taille : 4 octets)
typedef struct __attribute__((packed)) {
    uint16_t rpm;          // Giri al minuto (0 a 5000)
    int8_t   temperature;  // Temperatura in °C (-40 a 125)
    uint8_t  fuel_level;   // Livello carburante in % (0 a 100)
} can_engine_data_t;

// Structure du message Heartbeat (Taille : 2 octets)
typedef struct __attribute__((packed)) {
    uint8_t status_mask;  // Bit 0: Motore ON/OFF, Bit 1: Allerta Temp, Bit 2: Errore Sensore
    uint8_t counter;      // Contatore ciclico di invio (0 a 255)
} can_heartbeat_t;

#endif // CAN_PROTOCOL_H
