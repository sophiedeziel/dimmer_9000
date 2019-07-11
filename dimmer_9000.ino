#include "AiEsp32RotaryEncoder.h"

#define ENCODER_I_A_PIN GPIO_NUM_34
#define ENCODER_I_B_PIN GPIO_NUM_35
#define ENCODER_T_A_PIN GPIO_NUM_19
#define ENCODER_T_B_PIN GPIO_NUM_21
#define LED_WARM_PIN GPIO_NUM_22
#define LED_COLD_PIN GPIO_NUM_23
#define INTENSITY_EEPROM_LOCATION 0
#define TEMPERATURE_EEPROM_LOCATION 16

AiEsp32RotaryEncoder intensityEncoder = AiEsp32RotaryEncoder(ENCODER_I_A_PIN, ENCODER_I_B_PIN, -1, -1);
AiEsp32RotaryEncoder temperatureEncoder = AiEsp32RotaryEncoder(ENCODER_T_A_PIN, ENCODER_T_B_PIN, -1, -1);

int16_t intensityValue = 0;
int16_t temperatureValue = 0;
const int16_t intensityMax = 15;
const int16_t temperatureMax = 30;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("Ready");

  intensityEncoder.begin();
  temperatureEncoder.begin();

  intensityEncoder.setup([] {intensityEncoder.readEncoder_ISR();});
  temperatureEncoder.setup([] {temperatureEncoder.readEncoder_ISR();});

  intensityEncoder.setBoundaries(0, intensityMax, false);
  temperatureEncoder.setBoundaries(0, temperatureMax, false);

  ledcSetup(0, 5000, 13);
  ledcSetup(1, 5000, 13);
  ledcAttachPin(LED_WARM_PIN, 0);
  ledcAttachPin(LED_COLD_PIN, 1);
}

void loop() {
  intensityValue   = intensityEncoder.readEncoder();
  temperatureValue = temperatureEncoder.readEncoder();

  delay(50);
  Serial.println("");
}

  int16_t globalIntensity = map(intensityValue, 0 , intensityMax, 0, 8191);
  int16_t warm_intensity = map(temperatureValue, 0, temperatureMax, 0 , globalIntensity);
  int16_t cold_intensity = map(temperatureMax - temperatureValue, 0, temperatureMax, 0 , globalIntensity);

  ledcWrite(0, warm_intensity);
  ledcWrite(1, cold_intensity);


}
