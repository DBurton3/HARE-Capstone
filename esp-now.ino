#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 2;

// MAC address of ESP32 #1
uint8_t PEER_MAC[] = { 0xb0, 0xcb, 0xd8, 0xce, 0x12, 0xb4};

//uint8_t PEER_MAC[] = { 0xe0, 0x8c, 0xfe, 0x57, 0x80, 0x54 };



typedef struct {
  char message[20];
} Message;

Message outgoingMessage;
Message incomingMessage;

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 1000;

unsigned long lastReceivedTime = 0;
const unsigned long CONNECTION_TIMEOUT = 3000;

bool peerConnected = false;


// --------------------------------------------------
// RECEIVE
// --------------------------------------------------
void OnDataRecv(const esp_now_recv_info_t *recvInfo,
                const uint8_t *incomingData,
                int len) {

  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));

  Serial.print("Received from peer: ");
  Serial.println(incomingMessage.message);

  lastReceivedTime = millis();

  if (!peerConnected) {
    peerConnected = true;
    Serial.println("PEER DETECTED!");
  }

  // Blink LED
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}


// --------------------------------------------------
// SEND
// --------------------------------------------------
void OnDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status) {

  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Heartbeat sent");
  }
  else {
    Serial.println("Heartbeat FAILED");
  }
}


void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);

  Serial.println();

  Serial.print("My MAC address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Add ESP32 #1
  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, PEER_MAC, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer!");
    return;
  }

  Serial.println("Peer added.");
  Serial.println("Waiting for other ESP32...");
}


void loop() {

  // Send heartbeat
  if (millis() - lastSendTime >= SEND_INTERVAL) {

    lastSendTime = millis();

    strcpy(outgoingMessage.message, "HELLO");

    esp_err_t result = esp_now_send(
      PEER_MAC,
      (uint8_t *)&outgoingMessage,
      sizeof(outgoingMessage)
    );

    if (result != ESP_OK) {
      Serial.println("Error sending heartbeat");
    }
  }


  // Detect lost connection
  if (peerConnected &&
      millis() - lastReceivedTime > CONNECTION_TIMEOUT) {

    peerConnected = false;

    Serial.println("PEER LOST");

    digitalWrite(LED_PIN, LOW);
  }
}