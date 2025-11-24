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
#define LED_PIN 12  // D12 pin for LED

// ============================================================================
// Board-specific WiFi Library
// ============================================================================
#include <WiFiNINA.h>   // SAMD family (e.g., Nano 33 IoT)

// ============================================================================
// Global Variables
// ============================================================================
WiFiServer server(SERVER_PORT);
static bool ledOn = false;

// ============================================================================
// Function Prototypes
// ============================================================================
void connectWiFi();
void runServer();
void setLed(bool on);
void toggleLed();

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

  // Configure LED pin as output and initialize to OFF
  pinMode(LED_PIN, OUTPUT);
  setLed(false);

  // Connect to WiFi network
  connectWiFi();

  // Start server and display connection info
  Serial.println("[MODE] SERVER");
  server.begin();
  Serial.print("Server port: ");
  Serial.println(SERVER_PORT);
  Serial.print("This board IP: ");
  Serial.println(WiFi.localIP());
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
 * Accepts new clients and processes TOGGLE commands
 * Closes connection after 30 seconds of inactivity
 */
void runServer() {
  // Check for new client connection
  WiFiClient clientConn = server.available();

  if (clientConn) {
    Serial.println("[SERVER] Client connected!");
    
    // Set shorter timeout for faster response (default is 1000ms)
    clientConn.setTimeout(50);

    unsigned long lastRecv = millis();

    // Process client messages while connected
    while (clientConn.connected()) {
      if (clientConn.available()) {
        String msg = clientConn.readStringUntil('\n');
        msg.trim();
        msg.toUpperCase();  // Convert to uppercase for case-insensitive comparison
        
        if (msg.length() > 0) {
          Serial.print("[SERVER] Received: ");
          Serial.println(msg);

          // Process TOGGLE command
          if (msg == "TOGGLE") {
            toggleLed();
            clientConn.println("LED toggled");
          }
        }
        lastRecv = millis();
      }
    }

    // Clean up connection
    clientConn.stop();
    Serial.println("[SERVER] Client disconnected");
  }
}

// ============================================================================
// LED Control Functions
// ============================================================================

/**
 * Set LED state (ON/OFF)
 * @param on true to turn on, false to turn off
 */
void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  Serial.print("[LED] State -> ");
  Serial.println(on ? "ON" : "OFF");
}

/**
 * Toggle LED state
 * Switches LED between ON and OFF states
 */
void toggleLed() {
  setLed(!ledOn);
}
