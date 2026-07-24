#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "utils.h"

// UUIDs e nome PROPRIOS deste robo - diferentes dos da Bia de proposito
// (PLANEJAMENTO.md secao 7), pra dar pra ter os dois robos ligados perto
// um do outro sem confundir na hora de conectar pelo app/celular.
#define SERVICE_UUID "7e5a2f10-3c4b-4d8e-9a1f-6b2c8e4d7f30"
#define MESSAGE_UUID "9c3e7a20-1b5d-4f6a-8e2c-3a7f9d1b4e60"

#define DEVINFO_UUID              (uint16_t)0x180a
#define DEVINFO_MANUFACTURER_UUID (uint16_t)0x2a29
#define DEVINFO_NAME_UUID         (uint16_t)0x2a24
#define DEVINFO_SERIAL_UUID       (uint16_t)0x2a25

#define DEVICE_MANUFACTURER "Phoenix Unicamp"
#define DEVICE_NAME          "Phoenix RC-ROU"

void bluetooth_init();
void send_bluetooth_message(String message);
String read_bluetooth_message();
void bluetooth_resume();
void bluetooth_check_connection();
bool bluetooth_is_connected();
void validar_bluetooth();

#endif /* __BLUETOOTH_H__ */
