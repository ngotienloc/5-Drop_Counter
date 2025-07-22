#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define RXD2 16  // ESP32 UART2 RX <- STM32 TX
#define TXD2 17  // ESP32 UART2 TX -> STM32 RX

//gcloud auth application-default print-access-token

// WiFi
const char* ssid = "MSI 9113";
const char* password = "12345678";
const String ACCESS_TOKEN = "ya29.a0AW4Xtxga7nlNYCqRxuOEkaGQDkble8NqNJ_cgFtu_MgIqxMXhV5yoUBlHOisRf87eBOd10YkRAFoIwvorQ58Fr2k5qLw2XWTzbV9Bm6OInBerkUZuz3nRT18GOhhWi2W6miVsxQtYAr1ysl1uM86nFqNjOwg1K7TZa9Fhk9flAaCgYKAfMSARcSFQHGX2MiQfUoZqPEHI50VJrct8lL-g0177";  // Tạm thời dùng token thủ công

// Firebase project
const String PROJECT_ID = "drop-counter";
String fieldName; 
int drop_count = 0;

const char* DOCUMENT_PATH = "drop_counter/Counter";

unsigned long lastUpdate = 0;           // Thời điểm lần cập nhật gần nhất
unsigned long lastDocumentSwitch = 0;   // Thời điểm chuyển document gần nhất
int fieldInterval = 300000;           // Thời gian tạo Field mới: 5 phút

// Định nghĩa NTP Client để lấy thời gian
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 600000); // +7 giờ cho Việt Nam

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");


  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART2 - giao tiếp STM32

  Serial.println("ESP32 UART2 bắt đầu...");

  fieldName = updateFieldPath();

  // Khởi động NTP Client
  timeClient.begin();

    // Gửi dữ liệu lên Firestore
  sendToFirestore(drop_count);
}

  void loop() {
  unsigned long now = millis();
  if(now - lastDocumentSwitch  > fieldInterval){
    fieldName = updateFieldPath();
    lastDocumentSwitch = now;
    Serial2.println("3"); 
  }
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');  // Đọc tới khi gặp '\n' hoặc timeout
    Serial.print("ESP nhận từ STM32: ");
    Serial.println(data);
    int drop_count = data.toInt(); 
    sendToFirestore(drop_count);
  }
} 

void sendToFirestore(int count) {
  HTTPClient http;

 String url = "https://firestore.googleapis.com/v1/projects/" + PROJECT_ID +
             "/databases/(default)/documents/" + DOCUMENT_PATH + "?updateMask.fieldPaths=" + fieldName; 


  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + ACCESS_TOKEN);

  // JSON theo Firestore định dạng
  String jsonPayload = "{ \"fields\": { \"" + fieldName + "\": { \"integerValue\": \"" + String(count) + "\" } } }";
  int httpResponseCode = http.sendRequest("PATCH", jsonPayload);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.println("Error sending data");
  }
  http.end();
}
String updateFieldPath(){
  char fieldName[20];
  timeClient.update();                                                //Update thời gian thực 
  time_t epochTime = timeClient.getEpochTime();
  struct tm *timeInfo = localtime(&epochTime);                        // Lấy về dạng echp time
  strftime(fieldName, sizeof(fieldName), "drop_%H_%M_%S", timeInfo); 
  return fieldName;
}

