#define BLYNK_PRINT Serial

/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPLyXkCfiKb"
#define BLYNK_TEMPLATE_NAME "ESE3500 Lab 4"
#define BLYNK_AUTH_TOKEN "DOpuaQzwvyPykiSPOuxQ0YtdFUlYkySO"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Emily Phone";
char pass[] = "12345678";

int sensorValue;

BLYNK_WRITE(V2) {
  sensorValue = param.asInt();
  analogWrite(A0, sensorValue);
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
}

void loop()
{
  Blynk.run(); 

  Serial.print("Sensor value: ");
  Serial.println(sensorValue);
}
