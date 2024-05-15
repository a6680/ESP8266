void handleRoot() {
  authenticationMode();
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html.concat("<title>ESP8266 Web 服务器</title></head><body style='text-align: center; margin-top: 50px;'>");
  html.concat("<h2 style='margin-bottom: 20px;'>ESP8266 Web 服务器</h2>");
  html.concat("<p>欢迎使用</p>");
  html.concat("<button onclick='performAction(\"/toggleLED\", this)'>切换LED</button><br><br>");
  html.concat("<button onclick='confirmAction(\"/openGate\", \"\", \"确定要开门吗？要不再想想？\", this)'>开启大门</button><br><br>");
  html.concat("<button onclick='confirmAction(\"/restartDevice\", \"重启设备成功\", \"确定要重启设备吗？\", this)'>重启设备</button><br><br>");
  html.concat("<button onclick='confirmAction(\"/handleEnable\", \"\", \"确定要启动更新吗？\", this)'>启动固件更新</button><br><br>");
  html.concat("<button onclick='location.href=\"/status\"'>设备运行状态</button>");
  html.concat("<script>");
  html.concat("const urlParams = new URLSearchParams(window.location.search);"); // 获取 URL 参数
  html.concat("const key = urlParams.get('key');");
  html.concat("function showResult(result) { alert(result); }");
  html.concat("function performAction(url, button) {");
  html.concat("  button.disabled = true;");
  html.concat("  fetch(`${url}?key=${key}`).then(response => response.text()).then(text => { showResult(text); button.disabled = false; }).catch(err => { showResult('操作失败'); button.disabled = false; });");
  html.concat("}");
  html.concat("function confirmAction(url, message, confirmation, button) {");
  html.concat("  if (confirm(confirmation)) { performAction(`${url}?key=${key}`, button); }");
  html.concat("}");
  html.concat("</script></body></html>");
  server.send(200, "text/html", html);
}

void toggleLED() {
    authenticationMode();
    bool isOn = digitalRead(ledPin);
    digitalWrite(ledPin, !isOn);
    if (isOn) {
        server.send(200, "text/plain", "LED状态已开启");
    } else {
        server.send(200, "text/plain", "LED状态已关闭");
    }
}

void openGate() {
        authenticationMode();
        digitalWrite(gatePin, HIGH);
        server.send(200, "text/plain", "大门已开启！");
        delay(300);
        digitalWrite(gatePin, LOW);
        sendPushPlusMessage("大门已开启！");
}


void restartDevice() {
    server.send(200, "text/plain", "设备即将重启...");
    delay(1000);
    ESP.restart();
}

void handleStatus() {
  // 设置响应头信息
  server.sendHeader("Connection", "close");
  server.sendHeader("Content-Type", "text/html; charset=utf-8");

  // 定义HTML模板
  const char* htmlTemplate = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>%s</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        pre {
          font-family: monospace;
          white-space: pre-wrap;
          font-size: 15px;  
        }
      </style>
      <script>
        function updateStatus() {
          fetch('/script', {cache: 'no-cache'})
            .then(response => response.text())
            .then(data => document.getElementById('script').textContent = data)
            .catch(error => console.error('Error:', error));
        }
        updateStatus();
        setInterval(updateStatus, 1000);
      </script>
    </head>
    <body>
      <h1>%s</h1>
      <pre id="script"></pre>
    </body>
    </html>
  )";

  // 定义HTML缓冲区，使用snprintf填充HTML模板
  char html[2048];
  snprintf(html, sizeof(html), htmlTemplate, "ESP8266 状态信息", "ESP8266 状态信息"); 

  // 发送HTTP响应
  server.send(200, "text/html", html); 
}


void handleScript() {
  uint32_t freeHeap = ESP.getFreeHeap();
  float heapFragmentation = ESP.getHeapFragmentation();
  uint32_t totalFlash = ESP.getFlashChipSize();
  uint32_t freeFlash = ESP.getFreeSketchSpace();
  unsigned long uptime = millis();

  unsigned long days = uptime / (1000 * 60 * 60 * 24);
  unsigned long hours = (uptime % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60);
  unsigned long minutes = (uptime % (1000 * 60 * 60)) / (1000 * 60);
  unsigned long seconds = (uptime % (1000 * 60)) / 1000;

  char responseBuffer[512];
  snprintf(responseBuffer, sizeof(responseBuffer),
           "可用堆内存: %u KB\n"
           "堆内存碎片化: %.2f%%\n"
           "总闪存: %.2f MB\n"
           "可用闪存: %.2f MB\n"
           "运行时间: %lu 天 %lu 小时 %lu 分钟 %lu 秒",
           freeHeap / 1024, heapFragmentation,
           totalFlash / (1024.0 * 1024), freeFlash / (1024.0 * 1024),
           days, hours, minutes, seconds);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0"); 
  server.send(200, "text/plain", responseBuffer); 
}


