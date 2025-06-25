// CAN-DataLogger
/*The following code uploads the current time information from the ESP32 to the Mobius server every second using the REST API.
To ensure the proper operation of this code, the Wi-Fi SSID and password to which the ESP32 is connected are required, along with the Mobius server’s IP address, port number, AE, and container (cnt) information.

Written by Jeongwon Choi, Department of Internet of Things, Soonchunhyang University.
2025-06-25
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// ======= [1] Wi-Fi Config =======
const char* ssid = ""; // your wifi ssid here
const char* password = ""; // your wifi password here

// ======= [2] Mobius Server Info =======
const char* mobiusHost = ""; // Server ip
const char* origin = ""; // server aei
const char* requestId = "req12345";
const char* contentType = "application/json;ty=4";

// ======= [3] NTP Config =======
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 9 * 3600;  // GMT+9 (KST) South.Korea
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.print("WiFi 연결 중");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n WiFi 연결됨");

  // NTP time Config
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("시간 동기화 중...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n 시간 동기화 완료");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeStr[9];  // "HH:MM:SS"
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

      Serial.print("현재 시간 전송: ");
      Serial.println(timeStr);

      // Mobius로 POST 요청 보내기
      HTTPClient http;
      http.begin(mobiusHost);
      http.addHeader("X-M2M-Origin", origin);
      http.addHeader("X-M2M-RI", requestId);
      http.addHeader("Content-Type", contentType);

      String payload = String("{\"m2m:cin\":{\"con\":\"") + timeStr + "\"}}";
      int code = http.POST(payload);

      if (code > 0) {
        Serial.print("응답 코드: ");
        Serial.println(code);
        if (code != 201) {
          Serial.println("서버 응답 에러 내용:");
          Serial.println(http.getString());
        }
      } else {
        Serial.print("전송 실패, 오류 코드: ");
        Serial.println(code);
      }

      http.end();
    } else {
      Serial.println("시간 정보 가져오기 실패");
    }
  }

  delay(1000);  // 1초마다 전송
}