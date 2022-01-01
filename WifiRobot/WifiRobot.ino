/*
  This ESP8266 Web Socket Remote based on:
  https://github.com/tzapu/WiFiManager
  https://github.com/Links2004/arduinoWebSockets
  https://gist.github.com/rjrodger/1011032
*/
#define AP_NAME "WifiCar"

// Pleaee use PWM pins connect to motor drive
#define RightMotorSpeed 5
#define RightMotorDir 0
#define LeftMotorSpeed 4
#define LeftMotorDir 2

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>

#include "DifferentialSteering.h"

#define echoPin 14 // attach pin D5 Arduino to pin Echo of HC-SR04
#define trigPin 12 //attach pin D6 Arduino to pin Trig of HC-SR04

#define MIN_DISTANCE 15

int distance;

String htmlBody = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta name='viewport' content='user-scalable=no,initial-scale=1.0,maximum-scale=1.0' />
        <style>
            body {
                padding: 0 24px 0 24px;
                background-color: #ccc;
            }
            #main {
                margin: 0 auto 0 auto;
            }
        </style>
        <script>
      var WIDTH = 255;
      var HEIGHT = 255;
            function normalize(val) { 
              var max = 100;
              var min = 0;
              return ((val - min) / (max - min))*max; 
            }
            function nw() {
                return new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
            }
            var ws = nw();
            window.onload = function () {
                document.ontouchmove = function (e) {
                    e.preventDefault();
                };
                var cv = document.getElementById('main');
                var ctop = cv.offsetTop;
                var c = cv.getContext('2d');
                function clr() {
                    c.fillStyle = '#fff';
                    c.rect(0, 0, WIDTH, HEIGHT);
                    c.fill();
                }
                function t(e) {
                    e.preventDefault();
                    var x,
                        y,
                        u = e.touches[0];
                    if (u) {
                        x = u.clientX;
                        y = u.clientY - ctop;
                        c.beginPath();
                        c.fillStyle = 'rgba(96,96,208,0.5)';
                        c.arc(x, y, 5, 0, Math.PI * 2, true);
                        c.fill();
                        c.closePath();
                    } else {
                        x = Math.round(WIDTH / 2);
                        y = Math.round(HEIGHT / 2);
                    }
          var offsetX = Math.round(WIDTH / 2);
          var offsetY = Math.round(HEIGHT / 2);
          x = (x - offsetX);
          y = -(y - offsetY);
          x = normalize(x);
          y = normalize(y);
                    //x = '000' + x.toString(16);
                    //y = '000' + y.toString(16);
                    if (ws.readyState === ws.CLOSED) {
                        ws = nw();
                    }
                    ws.send(x + ";" + y);
          
                }
                cv.ontouchstart = function (e) {
                    t(e);
                    clr();
                };
                cv.ontouchmove = t;
                cv.ontouchend = t;
                clr();
            };
        </script>
    </head>
    <body>
        <h2>ESP TOUCH REMOTE</h2>
        <canvas id='main' width='255' height='255'></canvas>
    </body>
</html>

)=====";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int noActionLimit = 16;
int minRange = -127;
int maxRange = 127;
int fPivYLimit = 32;
DifferentialSteering DiffSteer;

String getValueSplitString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void motorControl(int left, int right){

  int dirRight = right < 0;
  int dirLeft = left > 0;

  if(distance < MIN_DISTANCE && dirLeft == 1 && dirRight == 0){
    analogWrite(RightMotorSpeed, 0);
    analogWrite(LeftMotorSpeed, 0);
  }
  else{
    int speedRight = 150 + abs(right);
    int speedLeft = 150 + abs(left);
  
    if(abs(left) < 5){
      speedLeft = 0;
    }
    if(abs(right) < 5){
      speedRight = 0;
    }
    
    digitalWrite(RightMotorDir, dirRight); 
    analogWrite(RightMotorSpeed, speedRight);
    digitalWrite(LeftMotorDir, dirLeft);
    analogWrite(LeftMotorSpeed, speedLeft);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    motorControl(0, 0);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  }
  break;
  case WStype_TEXT:  

    _ultrasonic();

    String data = (char*)payload;
    int XValue = getValueSplitString(data,';',0).toInt();
    int YValue = getValueSplitString(data,';',1).toInt();
  
    int lowLimit = 5;
    int highLimit = 5;

    int leftMotor = 0;
    int rightMotor = 0;

    // Outside no action limit joystick
    if (!((XValue > lowLimit) && (XValue < highLimit) && (YValue > lowLimit) && (YValue < highLimit)))
    {
        DiffSteer.computeMotors(XValue, YValue);
        int leftMotor = DiffSteer.computedLeftMotor();
        int rightMotor = DiffSteer.computedRightMotor();

        // map motor outputs to your desired range
        motorControl(leftMotor, rightMotor);
        //Serial.println("Differential | " + DiffSteer.toString());
    } else
    {
        Serial.println("idle");
    }
    break;
  }
}

void _ultrasonic(void){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; 
  Serial.print("Distance: ");
  Serial.println(distance);
}

void setup()
{
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT  
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT

  DiffSteer.begin(fPivYLimit);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  wifiManager.autoConnect(AP_NAME);
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  pinMode(RightMotorSpeed, OUTPUT);
  pinMode(RightMotorDir, OUTPUT);
  pinMode(LeftMotorSpeed, OUTPUT);
  pinMode(LeftMotorDir, OUTPUT);

  motorControl(0, 0);

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin(AP_NAME))
  {
    Serial.println("MDNS responder started");
  }

  // handle index
  server.on("/", []() {
    // send index.html
    server.send(200, "text/html", htmlBody);
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

unsigned long last_10sec = 0;
unsigned int counter = 0;

void loop()
{  
  unsigned long t = millis();
  webSocket.loop();
  server.handleClient();

  if ((t - last_10sec) > 10 * 1000)
  {
    counter++;
    bool ping = (counter % 2);
    int i = webSocket.connectedClients(ping);
    Serial.printf("%d Connected websocket clients ping: %d\n", i, ping);
    last_10sec = millis();
  }
}
