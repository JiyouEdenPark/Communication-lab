// ============================================================================
// HM-10 Peripheral - BLE 주변 장치
// ============================================================================
// 이 코드는 BLE(블루투스) 주변 장치로, 중앙 장치(Central)가 연결하기를 기다립니다.
// ============================================================================

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

// ============================================================================
// 설정 값들
// ============================================================================

// HM-10 블루투스 모듈과 통신하기 위한 설정
SoftwareSerial BT(2, 3);  // 2번 핀: 데이터 받기, 3번 핀: 데이터 보내기

// 네오픽셀 LED 설정
#define NEOPIXEL_PIN 5    // 네오픽셀 LED 데이터 핀 (D5)
#define NEOPIXEL_COUNT 3  // 네오픽셀 LED 개수 (3개)

// 신호 강도에 따른 LED 제어 기준값
// RSSI: 실제 RSSI 값에 100을 더한 값 (예: 실제 RSSI -65 -> 반환값 35)
const int RSSI_THRESHOLD_HIGH = 35;  // RSSI > 35: 녹색 LED 3개 켜기
const int RSSI_THRESHOLD_LOW = 20;   // RSSI > 20: 녹색 LED 2개 켜기

// 네오픽셀 LED 초기화
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================================
// 상태를 저장하는 변수들
// ============================================================================

bool isConnected = false;  // 연결되었는지 여부 (true = 연결됨, false = 연결 안됨)

unsigned long lastRssiCheck = 0;  // 마지막으로 신호 강도를 확인한 시간
const unsigned long RSSI_CHECK_INTERVAL = 1000;  // 1초마다 신호 강도 확인

// 연결이 끊어졌을 때 LED 깜빡임을 위한 변수들
unsigned long lastBlinkTime = 0;  // 마지막으로 LED를 깜빡인 시간
const unsigned long BLINK_INTERVAL = 500;  // 0.5초마다 깜빡임 (켜짐/꺼짐)
bool blinkState = false;  // 현재 깜빡임 상태 (true = 켜짐, false = 꺼짐)

// ============================================================================
// 도구 함수들 (복잡한 작업을 쉽게 해주는 함수들)
// ============================================================================

// HM-10에서 한 줄의 메시지를 읽어오는 함수
// 타임아웃: 일정 시간 안에 메시지가 오지 않으면 그만 읽습니다
static String readLine(unsigned long timeoutMs = 400) {
  String line = "";  // 받은 메시지를 저장할 변수
  unsigned long startTime = millis();  // 시작 시간 기록
  
  // 타임아웃 시간이 지나지 않았으면 계속 읽기
  while (millis() - startTime < timeoutMs) {
    // 받을 데이터가 있으면
    while (BT.available()) {
      char c = BT.read();  // 한 글자씩 읽기
      
      // 줄바꿈 문자를 만나면 (한 줄이 끝났다는 뜻)
      if (c == '\r' || c == '\n') {
        if (line.length() > 0) {
          return line;  // 받은 메시지 반환
        }
      } else {
        line += c;  // 받은 글자를 메시지에 추가
      }
    }
  }
  
  return line;  // 타임아웃이 되면 받은 메시지 반환 (비어있을 수도 있음)
}

// HM-10에 명령을 보내고 응답을 받는 함수
// AT 명령: HM-10을 제어하는 특별한 명령어입니다
static String at(const String& command, uint16_t timeout = 1000) {
  // 1단계: 이전에 남아있던 데이터 모두 지우기
  while (BT.available()) {
    BT.read();
  }
  
  // 2단계: 명령 보내기
  BT.print(command);
  delay(50);  // HM-10이 명령을 처리할 시간 주기
  
  // 3단계: 응답 받기 (타임아웃 시간 안에)
  unsigned long startTime = millis();
  String response = "";
  
  while (millis() - startTime < timeout) {
    while (BT.available()) {
      char ch = (char)BT.read();
      response += ch;
      
      // "OK" 또는 "ERROR" 응답을 받으면 조금 더 기다린 후 종료
      if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) {
        delay(10);
      }
    }
    
    // 완전한 응답을 받았으면 종료
    if (response.length() > 0 && (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0)) {
      break;
    }
  }
  
  response.trim();  // 앞뒤 공백 제거
  return response;
}

