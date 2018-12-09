#include <Credentials.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <pms.h>
#include <Adafruit_BME280.h>
#include <Wire.h>

Pmsx003 pms(D5, D6);
Pmsx003::pmsData pmsLatestData[Pmsx003::Reserved];

Adafruit_BME280 bme;
float latestTemp;
float latestHum;
float latestPress;

WiFiClient client;

Adafruit_MQTT_Client mqtt = Adafruit_MQTT_Client(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish pm2dot5_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.pm2dot5");
Adafruit_MQTT_Publish pm10_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.pm10");
Adafruit_MQTT_Publish temp_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.temp");
Adafruit_MQTT_Publish hum_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.hum");
Adafruit_MQTT_Publish press_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.press");

auto lastRead = millis();

void MQTT_connect();

// SETUP

void setupInternetConnection() {
    WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID_S9, WIFI_PASS_S9);
	Serial.print("Connecting");
	while (WiFi.status() != WL_CONNECTED) {
    	delay(500);
    	Serial.print(".");
  	}
	Serial.println();
}

void setupPMS() {
	pms.begin();
    pms.waitForData(Pmsx003::wakeupTime);
    pms.write(Pmsx003::cmdModeActive);
}

void setupBME() {
    bme.begin();
}

void setup() {
	Serial.begin(115200);
	
	setupInternetConnection();
	setupPMS();
    setupBME();
}

// RUN CODE

void MQTT_connect() {
	int8_t ret;

	if(mqtt.connected()) { return; }

	uint8_t retries = 3;
	while ((ret = mqtt.connect()) != 0) {
		Serial.println(mqtt.connectErrorString(ret));
       	mqtt.disconnect();
		retries--;
		if (retries == 2) { delay(5000); } // wait 5 seconds
		else if (retries == 1) { delay(60000); } // wait 1 minute		
		else if (retries == 0) { delay(60000); } // wait 10 minutes
		else { 
			Serial.println("Going to sleep.");
			pms.write(Pmsx003::cmdSleep);
			while(1);
		}
	}
	Serial.println("MQTT connected.");
}

void getData() {
    pms.write(Pmsx003::cmdWakeup);
    delay(10000);
    pms.read(pmsLatestData, Pmsx003::Reserved);
	pms.write(Pmsx003::cmdSleep);

	if(bme.begin()) {
		latestTemp = bme.readTemperature();
    	latestHum = bme.readHumidity();
    	latestPress = bme.readPressure() / 100.0F;
	} else {
		Serial.println("No BME module found.");
	}
}

void publishData() {
	Serial.print("Publishing data: ");
	if (
		!pm2dot5_feed.publish(pmsLatestData[Pmsx003::PM2dot5]) ||
		!pm10_feed.publish(pmsLatestData[Pmsx003::PM10dot0]) ||
        !temp_feed.publish(latestTemp) ||
        !hum_feed.publish(latestHum) ||
        !press_feed.publish(latestPress)
	) {
		Serial.println(F("Failed publishing."));
	} else {
		Serial.println(F("OK!"));
	}
}

void loop(void) {
	MQTT_connect();

	if (millis() - lastRead > 10000) {
        getData();
		lastRead = millis();
		publishData();
	}
}