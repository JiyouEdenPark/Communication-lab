# Arduino Wireless Communication Project

This project provides example code for learning and comparing various wireless communication methods available on ESP32/ESP8266 based boards.

## 📡 Supported Wireless Communication Methods

### 1. ESP-NOW
**ESP32/ESP8266 Exclusive Peer-to-Peer Communication**

#### Features
- **Direct Communication**: Direct communication between ESP32/ESP8266 devices without WiFi router
- **Low Power**: Lower power consumption than WiFi connection
- **Fast Speed**: Low latency
- **Simple Setup**: Communication possible with just MAC address

#### When to Use?
- ✅ Sensor networks (temperature, humidity, etc.)
- ✅ Remote control or button-based device control
- ✅ Direct communication between IoT devices
- ✅ Environments without WiFi router
- ✅ Battery-powered devices

#### Communication Architecture: Peer-to-Peer (P2P)
**Symmetric Communication Structure**

In peer-to-peer architecture, all devices have equal roles and can communicate directly with each other:
- **No Central Authority**: No server or master device required
- **Bidirectional**: Any device can send or receive messages
- **Direct Connection**: Devices communicate directly without intermediaries
- **Flexible Roles**: Each device can act as both sender and receiver simultaneously

```
Device A ←→ Device B
   ↕         ↕
Device C ←→ Device D
```

**How it works:**
- Each device knows the MAC address of peers it wants to communicate with
- Messages are sent directly to target peer's MAC address
- Any device can initiate communication at any time
- No connection establishment required (connectionless)

#### Example Code
- `esp-now_peer-a/` - ESP-NOW peer device A (button controlled, LED control)
- `esp-now_peer-b/` - ESP-NOW peer device B (button controlled, LED control)

**Note**: Both devices support bidirectional communication - they can send and receive messages. Each device has a button to send TOGGLE commands and LED control to respond to received commands.

#### Usage
```cpp
// Communication possible with just MAC address
// Define peer MAC address as byte array
static const uint8_t PEER_MAC[6] = {0x54, 0x32, 0x04, 0x1F, 0x6B, 0xDC};
sendToPeer(PEER_MAC, "TOGGLE");  // Send message
```

---

### 2. WiFi Server/Client
**Internet-Based Communication**

#### Features
- **Internet Access**: Can access internet through router
- **Long Range**: Can communicate from anywhere in the world via internet
- **Standard Protocols**: Uses standard protocols like HTTP, TCP/IP
- **High Power Consumption**: Higher power consumption than ESP-NOW

#### Server Mode
- Receives and responds to connection requests from other devices
- Operates like a web server
- Requires fixed IP address

#### Client Mode
- Connects to other servers to request/send data
- Operates like a web browser
- Requires knowledge of server IP address

#### Communication Architecture: Server-Client
**Asymmetric Communication Structure**

In server-client architecture, devices have distinct roles with clear responsibilities:
- **Server**: Waits for and responds to connection requests
  - Passive role: Listens for incoming connections
  - Can handle multiple clients simultaneously
  - Provides services or resources
  - Must have a known IP address or hostname
  
- **Client**: Initiates connections to servers
  - Active role: Connects to servers when needed
  - Requests services or sends data
  - Must know server's IP address or hostname
  - Can connect to multiple servers

```
Client A ──┐
Client B ──┼──→ Server
Client C ──┘
```

**How it works:**
1. Server starts listening on a specific port (e.g., port 80)
2. Client initiates connection to server's IP address and port
3. Server accepts connection and establishes communication channel
4. Client sends requests, server responds
5. Connection can be closed by either party

**Key Differences from Peer-to-Peer:**
- **Role Separation**: Clear distinction between server and client roles
- **Connection-Based**: Requires establishing connection before communication
- **Centralized**: Server acts as central point for multiple clients
- **Request-Response**: Client requests, server responds pattern

#### When to Use?
- ✅ Remote control/monitoring via internet
- ✅ Web server setup (display sensor data on web page)
- ✅ Integration with cloud services (AWS, Firebase, etc.)
- ✅ When multiple devices need to communicate simultaneously
- ✅ When long-range communication is needed

