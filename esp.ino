#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>  
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

HTTPClient http;

ESP8266WebServer server(80);

const char* defaultSSID = "一楼";
const char* defaultPassword = "85585315";

const char* defaultAPssid = "ESP_AP";
const char* defaultAPpassword = "85585315";

const char* defaultusername = "admin";
const char* defaultpassword = "esp8266";

const char* pushplusToken = "0e1e35e102694f8ea708b143096c849c";

const int ledPin = 2; // LED连接到GPIO2
const int gatePin = 5; // 大门连接到GPIO5 (D1)


String ssid = defaultSSID;
String password = defaultPassword;

String ap_ssid = defaultAPssid;
String ap_password = defaultAPpassword;

String username = defaultusername;
String userpassword = defaultpassword;


void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(gatePin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(gatePin, LOW);
  LittleFS.begin();
  readConfigFile();
  connectToWiFi();  
  ArduinoOTA.begin();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggleLED", HTTP_GET, toggleLED);
  server.on("/openGate", HTTP_GET, openGate);
  server.on("/restartDevice", HTTP_GET, restartDevice);
  
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/script", HTTP_GET, handleScript);

  server.on("/config", HTTP_GET, handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/read", handleRead);
  server.on("/delete", deleteConfig);
  server.begin();
  }

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}

// 读取配置
void readConfigFile() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("配置文件读取失败，使用默认配置");
    return;
  }
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  DynamicJsonDocument doc(200);
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("解析配置文件失败");
    return;
  }
  ssid = doc["WiFi"]["ssid"].as<String>();
  password = doc["WiFi"]["password"].as<String>();
  ap_ssid = doc["AP"]["ssid"].as<String>();
  ap_password = doc["AP"]["password"].as<String>();
  username = doc["Web"]["user"].as<String>();
  userpassword = doc["Web"]["password"].as<String>();
  doc.clear();
  configFile.close();
}

// 连接WiFi
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  Serial.println("\n正在连接WiFi...");
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi连接失败!");
   // startAPMode();
  }
}




// AP模式
void startAPMode() {
  if (WiFi.getMode() != WIFI_AP) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("启动AP模式");
  } else {
    Serial.println("AP模式未关闭");
  }
}


// 身份认证
void BasicAuthentication() {
   if (!server.authenticate(username.c_str(), userpassword.c_str())) {
      return server.requestAuthentication();
    }
}

// pushplus消息推送
void sendPushPlusMessage(String message) {
  String url = "http://www.pushplus.plus/send";  // PushPlus消息推送API接口地址
  String body = "token=" + String(pushplusToken) + "&title=ESP8266&content=" + message;  // POST请求的消息体

  http.begin(url);  // 开始HTTP请求
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");  // 添加请求头

  int httpCode = http.POST(body);  // 发送POST请求

  if (httpCode > 0) {
    Serial.printf("[HTTP] PushPlus返回HTTP状态码: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("[HTTP] 响应内容: " + payload);
  } else {
    Serial.printf("[HTTP] 发送POST请求到PushPlus失败，错误码: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();  // 结束HTTP请求，释放资源
}


    
    
    