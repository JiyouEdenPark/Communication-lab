// ============================================================================
// Configuration
// ============================================================================
// Wi-Fi settings
const char* WIFI_SSID = "YOUR_WIFI_SSID";       // WiFi SSID
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";   // WiFi password

// Server info (used by client)
// Put the server board's IP here (printed on Serial by the server)
const char* SERVER_IP   = "YOUR_SERVER_IP";  // Example, change to real IP
const uint16_t SERVER_PORT = 80;

// ============================================================================
// Pin Definitions
// ============================================================================
#define BUTTON_PIN 3  // D3 pin for button switch (INPUT_PULLUP)

// ============================================================================
// Board-specific WiFi Library
// ============================================================================
#include <WiFiNINA.h>   // SAMD family (e.g., Nano 33 IoT)

// ============================================================================
// Global Variables
// ============================================================================
WiFiClient client;

// ============================================================================
// Function Prototypes
// ============================================================================
void connectWiFi();
void runClient();

// ============================================================================
// Setup Function
// ============================================================================

/**
 * Initialize the device and WiFi communication
 */
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== WiFi Example (SAMD, Client) ===");

  // Configure button pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to WiFi network
  connectWiFi();

  Serial.println("[MODE] CLIENT");
}

// ============================================================================
// Main Loop
// ============================================================================

/**
 * Main program loop
 * Handles button press and server communication
 */
void loop() {
  runClient();
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
// Client Mode Functions
// ============================================================================

/**
 * Main client loop
 * Handles button press, server connection, and message reception
 * Automatically reconnects on disconnection
 */
void runClient() {
  static bool connectedToServer = false;
  static bool lastButtonState = HIGH;  // For button edge detection

  // Handle button press (send TOGGLE command when button is pressed)
  bool buttonState = digitalRead(BUTTON_PIN);
  
  // Detect button press edge (LOW = pressed due to INPUT_PULLUP)
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button was just pressed - send TOGGLE command to server
    if (client.connected()) {
      Serial.println("[BUTTON] Sending TOGGLE to server");
      client.println("TOGGLE");
      client.flush();  // Force immediate transmission
    } else {
      Serial.println("[BUTTON] Not connected to server, cannot send TOGGLE");
    }
  }
  lastButtonState = buttonState;

  // Connect to server if not connected
  if (!connectedToServer) {
    Serial.print("[CLIENT] Trying to connect to server: ");
    Serial.print(SERVER_IP);
    Serial.print(":");
    Serial.println(SERVER_PORT);

    if (client.connect(SERVER_IP, SERVER_PORT)) {
      Serial.println("[CLIENT] Connected to server!");
      connectedToServer = true;

      // Set shorter timeout for faster response (default is 1000ms)
      client.setTimeout(50);

      // Send initial greeting message
      client.println("hello from client");
    } else {
      Serial.println("[CLIENT] Failed to connect, retrying in 5 seconds");
      delay(5000);
      return;
    }
  }

  // Receive and process messages from server
  if (client.connected() && client.available()) {
    String resp = client.readStringUntil('\n');
    resp.trim();
    if (resp.length() > 0) {
      Serial.print("[CLIENT] Server response: ");
      Serial.println(resp);
    }
  }

  // Handle disconnection - reset connection state and wait before retry
  if (!client.connected()) {
    Serial.println("[CLIENT] Disconnected from server. Will try to reconnect.");
    client.stop();
    connectedToServer = false;
    delay(2000);  // Wait before attempting reconnection
  }
}
