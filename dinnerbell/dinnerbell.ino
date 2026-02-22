// dinnerbell.ino
// Two identical units "talk" to each other. Either button toggles the LEDs on both units.
// One unit in the kitchen presses the button to turn on the lights, and the identical button in the office can stop them.

#include <ESP8266WiFi.h>
#include <espnow.h>

extern "C" {
#include "user_interface.h"
}


// -----------------------------
// Pins
// -----------------------------
const int BUTTON_PIN = D2;  // GPIO4, active High

// Five LEDs (change pins as needed)
const int LED_PINS[5] = { D4, D5, D6, D7, D8 };

// -----------------------------
// Shared state
// -----------------------------
volatile bool ledState = false;
bool lastButtonState = HIGH;

// Animation timing
unsigned long lastStep = 0;
int currentLed = 0;

// -----------------------------
// ESP-NOW message format
// -----------------------------
struct Message {
  bool ledState;
};

// -----------------------------
// Button debounce
// -----------------------------
bool lastReading = LOW;     // last raw read
bool debouncedState = LOW;  // last stable state
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 60;  // ms


// -----------------------------
// Receive callback
// -----------------------------
void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(Message)) return;

  Message msg;
  memcpy(&msg, data, sizeof(msg));

  ledState = msg.ledState;

  // Print RSSI of this packet
  Serial.print("Received packet from ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" RSSI: ");
  Serial.println(WiFi.RSSI());

  if (!ledState) {
    // Turn all LEDs off
    for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
  }
}

// -----------------------------
// Send helper
// -----------------------------
void broadcastState() {
  Message msg;
  msg.ledState = ledState;

  uint8_t broadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  esp_now_send(broadcastAddr, (uint8_t *)&msg, sizeof(msg));
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(BUTTON_PIN, INPUT);

  for (int i = 0; i < 5; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setOutputPower(20.5f);  // max TX power
  WiFi.disconnect();           // required for ESP-NOW


  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onReceive);

  uint8_t broadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  esp_now_add_peer(broadcastAddr, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("Ready.");
}

// -----------------------------
// Loop
// -----------------------------

void loop() {
bool reading = digitalRead(BUTTON_PIN);

// If the reading changed from last time, reset debounce timer
if (reading != lastReading) {
    lastDebounceTime = millis();
}

if ((millis() - lastDebounceTime) > debounceDelay) {
    // If stable and changed from previous debounced state
    if (reading != debouncedState) {
        debouncedState = reading;

        // Rising edge (LOW → HIGH)
        if (debouncedState == HIGH) {
            ledState = !ledState;

            if (!ledState) {
                for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
            }

            broadcastState();
        }
    }
}

lastReading = reading;


  /*
  if (button == LOW && lastButtonState == HIGH) {
    ledState = !ledState;  // Toggle

    if (!ledState) {
      // Turn all LEDs off
      for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
    }

    broadcastState();
    delay(200);  // debounce
  }
*/

  // -----------------------------
  // LED chase animation
  // -----------------------------
  if (ledState) {
    unsigned long now = millis();
    if (now - lastStep >= 200) {  // ms per LED
      lastStep = now;

      // Turn all LEDs off
      for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);

      // Turn on the current LED
      digitalWrite(LED_PINS[currentLed], HIGH);

      // Advance to next LED
      currentLed = (currentLed + 1) % 5;
    }
  }
}
