/*
WiFiDisplay

This is example how to implement following HW modules: air quality sensor, humidity sensor, wifi, OLED display (128x32)

The air quality sensor, humidity sensor and OLED display is connected via I2C

Main function: provide sensor data on display and via simple HTML web page. The web page enables 
control one LED and display (on/off). If temperature is exceeded the second LED is lighting and web page shows warning.

Data from air quality sensor (VOC index) and humidity sensor(temperature, humidity, pressure) are read in defined periods. This data 
is send via WebSocket and also shown on the OLED display

I use WiFi manager which allows configures WiFi via AP and it stores configured WiFi settings

Data via WebSockets is send in very simple string temp|humid|press|voc index|LED_state|DISPLAY_state|WARNING_state

Copy folder SensorData to /libraries folder

*/


#include "htmlDashboard.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WebSocketsServer.h>
#include <SensorData.h>
 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_BME280.h>

#include <Adafruit_SGP40.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
// OLED display instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BMP280_I2C_ADDRESS  0x76   // Adresa I2C senzoru
// Humidity sensor instance
Adafruit_BME280  bmp;
// Air quality sensor instance
Adafruit_SGP40 sgp;

// WiFi network name
const String wifiNetwork = "WifiSensorAP";
 
WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String line1 = "";
String line2 = "";
String line3 = "";
String line4 = "";

SensorData sensorData;

/*
0 = LED ON/OFF
1 = DISPLAY ON/OFF
2 = LED WARNING ON/OFF - only readonly
*/
boolean data_State[] = {0, 1, 0};

// Current time (in millis)
unsigned long period_TimeNow = 0;
// Setup of periods for tasks (in millis)
const int period_SensorRefresh = 100;
const int period_Send = 600;
const int period_Display = 400;
// Time in millis of last update
unsigned long previous_Sensor = 0;
unsigned long previous_Send = 0;
unsigned long previous_Display = 0;

// Pins for LEDs
const int LED_meeting1 = 12;
const int LED_meeting2 = 13;
const int LED_warning = 14;

unsigned blinkTimePrev = 0;

WiFiManager wifiManager;

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
  Serial.begin(115200);

  // Setup LED
  pinMode(LED_meeting1, OUTPUT);
  pinMode(LED_meeting2, OUTPUT);
  pinMode(LED_warning, OUTPUT);

  // I2C start
  Wire.begin(); // Wire.begin use pins 4 a 5 as default

  // Start I2C displej
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Display default address is 0x3C
    Serial.println(F("Could not find a valid SSD1306 sensor, check wiring!"));
    while (1);
  }  

  // Start I2C sensor
  if(!bmp.begin(0x76)){  // Display default address is 0x76
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");  
    while (1);
  }

  // Start I2C air quality sensor
  if (! sgp.begin()){
    Serial.println("Could not find a valid SGD sensor, check wiring!");
    while (1);
  }

  printStart();
  
  wifiManager.autoConnect((const char*)wifiNetwork.c_str());
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  Serial.println(WiFi.localIP());

  // Start server
  server.begin();

  // Start websocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Show network info (IP and MAC) for a while
  printNetworkInfo();
  
}
 
void loop() {

  webSocket.loop();
 
  WiFiClient client = server.available();     // Check if a client has connected
  if (client)
  {
      client.flush();
      client.print( htmlPageHeader );
      client.print( htmlPageBody ); 
      Serial.println("New page served");
  }

  handleLoop();

  if(sensorData.stateWarning == 1){
    unsigned long currentMillis = millis();
    if(currentMillis - blinkTimePrev >= 1000){
      blinkTimePrev = currentMillis;
      if(digitalRead(LED_warning) == HIGH){
        digitalWrite(LED_warning, LOW);
      }
      else{
        digitalWrite(LED_warning, HIGH);
      }
    }
  }
  
}

void prepareDisplay(void){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);
  display.dim(true);
}

void displayLines(){
  prepareDisplay();
  display.println(line1);
  display.println(line2);
  display.println(line3);
  display.println(line4);
  display.display();
}

void printNetworkInfo(void) {

  line1 = "IP adresa:";
  line2 = WiFi.localIP().toString().c_str();
  line3 = "MAC:";
  line4 = WiFi.macAddress();
  displayLines();
  delay(4000);
}


void printStart(void) {
  
  if(!WiFi.localIP()){    
    line1 = "Neni pripojeno k WIFI";
    line2 = "Vyber Wifi:";
    line3 = wifiNetwork;
    line4 = "";
  }
  displayLines();  
}

