#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
// #include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "WiFiConfig.h"

const char* defaultSSID = "一楼";
const char* defaultPassword = "85585315";

const char* defaultAPssid = "ESP_AP";
const char* defaultAPpassword = "85585315";

const char* defaultusername = "admin";
const char* defaultpassword = "esp8266";

ESP8266WebServer server(80);


// loop 循环执行检查
unsigned long lastConnectionCheckTime = 0; // 上一次检查连接的时间
const unsigned long connectionInterval = 5 * 60 * 1000; // 连接尝试间隔时间（毫秒）

bool apModeActive = false; // 记录AP模式是否处于激活状态

bool wifiModeHasBeenSet = false; // 全局变量，用于跟踪WiFi模式是否已经设置过


const int ledPin = 2; // LED连接到GPIO2
const int gatePin = 5; // 大门连接到GPIO5 (D1)

// 函数读取配置文件并返回 WiFiConfig 结构体
WiFiConfig readWiFiConfig() {
  WiFiConfig config;
  if (!LittleFS.begin()) {
    Serial.println("挂载LittleFS时发生错误");
    // 使用默认配置
    config.ssid = defaultSSID;
    config.password = defaultPassword;
    config.APssid = defaultAPssid;
    config.APpassword = defaultAPpassword;
    config.Webuser = defaultusername;
    config.Webpass = defaultpassword;

    return config; // 返回空的配置
  }

  if (!LittleFS.exists("/config.json")) {
    Serial.println("配置文件不存在");
    // 使用默认配置
    config.ssid = defaultSSID;
    config.password = defaultPassword;
    config.APssid = defaultAPssid;
    config.APpassword = defaultAPpassword;
    config.Webuser = defaultusername;
    config.Webpass = defaultpassword;
    return config; // 返回空的配置
  }

    // 查看配置内容
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("打开配置文件失败");
    return config; // 返回空的配置
  }

  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  configFile.close();

  if (error) {
    Serial.print("JSON配置解析失败: ");
    Serial.println(error.c_str());
    return config; // 返回空的配置
  }

  config.ssid = doc["wifi"]["ssid"].as<String>();
  config.password = doc["wifi"]["password"].as<String>();

  config.APssid = doc["ap"]["ssid"].as<String>();
  config.APpassword = doc["ap"]["password"].as<String>();

  config.Webuser = doc["web"]["username"].as<String>();
  config.Webpass = doc["web"]["password"].as<String>();

  return config;
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(gatePin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(gatePin, LOW);
  
    // 读取配置
  WiFiConfig config = readWiFiConfig();
 

  // 使用读取的配置连接 WiFi
  if (!config.ssid.isEmpty() && !config.password.isEmpty()) {
    // 连接WiFi
    connectWiFi();
    // 连接重试次数计数器
    int retries = 0;
    // 循环等待WiFi连接成功，最多尝试20次
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500); // 等待500毫秒
        Serial.print("."); // 打印连接状态指示
        retries++; // 增加重试次数
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi连接失败...");
        // 启动AP模式
        startAPMode();
    } else {
        Serial.println("\nWiFi连接成功...");
        Serial.print("当前WiFi: ");
        Serial.println(config.ssid.c_str());
        Serial.print("当前密码: ");
        Serial.println(config.password.c_str());
        Serial.print("获取的IP地址: ");
        Serial.println(WiFi.localIP());
        Serial.println("启动OTA更新...");
        ArduinoOTA.begin();
    }
  } else {
    Serial.println("无法读取 WiFi 配置，启动 AP 模式...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(config.APssid.c_str(), config.APpassword.c_str()); 
    apModeActive = true; 
  }
  configweb();
}


void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  
  // 检查串口是否有数据
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // 清除可能的空白字符
    if (command == "delete") {
      if (LittleFS.exists("/config.json")) {
        LittleFS.remove("/config.json");
        Serial.println("配置文件已删除, 正在重启...");
        ESP.restart();
      } else {
        Serial.println("配置文件不存在，无法删除。");
      }
    }
  }

  unsigned long currentMillis = millis();
  // 检查是否到达间隔时间
  if (currentMillis - lastConnectionCheckTime >= connectionInterval) {
    // 更新上次检查的时间
    lastConnectionCheckTime = currentMillis;
    // 执行WiFi连接检查
    checkWiFiConnect();
  }
}


void checkWiFiConnect() {
    WiFiConfig config = readWiFiConfig();
    // 判断WiFi是否已连接
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("已连接到WiFi！");
        if (wifiModeHasBeenSet) {
           ESP.restart();
        }
    } else {
        Serial.println("WiFi连接中断！");
        if (!wifiModeHasBeenSet) { // 检查WiFi模式是否已经设置
            // 设置WIFI_AP_STA，WiFi尝试连接
            WiFi.mode(WIFI_AP_STA);
            wifiModeHasBeenSet = true; // 设置WiFi模式标志
            WiFi.begin(config.ssid.c_str(), config.password.c_str());
            WiFi.softAP(config.APssid.c_str(), config.APpassword.c_str());
        }
    }
}

void connectWiFi() {
    // 读取 WiFi 配置
    WiFiConfig config = readWiFiConfig();
    // 设置WiFi工作模式为STA模式（Station模式）
    WiFi.mode(WIFI_STA);
    Serial.println("正在连接到WiFi...");
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
}

// 启动AP模式函数
void startAPMode() {
  // 读取 AP 配置
  WiFiConfig config = readWiFiConfig();
  // 将WiFi模式设置为AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config.APssid.c_str(), config.APpassword.c_str());
  apModeActive = true; // 标记AP模式处于激活状态
}

// 关闭AP模式函数
void turnOffAPMode() {
  WiFi.softAPdisconnect(true); // 关闭AP模式
  apModeActive = false; // 标记AP模式处于非激活状态
}

