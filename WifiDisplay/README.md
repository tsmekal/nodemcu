# WifiDisplay

This is example how to implement three HW modules: humidity sensor, wifi, OLED display (128x32)

Project HW: 
* NodeMCU
* BMP280 (humidity sensor)
* OLED display (128x32)
* two ordinary LEDs
* two resistors for LEDs

The humidity sensor and OLED display is connected via I2C

Main function: provide sensor data on display and via simple HTML web page. The web page enables control one LED and display (on/off). If temperature is exceeded the second LED is lighting and web page shows warning.

Himidity sensors reads data (temperature, humidity, pressure) and in defined periods these data is send via WebSocket and also shown on the OLED display

I use WiFi manager which allows configures WiFi via AP and it stores configured WiFi settings

Data via WebSockets is send in very simple string temp|humid|press|LED_state|DISPLAY_state|WARNING_state
