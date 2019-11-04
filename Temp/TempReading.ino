#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>
#include <WiFiEspUdp.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(2, 4, 5, 6, 7, 8);
WiFiEspClient espClient;
PubSubClient client(espClient);
SoftwareSerial ESPserial(10, 11); // RX | TX

#define WIFI_AP "XXXXX"
#define WIFI_PASSWORD "XXXXX"

int status = WL_IDLE_STATUS;
const int TEMP_PIN = A2;
const int READINGS_PER_MIN = 60;

const char* mqttServer = "xxxxx";
const int mqttPort = 12345;
const char* mqttUser = "xxxxx";
const char* mqttPassword = "xxxxx";

float averageTemp;

void showOnDisplay(String firstRow, String secondRow, bool clearAll = false) {
  if (clearAll) {
    lcd.clear();
  }

  if (firstRow.length() > 0) {
    lcd.setCursor(0, 0);
    for (int i = 0; i < 16; i++) {
      lcd.print(" ");
    }
    lcd.setCursor(0,0);
    lcd.print(firstRow);
  }

  if (secondRow.length() > 0) {
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      lcd.print(" ");
    }
    lcd.setCursor(0,1);
    lcd.print(secondRow);
  }
}

void setup() {
  analogWrite(3, 75);
  lcd.begin(16, 2);
  showOnDisplay("RTS | IBU", "Amel Spahic", false);
  initWiFi();
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  checkWifiStatusAndConnect();

  if (!client.connected()) {
    reconnect();
  }

  // Task: Getting temp samples
  float allReadings = getTemperatureReadings();

  // Task: Find average
  float average = getAverage(allReadings);

  // Task: Printing and publishing
  publishResults(average);

  client.loop();
}

// Getting average value
float getAverage(float total) {
  return total / READINGS_PER_MIN;
}


float getTemperatureReadings() {
  showOnDisplay("Measuring avg...", "");

  // Getting delay base on readings per minute (60000 ms in 1 minute divided by readings per minute)
  int delayMs = 60000 / READINGS_PER_MIN;
  int counter = 0;

  float totalTemp = 0;

  // We will reading temperature based on readings per minute
  while (counter < READINGS_PER_MIN) {
    int tempAdcVal = analogRead(TEMP_PIN);
    float tempVal = (tempAdcVal * 4.88); // Since we are providing 5V == 5.000 mV divided by converted digital reading of voltage 1024)
    tempVal = (tempVal / 10); // LM35 cheatsheet - every 1 C degree is 10mV
    totalTemp = totalTemp + tempVal; // Getting total temperature value for 1 minute
    showOnDisplay("", "Current: " + String(tempVal));
    counter++;
    delay(delayMs);
  }

  return totalTemp;
}

void checkWifiStatusAndConnect() {
  status = WiFi.status();
  if (status != WL_CONNECTED) {
    while (status != WL_CONNECTED) {
      showOnDisplay("Connecting to", WIFI_AP, true);
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      // Wait a bit
      delay(500);
    }
    showOnDisplay("Connected to:", WIFI_AP, true);
  }
}

void publishResults(float average) {
  char result[8];
  dtostrf(average, 6, 2, result);
  showOnDisplay("Average: " + String(average), "", false);

  if (!client.connected()) {
    reconnect();
  }

  // Topic on which we will subscribe
  client.publish("arduino/rts/temperature", result);
}

// If not connected, try every 5 seconds
void reconnect() {
  while (!client.connected()) {
    showOnDisplay("", "Connecting MQTT");
    if (client.connect("ArduinoSensor", mqttUser, mqttPassword) ) {
      showOnDisplay("", "CONNECTED MQTT");
    } else {
      showOnDisplay("", "FAILED");
      showOnDisplay("", "RETRYING IN 5 s");
      delay(5000);
    }
  }
}

void initWiFi()
{
  ESPserial.begin(9600);
  WiFi.init(&ESPserial);
  if (WiFi.status() == WL_NO_SHIELD) {
    showOnDisplay("WiFi shield", "not present", true);
    while (true);
  }

  showOnDisplay("Connecting WiFi", WIFI_AP, true);
  while ( status != WL_CONNECTED) {
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  showOnDisplay("Connected to", WIFI_AP, true);
}
