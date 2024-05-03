void configweb() {
  server.on("/", [config = readWiFiConfig()]() {
    if (!server.authenticate(config.Webuser.c_str(), config.Webpass.c_str())) {
      return server.requestAuthentication();
    }
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP8266 Web 服务器</title></head><body style='text-align: center; margin-top: 50px;'>";
    html += "<h2 style='margin-bottom: 20px;'>ESP8266 Web 服务器</h2>";
    html += "<p>欢迎使用</p>";
    html += "<button onclick='toggleLED(this)'>切换LED</button><br><br>";
    html += "<button onclick='openGate(this)'>开启大门</button><br><br>";
    html += "<button onclick='restartDevice(this)'>重启设备</button><br><br>";
    html += "<button onclick='location.href=\"/status\"'>查看设备状态</button>";
    html += "<script>";
    html += "function toggleLED(button) { button.disabled = true; fetch('/toggleLED').then(response => response.text()).then(data => { alert(data); button.disabled = false; }); setTimeout(() => { button.disabled = false; }, 1000); }";
    html += "function openGate(button) { button.disabled = true; fetch('/openGate').then(response => response.text()).then(data => { alert(data); button.disabled = false; }); setTimeout(() => { button.disabled = false; }, 1000); }";
    html += "function restartDevice(button) { button.disabled = true; fetch('/restartDevice').then(response => response.text()).then(data => { alert(data); button.disabled = false; }); setTimeout(() => { button.disabled = false; }, 1000); }";
    html += "</script></body></html>";
    server.send(200, "text/html", html);
  });

server.on("/toggleLED", []() {
    bool isOn = digitalRead(ledPin);
    digitalWrite(ledPin, !isOn);
    if (isOn) {
        server.send(200, "text/plain", "LED状态已开启");
    } else {
        server.send(200, "text/plain", "LED状态已关闭");
    }
});

  server.on("/openGate", []() {
    digitalWrite(gatePin, HIGH);
    delay(300);
    digitalWrite(gatePin, LOW);
    server.send(200, "text/plain", "大门已开启");
  });

  server.on("/restartDevice", []() {
    server.send(200, "text/plain", "设备即将重启...");
    delay(1000);
    ESP.restart();
  });
  

  
server.on("/config", HTTP_GET, [config = readWiFiConfig()]() {
    if (!server.authenticate(config.Webuser.c_str(), config.Webpass.c_str())) {
      return server.requestAuthentication();
    }
    // 构建HTML页面
    String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    page += "<style>";
    page += "body {font-family: Arial, sans-serif;}";
    // 按钮样式 margin-top 上边距 margin-right 右边距
    page += ".btn { margin-top: 15px; margin-right: 10px; }";
    page += "</style>";
    // JavaScript函数用于切换密码可见性
    page += "<script>function togglePasswordVisibility() {";
    page += "var passwords = document.querySelectorAll('input[type=password]');";
    page += "passwords.forEach(function(password) {";
    page += "if (password.type === 'password') {password.type = 'text';} else {password.type = 'password';}});}";
    // JavaScript函数用于发送重启请求
    page += "function restartDevice() {";
    page += "if (confirm('确定要重启设备吗？')) {";
    page += "var xhr = new XMLHttpRequest();";
    page += "xhr.open('GET', '/restartDevice', true);";
    page += "xhr.send();";
    page += "alert('设备即将重启...');";
    page += "}}";
    page += "</script></head><body>";
    // 检查是否存在配置文件
    page += "<h2>配置文件</h2>";
    if (LittleFS.exists("/config.json")) {
        page += "<p>状态：已找到配置文件。</p>";
        page += "<p>当前IP：" + WiFi.localIP().toString() + "</p>";
    } else {
        page += "<p>状态：未找到配置文件。</p>";
    }
    // WiFi配置部分
    page += "<h2>WiFi配置</h2>";
    page += "<form method='post'>";
    page += "<label style='display: inline-block; width: 100px;'>WiFi SSID:</label><input style='width: calc(100% - 120px);' name='ssid' placeholder='WiFi SSID' value='";
    page += config.ssid.c_str();
    page += "' required><br>"; // 添加 required 属性以防止空SSID
    page += "<label style='display: inline-block; width: 100px;'>WiFi密码:</label><input style='width: calc(100% - 120px);' name='password' placeholder='WiFi密码' type='password' pattern='.{8,}' title='密码长度至少8个字符' required value='";
    page += config.password.c_str();
    page += "'><br>"; // 添加 required 和 pattern 属性以确保密码至少8个字符
    // AP热点配置部分
    page += "<h2>AP热点配置</h2>";
    page += "<label style='display: inline-block; width: 100px;'>AP SSID:</label><input style='width: calc(100% - 120px);' name='ap_ssid' placeholder='AP SSID' value='";
    page += config.APssid.c_str();
    page += "' required><br>"; // 添加 required 属性以防止空SSID
    page += "<label style='display: inline-block; width: 100px;'>AP密码:</label><input style='width: calc(100% - 120px);' name='ap_password' placeholder='AP密码' type='password' pattern='.{8,}' title='密码长度至少8个字符' required value='";
    page += config.APpassword.c_str();
    page += "'><br>"; // 添加 required 和 pattern 属性以确保密码至少8个字符
    // 管理账号配置部分
    page += "<h2>管理账号配置</h2>";
    page += "<label style='display: inline-block; width: 100px;'>Web用户名:</label><input style='width: calc(100% - 120px);' name='www_username' placeholder='Web用户名' required value='";
    page += config.Webuser.c_str();
    page += "'><br>"; // 添加 required 属性以防止空用户名
    page += "<label style='display: inline-block; width: 100px;'>Web密码:</label><input style='width: calc(100% - 120px);' name='www_password' placeholder='Web密码' type='password' required value='";
    page += config.Webpass.c_str();
    page += "'><br>"; // 添加 required 属性以防止空密码
    // 按钮容器
    page += "<div class='btn-container'>";
    page += "<button type='submit' class='btn'>保存</button>";
    page += "<button type='button' onclick='restartDevice()' class='btn'>重启</button>";
    page += "<button type='button' onclick='confirmDelete()' class='btn'>删除配置</button>";
    page += "<button type='button' onclick='togglePasswordVisibility()' class='btn'>显示/隐藏密码</button>";
    page += "</div>";
    page += "<script>function confirmDelete() {";
    page += "if (confirm('确定要删除配置吗？')) {";
    page += "var xhr = new XMLHttpRequest();";
    page += "xhr.onreadystatechange = function() {";
    page += "if (xhr.readyState == 4 && xhr.status == 200) {";
    page += "alert(xhr.responseText);"; // 显示返回结果
    page += "location.reload();"; // 刷新页面
    page += "}";
    page += "};";
    page += "xhr.open('GET', '/delete', true);";
    page += "xhr.send();";
    page += "}}";
    page += "</script>";
    page += "<script>function confirmDelete() {";
    page += "if (confirm('确定要删除配置吗？')) {";
    page += "var xhr = new XMLHttpRequest();";
    page += "xhr.onreadystatechange = function() {";
    page += "if (xhr.readyState == 4 && xhr.status == 200) {";
    page += "alert(xhr.responseText);"; // 显示返回结果
    page += "location.reload();"; // 刷新页面
    page += "}";
    page += "};";
    page += "xhr.open('GET', '/delete', true);";
    page += "xhr.send();";
    page += "}}";
    page += "</script>";
    page += "</form></body></html>";
    // 发送网页
    server.send(200, "text/html", page);
});

  // 更新配置
  server.on("/config", HTTP_POST, []() {
    DynamicJsonDocument doc(1024);
    doc["wifi"]["ssid"] = server.arg("ssid");
    doc["wifi"]["password"] = server.arg("password");

    doc["ap"]["ssid"] = server.arg("ap_ssid");
    doc["ap"]["password"] = server.arg("ap_password");

    doc["web"]["username"] = server.arg("www_username");
    doc["web"]["password"] = server.arg("www_password");

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      server.send(500, "text/plain", "打开配置文件写入失败");
      return;
    }
    serializeJson(doc, configFile);
    configFile.close();   
    // 构建弹出提示框的 JavaScript 代码
    String response = "<html><head><title>保存成功</title>";
    response += "<script>";
    response += "alert('保存成功！请重启应用。');"; // 弹出保存成功提示框
    response += "window.location='/config';"; // 重定向到配置
    response += "</script></head><body>";
    response += "</body></html>";
    // 发送 HTML 响应，弹出保存成功提示框
    server.send(200, "text/html; charset=utf-8", response);
});

  server.on("/delete", HTTP_GET, []() {
    LittleFS.remove("/config.json");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(200, "text/plain", "配置已删除，重启设备以应用默认设置。");
  });
  server.begin();
  Serial.println("HTTP服务器已启动");


