#include "AiEsp32RotaryEncoder.h"

#define ENCODER_I_A_PIN GPIO_NUM_34
#define ENCODER_I_B_PIN GPIO_NUM_35
#define ENCODER_T_A_PIN GPIO_NUM_19
#define ENCODER_T_B_PIN GPIO_NUM_21


AiEsp32RotaryEncoder intensityEncoder = AiEsp32RotaryEncoder(ENCODER_I_A_PIN, ENCODER_I_B_PIN, -1, -1);
AiEsp32RotaryEncoder temperatureEncoder = AiEsp32RotaryEncoder(ENCODER_T_A_PIN, ENCODER_T_B_PIN, -1, -1);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("Ready");

  intensityEncoder.begin();
  temperatureEncoder.begin();
  
  intensityEncoder.setup([]{intensityEncoder.readEncoder_ISR();});
  temperatureEncoder.setup([]{temperatureEncoder.readEncoder_ISR();});

  intensityEncoder.setBoundaries(0, 10, false);
  temperatureEncoder.setBoundaries(0, 30, false);
}

void loop() {
  // put your main code here, to run repeatedly:
  rotary_loop(intensityEncoder, "Intensity");
  delay(50);
  Serial.print(" ");
  rotary_loop(temperatureEncoder, "Temperature");
  delay(50);
  Serial.println("");
}

void rotary_loop(AiEsp32RotaryEncoder rotaryEncoder, String type) {
  int16_t encoderValue = rotaryEncoder.readEncoder();
  //process new value. Here is simple output.
  Serial.print(encoderValue);
}
