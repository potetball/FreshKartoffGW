#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Esp32MQTTClient.h>
//#include <AzureIotHub.h>

union temp_values {
  struct {
    float crc; // Control check
    float temperature;
    byte ambient_temperature;
    byte humidity;
  } payload;
    uint8_t bytes[12];
};



void connectToWiFi(const char *ssid, const char *pwd);
void WiFiEvent(WiFiEvent_t event);
void sendMessage(String message);
void onReceive(int packetSize);
void sendAzureEvent(byte device, temp_values values);
void startAzureListener();
void wifi_led_status();

#define MSG_FAILSENDAZURE "*** Error sending Azure message"