server.on("/status", HTTP_GET, []() {
    // 获取状态信息
    uint32_t freeHeap = ESP.getFreeHeap();
    float heapFragmentation = ESP.getHeapFragmentation();
    uint32_t totalFlash = ESP.getFlashChipSize();
    uint32_t freeFlash = ESP.getFreeSketchSpace();
    unsigned long uptime = millis();

    // 转换为易读单位
    float freeHeapKB = freeHeap / 1024.0;
    float totalFlashMB = totalFlash / (1024.0 * 1024.0);
    float freeFlashKB = freeFlash / 1024.0;
    unsigned long uptimeSeconds = uptime / 1000;

    // 计算运行时间的天、小时、分钟和秒
    uint32_t days = uptimeSeconds / (60 * 60 * 24);
    uptimeSeconds %= (60 * 60 * 24);
    uint32_t hours = uptimeSeconds / (60 * 60);
    uptimeSeconds %= (60 * 60);
    uint32_t minutes = uptimeSeconds / 60;
    uint32_t seconds = uptimeSeconds % 60;

    // 构建运行时间字符串
    String uptimeString = String(days) + " 天 ";
    uptimeString += String(hours) + " 小时 ";
    uptimeString += String(minutes) + " 分钟 ";
    uptimeString += String(seconds) + " 秒";

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP8266 状态</title></head><body>";
    // 使用表格格式化输出
    html += "<table>";
    html += "<tr><th>项目</th><th>值</th></tr>";
    html += "<tr><td>剩余堆内存</td><td>" + String(freeHeapKB) + " KB</td></tr>";
    html += "<tr><td>堆内存碎片化程度</td><td>" + String(heapFragmentation) + "</td></tr>";
    html += "<tr><td>总存储容量</td><td>" + String(totalFlashMB) + " MB</td></tr>";
    html += "<tr><td>剩余存储容量</td><td>" + String(freeFlashKB) + " KB</td></tr>";
    html += "<tr><td>运行时间</td><td>" + uptimeString + "</td></tr>";
    html += "</table>";
    html += "</body></html>";
    server.send(200, "text/html", html);
});

}
