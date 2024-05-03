#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H


// 结构体存储 WiFi 配置
struct WiFiConfig {
  String ssid;
  String password;
  String APssid;
  String APpassword;
  String Webuser;
  String Webpass;
};

#endif // WIFI_CONFIG_H
