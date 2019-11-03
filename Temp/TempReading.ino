#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>
#include <WiFiEspUdp.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>

WiFiEspClient espClient;
PubSubClient client(espClient);
SoftwareSerial ESPserial(10, 11); // RX | TX

#define WIFI_AP "Amel"
#define WIFI_PASSWORD "RijesiZadataK!"

int status = WL_IDLE_STATUS;
const int TEMP_PIN = A2;  /* LM35 O/P pin */
const int READINGS_PER_MIN = 60;
//unsigned long lastSend;

const char* mqttServer = "farmer.cloudmqtt.com";
const int mqttPort = 13691;
const char* mqttUser = "ogsoispv";
const char* mqttPassword = "Udp8SAfuevEN";

void setup() {
  Serial.begin(9600);
  initWiFi();
  client.setServer(mqttServer, mqttPort);
  //  lastSend = 0;
}

void loop() {
  checkWifiStatusAndConnect();

  if (!client.connected()) {
    reconnect();
  }

  // Zadatak semplovanja
  float allReadings = getTemperatureReadings();

  // Zadatak usrednjavanja
  float average = getAverage(allReadings);

  // Zadatak prikaza
  publishResults(average);

  client.loop();
}

float getAverage(float total) {
  return total / READINGS_PER_MIN;
}

float getTemperatureReadings() {
  Serial.println("Measuring...");
  int delayMs = 60000 / READINGS_PER_MIN;
  int counter = 0;

  float totalTemp = 0;

  while (counter < READINGS_PER_MIN) {
    int tempAdcVal = analogRead(TEMP_PIN);
    float tempVal = (tempAdcVal * 4.88);
    tempVal = (tempVal / 10);
    totalTemp = totalTemp + tempVal;

    // Prikaz na displeju
    Serial.print("Trenutna: ");
    Serial.println(tempVal);

    counter++;
    delay(delayMs);
  }

  return totalTemp;
}

void checkWifiStatusAndConnect() {
  status = WiFi.status();
  if (status != WL_CONNECTED) {
    while (status != WL_CONNECTED) {
      Serial.print("Connecting to AP: ");
      Serial.println(WIFI_AP);
      WiFi.mode(WIFI_STA);
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("Connected to WIFI");
  }
}

void publishResults(float average) {
  char result[8];
  dtostrf(average, 6, 2, result);

  // Prikaz na displeju
  Serial.print("Prosječna: ");
  Serial.println(average);

  if (!client.connected()) {
    reconnect();
  }

  client.publish("average_temperature", result);
}

// If not connected, try every 5 seconds
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT ...");
    if (client.connect("ArduinoSensor", mqttUser, mqttPassword) ) {
      Serial.println( "[CONNECTED]" );
    } else {
      Serial.print("[FAILED] [ state = ");
      Serial.print(client.state());
      Serial.println(" : retrying in 5 seconds]");
      delay(5000);
    }
  }
}

void initWiFi()
{
  // initialize serial for ESP module
  ESPserial.begin(9600);
  // initialize ESP module
  WiFi.init(&ESPserial);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("Connected to AP");
}