void handleConfig() {
    if (!server.authenticate(username.c_str(), userpassword.c_str())) {
    server.requestAuthentication();
    return;
    }
    // 生成包含已保存的SSID和密码的表单
    String html = "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>#container { text-align: center; }</style>";
    html += "<div id='container'>"; // 包裹内容的 div
    html += "<h3>WiFi配置</h3>";

    html += "<form id='configForm' onsubmit='saveConfig(event)' action='/save' method='post'>";
    html += "<div style='display: flex; justify-content: center; align-items: center; flex-direction: column;'>"; // 使用flex布局来居中显示
    html += "<div style='margin-bottom: 10px;'>"; // 设置上下间距
    html += "<label for='ssid' style='width: 150px;'>WiFi名称: </label>"; // 设置固定宽度
    html += "<input type='text' id='ssid' name='ssid' value='" + String(ssid) + "' required>";
    html += "</div>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='password' style='width: 150px;'>WiFi密码: </label>";
    html += "<input type='password' id='password' name='password' minlength='8' value='" + String(password) + "' required>";
    html += "</div>";
    
    html += "<h3>AP配置</h3>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='ap_ssid' style='width: 150px;'>AP名称: </label>";
    html += "<input type='text' id='ap_ssid' name='ap_ssid' value='" + String(ap_ssid) + "' required>";
    html += "</div>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='ap_password' style='width: 150px;'>AP密码: </label>";
    html += "<input type='password' id='ap_password' name='ap_password' minlength='8' value='" + String(ap_password) + "' required>";
    html += "</div>";

    html += "<h3>管理账号配置</h3>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='username' style='width: 150px;'>用户名: </label>";
    html += "<input type='text' id='username' name='username' value='" + String(username) + "' required>";
    html += "</div>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='userpassword' style='margin-left: 17px;'>密码: </label>";
    html += "<input type='password' id='userpassword' name='userpassword' minlength='5' value='" + String(userpassword) + "' required>";
    html += "</div>";
    
    html += "<h3>KEY密钥</h3>";
    html += "<div style='margin-bottom: 10px;'>";
    html += "<label for='key' style='margin-left: 17px;'>密钥: </label>";
    html += "<input type='password' id='key' name='key' minlength='5' value='" + String(key) + "' required>";
    html += "</div>";
    html += "";
    html += "<style>.radio-group {display: flex;}.radio-group label {margin-right: 10px;}</style>";
    html += "<h3>认证模式</h3>";
    html += "<div class='radio-group'>";
    html += "<label for='option1'><input type='radio' id='option1' name='options' value='KEY'";
    if (authMode == "KEY") {
      html += " checked";
    }
    html += "> KEY密钥</label>";
    html += "<label for='option2'><input type='radio' id='option2' name='options' value='USER'";
    if (authMode == "USER") {
      html += " checked";
    }
    html += "> 管理账号</label>";
    html += "</div>";
    html += "<div style='margin-top: 15px;'>"; // 上边距
    html += "<button type='submit' style='margin-right: 10px;'>保存</button>";
    html += "<button type='button' style='margin-right: 10px;' onclick='readConfig()'>读取</button>";
    html += "<button type='button' style='margin-right: 10px;' onclick='deleteConfig()'>删除</button>";
    html += "<button type='button' style='margin-right: 10px;' onclick='togglePasswordVisibility()'>显示密码</button>";
    html += "</div>"; // 关闭包含按钮的 div
    html += "</form>"; // 关闭表单
    html += "<div id='output'></div>"; // 输出结果的 div
    
    // JavaScript 部分
    html += "<script>";
    html += "function readConfig() {";
    html += "  fetch('/read')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      document.getElementById('output').innerHTML = data;";
    html += "    })";
    html += "    .catch(error => console.error('读取配置错误:', error));"; // 添加错误处理
    html += "}";

    html += "function saveConfig(event) {";
    html += "  event.preventDefault();"; // 阻止表单默认提交行为
    html += "  var ssid = document.getElementById('ssid').value.trim();";
    html += "  var password = document.getElementById('password').value.trim();";
    html += "  var username = document.getElementById('username').value.trim();"; // 获取用户名输入框的值
    html += "  var userpassword = document.getElementById('userpassword').value.trim();"; // 获取用户密码输入框的值
    html += "  if(ssid === '' || password.length < 8 || username === '' || password.length < 5) {"; // 检查用户名和用户密码是否为空以及长度
    html += "    return;";
    html += "  }";
    html += "  if(!confirm('确定要保存配置吗？')) return;"; // 提示是否确定保存
    html += "  fetch('/save', {";
    html += "    method: 'POST',";
    html += "    body: new FormData(document.getElementById('configForm'))";
    html += "  })";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);"; // 弹出保存结果
    html += "    })";
    html += "    .catch(error => console.error('保存配置错误:', error));"; // 添加错误处理
    html += "}";

    html += "function deleteConfig() {";
    html += "  if(!confirm('确定要删除配置吗？')) return;"; // 确认删除
    html += "  fetch('/delete', {";
    html += "    method: 'POST',";
    html += "    body: new FormData(document.getElementById('configForm'))";
    html += "  })";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);"; // 弹出删除结果
    html += "    })";
    html += "    .catch(error => console.error('删除配置错误:', error));"; // 添加错误处理
    html += "}";

    html += "function togglePasswordVisibility() {";
    html += "  var passwordInput = document.getElementById('password');";
    html += "  var apPasswordInput = document.getElementById('ap_password');"; // 获取 AP 密码输入框元素
    html += "  var userasswordInput = document.getElementById('userpassword');"; // 获取 用户 密码输入框元素
    html += "  var keyInput = document.getElementById('key');"; // 获取 key 密码输入框元素

    html += "  passwordInput.type = (passwordInput.type === 'password') ? 'text' : 'password';"; // 切换密码显示状态
    html += "  apPasswordInput.type = (apPasswordInput.type === 'password') ? 'text' : 'password';"; // 切换 AP 密码显示状态
    html += "  userasswordInput.type = (userasswordInput.type === 'password') ? 'text' : 'password';"; // 切换 用户 密码显示状态
    html += "  keyInput.type = (keyInput.type === 'password') ? 'text' : 'password';"; // 切换 key 密码显示状态
    html += "}";
    html += "</script>";
    html += "</div>"; // 关闭包含所有内容的 div
    // 发送 HTML 响应
    server.send(200, "text/html", html);
}



