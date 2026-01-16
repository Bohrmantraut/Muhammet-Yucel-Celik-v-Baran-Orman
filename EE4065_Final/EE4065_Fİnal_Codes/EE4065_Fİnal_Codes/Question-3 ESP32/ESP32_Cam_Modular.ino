#include <WiFi.h>
#include "config.h"
#include "camera_mgr.h"
#include "sd_mgr.h"
#include "http_server.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  if (!camera_init_jpeg()) {
    Serial.println("Camera init failed. Halt.");
    while (true) delay(1000);
  }

  // SD init (SD yoksa da capture çalışsın istiyorsan burada while(true) yapma)
  if (!sd_init()) {
    Serial.println("SD not ready (capture still works, but won't save).");
  }

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

  http_server_begin();
}

void loop() {
  // WebServer handleClient, http_server.cpp içinde server global.
  // Bu fonksiyonu çağırmak için küçük bir trick: server'ı orada sakladık.
  // En temiz çözüm: http_server.cpp içine http_server_loop() eklemek.
  // Basitlik için burada WebServer referansı yok, o yüzden aşağıdaki gibi yapacağız:

  // Çözüm: http_server.cpp'de server.handleClient() çağrısını açığa çıkaralım.
  // Şimdilik pratik hack: server'ı statik tuttuğumuz için bu dosyadan erişemiyoruz.
  // O yüzden 10 saniyede bir otomatik işlem yok; sadece server loop fonksiyonu ekleyelim.
  http_server_loop();
}
