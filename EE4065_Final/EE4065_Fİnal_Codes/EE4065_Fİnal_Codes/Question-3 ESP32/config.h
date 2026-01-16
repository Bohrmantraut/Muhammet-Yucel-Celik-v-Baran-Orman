#pragma once

// WiFi
#define WIFI_SSID "CELIK"
#define WIFI_PASS "CelikEvim5.3!"

// HTTP
#define HTTP_PORT 80

// Capture endpoint
#define CAPTURE_PATH "/capture"

// SD ayarları
// true => 1-bit mode (ESP32-CAM için genelde daha stabil)
#define SD_1BIT_MODE true
// Düşük frekans bazı kartlarda init sorununu çözer (10 MHz)
#define SD_FREQ_HZ 10000000

// Kamera çözünürlük/kalite
// PSRAM varsa XGA/SXGA/UXGA denenebilir
#define JPEG_QUALITY_PSRAM 10
#define JPEG_QUALITY_NOPSRAM 12
