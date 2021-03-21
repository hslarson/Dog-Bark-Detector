#ifndef constants
#define constants


#define DEBUG true

#define READ_API_KEY "Your Read API Key"
#define WRITE_API_KEY "Your Write API Key"
#define CHANNEL 0123456

#define WIFI_NAME "Your SSID"
#define PASSWORD "Your Password"
#define WIFI_TIMEOUT 60 // Seconds

#define MIC_PIN A0
#define LED_PIN D4
#define BUZZER_PIN D5

#define NOISE_FLOOR 15
#define BARK_THRESHOLD 50
#define VOLUME_SAMPLES 32
#define VOLUME_SAMPLING_FREQUENCY 6400 // Hz

#define BARK_SEND_INTERVAL 300 // Seconds
#define TONE_CHECK_INTERVAL 15 // Minutes

#define BUZZER_FREQUENCY 12000 // Hz
#define BUZZER_LINGERING_TIME 5 // Seconds

#define SAMPLES 256  // This Value MUST ALWAYS be a Power of 2
#define SAMPLING_FREQUENCY (double)8000 // Hz, must be less than 10,000 due to ADC

#define BARK_FREQ_MIN 300 // Hz
#define BARK_FREQ_MAX 600 // Hz
#define RETRY_ATTEMPTS 10
#define BARK_DURATION_MIN 60  // Milliseconds
#define BARK_DURATION_MAX 250 // Milliseconds


#endif