void toggleLED(void){
  if(sensorData.stateLed == 0){
    //data_State[0] = 1;
    sensorData.stateLed = 1;
  }
  else{
    //data_State[0] = 0;
    sensorData.stateLed = 0;
  }
  digitalWrite(LED_meeting1, sensorData.stateLed);
  digitalWrite(LED_meeting2, sensorData.stateLed);
}

void toggleDisplay(void){
  if(sensorData.stateDisplay == 0){
    //data_State[1] = 1;
    sensorData.stateDisplay = 1;
    display.ssd1306_command(SSD1306_DISPLAYON);
  }
  else{
    //data_State[1] = 0;
    sensorData.stateDisplay = 0;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
}

// Incoming events from websocket clients
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length)
{
    String incomingMsg = "";
    for(int i = 0; i < length; i++) { 
      incomingMsg += (char) payload[i]; 
    }
    if(incomingMsg == "LED"){
      toggleLED();
    }
    else if(incomingMsg == "DISPLAY"){
      toggleDisplay();
    }
    else if(incomingMsg == "RESET_WIFI"){
      // Enable to reset wifi settings. Or implement action on a button
      wifiManager.resetSettings();
      resetFunc();
    }
}

// Main loop program which handles sensor reading, display render and websocket sending
void handleLoop(){
  if(_canRefreshSensor()){
      float temperature = bmp.readTemperature();
      float humidity = bmp.readHumidity();
      float pressure = bmp.readPressure() / 100.0F;   
      int vocIndex = _getAirQualityIndex(temperature, humidity);
     
      sensorData.temperature = temperature;
      sensorData.humidity = humidity;
      sensorData.pressure = pressure;
      sensorData.VOC = vocIndex;
      sensorData.label = _getAirQualityText(vocIndex);
      
      handleWarning(vocIndex);
  }
  if(_canSend()){            
      webSocket.broadcastTXT(sensorData.toJson().c_str());
  }
  if(_canRefreshDisplay() && data_State[1] == 1){ // Update display only if time exceeded and display is turned on
      line1 = "Air: " + sensorData.label;
      line2 = "Temperature: " + String(sensorData.temperature) + "C";
      line3 = "Humidity:   " + String(sensorData.humidity) + "%";
      line4 = "Pressure:   " + String(sensorData.pressure) + "hPa";      
      displayLines();
  }
}

void handleWarning(float airQuality){
  if(airQuality > 200){
    //digitalWrite(LED_warning, HIGH);
    //data_State[2] = 1;
    sensorData.stateWarning = 1;
  }
  else{
    digitalWrite(LED_warning, LOW); // To ensure that warning LED is turned off
    //data_State[2] = 0;
    sensorData.stateWarning = 0;
  }
}

int32_t _getAirQualityIndex(float temperature, float humidity){  
  return sgp.measureVocIndex(temperature, humidity);
}

String _getAirQualityText(int32_t vocIndex){
  //https://www.breeze-technologies.de/blog/calculating-an-actionable-indoor-air-quality-index/
  if(vocIndex == 0){
    return "Loading";
  }
  else if(vocIndex > 0 && vocIndex <= 50){
    return "Excellent (1)";
  }
  else if(vocIndex > 50 && vocIndex <= 100){
    return "Very good (2)";
  }
  else if(vocIndex > 100 && vocIndex <= 150){
    return "Good (3)";
  }
  else if(vocIndex > 150 && vocIndex <= 200){
    return "Fair (4)";
  }
  else if(vocIndex > 200 && vocIndex <= 300){
    return "Poor (5)";
  }
  else if(vocIndex > 300 && vocIndex <= 500){
    return "Very bad (6)";
  }
  else{
    return "Unknown";
  }
}

boolean _canRefreshSensor(){
  unsigned long timeNow = millis();
  if (timeNow - previous_Sensor >= period_SensorRefresh){
    previous_Sensor = timeNow;
    return true;
  }
  return false;
}

boolean _canSend(){
  unsigned long timeNow = millis();
  if (timeNow - previous_Send >= period_Send){
    previous_Send = timeNow;
    return true;
  }
  return false;
}

boolean _canRefreshDisplay(){
  unsigned long timeNow = millis();
  if (timeNow - previous_Display >= period_Display){
    previous_Display = timeNow;
    return true;
  }
  return false;
}
