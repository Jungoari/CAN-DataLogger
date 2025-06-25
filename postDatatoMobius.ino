//CAN-DataLogger

/*The following code enables an ESP32 to read CAN data using an MCP2515 module and upload it to a Mobius server.
For stable CAN communication, a 120Ω termination resistor must be installed in parallel between CAN_H and CAN_L at both ends of the CAN bus.
To ensure proper operation of this code, you need to configure the SSID and password of the Wi-Fi network connected to the ESP32, as well as the IP address, port number, AE (Application Entity), and container (cnt) information of the Mobius server.

Written by Jeongwon Choi,
Department of Internet of Things, Soonchunhyang University
2025-06-25
*/

#include <SPI.h>
#include <mcp_can.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== [1] Wi-Fi 설정 =====
const char* ssid = ""; // your wifi ssid here
const char* password = ""; // your wifi password here

// ===== [2] Mobius 설정 =====
const char* mobiusHost = ""; // Server ip
const char* origin = ""; // server aei
const char* requestId = "req12345";
const char* contentType = "application/json;ty=4";

// ===== [3] CAN 설정 =====
const int SPI_CS_PIN = 5;
MCP_CAN CAN(SPI_CS_PIN);

void setup() {
  Serial.begin(9600);

  // Wi-Fi 연결
  WiFi.begin(ssid, password);
  Serial.print("Wi-Fi 연결 중");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Wi-Fi 연결 완료");

  // CAN 초기화
  while (CAN_OK != CAN.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ)) {
    Serial.println(" CAN 초기화 실패, 다시 시도 중...");
    delay(500);
  }

  Serial.println(" CAN 초기화 완료 (수신 모드)");
  CAN.setMode(MCP_NORMAL);
}

void loop() {
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char buf[8];

    CAN.readMsgBuf(&rxId, &len, buf);

    // 시리얼 출력
    Serial.print(" 수신된 CAN ID: 0x");
    Serial.println(rxId, HEX);

    Serial.print("데이터: ");
    for (int i = 0; i < len; i++) {
      Serial.print(buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // JSON 형태로 Mobius에 업로드
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(mobiusHost);
      http.addHeader("X-M2M-Origin", origin);
      http.addHeader("X-M2M-RI", requestId);
      http.addHeader("Content-Type", contentType);

      // 데이터 → 문자열로 변환
      String payload = "{\"m2m:cin\":{\"con\":\"";
      payload += "ID=0x" + String(rxId, HEX) + " ";
      for (int i = 0; i < len; i++) {
        if (buf[i] < 0x10) payload += "0";
        payload += String(buf[i], HEX);
        payload += " ";
      }
      payload += "\"}}";

      int code = http.POST(payload);
      if (code > 0) {
        Serial.print(" Mobius 응답 코드: ");
        Serial.println(code);
      } else {
        Serial.print(" 업로드 실패, 코드: ");
        Serial.println(code);
      }

      http.end();
    } else {
      Serial.println(" Wi-Fi 연결 안 됨, 업로드 생략");
    }
  }

  delay(10);  // 너무 빠르게 루프 돌지 않도록 대기
}