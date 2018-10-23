#include <Arduino.h>
#include <pms.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson-v5.13.3.h>

Pmsx003 pms(D5, D6);

const char* ssid = "No Password For Ya";
const char* pass = "MightyP@ssw0rd";

auto lastRead = millis();

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

void loop(void) {
	const auto n = Pmsx003::Reserved;
	Pmsx003::pmsData data[n];

	Pmsx003::PmsStatus status = pms.read(data, n);

	switch (status) {
		case Pmsx003::OK:
		{
			Serial.println("_________________");
			auto newRead = millis();
			Serial.print("Wait time: ");
			Serial.println(newRead - lastRead);
			lastRead = newRead;

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

				Serial.print(pmName);
				Serial.print(":\t");
				Serial.print(pm);
				Serial.print(" ");
				Serial.println(pmMetric);

				measurements[pmName] = pm;
			}

			if (WiFi.status() == WL_CONNECTED) {
				Serial.println("");
				Serial.println("");
				char JSONmessageBuffer[300];
				JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
				Serial.println(JSONmessageBuffer);

				HTTPClient http;
				http.begin("http://localhost:3000/measurements");
				http.addHeader("Content-Type", "application/json");
				int httpCode = http.POST(JSONmessageBuffer);
				String payload = http.getString();
				Serial.print("HTTP CODE: ");
				Serial.println(httpCode);
				Serial.println("Payload:");
				Serial.println(payload);
				http.end();
			}
			
			// For loop starts from 3
			// Skip the first three data (PM1dot0CF1, PM2dot5CF1, PM10CF1)
			// for (size_t i = Pmsx003::PM1dot0; i < n; ++i) { 
			// 	Serial.print(data[i]);
			// 	Serial.print("\t");
			// 	Serial.print(Pmsx003::dataNames[i]);
			// 	Serial.print(" [");
			// 	Serial.print(Pmsx003::metrics[i]);
			// 	Serial.print("]");
			// 	Serial.println();
			// }
			break;
		}
		case Pmsx003::noData:
			break;
		default:
			Serial.println("_________________");
			Serial.println(Pmsx003::errorMsg[status]);
	};
}