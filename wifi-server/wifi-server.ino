// ============================================================================
// Configuration
// ============================================================================
// Wi-Fi settings
const char* WIFI_SSID = "YOUR_WIFI_SSID";       // WiFi SSID
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";   // WiFi password

const uint16_t SERVER_PORT = 80;

// ============================================================================
// Pin Definitions
// ============================================================================
#define LED_PIN  12  // D12 pin for LED (Client 1)
#define LED_PIN2 7   // D7 pin for LED (Client 2)

// ============================================================================
// Board-specific WiFi Library
// ============================================================================
#include <WiFiNINA.h>   // SAMD family (e.g., Nano 33 IoT)

// ============================================================================
// Global Variables
// ============================================================================
WiFiServer server(SERVER_PORT);
static bool led1On = false;  // LED 1 (D12) state
static bool led2On = false;  // LED 2 (D7) state

// No client management needed - clients send TOGGLE1 or TOGGLE2 directly

// ============================================================================
// Function Prototypes
// ============================================================================
void connectWiFi();
void runServer();
void setLed(uint8_t ledNum, bool on);
void toggleLed(uint8_t ledNum);
void processClient(WiFiClient client);

// ============================================================================
// Setup Function
// ============================================================================

/**
 * Initialize the device and WiFi communication
 */
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== WiFi Example (SAMD, Server) ===");

  // Configure LED pins as output and initialize to OFF
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  setLed(1, false);
  setLed(2, false);

  // Initialize LED clients (no explicit initialization needed)

  // Connect to WiFi network
  connectWiFi();

  // Start server and display connection info
  Serial.println("[MODE] SERVER");
  server.begin();
  Serial.print("Server port: ");
  Serial.println(SERVER_PORT);
  Serial.print("This board IP: ");
  Serial.println(WiFi.localIP());
  
  // Display LED control mode
  Serial.println("\n[LED Control Mode]");
  Serial.println("Clients send TOGGLE1 or TOGGLE2 to control LED 1 or LED 2");
  Serial.println("Waiting for client connections...");
  Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

/**
 * Main program loop
 * Handles client connections and processes received messages
 */
void loop() {
  runServer();
}

// ============================================================================
// WiFi Connection Functions
// ============================================================================

/**
 * Connect to WiFi network
 * Retries connection until successful or timeout (30 seconds)
 * Restarts device if connection fails
 */
void connectWiFi() {
  Serial.print("Connecting to WiFi... SSID: ");
  Serial.println(WIFI_SSID);

  uint8_t retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(500);
    Serial.print(".");
    retry++;
    if (retry > 60) { // About 30 seconds timeout
      Serial.println();
      Serial.println("WiFi connection failed, restarting...");
      delay(2000);
      NVIC_SystemReset();
    }
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ============================================================================
// Server Mode Functions
// ============================================================================

/**
 * Handle server connections and process client messages
 * Processes clients as they become available
 */
void runServer() {
  // Process any available client
  WiFiClient client = server.available();
  if (client) {
    processClient(client);
  }
}

/**
 * Process messages from a client
 * Handles TOGGLE1 and TOGGLE2 commands
 */
void processClient(WiFiClient client) {
  if (!client || !client.connected()) return;
  if (!client.available()) return;
  
  client.setTimeout(50);
  
  String msg = client.readStringUntil('\n');
  msg.trim();
  if (msg.length() == 0) return;
  
  msg.toUpperCase();
  
  // Handle TOGGLE1 command
  if (msg == "TOGGLE1") {
    toggleLed(1);
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println(" toggled LED 1");
    client.println("LED 1 toggled");
  }
  // Handle TOGGLE2 command
  else if (msg == "TOGGLE2") {
    toggleLed(2);
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println(" toggled LED 2");
    client.println("LED 2 toggled");
  }
}

// ============================================================================
// LED Control Functions
// ============================================================================

/**
 * Set LED state (ON/OFF)
 * @param ledNum LED number (1 for D12, 2 for D7)
 * @param on true to turn on, false to turn off
 */
void setLed(uint8_t ledNum, bool on) {
  uint8_t pin = (ledNum == 1) ? LED_PIN : LED_PIN2;
  bool* ledState = (ledNum == 1) ? &led1On : &led2On;
  
  *ledState = on;
  digitalWrite(pin, on ? HIGH : LOW);
  Serial.print("[LED");
  Serial.print(ledNum);
  Serial.print("] State -> ");
  Serial.println(on ? "ON" : "OFF");
}

/**
 * Toggle LED state
 * Switches LED between ON and OFF states
 * @param ledNum LED number (1 for D12, 2 for D7)
 */
void toggleLed(uint8_t ledNum) {
  bool currentState = (ledNum == 1) ? led1On : led2On;
  setLed(ledNum, !currentState);
}
