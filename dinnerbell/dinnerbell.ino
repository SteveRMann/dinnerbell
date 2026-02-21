// dinnerbell.ino
// Two identical units "talk" to each other. Either button toggles the LEDs on both units.
// One unit in the kitchen presses the button to turn on the lights, and the identical button in the office can stop them.

#include <ESP8266WiFi.h>
#include <espnow.h>

// -----------------------------
// Pins
// -----------------------------
const int BUTTON_PIN = D3;   // GPIO0, active LOW

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
// Receive callback
// -----------------------------
void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(Message)) return;

  Message msg;
  memcpy(&msg, data, sizeof(msg));

  ledState = msg.ledState;

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

  uint8_t broadcastAddr[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  esp_now_send(broadcastAddr, (uint8_t*)&msg, sizeof(msg));
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  for (int i = 0; i < 5; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onReceive);

  uint8_t broadcastAddr[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  esp_now_add_peer(broadcastAddr, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("Ready.");
}

// -----------------------------
// Loop
// -----------------------------
void loop() {
  bool button = digitalRead(BUTTON_PIN);

  // Detect falling edge (button press)
  if (button == LOW && lastButtonState == HIGH) {
    ledState = !ledState;  // Toggle

    if (!ledState) {
      // Turn all LEDs off
      for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
    }

    broadcastState();
    delay(200); // debounce
  }

  lastButtonState = button;

  // -----------------------------
  // LED chase animation
  // -----------------------------
  if (ledState) {
    unsigned long now = millis();
    if (now - lastStep >= 200) {   // ms per LED
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
