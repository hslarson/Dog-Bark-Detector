#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "constants.h" // Not Included Because it Contains Passwords & Stuff


// Declare Functions & Global Variables
WiFiClient client;

int bark_count = 0;
bool barking = false, tone_enabled = false;

void sendToServer(int, int);
bool connectToWifi();
bool timer();


void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        delay(1000);
    }
    connectToWiFi();

    // Start ThingSpeak
    WiFi.mode(WIFI_STA); 
    ThingSpeak.begin(client);

    // Read Settings Field
    tone_enabled = (ThingSpeak.readIntField(CHANNEL, 2, READ_API_KEY) == 1 ? true : false);
    if (DEBUG) {
        Serial.print("Buzzer Status: ");
        Serial.println((tone_enabled ? "ON" : "OFF"));
    }
}


void loop() {
    // Ping the Timer
    if (timer()) {
        sendToServer(1, bark_count);
        bark_count = 0;
    }

    // Read Microphone Data
    if (analogRead(MIC_PIN) > VOLUME_THRESHOLD && !barking) {
        if (DEBUG) {Serial.println("Bark Detected!");};
        barking = true;
        bark_count++;

        // Play the Tone
        if (tone_enabled)
            tone(BUZZER_PIN, BUZZER_FREQUENCY, BUZZER_LINGERING_TIME * 1000);
    }

    // Wait Until the Bark is Finished
    int retries = RETRY_ATTEMPTS;
    while (barking && retries > 0) {
        if (analogRead(MIC_PIN) < VOLUME_THRESHOLD)
            retries--;
        else
            retries = RETRY_ATTEMPTS;
        delay(1);
    }
    barking = false;
}


// Write Data to the Channel
void sendToServer(int field, int data) {
  if (DEBUG) {
    Serial.print("Sending \"");
    Serial.print(data);
    Serial.print("\" to Field: ");
    Serial.println(field);
  }

  int response = -1;
  if (connectToWiFi())
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


// Attempts to Connect to WiFi, Returns True if Connected
bool connectToWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return true;

    long timer = millis();
    if (DEBUG) {
        Serial.print("Connecting to ");
        Serial.println(WIFI_NAME);
    }
 
    WiFi.begin(WIFI_NAME, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (DEBUG) {Serial.print(".");};

        if ( abs(millis() - timer) / (WIFI_TIMEOUT * 1000) )
          return false;
    }
    
    if (DEBUG) {Serial.println("\nWiFi connected");};
    return true;
}


// Return True if it's Time to Send Data to ThingSpeak
bool timer() {
    static long bark_start = millis();

    if ( abs(millis() - bark_start) / (BARK_SEND_INTERVAL * 1000) ) {
        bark_start = millis();
        return true;
    }
    
    return false;
}
