# WifiDisplay

This is example how to implement three HW modules: humidity sensor, wifi, OLED display (128x32)

Project HW: 
* NodeMCU
* SGP40 (air quality sensor)
* BMP280 (humidity sensor)
* OLED display (128x32)
* two ordinary LEDs
* two resistors for LEDs

# Installation

1. Copy the folder SensorData to /libraries/
2. Install libraries used in source code

The air quality sensor, humidity sensor and OLED display is connected via I2C

Main function: provide sensor data on display and via simple HTML web page. The web page enables control one LED and display (on/off). If temperature is exceeded the second LED is lighting and web page shows warning.

Humidity sensors reads data (temperature, humidity, pressure) and in defined periods these data is send via WebSocket and also shown on the OLED display

I use WiFi manager which allows configures WiFi via AP and it stores configured WiFi settings

Data via WebSockets is send in very simple string temp|humid|press|LED_state|DISPLAY_state|WARNING_state

After connection to WiFi the IP is shown on the display (after every start). On this IP address you can find data from humidity sensor:
![image](https://user-images.githubusercontent.com/20030614/147695937-387c110a-5375-40d2-bcbb-5b10753dc28e.png)

# Connection schema

![image](https://user-images.githubusercontent.com/20030614/147695341-e821a6e5-b317-4f6c-8a14-0101559744de.png)