void handleRead() {
    // 读取配置文件
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        server.send(500, "text/plain", "无法打开配置文件进行读取");
        return;
    }

    // 读取配置文件内容
    String configContent;
    while (configFile.available()) {
        configContent += configFile.readString();
    }

    // 关闭文件
    configFile.close();

    // 将配置内容发送回客户端
    server.send(200, "text/plain", configContent);

    // 释放内存
    configContent = ""; // 清空字符串
}


void handleSave() {
  // 从POST请求中获取SSID和密码
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String ap_ssid = server.arg("ap_ssid");
  String ap_password = server.arg("ap_password");
  String username = server.arg("username");
  String userpassword = server.arg("userpassword");
  String key = server.arg("key");
  String authMode = server.arg("options"); // 获取认证模式选项值


  // 创建一个JSON文档并将SSID和密码写入其中
  DynamicJsonDocument doc(200);
  doc["WiFi"]["ssid"] = ssid;
  doc["WiFi"]["password"] = password;
  doc["AP"]["ssid"] = ap_ssid;
  doc["AP"]["password"] = ap_password;
  doc["Web"]["user"] = username;
  doc["Web"]["password"] = userpassword;
  doc["url"]["key"] = key;
  doc["authMode"] = authMode; // 将认证模式写入JSON文档



  
  // 打开配置文件并写入JSON数据
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("无法打开配置文件进行写入");
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();

  // 释放JSON文档使用的内存
  doc.clear();
  // 释放文件资源
  configFile.close();
  
  // 发送响应，表示配置已保存
  server.send(200, "text/plain", "配置已保存，正在重启");
  delay(500); // 等待500毫秒
  ESP.restart();
}

void deleteConfig() {
  // 尝试删除配置文件
  if (LittleFS.remove("/config.json")) {
    server.send(200, "text/plain", "配置文件删除成功,，正在重启");
    delay(500); // 等待500毫秒
    ESP.restart();
  } else {
    server.send(200, "text/plain", "无法删除配置文件");

  }
}

void handleEnable() {
  if (!otaEnabled) {
    otaEnabled = true;
    ArduinoOTA.begin();
    server.send(200, "text/plain", "OTA 更新已启用，要关闭请重启");
  } else {
    server.send(200, "text/plain", "OTA 更新已经启用了，要关闭请重启");
  }
}
