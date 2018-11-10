#include <Credentials.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <pms.h>

Pmsx003 pms(D5, D6);
Pmsx003::pmsData latestData[Pmsx003::Reserved];

WiFiClient client;

Adafruit_MQTT_Client mqtt = Adafruit_MQTT_Client(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish pm2dot5feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.pm2dot5");
Adafruit_MQTT_Publish pm10feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/air-quality-monitor.pm10");

auto lastRead = millis();

void MQTT_connect();

// SETUP

void setupInternetConnection() {
    WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID_2, WIFI_PASS_2);
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

void setup() {
	Serial.begin(115200);
    // while(!Serial) {};
	
	setupInternetConnection();
	setupPMS();
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

void getPMSdata() {
	pms.write(Pmsx003::cmdWakeup);
	delay(10000);
	pms.read(latestData, Pmsx003::Reserved);
	pms.write(Pmsx003::cmdSleep);
}

void publishData() {
	Serial.print("Publishing data: ");
	if (
		!pm2dot5feed.publish(latestData[Pmsx003::PM2dot5]) ||
		!pm10feed.publish(latestData[Pmsx003::PM10dot0])
	) {
		Serial.println(F("Failed publishing."));
	} else {
		Serial.println(F("OK!"));
	}
}

void loop(void) {
	MQTT_connect();

	if (millis() - lastRead > 10000) {
		getPMSdata();
		lastRead = millis();
		publishData();
	}
}