#ifndef Morse_h
#define Morse_h

#include "Arduino.h"
#include <ArduinoJson.h>

class SensorData
{
  public:    
    float temperature;
  	float humidity;
  	float pressure;
  	float VOC;
  	String label;
  	
  	SensorData();
    String toJson();
};

#endif