#### Example Code
- `wifi-server/` - WiFi server (for SAMD boards)
- `wifi-client/` - WiFi client (for SAMD boards)

#### Usage
```cpp
// WiFi connection
WiFi.begin(WIFI_SSID, WIFI_PASS);

// Server mode
WiFiServer server(80);
server.begin();

// Client mode
WiFiClient client;
client.connect(SERVER_IP, 80);
```

---

## 🏗️ Communication Architecture Comparison

### Peer-to-Peer (P2P) vs Server-Client

| Aspect | Peer-to-Peer (ESP-NOW) | Server-Client (WiFi) |
|--------|------------------------|---------------------|
| **Structure** | Symmetric - All devices equal | Asymmetric - Distinct roles |
| **Roles** | All devices are peers | Server (passive) + Client (active) |
| **Connection** | Connectionless (direct send) | Connection-based (must connect first) |
| **Initiation** | Any device can send anytime | Client initiates, server responds |
| **Scalability** | Limited by broadcast range | Server can handle many clients |
| **Setup** | Just need MAC addresses | Need IP addresses, ports, protocols |
| **Flexibility** | Highly flexible, any-to-any | Structured, request-response pattern |
| **Use Case** | Direct device-to-device | Centralized services, web apps |

### Visual Comparison

**Peer-to-Peer (ESP-NOW):**
```
Device A ←────→ Device B
   ↕               ↕
Device C ←────→ Device D
```
- All devices communicate directly
- No central point
- Any device can message any other device

**Server-Client (WiFi):**
```
         ┌─────────┐
         │ Server  │
         └────┬────┘
              │
    ┌─────────┼─────────┐
    │         │         │
Client A  Client B  Client C
```
- Clients connect to server
- Server manages connections
- Clients request, server responds

---

## 🔄 Technology Comparison Table

| Feature | ESP-NOW | WiFi Server/Client |
|---------|---------|-------------------|
| **Range** | 100-200m | Router range + Internet |
| **Power Consumption** | Low | High |
| **Setup Complexity** | Very Simple | Complex |
| **Speed** | Fast (low latency) | Very Fast (high throughput) |
| **Internet Access** | No | Yes |
| **Smartphone Compatible** | No | Yes (with app) |
| **Multi-Device Communication** | Yes (point-to-point, broadcast supported) | Yes (sequential, concurrent requires additional implementation) |
| **Use Cases** | Sensor networks, Remote control | Web server, Cloud integration |

---

## 🎯 Project Selection Guide

### Want to send sensor data to another ESP32?
→ Use **ESP-NOW**
- Simplest and fastest
- No router configuration needed

### Want remote control via internet?
→ Use **WiFi Server/Client**
- Accessible via web browser
- Can integrate with cloud services

### Want to control multiple devices simultaneously?
→ Use **WiFi Server** or **ESP-NOW Broadcast**
- WiFi: Remote control via internet
- ESP-NOW: Fast control in local network

---

## 📚 Recommended Learning Order

1. **ESP-NOW** (Simplest)
   - Understand basic concepts
   - Practice peer-to-peer communication
   - Examples: `esp-now_peer-a`, `esp-now_peer-b`

2. **WiFi Server/Client** (Advanced)
   - Understand network concepts
   - Learn TCP/IP protocols
   - Examples: `wifi-server`, `wifi-client`

---

## 🛠️ Hardware Requirements

### ESP-NOW
- ESP32 or ESP8266 board
- No additional hardware required

### WiFi
- Board with WiFi module (ESP32, ESP8266, Nano 33 IoT, etc.)
- WiFi router required

---

## 📝 Notes

1. **ESP-NOW**: Exclusive to ESP32/ESP8266, cannot be used on other boards
2. **WiFi**: Router SSID and password must be configured in code
3. **Power Consumption**: WiFi > ESP-NOW in order of power consumption

---

## 🔗 References

- [ESP-NOW Official Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Arduino WiFi Library](https://www.arduino.cc/reference/en/libraries/wifi/)

---

## 📧 Contact

If you have questions or suggestions about this project, please open an issue.
