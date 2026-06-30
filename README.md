# Rete CAN Automotive Distribuita con Gateway Wi-Fi su ESP32

[![Framework](https://shields.io)](https://espressif.com)
[![RTOS](https://shields.io)](https://freertos.org)
[![Protocollo](https://shields.io)](https://wikipedia.org)

Questo progetto implementa un sistema embedded distribuito che simula l'architettura elettronica di un veicolo moderno. È composto da due nodi di calcolo basati su microcontrollori ESP32, che comunicano tramite un bus CAN (TWAI) fisico ed espongono un'interfaccia di diagnostica in tempo reale attraverso un gateway Wi-Fi / HTTP di livello industriale.

---

## 🏗️ Architettura del Sistema

Il sistema è segmentato in due unità distinte (ECU) indipendenti:

1. **ECU Motore (`ecu_engine`)**: 
   * Simula la telemetria fisica del motore in tempo reale.
   * Utilizza il multitasking **FreeRTOS** per temporizzare l'invio dei dati e delle trame di diagnostica.
2. **Gateway + Dashboard (`gateway_dash`)**:
   * Agisce come un nodo di ascolto passivo sul bus CAN.
   * Decodifica le trames binarie in ingresso e incapsula le metriche in formato **JSON**.
   * Ospita un punto d'accesso Wi-Fi privato e un server HTTP che trasmette un pannello di controllo dinamico ad alta frequenza (5Hz).

```text
                  [ BUS CAN FISICO - 500 kbps ]
┌──────────────────────────────────────────────────────────────┐
  (CANH) ────────────────────────────────────────────── (CANH)
  (CANL) ────────────────────────────────────────────── (CANL)
  (GND)  ────────────────────────────────────────────── (GND)
     │                                                    │
[ R: 120 Ω ]                                         [ R: 120 Ω ]
     │                                                    │
┌────┴────────────────────────┐       ┌───────────────────┴────┐
│      Transceiver CAN        │       │      Transceiver CAN   │
└────┬───────────────────┬────┘       └────┬───────────────────┬┘
     │ (RXD)       (TXD) │                 │ (RXD)       (TXD) │
     │                   │                 │                   │
┌────┴───────────────────┴────┐       ┌────┴───────────────────┴────┐
│          ESP32 #1           │       │          ESP32 #2           │
│         ECU MOTORE          │       │      GATEWAY DIAGNOSTICA    │
├─────────────────────────────┤       ├─────────────────────────────┤
│  • Task Simulazione Motore  │       │  • Task Ricevitore CAN Sinc  │
│  • Task Heartbeat Ciclico   │       │  • Server Wi-Fi SoftAP      │
└─────────────────────────────┘       └──────────────┬──────────────┘
                                                     │
                                             ((  Wi-Fi AP  ))
                                                     │
                                           ┌─────────┴─────────┐
                                           │   Browser Web     │
                                           │  Dashboard (HTTP) │
                                           └───────────────────┘
```

---

## 📊 Specifiche del Protocollo CAN (TWAI)

La comunicazione avviene tramite identificatori standard a 11 bit con una velocità di rete di **500 kbps**:

| ID CAN | Messaggio / Componente | Frequenza | Dimensione | Payload (Byte Grezzi) |
| :--- | :--- | :--- | :--- | :--- |
| **`0x100`** | `CAN_ID_ENGINE_DATA` | 5 Hz (200ms) | 4 byte | `[0-1]`: Giri Motore (RPM, uint16_t)<br>`[2]`: Temperatura (°C, int8_t)<br>`[3]`: Carburante (%, uint8_t) |
| **`0x104`** | `CAN_ID_HEARTBEAT` | 1 Hz (1000ms) | 2 byte | `[0]`: Maschera di stato (uint8_t)<br>`[1]`: Contatore ciclico (uint8_t) |

### Algoritmo di Diagnostica e Maschera d'Errore (`status_mask`):
* `0x01`: Motore in condizioni nominali (Engine Running)
* `0x03`: **ALLERTA CRITICA** - Rilevato surriscaldamento termico (T° > 90°C)

---

## 🛠️ Schema di Cablaggio Hardware

| ESP32 #1 & #2 (Pin) | Transceiver CAN (Pin) | Connessione Bus di Rete |
| :--- | :--- | :--- |
| **3V3** | VCC | Alimentazione locale isolata per nodo |
| **GND** | GND | Riferimento di massa comune tra i nodi |
| **GPIO 21** | RXD | Linea di ricezione logica |
| **GPIO 22** | TXD | Linea di trasmissione logica |
| — | **CANH** | Interconnessione CANH fisica + Resistenza 120 Ω |
| — | **CANL** | Interconnessione CANL fisica + Resistenza 120 Ω |

---

## 🛡️ Funzionalità di Robustezza & Simulazione Guasti

Il firmware integra un iniettore di guasti applicativo per validare il comportamento del sistema in condizioni degradate:
1. **Consumo Lineare**: Il livello di carburante scende in modo continuo per simulare lo stato reale dei serbatoi.
2. **Stabilizzazione Termica**: Il motore esegue una curva di riscaldamento realistica fino a stabilizzarsi alla temperatura nominale di 90°C.
3. **Iniezione Guasto Critico (Diagnostica)**: Dopo 30 secondi di funzionamento, il sistema simula un'avaria al sistema di raffreddamento. La temperatura sale a **115°C** e il protocollo attiva il bit di guasto `0x02` nel messaggio `0x104`. Il gateway web intercetta la trama e attiva istantaneamente un'allerta grafica lampeggiante sullo schermo.

---

## 🚀 Compilazione e Installazione

### Prerequisiti
* Visual Studio Code con l'estensione ufficiale **ESP-IDF v5.4.x** installata.
* Due moduli ESP32 connessi contemporaneamente.

### 1. Flash della ECU Motore
Apri la cartella `ecu_engine` in VS Code, seleziona la porta COM associata alla tua prima scheda (es: `COM9`) ed esegui il flash:
```bash
# Tramite interfaccia grafica o terminale ESP-IDF
idf.py build flash monitor
```

### 2. Flash del Gateway Diagnostico
Apri la cartella `gateway_dash` in VS Code, seleziona la porta COM associata alla tua seconda scheda (es: `COM8`) ed esegui il flash:
```bash
idf.py build flash monitor
```

### 3. Accesso al Pannello di Controllo (Dashboard)
1. Connetti il tuo computer o smartphone al punto di accesso Wi-Fi generato dal Gateway:
   * **SSID**: `ESP32_Automotive_Network`
   * **Password**: `12345678`
2. Apri il browser e vai all'indirizzo IP statico del server: **`http://11.22.33.44`**

---

## 🔧 Tecnologie Utilizzate

* **Linguaggio**: C per Sistemi Embedded (Standard ISO C99)
* **OS Embedded**: FreeRTOS (Multitasking preventivo, Task, Sincronizzazione)
* **Periferiche**: Driver nativo TWAI (Two-Wire Automotive Interface), NVS Flash, SoftAP Wi-Fi
* **Stack Web**: Server HTTP ESP-IDF, Serializzazione JSON, Fetch API Asincrona
* **Build System**: CMake & Ninja
