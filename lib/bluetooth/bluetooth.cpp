#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static BLEServer *pServer = NULL;
static BLECharacteristic *characteristicMessage = NULL;
static String message = "";

// message e escrito pela task da pilha BLE (dentro de MessageCallbacks::
// onWrite(), que roda numa task PROPRIA do Bluetooth, diferente da task
// que roda o loop() do Arduino) e lido pela task principal (dentro de
// read_bluetooth_message(), chamada por Robot::run()). Sem protecao, uma
// escrita (que pode realocar o buffer interno do String) acontecendo ao
// mesmo tempo que uma leitura (.indexOf()/.substring()) e uma condicao
// de corrida real - pode corromper memoria/travar, nao so ler valor
// desatualizado. Achado em revisao de codigo, 22/07/2026. Usamos um
// mutex do FreeRTOS (nao um spinlock/critical section) porque as
// operacoes de String podem alocar memoria, o que nao e seguro dentro
// de uma critical section.
static SemaphoreHandle_t message_mutex = NULL;

// deviceConnected e escrito pela task do BLE (onConnect/onDisconnect) e
// lido pela task principal - volatile garante que o compilador nao
// guarde um valor antigo em registrador entre as duas tasks.
static volatile bool deviceConnected = false;
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
        xSemaphoreTake(message_mutex, portMAX_DELAY);
        message = data.c_str();
        xSemaphoreGive(message_mutex);
    }

    void onRead(BLECharacteristic *characteristic) {
        characteristic->setValue("Comunicacao OK");
    }
};

void bluetooth_init() {
    Serial.println("[bluetooth] Inicializando...");

    message_mutex = xSemaphoreCreateMutex();

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
    xSemaphoreTake(message_mutex, portMAX_DELAY);
    message = "";
    xSemaphoreGive(message_mutex);
}

// Mensagens terminam com \r (mesmo protocolo do app usado com a Bia) -
// so retorna algo quando o terminador chegou, senao devolve string vazia.
// NAO chama bluetooth_resume() aqui dentro (deixaria o mutex - que nao e
// reentrante - travado esperando ele mesmo) - limpa direto.
String read_bluetooth_message() {
    String result = "";

    xSemaphoreTake(message_mutex, portMAX_DELAY);
    int index = message.indexOf('\r');
    if (index > 0) {
        result = message.substring(0, index);
        message = "";
    }
    xSemaphoreGive(message_mutex);

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

static unsigned long disconnected_at = 0;
static bool waiting_to_readvertise = false;

// Chamar periodicamente (ex.: a cada loop()) - reinicia o advertising
// depois de uma desconexao, senao o robo some do scan do celular pra
// sempre depois da primeira conexao cair. NAO usa delay() (usava antes -
// travava o loop inteiro, incluindo o PID em corrida, por meio segundo
// toda vez que o Bluetooth caia - achado em revisao de codigo,
// 22/07/2026): espera os mesmos ~500ms pro stack BLE se resetar, mas sem
// bloquear nada enquanto isso.
void bluetooth_check_connection() {
    if (!deviceConnected && oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        disconnected_at = millis();
        waiting_to_readvertise = true;
    }

    if (waiting_to_readvertise && (millis() - disconnected_at >= 500)) {
        pServer->startAdvertising();
        Serial.println("[bluetooth] Aguardando nova conexao...");
        waiting_to_readvertise = false;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        waiting_to_readvertise = false; // reconectou antes do timer completar
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
