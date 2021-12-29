/*
WiFiDisplay

This is example how to implement three HW modules: humidity sensor, wifi, OLED display (128x32)

The humidity sensor and OLED display is connected via I2C

Main function: provide sensor data on display and via simple HTML web page. The web page enables 
control one LED and display (on/off). If temperature is exceeded the second LED is lighting and web page shows warning.

Himidity sensors reads data (temperature, humidity, pressure) and in defined periods these data 
is send via WebSocket and also shown on the OLED display

I use WiFi manager which allows configures WiFi via AP and it stores configured WiFi settings

Data via WebSockets is send in very simple string temp|humid|press|LED_state|DISPLAY_state|WARNING_state

*/

String htmlHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
String htmlPage = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3' crossorigin='anonymous'>
  <style>
    body     { font-size:100%;} 
    .container{ margin-top: 30px;}
  </style>
 
  <title>Data</title>
  <style>
    html, body{
      background-color: #ff9900
    }
  </style>
</head>
<body>
   <div class='container mt-5'>
      <div class='row d-flex justify-content-center'>
         <div class='col-md-4 col-sm-6 col-xs-12'>
            <div class='card px-3 py-3' id='form1'>
               <div class='row'>
                  <div class='col'>
                     <p>Teplota: <strong><span id='temp'></span>°C</strong></p>
                     <p>Vlhkost: <strong><span id='humid'></span>%</strong></p>
                     <p>Tlak: <strong><span id='pres'></span>hPa</strong></p>
                  </div>
               </div>
               <div class='row'>
                  <div class='col text-center d-grid'>
                     <button class='btn btn-outline-primary btn-sm btn-block' id='btnLED' onclick='onLedClick()'>LED</button>
                  </div>
                  <div class='col text-center d-grid'>
                     <button class='btn btn-outline-info btn-sm btn-block' id='btnDisplay' onclick='onDisplayClick()'>DISPLAY</button>
                  </div>
               </div>
               <div class='row mt-3 mb-0' id='lblWarning'>
                  <div class='col'>
                     <div class='alert alert-danger'>Temperature is too high!</div>
                  </div>
               </div>
            </div>
         </div>
      </div>
   </div>
</body>
 
<script>
var Socket;
function init() 
{
  Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
  Socket.onmessage = function(event) { processReceivedCommand(event); };
}
 
function processReceivedCommand(evt) 
{
    var data = evt.data;
    var tmp = data.split('|');
    document.getElementById('temp').innerHTML = tmp[0];  
    document.getElementById('humid').innerHTML = tmp[1];
    document.getElementById('pres').innerHTML = tmp[2];
    // LED status
    if(tmp[3] == 1){
      document.getElementById('btnLED').innerHTML = "Turn OFF LED";
    }
    else{
      document.getElementById('btnLED').innerHTML = "Turn ON LED";
    }
    // Display status
    if(tmp[4] == 1){
      document.getElementById('btnDisplay').innerHTML = "Turn OFF display";
    }
    else{
      document.getElementById('btnDisplay').innerHTML = "Turn ON display";
    }
    if(tmp[5] == 1){
      document.getElementById('lblWarning').style.display = "block";
    }
    else{
      document.getElementById('lblWarning').style.display = "none";
    }
}
 
function sendText(data) { 
  Socket.send(data); 
}

function onLedClick(){
  sendText("LED");
}

function onDisplayClick(){
  sendText("DISPLAY");
}

window.onload = function(e) { 
  init(); 
}
</script>
 
</html>
)=====";


#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WebSocketsServer.h>
 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_BME280.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
// OLED display instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BMP280_I2C_ADDRESS  0x76   // Adresa I2C senzoru
// Humidity sensor instance
Adafruit_BME280  bmp;

// WiFi network name
const String wifiNetwork = "AutoConnectAP";
 
WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String line1 = "";
String line2 = "";
String line3 = "";
String line4 = "";

