// Wi-Fi network to connect to (if not in AP mode)
const char* ssid = "";
const char* password = "";

// OTA upload
const char *hostname = "panneau_led/1";

// MQTT Broker settings
const char* mqtt_server = "10.0.1.7";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* mqtt_topic = "panneau_led/1";
const char* mqtt_clientid = "panneau_led/1";

// EEPROM settings
#define INTENSITY_EEPROM_LOCATION 0
#define TEMPERATURE_EEPROM_LOCATION 16
#define EEPROM_SIZE 32

// Pins settings
#define ENCODER_I_A_PIN GPIO_NUM_34
#define ENCODER_I_B_PIN GPIO_NUM_35
#define ENCODER_T_A_PIN GPIO_NUM_19
#define ENCODER_T_B_PIN GPIO_NUM_21
#define LED_WARM_PIN GPIO_NUM_22
#define LED_COLD_PIN GPIO_NUM_23

// Initial and scale LED values
int16_t intensityValue = 15;
int16_t temperatureValue = 15;
const int16_t intensityMax = 60;
const int16_t temperatureMax = 60;
