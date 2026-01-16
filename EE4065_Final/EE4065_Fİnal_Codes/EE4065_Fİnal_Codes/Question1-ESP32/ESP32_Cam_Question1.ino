#include <WiFi.h>
#include "config.h"
#include "camera_mgr.h"
#include "sd_mgr.h"
#include "http_server.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  // Initialize Camera
  if (!camera_init_jpeg()) {
    Serial.println("Camera init failed. Halt.");
    while (true) delay(1000);
  }

  // Initialize SD Card
  if (!sd_init()) {
    Serial.println("SD not ready (capture works, saving won't).");
  }

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Start Web Server
  http_server_begin();
}

void loop() {
  // Handle web client requests
  http_server_loop();
}