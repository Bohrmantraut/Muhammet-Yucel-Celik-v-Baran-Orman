#include "http_server.h"
#include <WiFi.h>
#include "config.h"
#include "camera_mgr.h"
#include "sd_mgr.h"
#include "img_proc.h"
#include "img_converters.h"

static WebServer server(HTTP_PORT);

// State
static camera_fb_t *g_fb = NULL;         
static raw_image_t g_output = {0}; // Holds the Q1 result

void clear_output() {
    free_raw_image(&g_output);
}

// 1. Root
static void handle_root() {
    String html = "<html><body style='font-family:sans-serif; text-align:center;'>"
                  "<h1>ESP32-CAM Q1 Project</h1>"
                  "<button onclick=\"location.href='/capture'\" style='font-size:20px; padding:20px;'>START CAPTURE</button>"
                  "</body></html>";
    server.send(200, "text/html", html);
}

// 2. Capture & Menu
static void handle_capture() {
    clear_output();
    if (g_fb) {
        camera_release(g_fb);
        g_fb = NULL;
    }
    g_fb = camera_capture();
    if (!g_fb) {
        server.send(500, "text/plain", "Capture Failed");
        return;
    }

    String html = "<html><body style='font-family:sans-serif; text-align:center; padding:10px;'>"
                  "<h2>Original Image</h2>"
                  "<img src='/view?type=orig&t=" + String(millis()) + "' style='max-width:100%; border:2px solid black;'><br>"
                  "<a href='/save?type=orig'><button style='margin:10px;'>SAVE ORIGINAL</button></a><hr>"
                  // NEW BUTTON
                  "<a href='/save?type=raw'><button style='margin:5px; background:#FF9800; color:white;'>SAVE RAW (BIN)</button></a><hr>"
                  // Q1 Section ONLY
                  "<h3>Q1: Object Detection</h3>"
                  "<p>Find bright object (1000px)</p>"
                  "<a href='/threshold'><button style='padding:15px; background:#E91E63; color:white; font-size:16px;'>RUN SIZE-BASED THRESHOLD</button></a>"
                  "</body></html>";
    server.send(200, "text/html", html);
}

// 3. Q1 Handler
static void handle_threshold() {
    if (!g_fb) {
        server.send(200, "text/html", "Capture first!");
        return;
    }
    clear_output();

    // Q1 Requirement: Extract object based on size (1000 pixels)
    if (!find_object_by_size(g_fb, &g_output, 1000)) {
        server.send(500, "text/plain", "Processing Failed");
        return;
    }

    String html = "<html><body style='font-family:sans-serif; text-align:center;'>"
                  "<h2>Q1 Result (Target: 1000px)</h2>"
                  "<img src='/view?type=out&t=" + String(millis()) + "' style='max-width:100%; border:2px solid red;'><br>"
                  "<a href='/save?type=out'><button style='margin:10px; padding:10px; background:#4CAF50; color:white;'>SAVE BINARY</button></a><br><br>"
                  "<a href='/capture'>Back to Capture</a></body></html>";
    server.send(200, "text/html", html);
}

// 4. Save & View Helpers
static void handle_save() {
    String type = server.arg("type");
    String path = "";
    bool success = false;

    if (type == "orig" && g_fb) success = sd_save_frame(g_fb, path);
    else if (type == "raw" && g_fb) success = sd_save_raw(g_fb, path);
    else if (type == "out" && g_output.data) success = sd_save_processed(&g_output, path);

    if (success) server.send(200, "text/html", "Saved: " + path + " <button onclick='history.back()'>Back</button>");
    else server.send(500, "text/plain", "Save Failed");
}

static void handle_view() {
    String type = server.arg("type");
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool ok = false;

    if (type == "orig" && g_fb) {
        ok = fmt2jpg(g_fb->buf, g_fb->len, g_fb->width, g_fb->height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    } 
    else if (type == "out" && g_output.data) {
        ok = fmt2jpg(g_output.data, g_output.len, g_output.width, g_output.height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    }

    if (ok) {
        server.setContentLength(jpg_len);
        server.send(200, "image/jpeg", "");
        server.client().write(jpg_buf, jpg_len);
        free(jpg_buf);
    } else {
        server.send(404, "text/plain", "No Data");
    }
}

bool http_server_begin() {
  server.on("/", handle_root);
  server.on("/capture", handle_capture);
  server.on("/threshold", handle_threshold); // Q1 Only
  server.on("/save", handle_save);
  server.on("/view", handle_view);
  server.begin();
  return true;
}

void http_server_loop() {
  server.handleClient();
}