#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

// identificatori di messaggi CAN (11-bit Standard ID)
#define CAN_ID_ENGINE_DATA  0x100  // Données dynamiques du moteur
#define CAN_ID_HEARTBEAT    0x104  // Statut système et erreurs

// Struttura del messaggio del motore (dimensione: 4 byte)
typedef struct __attribute__((packed)) {
    uint16_t rpm;          // Tr/min (0 à 5000)
    int8_t   temperature;  // Température en °C (-40 à 125)
    uint8_t  fuel_level;   // Niveau de carburant en % (0 à 100)
} can_engine_data_t;

// Structure du message Heartbeat (Taille : 2 octets)
typedef struct __attribute__((packed)) {
    uint8_t status_mask;  // Bit 0: Moteur ON/OFF, Bit 1: Alerte Temp, Bit 2: Erreur Capteur
    uint8_t counter;      // Compteur cyclique d'envoi (0 à 255)
} can_heartbeat_t;

#endif // CAN_PROTOCOL_H
