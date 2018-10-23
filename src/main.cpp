#include <Arduino.h>
#include <pms.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson-v5.13.3.h>

const char* ssid = "No Password For Ya";
const char* pass = "MightyP@ssw0rd";

Pmsx003 pms(D5, D6);

// SETUP

void setupInternetConnection() {
    WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) {
    	delay(500);
    	Serial.println("Waiting for connection");
  }
}

void setupPMS() {
	pms.begin();
    pms.waitForData(Pmsx003::wakeupTime);
    pms.write(Pmsx003::cmdModeActive);
}

void setup() {
	Serial.begin(115200);
    while(!Serial) {};
	
	setupInternetConnection();
	setupPMS();
}

// RUN CODE

void postData(JsonObject &object) {
    if (WiFi.status() == WL_CONNECTED) {
        const char* url = "https://us-central1-air-quality-monitor-server.cloudfunctions.net/aqmDataReceiver";
        const char* thumbprint = "695821299d57b0e2eb0c713640e8D9e4da16f230";
		
        char messageBuffer[300];
		object.prettyPrintTo(messageBuffer, sizeof(messageBuffer));

		HTTPClient http;
		http.begin(url, thumbprint);
		http.addHeader("Content-Type", "application/json");
		int httpCode = http.POST(messageBuffer);
        if(httpCode != HTTP_CODE_OK) {
            Serial.println("Response code: " + httpCode);
        }
		http.end();
	} else {
        Serial.println("Lost connection");
        delay(500);
    }
}

void loop(void) {
	const auto n = Pmsx003::Reserved;
	Pmsx003::pmsData data[n];

	Pmsx003::PmsStatus status = pms.read(data, n);

	switch (status) {
		case Pmsx003::OK: {
			const int N = 3;
			u_int pmDataIndexes [N] = {Pmsx003::PM1dot0, Pmsx003::PM2dot5, Pmsx003::PM10dot0};

			StaticJsonBuffer<300> JSONbuffer;
			JsonObject& JSONencoder = JSONbuffer.createObject();
			JsonObject& measurements = JSONencoder.createNestedObject("measurements");

			for(size_t i = 0; i < N; ++i) {
				u_int index = pmDataIndexes[i];
				auto pmName = Pmsx003::dataNames[index];
				auto pmMetric = Pmsx003::metrics[index];
				auto pm = data[index];
                
				measurements[pmName] = pm;
			}

            postData(JSONencoder);
			break;
		}
		case Pmsx003::noData:
			break;
		default:
			Serial.print("___ ERROR ___ :");
			Serial.println(Pmsx003::errorMsg[status]);
	};
}