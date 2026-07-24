#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

static BLEServer *pServer = NULL;
static BLECharacteristic *characteristicMessage = NULL;
static String message = "";
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) {
        (void)server;
        deviceConnected = true;
        Serial.println("[bluetooth] Conectado!");
    };

    void onDisconnect(BLEServer *server) {
        (void)server;
        deviceConnected = false;
        Serial.println("[bluetooth] Desconectado.");
    }
};

class MessageCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
        std::string data = characteristic->getValue();
        message = data.c_str();
    }

    void onRead(BLECharacteristic *characteristic) {
        characteristic->setValue("Comunicacao OK");
    }
};

void bluetooth_init() {
    Serial.println("[bluetooth] Inicializando...");

    BLEDevice::init(DEVICE_NAME);
    BLEDevice::setMTU(517); // MTU maximo, evita erro GATT 133 em alguns celulares

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *service = pServer->createService(SERVICE_UUID);

    characteristicMessage = service->createCharacteristic(
        MESSAGE_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    characteristicMessage->setCallbacks(new MessageCallbacks());
    characteristicMessage->addDescriptor(new BLE2902());
    characteristicMessage->setValue("Pronto");

    service->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // ajuda com conexao em iOS
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();

    Serial.print("[bluetooth] Pronto, anunciando como \"");
    Serial.print(DEVICE_NAME);
    Serial.println("\"");
}

void bluetooth_resume() {
    message = "";
}

// Mensagens terminam com \r (mesmo protocolo do app usado com a Bia) -
// so retorna algo quando o terminador chegou, senao devolve string vazia.
String read_bluetooth_message() {
    String result = "";
    int index = message.indexOf('\r');
    if (index > 0) {
        result = message.substring(0, index);
        bluetooth_resume();
    }

    return result;
}

void send_bluetooth_message(String msg) {
    if (deviceConnected && characteristicMessage != NULL) {
        characteristicMessage->setValue(msg.c_str());
        characteristicMessage->notify();
        delay(10); // pequeno delay pra garantir o envio
    }
}

bool bluetooth_is_connected() {
    return deviceConnected;
}

// Chamar periodicamente (ex.: a cada loop()) - reinicia o advertising
// depois de uma desconexao, senao o robo some do scan do celular pra
// sempre depois da primeira conexao cair.
void bluetooth_check_connection() {
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // tempo pra stack BLE se resetar
        pServer->startAdvertising();
        Serial.println("[bluetooth] Aguardando nova conexao...");
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}

// Validacao de bancada: fica 60s esperando conexao e ecoando de volta
// qualquer mensagem recebida, prefixada com "ECO: ". Conectar com um
// app de BLE generico (ex. nRF Connect) e mandar algo terminado em \r.
void validar_bluetooth() {
    Serial.println("[validar_bluetooth] Aguardando conexao por 60s...");
    unsigned long start_time = millis();

    while (millis() - start_time < 60000) {
        bluetooth_check_connection();

        String received = read_bluetooth_message();
        if (received != "") {
            Serial.printf("[validar_bluetooth] Recebido: \"%s\"\n", received.c_str());
            send_bluetooth_message("ECO: " + received);
        }

        delay(20);
    }

    Serial.println("[validar_bluetooth] fim");
}