// 연결된 장치의 신호 강도(RSSI)를 가져오는 함수
// 반환값: 실제 RSSI 값에 100을 더한 값 (예: RSSI -50 -> 반환값 50, 오류 시 -100)
static int getRSSI() {
  String response = at("AT+RSSI?", 1000);  // RSSI 값을 물어보는 명령 보내기

  // 연결이 끊어진 경우 (OK+LOST 응답)
  if (response.indexOf("OK+LOST") >= 0) {
    isConnected = false;
    Serial.println(F("[연결 끊김] 연결이 끊어졌습니다."));
    return -100;  // 연결 끊김으로 -100 반환
  }
  
  // 응답에서 "OK+Get:" 다음에 있는 숫자를 찾기
  int rssiIndex = response.indexOf("OK+Get:");
  if (rssiIndex >= 0) {
    int startIdx = rssiIndex + 7;  // "OK+Get:" 다음 위치
    String rssiStr = response.substring(startIdx);
    rssiStr.trim();
    
    // 숫자와 마이너스 기호만 남기고 나머지 제거
    String cleanRssi = "";
    for (int i = 0; i < rssiStr.length(); i++) {
      char c = rssiStr.charAt(i);
      if ((c >= '0' && c <= '9') || c == '-') {
        cleanRssi += c;
      }
    }
    
    // 숫자로 변환하여 100을 더한 후 반환 (RSSI 값 정규화)
    if (cleanRssi.length() > 0) {
      return cleanRssi.toInt() + 100;
    }
  }
  
  return -100;  // 오류가 발생했으면 -100 반환
}

// ============================================================================
// 시작할 때 한 번만 실행되는 함수 (초기화)
// ============================================================================

void setup() {
  // 1단계: 컴퓨터와 통신하기 위한 시리얼 통신 시작
  Serial.begin(9600);
  delay(100);
  Serial.println(F("\n=== HM-10 Peripheral 시작 ==="));

  // 2단계: HM-10 블루투스 모듈과 통신 시작
  BT.begin(9600);
  delay(500);
  Serial.println(F("HM-10 모듈 초기화 중..."));

  // 3단계: HM-10을 Peripheral 모드로 설정
  Serial.println(F("\n[설정] HM-10 모듈 설정 중..."));
  Serial.println(at("AT", 3000));              // 연결 테스트
  Serial.println(at("AT+NOTI1", 1000));        // 알림 기능 활성화
  Serial.println(at("AT+ROLE0", 1000));       // Peripheral 모드로 설정 (0 = 주변 장치)
  Serial.println(at("AT+IMME0", 1000));       // 자동 연결 모드 (즉시 광고 시작)
  Serial.println(at("AT+MODE2", 1000));       // 모드 2 설정
  Serial.println(at("AT+RESET", 1500));       // 설정 적용을 위해 리셋
  delay(1000);
  Serial.println(F("[설정] 완료"));

  // 4단계: 내 장치의 주소(MAC 주소) 확인 및 표시
  String macResp = at("AT+ADDR?", 1000);
  Serial.print(F("\n[내 주소] MAC: "));
  Serial.println(macResp);

  // 5단계: 네오픽셀 LED 초기화 및 테스트
  pixels.begin();
  pixels.setBrightness(50);  // 밝기 설정 (0-255, 필요에 따라 조절)
  pixels.clear();
  pixels.show();
  
  Serial.println(F("\n[LED 테스트] 네오픽셀 LED 테스트 중..."));
  delay(500);
  
  // 빨간색 테스트 (모든 LED)
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));  // 빨간색
  }
  pixels.show();
  Serial.println(F("  빨간색 (LED 3개)"));
  delay(500);
  
  // 녹색 테스트 (모든 LED)
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));  // 녹색
  }
  pixels.show();
  Serial.println(F("  녹색 (LED 3개)"));
  delay(500);
  
  // 개별 LED 테스트
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // LED 0번만 녹색
  pixels.show();
  Serial.println(F("  녹색 LED 0번"));
  delay(300);
  
  pixels.setPixelColor(1, pixels.Color(0, 255, 0));  // LED 0-1번 녹색
  pixels.show();
  Serial.println(F("  녹색 LED 0-1번"));
  delay(300);
  
  pixels.setPixelColor(2, pixels.Color(0, 255, 0));  // LED 0-2번 모두 녹색
  pixels.show();
  Serial.println(F("  녹색 LED 0-2번 (모두) - LED 테스트 완료"));
  delay(500);
  
  Serial.println(F("\n[대기] 중앙 장치의 연결을 기다리는 중..."));
  Serial.println(F("\n=== 준비 완료! ===\n"));
}

