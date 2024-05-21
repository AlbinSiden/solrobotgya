#include <Servo.h>

#include <SoftwareSerial.h>
#include "WiFiEsp.h"

SoftwareSerial Serial1(6, 7);

char ssid[] = "ssid";
char pass[] = "lösenord";
int status = WL_IDLE_STATUS;

String apiEndpoint = "/save";
String apiHost = "172.20.10.4";
int apiPort = 5000;

#define SERVO_X 10
#define SERVO_Y 9

Servo xServo;
Servo yServo;

int threshold = 10;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);

  WiFi.init(&Serial1);

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000); 
  }

  Serial.println("Connected to WiFi");

  xServo.attach(SERVO_X);
  yServo.attach(SERVO_Y);
}


WiFiEspClient connectToAPIServer() {
  WiFiEspClient client;

  client.connect(apiHost.c_str(), apiPort);
  Serial.println("Connected to server");

  return client;
}

void makePostRequest(float voltage, float current, WiFiEspClient &client) {  
    String postData = "{\"voltage\":" + String(voltage, 2) + ",\"current\":" + String(current, 2) + "}";
    String postRequest = "POST " + apiEndpoint + " HTTP/1.1\r\n";
    postRequest += "Host: " + apiHost + "\r\n";
    postRequest += "Content-Type: application/json\r\n";
    postRequest += "Content-Length: " + String(postData.length()) + "\r\n";
    postRequest += "\r\n";
    postRequest += postData;

    // Send the POST request
    client.print(postRequest);
    Serial.println("Sent data to API");   

    client.stop();
}



void printLDR(int pin) {
  float volt = analogRead(pin);
  Serial.println(volt);
}

float getSolarPanelVoltage() {
  int voltageReading = analogRead(4);
  float voltageDivider = (1000 + 1000) / 1000;
  float realVoltage = (voltageReading / 1024.0) * 5.0 * voltageDivider;
  return realVoltage;
}

void followLight() {
  int ldrTopRight = analogRead(0);
  int ldrTopLeft = analogRead(1);
  int ldrBotRight = analogRead(2);
  int ldrBotLeft = analogRead(3);

  int diffInY = (ldrTopRight + ldrTopLeft) - (ldrBotRight + ldrBotLeft);
  int diffInX = (ldrTopRight + ldrBotRight) - (ldrTopLeft + ldrBotLeft);

  if (abs(diffInX) >= threshold) {
    int newXPosition = xServo.read() + map(diffInX, -1023, 1023, -5, 5);
    newXPosition = constrain(newXPosition, 0, 180);
    xServo.write(newXPosition);
  }

  if (abs(diffInY) >= threshold) {
    int newYPosition = yServo.read() + map(diffInY, -1023, 1023, -5, 5);
    newYPosition = constrain(newYPosition, 0, 180);
    yServo.write(newYPosition);
  }
}



void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

float readCurrent(float voltage) {
  int sensorValue = analogRead(A5);  // Read the raw ADC value from the ACS712
  float current = abs(voltage - (5.0 / 2.0)) / 0.185;  // Calculate current using the sensitivity of the ACS712
  return current;
}


unsigned long previousMillisFollow = 0;
unsigned long previousMillisPost = 0;

void loop() {
    unsigned long currentMillis = millis();

    // Run followLight every 300 milliseconds
    if (currentMillis - previousMillisFollow >= 300) {
        previousMillisFollow = currentMillis;
        followLight();
    }

    // Run makePostRequest once every ten minutes (600,000 milliseconds)
    if (currentMillis - previousMillisPost >= 30000) {
        previousMillisPost = currentMillis;
        float voltage = getSolarPanelVoltage();
        float current = readCurrent(voltage);
        WiFiEspClient apiClient = connectToAPIServer();
        makePostRequest(voltage, current, apiClient);  // Assuming current is 0.0 for simplicity
    }
}
