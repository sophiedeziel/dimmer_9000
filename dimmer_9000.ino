#include "AiEsp32RotaryEncoder.h"
#include <EEPROM.h>
#include <Ticker.h>

#include <WiFi.h>
#include <WebServer.h>

#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#include <PageBuilder.h>
#include <FS.h>
#include <SPIFFS.h>

#define CREDENTIAL_OFFSET 64
#define INTENSITY_EEPROM_LOCATION 0
#define TEMPERATURE_EEPROM_LOCATION 16
#define EEPROM_SIZE 32

#define ENCODER_I_A_PIN GPIO_NUM_34
#define ENCODER_I_B_PIN GPIO_NUM_35
#define ENCODER_T_A_PIN GPIO_NUM_19
#define ENCODER_T_B_PIN GPIO_NUM_21
#define LED_WARM_PIN GPIO_NUM_22
#define LED_COLD_PIN GPIO_NUM_23

WebServer Server;

AutoConnect Portal(Server);

String viewIntensity(PageArgument&);
String viewTemperature(PageArgument&);
String saveLightSettings(PageArgument&);

AiEsp32RotaryEncoder intensityEncoder = AiEsp32RotaryEncoder(ENCODER_I_A_PIN, ENCODER_I_B_PIN, -1, -1);
AiEsp32RotaryEncoder temperatureEncoder = AiEsp32RotaryEncoder(ENCODER_T_A_PIN, ENCODER_T_B_PIN, -1, -1);

int16_t intensityValue = 0;
int16_t temperatureValue = 0;
const int16_t intensityMax = 30;
const int16_t temperatureMax = 30;

Ticker tkDac;

volatile int last_save = 0;
static const char PROGMEM autoconnectMenu[] = { AUTOCONNECT_LINK(BAR_24) };

// URL path as '/'
PageElement elmList("file:/index.htm",{
 { "INTENSITY", viewIntensity },
 { "TEMPERATURE", viewTemperature},
 { "AUTOCONNECT_MENU", [](PageArgument& args) {
                          return String(FPSTR(autoconnectMenu));} }
});
PageBuilder rootPage("/", { elmList });

// URL path as '/del'
PageElement elmDel("{{DEL}}", {{ "DEL", saveLightSettings }});
PageBuilder settingPage("/setting", { elmDel });

String viewIntensity(PageArgument& args) {
  return String(intensityValue);
}

String viewTemperature(PageArgument& args) {
  return String(temperatureValue);
}

String saveLightSettings(PageArgument& args) {
  int16_t newIntensity = -1;
  int16_t newTemperature = -1;
  
  if (args.hasArg("intensity")) {
    newIntensity = args.arg("intensity").toInt();
    if (newIntensity >= 0) {
      setIntensity(newIntensity);
    }
  }

  if (args.hasArg("temperature")) {
    newTemperature = args.arg("temperature").toInt();
    if (newTemperature >= 0) {
      setTemperature(newTemperature);
    }
  }

  // Returns the redirect response. The page is reloaded and its contents
  // are updated to the state after deletion. It returns 302 response
  // from inside this token handler.
  Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
  Server.send(302, "text/plain", "");
  Server.client().flush();
  Server.client().stop();

  return "";
}

void ICACHE_RAM_ATTR _onTimer() {
  if (last_save < millis() - 5000) {
    last_save = millis();
    Serial.println("Saving");
    EEPROM.commit();
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

  Serial.println("Ready");
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS MOUNTED");
  }
  else {
    Serial.println("SPIFFS MOUNT FAIL");
  }
  if (SPIFFS.exists("/index.htm")) {
    Serial.println("FILE EXIST");
  }
  else {
    Serial.println("FILE NOT EXIST");
  }

  intensityEncoder.begin();
  temperatureEncoder.begin();

  intensityEncoder.setup([] {intensityEncoder.readEncoder_ISR();});
  temperatureEncoder.setup([] {temperatureEncoder.readEncoder_ISR();});

  intensityEncoder.setBoundaries(0, intensityMax, false);
  temperatureEncoder.setBoundaries(0, temperatureMax, false);

  Serial.println(EEPROM.readShort(INTENSITY_EEPROM_LOCATION));
  setIntensity(EEPROM.readShort(INTENSITY_EEPROM_LOCATION));
  setTemperature(EEPROM.readShort(TEMPERATURE_EEPROM_LOCATION));

  ledcSetup(0, 200, 13);
  ledcSetup(1, 200, 13);
  ledcAttachPin(LED_WARM_PIN, 0);
  ledcAttachPin(LED_COLD_PIN, 1);

  rootPage.insert(Server); 
  settingPage.insert(Server);

  // Set an address of the credential area.
  AutoConnectConfig Config;
  Config.boundaryOffset = CREDENTIAL_OFFSET;
  Portal.config(Config);

  // Start
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
}

void loop() {
  Portal.handleClient();
  intensityValue   = intensityEncoder.readEncoder();
  temperatureValue = temperatureEncoder.readEncoder();

  bool commit = false;
  if ( intensityValue != EEPROM.readShort(INTENSITY_EEPROM_LOCATION) ) {
    EEPROM.writeShort(INTENSITY_EEPROM_LOCATION, intensityValue);
    commit = true;
  }

  if ( temperatureValue != EEPROM.readShort(TEMPERATURE_EEPROM_LOCATION) ) {
    EEPROM.writeShort(TEMPERATURE_EEPROM_LOCATION, temperatureValue);
    commit = true;
  }

  if (commit) {
    tkDac.attach_ms(5000, _onTimer);
  }

  int16_t globalIntensity = map(intensityValue, 0 , intensityMax, 0, 8191);
  int16_t warm_intensity  = map(temperatureValue, 0, temperatureMax, 0 , globalIntensity);
  int16_t cold_intensity  = map(temperatureMax - temperatureValue, 0, temperatureMax, 0 , globalIntensity);

  ledcWrite(0, warm_intensity);
  ledcWrite(1, cold_intensity);
}

void setIntensity(int newIntensity) {
  if( 0 <= newIntensity && newIntensity <= intensityMax) {
    intensityEncoder.reset(newIntensity);
  }
}

void setTemperature(int newTemperature) {
  temperatureEncoder.reset(newTemperature);
}