// ============================================================================
// 계속 반복해서 실행되는 함수 (메인 루프)
// ============================================================================

void loop() {
  // 1단계: HM-10으로부터 메시지가 있는지 확인
  if (BT.available()) {
    String msg = readLine();
    msg.trim();
    
    if (msg.length() > 0) {
      // 연결되었다는 메시지를 받으면
      if (msg.indexOf("OK+CONN") >= 0) {
        isConnected = true;
        Serial.println(F("\n[연결됨] 중앙 장치와 연결되었습니다."));
      }
      // 연결이 끊어졌다는 메시지를 받으면
      else if (msg.indexOf("OK+LOST") >= 0) {
        isConnected = false;
        Serial.println(F("\n[연결 끊김] 중앙 장치와 연결이 끊어졌습니다."));
      }
      // 다른 메시지를 받으면
      else {
        Serial.print(F("[받은 메시지] "));
        Serial.println(msg);
      }
    }
  }

  // 2단계: 연결이 끊어진 경우 LED 깜빡임
  if (!isConnected) {
    unsigned long now = millis();
    // 연결이 끊어진 경우 빨간색 LED 깜빡이기
    // 마지막 깜빡임으로부터 0.5초가 지났으면
    if (now - lastBlinkTime >= BLINK_INTERVAL) {
      lastBlinkTime = now;
      blinkState = !blinkState;  // 깜빡임 상태 바꾸기 (켜짐 <-> 꺼짐)
      
      pixels.clear();
      if (blinkState) {
        // 깜빡임 상태가 켜짐이면 빨간색 LED 3개 모두 켜기
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));  // 빨간색
        }
      }
      // 깜빡임 상태가 꺼짐이면 LED 모두 끄기 (이미 clear 했음)
      pixels.show();
    }
  }
  
  // 3단계: 연결된 경우 주기적으로 신호 강도 확인 및 LED 제어
  if (isConnected) {
    unsigned long now = millis();
    // 마지막 확인으로부터 1초가 지났으면
    if (now - lastRssiCheck >= RSSI_CHECK_INTERVAL) {
      lastRssiCheck = now;
      
      int rssi = getRSSI();
      if (rssi > -100) {
        Serial.print(F("[신호 강도] "));
        Serial.print(rssi);
        
        // 신호 강도에 따라 LED 제어
        pixels.clear();  // 먼저 모든 LED 끄기
        
        if (rssi > RSSI_THRESHOLD_HIGH) {
          // 신호가 매우 강함: 녹색 LED 3개 켜기
          for (int i = 0; i < 3; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 255, 0));  // 녹색
          }
          Serial.println(F(" [녹색 LED 3개]"));
        } else if (rssi > RSSI_THRESHOLD_LOW) {
          // 신호가 보통: 녹색 LED 2개 켜기
          pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // 녹색
          pixels.setPixelColor(1, pixels.Color(0, 255, 0));  // 녹색
          Serial.println(F(" [녹색 LED 2개]"));
        } else {
          // 신호가 약함: 녹색 LED 1개 켜기
          pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // 녹색
          Serial.println(F(" [녹색 LED 1개]"));
        }
        pixels.show();  // LED 상태 업데이트
      }
    }
  }
  
  delay(10);  // 10밀리초 대기 (너무 빠르게 반복하지 않도록)
}
