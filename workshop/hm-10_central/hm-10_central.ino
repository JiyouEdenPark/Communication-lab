// ============================================================================
// HM-10 Central - BLE 중앙 장치
// ============================================================================
// 이 코드는 BLE(블루투스) 중앙 장치로, 다른 장치(Peripheral)에 연결합니다.
// 연결된 장치의 신호 강도(RSSI)를 확인하여 표시합니다.
// ============================================================================

#include <SoftwareSerial.h>

// ============================================================================
// 설정 값들
// ============================================================================

// HM-10 블루투스 모듈과 통신하기 위한 설정
SoftwareSerial BT(2, 3);  // 2번 핀: 데이터 받기, 3번 핀: 데이터 보내기

// 연결할 상대방 장치의 주소 (MAC 주소라고 부릅니다)
const char* TARGET_ADDR = "685E1C269427";

// ============================================================================
// 상태를 저장하는 변수들
// ============================================================================

bool isConnected = false;  // 연결되었는지 여부 (true = 연결됨, false = 연결 안됨)
bool isConnecting = false;  // 연결 시도 중인지 여부 (true = 연결 시도 중, false = 연결 시도 안함)

unsigned long lastReconnectAttempt = 0;  // 마지막으로 재연결을 시도한 시간
const unsigned long RECONNECT_INTERVAL = 3000;  // 3초마다 재연결 시도

unsigned long lastRssiCheck = 0;  // 마지막으로 신호 강도를 확인한 시간
const unsigned long RSSI_CHECK_INTERVAL = 1000;  // 1초마다 신호 강도 확인

int rssiErrorCount = 0;  // RSSI가 -100인 경우의 연속 횟수
const int RSSI_ERROR_THRESHOLD = 10;  // 연결 끊김으로 간주할 RSSI 오류 횟수

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
    isConnecting = false;  // 연결 시도 중 상태 해제
    Serial.println(F("[연결 끊김] 연결이 끊어졌습니다."));
    lastReconnectAttempt = 0;  // 즉시 재연결 시도하도록 설정
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
  Serial.println(F("\n=== HM-10 Central 시작 ==="));

  // 2단계: HM-10 블루투스 모듈과 통신 시작
  BT.begin(9600);
  delay(500);
  Serial.println(F("HM-10 모듈 초기화 중..."));

  // 3단계: HM-10을 Central 모드로 설정
  Serial.println(F("\n[설정] HM-10 모듈 설정 중..."));
  Serial.println(at("AT", 3000));              // 연결 테스트
  Serial.println(at("AT+ROLE1", 1000));        // Central 모드로 설정 (1 = 중앙 장치)
  Serial.println(at("AT+IMME1", 1000));        // 수동 모드 설정
  Serial.println(at("AT+MODE2", 1000));        // 모드 2 설정
  Serial.println(at("AT+RESET", 1500));        // 설정 적용을 위해 리셋
  delay(1000);
  Serial.println(F("[설정] 완료"));

  // 4단계: 내 장치의 주소(MAC 주소) 확인 및 표시
  String macResp = at("AT+ADDR?", 1000);
  Serial.print(F("\n[내 주소] MAC: "));
  Serial.println(macResp);

  // 5단계: 상대방 장치에 연결 시도
  Serial.print(F("\n[연결] 상대방 장치에 연결 시도 중... (주소: "));
  Serial.print(TARGET_ADDR);
  Serial.println(F(")"));
  
  String cmd = String("AT+CON") + TARGET_ADDR;
  String res = at(cmd, 2000);
  delay(1000);
  Serial.println(res);

  // 6단계: 연결 결과 확인
  processConnectionMessage(res);
  
  // 연결 관련 메시지가 아닌 경우
  if (res.indexOf("CONNE") < 0 && 
      res.indexOf("CONNA") < 0 && 
      !(res.lastIndexOf("CONN") > res.indexOf("CONNA"))) {
    Serial.println(F("\n[연결 상태 불명] 응답을 확인할 수 없습니다."));
    isConnected = false;
    isConnecting = false;  // 연결 시도 중 상태 해제
  }
  
  Serial.println(F("\n=== 준비 완료! ===\n"));
}

// ============================================================================
// 연결 메시지를 해석하여 상태를 업데이트하는 함수
// ============================================================================

