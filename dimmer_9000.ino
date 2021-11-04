#include "AiEsp32RotaryEncoder.h"
#include <EEPROM.h>
#include <Ticker.h>

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Settings.h"

WiFiClient espClient;
PubSubClient client(espClient);
Ticker tkDac;

AiEsp32RotaryEncoder intensityEncoder = AiEsp32RotaryEncoder(ENCODER_I_A_PIN, ENCODER_I_B_PIN, -1, -1);
AiEsp32RotaryEncoder temperatureEncoder = AiEsp32RotaryEncoder(ENCODER_T_A_PIN, ENCODER_T_B_PIN, -1, -1);

volatile int last_save = 0;
bool saving = false;

void ICACHE_RAM_ATTR _onTimer() {
  if (last_save < millis() - 1000) {
    last_save = millis();
    
    EEPROM.writeShort(INTENSITY_EEPROM_LOCATION, intensityValue);
    EEPROM.writeShort(TEMPERATURE_EEPROM_LOCATION, temperatureValue);
    EEPROM.commit();
    Serial.println("Saved");
    saving = false;
  }
}

void setup() {
  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  Serial.println("EEPROM ready");

  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected! With IP ");
  Serial.print(WiFi.localIP());
  Serial.println(" have FUN :) ");

  //  Mqtt Init
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  intensityEncoder.begin();
  temperatureEncoder.begin();
  Serial.println("Rotary Encoders configuring");

  intensityEncoder.setup([] {intensityEncoder.readEncoder_ISR();});
  temperatureEncoder.setup([] {temperatureEncoder.readEncoder_ISR();});
  intensityEncoder.setBoundaries(0, intensityMax, false);
  temperatureEncoder.setBoundaries(0, temperatureMax, false);
  Serial.println("Rotary Encoders configured");
  
  Serial.println("begin eeprom data read");
  Serial.println(EEPROM.readShort(INTENSITY_EEPROM_LOCATION));
  Serial.println(EEPROM.readShort(TEMPERATURE_EEPROM_LOCATION));

  Serial.println("Setting last used");
  setIntensity(EEPROM.readShort(INTENSITY_EEPROM_LOCATION));
  setTemperature(EEPROM.readShort(TEMPERATURE_EEPROM_LOCATION));

  ledcSetup(0, 120, 13);
  ledcSetup(1, 120, 13);
  ledcAttachPin(LED_WARM_PIN, 0);
  ledcAttachPin(LED_COLD_PIN, 1);

  Serial.println("ArduinoOTA configuring");
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade

  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
    ESP.restart();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    ESP.restart();
  });

  ArduinoOTA.begin();

  Serial.println("ArduinoOTA configured");
  Serial.println("Setup done.");
}

void loop() {
  yield();
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnectMqtt();
  }
 
  client.loop();

  updateLightSettings();
}

void updateLightSettings() {
  intensityValue   = intensityEncoder.readEncoder();
  temperatureValue = temperatureEncoder.readEncoder();

  if ( 
    (intensityValue != EEPROM.readShort(INTENSITY_EEPROM_LOCATION)) 
      || (temperatureValue != EEPROM.readShort(TEMPERATURE_EEPROM_LOCATION))
     ) {
      if (saving == false) {
        Serial.println("Sending save command");
        tkDac.once_ms(1000, _onTimer);
        saving = true;
      }
  }

  int16_t globalIntensity = map(intensityValue, 0 , intensityMax, 0, 8191);
  int16_t cold_intensity  = map(temperatureValue, 0, temperatureMax, 0 , globalIntensity);
  int16_t warm_intensity  = map(temperatureMax - temperatureValue, 0, temperatureMax, 0 , globalIntensity);

  ledcWrite(0, warm_intensity);
  ledcWrite(1, cold_intensity);
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(mqtt_clientid, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setIntensity(int newIntensity) {
  if ( 0 <= newIntensity && newIntensity <= intensityMax) {
    intensityEncoder.reset(newIntensity);
  }
}

void setTemperature(int newTemperature) {
  if ( 0 <= newTemperature && newTemperature <= temperatureMax) {
    temperatureEncoder.reset(newTemperature);
  }
}

// Format is: command:value
// value has to be a number, except rgb commands
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  char tmp[length + 1];
  strncpy(tmp, (char*)payload, length);
  tmp[length] = '\0';
  String data(tmp);

  Serial.printf("Received Data from Topic: %s", data.c_str());
  Serial.println();
  if ( data.length() > 0) {
    StaticJsonDocument<64> doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    if(doc.containsKey("intensity")) {
      int intensity = doc["intensity"]; 
      setIntensity(intensity);
    }

    if(doc.containsKey("temperature")) {
      int temperature = doc["temperature"]; 
      setTemperature(temperature);
    }
  }
  Serial.println("Finished Topic Data ...");
}
