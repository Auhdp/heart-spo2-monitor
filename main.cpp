#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "MAX30100_PulseOximeter.h"

#define THOI_GIAN_BAO_CAO_MS 3000

const char *serverIP = "192.168.1.67";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
PulseOximeter pox;
uint32_t tsLastReport = 0;

// Thông tin Wi-Fi
const char *ssid = "Phong 3";
const char *password = "88888888";

ESP8266WebServer server(80);

void onBeatDetected() {
    Serial.println("Nhịp!");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Kết nối Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());


    // Cài đặt máy chủ web
    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.begin();

    // Cài đặt màn hình OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;)
            ;
    }
    display.setFont(&FreeSerif9pt7b);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Tính toán vị trí để đặt chữ "Hello" giữa màn hình
    int16_t x = (display.width() - 6 * 5) / 2; // 6 là số kí tự trong "Hello", 5 là kích thước của mỗi kí tự
    int16_t y = (display.height() - 9) / 2;    // 9 là kích thước của kí tự lớn nhất

    display.setCursor(x, y);
    display.println("Hello!");
    display.display();
    delay(2000);

    // Cài đặt Pulse Oximeter
    Serial.print("Initializing pulse oximeter..");
    if (!pox.begin()) {
        Serial.println("FAILED");
        for (;;)
            ;
    } else {
        Serial.println("SUCCESS");
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
    server.handleClient(); // Xử lý yêu cầu máy chủ web
    pox.update();

    if (millis() - tsLastReport > THOI_GIAN_BAO_CAO_MS) {
        float heartRate = pox.getHeartRate();
        float spo2 = pox.getSpO2();

        if (heartRate > 60 && spo2 > 0) {
            // Tạo JSON để gửi đi
            String jsonData = "{\"heartRate\":" + String(heartRate) + ",\"spo2\":" + String(spo2) + "}";

            // Tạo một đối tượng WiFiClient
            WiFiClient wifiClient;

            // Tạo một đối tượng HTTPClient
            HTTPClient http;

            // Gửi yêu cầu POST đến máy chủ XAMPP
            String url = "http://" + String(serverIP) + "/max30100/data_max.php"; // Thay đổi địa chỉ và tên file PHP của bạn
            http.begin(wifiClient, url);

            http.addHeader("Content-Type", "application/json");

            int httpResponseCode = http.POST(jsonData);

            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
            }

            http.end();
        }

        display.clearDisplay();
        display.setCursor(10, 12);
        display.print("Pulse Oximeter");
        display.setCursor(0, 35);
        display.print("HeartR:");
        display.setCursor(62, 35);
        display.print(pox.getHeartRate(), 0);
        display.println(" bpm");
        display.setCursor(0, 59);
        display.print("SpO2  : ");
        display.setCursor(62, 59);
        display.print(pox.getSpO2());
        display.println(" %");
        display.display();
        tsLastReport = millis();
    }
}

void handleRoot() {
    String html = "<html><head>";
    html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
    html += "<style>";
    html += "  body {";
    html += "    font-family: 'Arial', sans-serif;";
    html += "    background-color: #f4f4f4;";
    html += "    text-align: center;";
    html += "    animation: fadeIn 1s ease-in-out;";
    html += "  }";
    html += "  h1 {";
    html += "    color: #333;";
    html += "    margin-top: 20px;";
    html += "    font-size: 24px;";
    html += "  }";
    html += "  #oximeterData {";
    html += "    margin: 20px auto;"; // Canh giữa
    html += "    padding: 20px;";
    html += "    background-color: #fff;";
    html += "    border-radius: 10px;";
    html += "    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);";
    html += "    animation: slideIn 1s ease-in-out;";
    html += "  }";
    html += "  canvas {";
    html += "    max-width: 100%;";
    html += "    height: auto;";
    html += "    display: block;";
    html += "  }";
    html += "  .chart-container {";
    html += "    display: inline-block;";
    html += "    margin: 0 10px;"; // Khoảng cách giữa hai biểu đồ
    html += "  }";
    html += "  @keyframes fadeIn {";
    html += "    from { opacity: 0; }";
    html += "    to { opacity: 1; }";
    html += "  }";
    html += "  @keyframes slideIn {";
    html += "    from { transform: translateY(-20px); }";
    html += "    to { transform: translateY(0); }";
    html += "  }";
    html += "</style>";
    html += "<script>";
    html += "var heartRateChart, spo2Chart;";
    html += "function createChart(chartId, label, color) {";
    html += "  var ctx = document.getElementById(chartId).getContext('2d');";
    html += "  return new Chart(ctx, {";
    html += "    type: 'doughnut',";
    html += "    data: {";
    html += "      labels: [label, ''],";
    html += "      datasets: [{";
    html += "        data: [0, 100],";
    html += "        backgroundColor: [color, '#f4f4f4'],";
    html += "        borderWidth: 0";
    html += "      }]";
    html += "    },";
    html += "    options: {";
    html += "      cutout: '80%',";
    html += "      responsive: true,";
    html += "      maintainAspectRatio: true"; // Giữ tỉ lệ khung hình
    html += "    }";
    html += "  });";
    html += "}";
    html += "function updateData() {";
    html += "  var xhttp = new XMLHttpRequest();";
    html += "  xhttp.onreadystatechange = function() {";
    html += "    if (this.readyState == 4 && this.status == 200) {";
    html += "      var data = JSON.parse(this.responseText);";
    html += "      document.getElementById('heartRate').innerHTML = 'Heart Rate: ' + data.heartRate + ' bpm';";
    html += "      updateChart(heartRateChart, data.heartRate);";
    html += "      document.getElementById('spo2').innerHTML = 'SpO2: ' + data.spo2 + ' %';";
    html += "      updateChart(spo2Chart, data.spo2);";
    html += "    }";
    html += "  };";
    html += "  xhttp.open('GET', '/data', true);";
    html += "  xhttp.send();";
    html += "}";
    html += "function updateChart(chart, value) {";
    html += "  chart.data.datasets[0].data[0] = value;";
    html += "  chart.data.datasets[0].data[1] = 100 - value;";
    html += "  chart.update();";
    html += "}";
    html += "document.addEventListener('DOMContentLoaded', function() {";
    html += "  heartRateChart = createChart('heartRateChart', 'Heart Rate', 'rgba(255, 0, 0, 0.8)');";
    html += "  spo2Chart = createChart('spo2Chart', 'SpO2', 'rgba(0, 0, 255, 0.8)');";
    html += "});";
    html += "setInterval(updateData, 3000);";  // Cập nhật mỗi 3000 mili giây (3 giây)
    html += "</script>";
    html += "</head><body>";
    html += "<h1>Pulse Oximeter Data</h1>";
    html += "<div id='oximeterData'>";
    html += "  <div class='chart-container'>";
    html += "    <p id='heartRate'>Heart Rate: Loading...</p>";
    html += "    <canvas id='heartRateChart' width='80' height='80'></canvas>";
    html += "  </div>";
    html += "  <div class='chart-container'>";
    html += "    <p id='spo2'>SpO2: Loading...</p>";
    html += "    <canvas id='spo2Chart' width='100' height='100'></canvas>";
    html += "  </div>";
    html += "</div>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleData() {
    String data = "{";
    data += "\"heartRate\":" + String(pox.getHeartRate()) + ",";
    data += "\"spo2\":" + String(pox.getSpO2());
    data += "}";
    server.send(200, "application/json", data);
}