#include <Arduino.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Esp32MQTTClient.h>

void connectToWiFi(const char *ssid, const char *pwd);
void WiFiEvent(WiFiEvent_t event);
void sendMessage(String message);
void onReceive(int packetSize);
