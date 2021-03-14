#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "constants.h" // Not included because it contains passwords & stuff

// Declare functions & global variables
WiFiClient client;
int bark_count = 0;
bool barking = false;

void sendToServer(int, int);
bool timer();


void setup() {
    // Start Serial Monitor
    Serial.begin(115200);
    
    // Connect to Wifi
    Serial.print("Connecting to ");
    Serial.println(SSID);
 
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");

    // Start ThingSpeak
    ThingSpeak.begin( client );
}

void loop() {
    // Ping the timer
    if (timer()) {
        sendToServer(1, bark_count);
        bark_count = 0;
    }

    // Read Microphone Data
    if (analogRead(MIC_PIN) > VOLUME_THRESHOLD && !barking) {
        Serial.println("Bark Detected!");
        barking = true;
        bark_count++;
    }

    // Only Count Each Bark Once
    while (barking) {
        if (analogRead(MIC_PIN) < VOLUME_THRESHOLD)
            barking = false;
    }
}

// Write Data to the Channel
void sendToServer(int field, int data) {
    if (ThingSpeak.writeField(CHANNEL, field, data, API_KEY)) {
        Serial.print("Sent Data: \"");
        Serial.print(barks);
        Serial.print(" Barks\" to Field: ");
        Serial.print(field)
        Serial.println("");
    }
    else
        Serial.println("")
    return;
}

// Signal if it's Time to Send Data to ThingSpeak
bool timer() {
    static long start = milis();

    if ( abs(milis() - start) / (SEND_INTERVAL * 1000) ) {
        start = milis();
        return true;
    }
    return false;
}