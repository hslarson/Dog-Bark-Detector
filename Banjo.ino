#include <ThingSpeak.h>
#include <arduinoFFT.h>
#include <ESP8266WiFi.h>
#include "constants.h" // Not Included Because it Contains Passwords & Stuff


// Declare Functions & Global Variables
WiFiClient client;
arduinoFFT FFT = arduinoFFT();

int bark_count = 0;
bool barking = false;

unsigned int sampling_period_us;
unsigned long microseconds;
double vReal[SAMPLES];
double vImag[SAMPLES];

int  timer();
bool isBarking();
double getAudioLevel();
bool connectToNetwork();
bool checkToneEnabled(bool);
void sendToServer(int, int);


void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        while(!Serial);
    }
    connectToNetwork();

    // Set PinModes
    pinMode(MIC_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Start ThingSpeak
    ThingSpeak.begin(client);
    checkToneEnabled(true);

    // Calculate Sampling Period
    sampling_period_us = round(1000000*(1.0 / SAMPLING_FREQUENCY));
}


void loop() {
    // Ping the Timer
    switch (timer()) {
        case 0:
            sendToServer(1, bark_count);
            bark_count = 0;
            break;
        case 1:
            checkToneEnabled(true);
            break;
    };

    // Check Microphone Volume
    if (getAudioLevel() > BARK_THRESHOLD) {
        if (DEBUG) { 
            Serial.println("\nBark Detected!");
            digitalWrite(LED_PIN, LOW); // Active Low
        }

        // Check if the Noise is a Bark
        barking = isBarking();
        
        if (DEBUG) {
            Serial.println((barking ? "Bark Confirmed" : "Not a Bark"));
            digitalWrite(LED_PIN, HIGH);
        }
    }

    // When a Bark is Detected...
    if (barking) {
        // Update Counters / Flags
        barking = false;
        bark_count++; 

        // Play Tone 
        if (checkToneEnabled(false))
            tone(BUZZER_PIN, BUZZER_FREQUENCY, BUZZER_LINGERING_TIME * 1000);
    }
}


// Return True if it's Time to Send Data to ThingSpeak
int timer() {
    static unsigned long bark_start = millis();
    static unsigned long tone_enabled_check = millis();

    if ( abs(millis() - bark_start) / (BARK_SEND_INTERVAL * 1000) ) {
        bark_start = millis();
        return 0;
    }
    if ( abs(millis() - tone_enabled_check) / (TONE_CHECK_INTERVAL * 1000 * 60) ) {
        tone_enabled_check = millis();
        return 1; 
    }
    
    return -1;
}


// Returns True if a Valid Bark is Detected
// Barks Must Meet Duration and Frequency Standards
bool isBarking() {
    unsigned long bark_timer = millis();  // Start a Timer
  
    // Collect Samples
    microseconds = micros();
    for (int i = 0; i < SAMPLES; i++) {
        vReal[i] = analogRead(MIC_PIN);
        vImag[i] = 0;
        while(micros() - microseconds < sampling_period_us);
        microseconds += sampling_period_us;
    }
  
    // Weigh data
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  
    // Compute FFT
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  
    // Compute magnitudes
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

    // Find Dominant Frequency
    double x = FFT.MajorPeak(vReal, SAMPLES / 2, SAMPLING_FREQUENCY);
    if (DEBUG) {
        Serial.print(" - Dominant Frequency: ");
        Serial.print(x);
        Serial.println("Hz");
    }
    
    // Wait Until the Bark is Finished
    double level = getAudioLevel();
    while (level > NOISE_FLOOR) {
        level *= 0.8;
        level += 0.2 * getAudioLevel();
        if (DEBUG) {Serial.println(level);}
        delay(1);
    }

    // Check the Timer
    unsigned long bark_duration = abs(millis() - bark_timer);
    if (DEBUG) {
        Serial.print(" - Bark Duartion: ");
        Serial.print(bark_duration);
        Serial.println("ms");
    }
    
    // Return True if the Bark is Valid
    return ((bark_duration >= BARK_DURATION_MIN && bark_duration <= BARK_DURATION_MAX) && (x >= BARK_FREQ_MIN && x <= BARK_FREQ_MAX));
}


// Returns the Standard Deviation of a Set of Sampled Audio Data
double getAudioLevel() {
    unsigned short readings[VOLUME_SAMPLES];
    unsigned int period = round(1000000*(1.0 / VOLUME_SAMPLING_FREQUENCY));
    unsigned long sum_readings = 0, differences = 0;
    unsigned long timer = micros();

    // Sanple Audio Data
    for (int i = 0; i < VOLUME_SAMPLES; i++) {
        readings[i] = analogRead(MIC_PIN);
        sum_readings += readings[i];

        while(micros() - timer < period);
        timer += period;
    }

    // Compute Mean
    double mean = (double)sum_readings / VOLUME_SAMPLES;

    // Compute Sum of Squared Differences
    for (unsigned short reading : readings) {
        differences += pow((double)reading - mean, 2);
    }

    // Return Standard Deviation
    return sqrt((double)differences / VOLUME_SAMPLES);
}


// Attempts to Connect to WiFi, Returns True if Connected
bool connectToNetwork() {
    if (WiFi.status() == WL_CONNECTED)
        return true;

    unsigned long timer = millis();
    if (DEBUG) {
        Serial.print("Connecting to ");
        Serial.println(WIFI_NAME);
    }
 
    WiFi.begin(WIFI_NAME, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (DEBUG) {Serial.print(".");}

        if ( abs(millis() - timer) / (WIFI_TIMEOUT * 1000) )
          return false;
    }
    
    if (DEBUG) {Serial.println("\nWiFi connected");}
    return true;
}


// Pulls Data from ThingSpeak to See if the Buzzer is Enabled
bool checkToneEnabled(bool check) {
        static bool tone_enabled = false;

        if (!check || !connectToNetwork())
            return tone_enabled;
            
        tone_enabled = (ThingSpeak.readIntField(CHANNEL, 2, READ_API_KEY) == 1 ? true : false);
        
        if (DEBUG) {
            Serial.print("Buzzer Status: ");
            Serial.println((tone_enabled ? "ENABLED" : "DISABLED"));
        }

        return tone_enabled;
}


// Write Data to the Channel
void sendToServer(int field, int data) {
  if (DEBUG) {
      Serial.print("\nSending \"");
      Serial.print(data);
      Serial.print("\" to Field: ");
      Serial.println(field);
  }

  int response = -1;
  if (connectToNetwork())
      response = ThingSpeak.writeField(CHANNEL, field, data, WRITE_API_KEY);
    
  if (DEBUG) {
      if(response == -1)
          Serial.println("Data Not Sent, WiFi Not Connected");

      else {
          Serial.print("Data Sent With Code: ");
          Serial.println(response);
      }
  }
  return;
}