static void processConnectionMessage(const String& msg) {
  // 연결이 끊어진 경우 (OK+LOST 응답)
  if (msg.indexOf("OK+LOST") >= 0) {
    isConnected = false;
    isConnecting = false;  // 연결 시도 중 상태 해제
    Serial.println(F("\n[연결 끊김] 상대방 장치와 연결이 끊어졌습니다."));
    lastReconnectAttempt = 0;  // 즉시 재연결 시도하도록 설정
  }
  // 연결 실패 메시지 (CONNF)
  else if (msg.indexOf("CONNF") >= 0) {
    isConnected = false;
    isConnecting = false;  // 연결 시도 중 상태 해제
    Serial.println(F("\n[연결 실패] 연결에 실패했습니다."));
    lastReconnectAttempt = 0;  // 즉시 재연결 시도하도록 설정
  }
  // 연결 실패 메시지 (CONNE)
  else if (msg.indexOf("CONNE") >= 0) {
    isConnected = false;
    isConnecting = false;  // 연결 시도 중 상태 해제
    Serial.println(F("[재연결 실패] 나중에 다시 시도하겠습니다."));
  }
  // 연결 성공 메시지 (CONN)
  else if (msg.lastIndexOf("CONN") > msg.indexOf("CONNA")) {
    isConnected = true;
    isConnecting = false;  // 연결 시도 중 상태 해제
    rssiErrorCount = 0;  // RSSI 오류 카운터 리셋
    Serial.println(F("\n[연결됨] 상대방 장치와 연결되었습니다."));
  }
  // 연결 중 메시지 (CONNA)
  else if (msg.indexOf("CONNA") >= 0) {
    isConnected = false;
    isConnecting = true;  // 연결 시도 중 상태 설정
    Serial.println(F("\n[연결 중] 연결을 시도하고 있습니다..."));
  }
}

// ============================================================================
// 연결이 끊어졌을 때 다시 연결을 시도하는 함수
// ============================================================================

static void reconnect() {
  Serial.print(F("\n[재연결] 다시 연결 시도 중... (주소: "));
  Serial.print(TARGET_ADDR);
  Serial.println(F(")"));
  
  String cmd = String("AT+CON") + TARGET_ADDR;
  String res = at(cmd, 3000);
  delay(1000);

  Serial.println(res);
  processConnectionMessage(res);
  
  // 연결 관련 메시지가 아닌 경우
  if (res.indexOf("CONNE") < 0 && 
      res.indexOf("CONNA") < 0 && 
      !(res.lastIndexOf("CONN") > res.indexOf("CONNA"))) {
    Serial.println(F("[재연결 상태 불명] 응답을 확인할 수 없습니다."));
    isConnected = false;
    isConnecting = false;  // 연결 시도 중 상태 해제
  }
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
      // 연결 관련 메시지 처리
      processConnectionMessage(msg);
      
      // 연결 관련 메시지가 아닌 경우에만 출력
      if (msg.indexOf("OK+LOST") < 0 && 
          msg.indexOf("CONNF") < 0 && 
          msg.indexOf("CONNE") < 0 && 
          msg.indexOf("CONNA") < 0 &&
          !(msg.lastIndexOf("CONN") > msg.indexOf("CONNA"))) {
        Serial.print(F("[받은 메시지] "));
        Serial.println(msg);
      }
    }
  }
  
  // 2단계: 연결이 끊어진 경우 자동으로 재연결 시도
  if (!isConnected) {
    unsigned long now = millis();
    // 마지막 재연결 시도로부터 3초가 지났으면
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL && !isConnecting) {
      lastReconnectAttempt = now;
      reconnect();
    }
  }
  
  // 3단계: 연결된 경우 주기적으로 신호 강도 확인 및 표시
  if (isConnected) {
    unsigned long now = millis();
    // 마지막 확인으로부터 1초가 지났으면
    if (now - lastRssiCheck >= RSSI_CHECK_INTERVAL) {
      lastRssiCheck = now;
      
      int rssi = getRSSI();
      if (rssi > -100) {
        // 정상적인 RSSI 값이 나오면 오류 카운터 리셋
        rssiErrorCount = 0;
        
        Serial.print(F("[신호 강도] "));
        Serial.println(rssi);
      } else {
        // RSSI가 -100인 경우 카운트 증가
        rssiErrorCount++;
        Serial.print(F("\n[RSSI 오류] RSSI: -100 (연속 "));
        Serial.print(rssiErrorCount);
        Serial.print(F("회)"));
        
        // 10번 이상이면 연결이 끊어진 것으로 간주
        if (rssiErrorCount >= RSSI_ERROR_THRESHOLD) {
          Serial.println(F(" -> 연결 끊김으로 간주"));
          isConnected = false;
          isConnecting = false;  // 연결 시도 중 상태 해제
          rssiErrorCount = 0;  // 카운터 리셋
          lastReconnectAttempt = 0;  // 즉시 재연결 시도하도록 설정
        } else {
          Serial.println(F(""));
        }
      }
    }
  }
  
  delay(10);  // 10밀리초 대기 (너무 빠르게 반복하지 않도록)
}
