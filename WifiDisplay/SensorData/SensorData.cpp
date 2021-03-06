#include "Arduino.h"
#include <ArduinoJson.h>
#include "SensorData.h"

SensorData::SensorData()
{
	temperature = 0.0;
	humidity = 0.0;
	pressure = 0.0;
	VOC = 0;
	label = "";
	stateLed = 0;
	stateDisplay = 1;
	stateWarning = 0;
}

String SensorData::toJson()
{
	DynamicJsonDocument doc(1024);

	doc["temperature"] = String(temperature, 2);
	doc["humidity"]   = String(humidity, 2);
	doc["pressure"][0] = String(pressure, 2);
	doc["voc"] = VOC;
	doc["label"] = label;
	doc["stateLed"] = stateLed;
	doc["stateDisplay"] = stateDisplay;
	doc["stateWarning"] = stateWarning;
	char json_string[1024];
	serializeJson(doc, json_string);
	return json_string;
}
