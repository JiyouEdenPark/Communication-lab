#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

// ============================================================================
// Pin Definitions
// ============================================================================
#define LED_PIN    20  // NeoPixel LED pin
#define ENABLE_PIN 19  // Enable pin for external hardware
#define NUM_LEDS   1   // Number of NeoPixel LEDs
#define BUTTON_PIN BOOT_PIN  // Boot button pin (NanoC6)

// ============================================================================
// Global Variables
// ============================================================================
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
static bool ledOn = false;

// Target peer MAC address (replace with actual MAC address of peer device)
static const uint8_t PEER_B_MAC[6] = {0x54, 0x32, 0x04, 0x1F, 0x6B, 0xDC};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert MAC address to string
 */
static String macToString(const uint8_t mac[6]) {
  char buf[18];
  sprintf(buf, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

/**
 * Print MAC address as 12-digit hex string
 */
static void printMac12(const uint8_t mac[6]) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print('0');
    Serial.print(mac[i], HEX);
  }
}

/**
 * Ensure peer is registered in ESP-NOW
 */
static bool ensurePeer(const uint8_t mac[6]) {
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = WiFi.channel();  // Use current WiFi channel
    peerInfo.ifidx = WIFI_IF_STA;       // Use station interface
    peerInfo.encrypt = false;
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result != ESP_OK) {
      Serial.print("[esp-now] add_peer failed: ");
      Serial.println(result);
      return false;
    }
  }
  return true;
}

/**
 * Send data to peer
 */
static void sendToPeer(const uint8_t mac[6], const char *str) {
  if (!ensurePeer(mac)) return;
  esp_now_send(mac, (const uint8_t*)str, strlen(str));
}

/**
 * Callback when data is sent
 */
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("[esp-now] sent to ");
  printMac12(mac_addr);
  Serial.print(" status=");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ============================================================================
// LED Control Functions
// ============================================================================

/**
 * Set LED state (ON/OFF)
 * @param on true to turn on (red), false to turn off
 */
static void setLed(bool on) {
  ledOn = on;
  strip.setPixelColor(0, on ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0));
  strip.show();
  Serial.printf("[LIGHT] State -> %s\n", on ? "ON" : "OFF");
}

/**
 * Toggle LED state
 */
static void toggleLed() {
  setLed(!ledOn);
}

// ============================================================================
// ESP-NOW Receive Callback
// ============================================================================

/**
 * Callback function called when ESP-NOW data is received
 * Parses the received message and controls LED accordingly
 * @param info ESP-NOW receive information (contains sender MAC address)
 * @param data Received data buffer
 * @param len Length of received data
 */
static void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len <= 0) return;
  
  // Identify sender by MAC address
  String senderMac = macToString(info->src_addr);
  
  // Print sender MAC address
  Serial.print("[esp-now] received from ");
  printMac12(info->src_addr);
  Serial.print(": ");
  
  // Extract printable characters from received data
  String s;
  for (int i = 0; i < len; i++) {
    char c = (char)data[i];
    if (isprint(c)) s += c;
  }
  s.trim();
  s.toUpperCase();
  Serial.println(s);

  // Check if message is from known peer
  Serial.println("[INFO] Message from PEER_B");
  // Process commands from PEER_B
  if (s == "ON") {
    setLed(true);
  } else if (s == "OFF") {
    setLed(false);
  } else if (s == "TOGGLE") {
    toggleLed();
  }
}

// ============================================================================
// Setup Function
// ============================================================================

/**
 * Initialize the device and ESP-NOW communication
 */
void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed! Rebooting...");
    delay(500);
    ESP.restart();
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Print own MAC address for peer configuration
  uint8_t selfMac[6];
  WiFi.macAddress(selfMac);
  Serial.print("Self MAC12: ");
  printMac12(selfMac);
  Serial.println();
  Serial.println("ESP-NOW PEER_A ready.");

  // Configure hardware pins
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize NeoPixel
  strip.begin();
  strip.show();
  setLed(false);
}

// ============================================================================
// Main Loop
// ============================================================================

/**
 * Main program loop
 * Handles button press and serial input for sending ESP-NOW messages
 */
void loop() {
  // Handle button press (send TOGGLE command when button is pressed)
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  
  // Detect button press (LOW = pressed due to INPUT_PULLUP)
  if (buttonState == LOW && lastButtonState == HIGH) {
    sendToPeer(PEER_B_MAC, "TOGGLE");
    Serial.println("[BUTTON] TOGGLE sent");
  }
  lastButtonState = buttonState;

  // Handle serial input (send custom commands to peer)
  static String line;
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      // End of line: send command
      if (line.length() > 0) {
        String cmd = line;
        line = "";
        cmd.trim();
        sendToPeer(PEER_B_MAC, cmd.c_str());
      }
    } else {
      // Accumulate characters
      line += c;
      // Prevent buffer overflow
      if (line.length() > 120) line.remove(0, line.length() - 120);
    }
  }
  delay(10);
}