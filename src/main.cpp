#include <main.h>
#include <pinconfig.h>

/*
 * Arbeidskrav 1
 * 
 *  Author: Erik Alvarez
 *  Description: Mottaker for sensordata (simulerer en gateway) 
 *  Based on:

  LoRa Duplex communication

  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Uses readString() from Stream class to read payload. The Stream class'
  timeout may affect other functuons, like the radio's callback. For an

  created 28 April 2017
  by Tom Igoe
*/

#define LED 2

byte msgCount = 0; // count of outgoing messages

// Ranges in this project
// 0xEA->0xEF (0xEF is gateway in this example)
byte localAddress = 0xEF;  // address of this device
byte destination = 0xFF;   // destination to send to
long lastSendTime = 0;     // last send time
int interval = 5000;       // interval between sends
boolean connected = false; //Connected state

// WiFi network name and password:
const char *networkName = "Garnes Data Gjestenett";
const char *networkPswd = "167GarnesData";

// Device Info
const char *deviceInfo = "sensorgateway_1";

// Azure IoThub
bool iotConnected = false;
const char *connectionString = "HostName=Fresh.azure-devices.net;DeviceId=sensorgateway_1;SharedAccessKey=wBz8xWKaZee9+yp4j7BSi6/PSHWAhGSVR+WgFksMZHY=";
const char *secondaryConnectionString = "HostName=Fresh.azure-devices.net;DeviceId=sensorgateway_1;SharedAccessKey=CZ7nh0V8eLnzFjRmNZtNxX4pGrGxqh1EVdJELcj0Ks8=";
// out default message template for our iot hub state
const char *messageData = "{\"deviceId\":\"%d\", \"messageId\":%d, \"Temperature\":%f,\"Humidity\":%d, \"AmbientTemperature\":%d}";

void setup()
{
  Serial.begin(9600); // initialize serial
  //while (!Serial)
  //  ;

  pinMode(LED, OUTPUT);
  Serial.println("Lora Duplex connect WiFi");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);

  if (!LoRa.begin(FREQ))
  { // initialize ratio at 866 Mhz
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ; // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.print("My address: 0x");
  Serial.println(localAddress, HEX);

  delay(500);
  connectToWiFi(networkName, networkPswd);
}

void wifi_led_status()
{
  const int ledBlinkTime = 50;
  static int old_millis = 0;
  static bool isOn = false;
  if (millis() - old_millis > 1000 + ledBlinkTime)
  {
    isOn = false;
    digitalWrite(LED, LOW);
    old_millis = millis();
  }
  else if (millis() - old_millis > 1000 && !isOn)
  {
    isOn = true;
    if (WiFi.isConnected())
    {
      digitalWrite(LED, HIGH);
    }
  };
}
void loop()
{
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
  wifi_led_status();
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();            // start packet
  LoRa.write(destination);       // add destination address
  LoRa.write(localAddress);      // add sender address
  LoRa.write(msgCount);          // add message ID
  LoRa.write(outgoing.length()); // add payload length
  LoRa.print(outgoing);          // add payload
  LoRa.endPacket();              // finish packet and send it
  msgCount++;                    // increment message ID
}

// Callback from LoraWAN
void onReceive(int packetSize)
{
  if (packetSize == 0)
    return; // if there's no packet, return

  // read packet header bytes:
  byte recipient = LoRa.read();      // recipient address
  byte sender = LoRa.read();         // sender address
  byte incomingMsgId = LoRa.read();  // incoming msg ID
  byte incomingLength = LoRa.read(); // incoming msg length

  char incoming[sizeof(temp_values)];
  int i = 0;
  while (LoRa.available() && i <= sizeof(temp_values))
  {
    incoming[i] = (char)LoRa.read();
    i++;
  }

  temp_values t;
  memcpy(t.bytes, incoming, sizeof(temp_values));
  double dcheck = (t.payload.humidity * t.payload.ambient_temperature * t.payload.temperature);
  if (t.payload.crc != dcheck)
  {
    Serial.print("val:");
    Serial.print(dcheck - t.payload.crc);
    Serial.print(" - dchk:");
    Serial.print(dcheck);
    Serial.print(" - crc:");
    Serial.print(t.payload.crc);
    Serial.print(" CRC error packet:");
  }
  delay(100);
  digitalWrite(LED, HIGH);
  Serial.print("Received from: 0x" + String(sender, HEX));
  Serial.print("RSSI: " + String(LoRa.packetRssi()));
  Serial.print(" - \"");
  Serial.print("temp1: ");
  Serial.print(t.payload.temperature);
  Serial.print("\ttemp2: ");
  Serial.print(t.payload.ambient_temperature);
  Serial.print("\thumidity: ");
  Serial.print(t.payload.humidity);
  Serial.println(".");
  sendAzureEvent(sender,t);
  Serial.println("\"");
  delay(100);
  digitalWrite(LED, LOW);
  return; // skip rest of function
}

void connectToWiFi(const char *ssid, const char *pwd)
{
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);

  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}
void sendAzureEvent(byte device, temp_values values)
{
  Serial.println("Sending msg from " + String(device) + String(values.payload.temperature));
  char buff[130];
  // string deviceId
  // int messageId
  // float  Temperature
  // byte Humidity
  // byte AmbientTemperature
  if (iotConnected)
  {
    int messageId = millis();
    snprintf(buff, 128, messageData,
             device,
             messageId,
             values.payload.temperature,
             values.payload.humidity,
             values.payload.ambient_temperature);
    Serial.println("*** Sending MQTT Message: \"" + String(buff) + "\"");
    if (!Esp32MQTTClient_SendEvent(buff)) {
      Serial.println(MSG_FAILSENDAZURE);
    };
  }
  else
  {
    startAzureListener();
  };
}
void startAzureListener()
{
  if (!Esp32MQTTClient_Init((const uint8_t *)connectionString))
  {
    iotConnected = false;
  }
  iotConnected = true;
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    //When connected set
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    startAzureListener();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    //connected = false;
    break;
  }
}