/*
0 = temperature
1 = humidity
2 = pressure
*/
float data_Sensor[3];
/*
0 = LED ON/OFF
1 = DISPLAY ON/OFF
2 = LED WARNING ON/OFF - only readonly
*/
boolean data_State[] = {0, 1, 0};

// Current time (in millis)
int period_TimeNow = 0;
// Setup of periods for tasks (in millis)
const int period_SensorRefresh = 100;
const int period_Send = 600;
const int period_Display = 400;
// Time in millis of last update
int previous_Sensor = 0;
int previous_Send = 0;
int previous_Display = 0;


// Pins for LEDs
const int LED_meeting = 12;
const int LED_warning = 14;

long sensorUpdateFrequency = 800;
long timeNow = 0;
long timePrev = 0;

void setup() {
  Serial.begin(115200);

  // Setup LED
  pinMode(LED_meeting, OUTPUT);
  pinMode(LED_warning, OUTPUT);

  // I2C start
  Wire.begin(); // Wire.begin use pins 4 a 5 as default

  // Start I2C displej
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Display default address is 0x3C
    Serial.println(F("SSD1306 alokace selhala"));
  }  

  // Start I2C sensor
  if(!bmp.begin(0x76)){  // Display default address is 0x76
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");  
  }

  printStart();

  WiFiManager wifiManager;

  // Enable to reset wifi settings. Or implement action on a button
  //wifiManager.resetSettings();
  
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
      client.print( htmlHeader );
      client.print( htmlPage ); 
      Serial.println("New page served");
  }

  handleLoop();
  
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
  if(data_State[0] == 0){
    data_State[0] = 1;
  }
  else{
    data_State[0] = 0;
  }
  digitalWrite(LED_meeting, data_State[0]);
}

void toggleDisplay(void){
  if(data_State[1] == 0){
    data_State[1] = 1;
    display.ssd1306_command(SSD1306_DISPLAYON);
  }
  else{
    data_State[1] = 0;
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
    Serial.println(incomingMsg);
    if(incomingMsg == "LED"){
      toggleLED();
    }
    else if(incomingMsg == "DISPLAY"){
      toggleDisplay();
    }
}

// Main loop program which handles sensor reading, display render and websocket sending
void handleLoop(){
  if(_canRefreshSensor()){
      float temperature = bmp.readTemperature();
      float humidity = bmp.readHumidity();
      float pressure = bmp.readPressure() / 100.0F;   
      data_Sensor[0] = temperature;
      data_Sensor[1] = humidity;
      data_Sensor[2] = pressure;
      handleWarning(temperature);
  }
  if(_canSend()){
      String msg = String(data_Sensor[0]) + "|" + String(data_Sensor[1]) + "|" + String(data_Sensor[2]) + "|" + String(data_State[0]) + "|" + String(data_State[1]) + "|" + String(data_State[2]);
      webSocket.broadcastTXT(msg);
  }
  if(_canRefreshDisplay() && data_State[1] == 1){ // Update display only if time exceeded and display is turned on
      line1 = "Teplota: " + String(data_Sensor[0]) + "C";
      line2 = "Vlhkost: " + String(data_Sensor[1]) + "%";
      line3 = "Tlak:    " + String(data_Sensor[2]) + "hPa";
      line4 = "";
      displayLines();
  }
}

void handleWarning(float value){
  if(value < 24 || value > 28){
    digitalWrite(LED_warning, HIGH);
    data_State[2] = 1;
  }
  else{
    digitalWrite(LED_warning, LOW);
    data_State[2] = 0;
  }
}

boolean _canRefreshSensor(){
  timeNow = millis();
  if (timeNow - previous_Sensor >= period_SensorRefresh){
    previous_Sensor = timeNow;
    return true;
  }
  return false;
}

boolean _canSend(){
  timeNow = millis();
  if (timeNow - previous_Send >= period_Send){
    previous_Send = timeNow;
    return true;
  }
  return false;
}

boolean _canRefreshDisplay(){
  timeNow = millis();
  if (timeNow - previous_Display >= period_Display){
    previous_Display = timeNow;
    return true;
  }
  return false;
}
