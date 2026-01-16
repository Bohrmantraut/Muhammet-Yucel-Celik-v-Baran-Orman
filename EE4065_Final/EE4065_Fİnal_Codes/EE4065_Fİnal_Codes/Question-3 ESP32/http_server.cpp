#include "http_server.h"
#include <WiFi.h>
#include "config.h"
#include "camera_mgr.h"
#include "sd_mgr.h"
#include "img_proc.h"
#include "img_converters.h"

static WebServer server(HTTP_PORT);

// --- GLOBAL STATE ---
// We hold these in memory to allow step-by-step processing
static camera_fb_t *g_fb = NULL;         // The Original Image
static raw_image_t g_resampled = {0};    // The Processed Image

// Helper: Free previous images to prevent memory leaks
void clear_resampled() {
    free_raw_image(&g_resampled);
}

// --- HANDLERS ---

// 1. Root / Menu
static void handle_root() {
    String html = "<html><body style='font-family:sans-serif; text-align:center;'>"
                  "<h1>ESP32-CAM Workflow</h1>"
                  "<div style='border:1px solid #ccc; padding:10px; margin:10px;'>"
                  "<h3>Step 1: Capture</h3>"
                  "<button onclick=\"location.href='/capture'\" style='font-size:20px; padding:10px;'>CAPTURE NEW PHOTO</button>"
                  "</div></body></html>";
    server.send(200, "text/html", html);
}

// 2. Capture & Review Original (NOW WITH INPUT FORM)
static void handle_capture() {
    // Clean up old state
    clear_resampled();
    if (g_fb) {
        camera_release(g_fb);
        g_fb = NULL;
    }

    // Capture new frame
    g_fb = camera_capture();
    if (!g_fb) {
        server.send(500, "text/plain", "Camera Capture Failed");
        return;
    }

    // Generate Interface with Input Form
    String html = "<html><body style='font-family:sans-serif; text-align:center; padding:20px;'>"
                  "<h2>1. Original Image Captured</h2>"
                  "<img src='/view?type=orig&t=" + String(millis()) + "' style='max-width:100%; border:2px solid black;'><br><br>"
                  
                  // --- SAVE SECTION ---
                  "<a href='/save?type=orig'><button style='padding:10px; background:#4CAF50; color:white;'>SAVE ORIGINAL to SD</button></a>"
                  "<hr style='margin:20px 0;'>"

                  // --- RESAMPLE SECTION ---
                  "<h3>Process Image</h3>"
                  "<form action='/process' method='GET'>"
                  "  <label>Enter Scale Factor (e.g., 1.5, 0.66): </label><br>"
                  // Changed max to 3.0 as requested
                  "  <input type='number' name='scale' step='0.01' min='0.1' max='3.0' value='1.0' style='padding:5px; font-size:16px; width:80px;'>"
                  "  <br><br>"
                  "  <input type='submit' value='Process Image' style='padding:10px; font-size:16px; background:#008CBA; color:white;'>"
                  "</form>"
                  
                  "</body></html>";
    server.send(200, "text/html", html);
}

// 3. Process (Resample) - CORRECTED FUNCTION
static void handle_process() {
    if (!g_fb) {
        server.send(200, "text/html", "No image! <a href='/capture'>Capture first</a>");
        return;
    }

    // 1. Read the user input
    float scale = 1.0;
    if (server.hasArg("scale")) {
        scale = server.arg("scale").toFloat();
    }

    // 2. Safety Bounds (Critical for ESP32)
    // 0.1 is the smallest useful size. 
    // 3.0 is the max limit you requested
    if (scale < 0.1) scale = 0.1;
    if (scale > 3.0) scale = 3.0;

    // Clear previous result
    clear_resampled();

    // 3. Perform Resampling with the custom scale
    // This supports non-integers like 1.5 or 0.66
    if (!resample_rgb565(g_fb, &g_resampled, scale)) {
        server.send(500, "text/plain", "Resampling Failed (Memory Full?)");
        return;
    }

    // Generate Result HTML
    String html = "<html><body style='font-family:sans-serif; text-align:center; padding:20px;'>"
                  "<h2>2. Processed Result (" + String(scale) + "x)</h2>"
                  "<img src='/view?type=resampled&t=" + String(millis()) + "' style='max-width:100%; border:2px solid blue;'><br><br>"
                  
                  "<b>Actions:</b><br><br>"
                  "<a href='/save?type=resampled'><button style='padding:15px; font-weight:bold; background:#4CAF50; color:white;'>SAVE RESULT to SD</button></a>"
                  "<br><br><br>"
                  "<a href='/capture'>&laquo; Start Over</a>"
                  "</body></html>";
    server.send(200, "text/html", html);
}

// 4. Save Handler
static void handle_save() {
    String type = server.arg("type");
    String savedPath = "";
    bool success = false;

    if (type == "orig" && g_fb) {
        success = sd_save_frame(g_fb, savedPath);
    } 
    else if (type == "resampled" && g_resampled.data) {
        success = sd_save_processed(&g_resampled, savedPath);
    } 
    else {
        server.send(400, "text/plain", "Nothing to save!");
        return;
    }

    if (success) {
        String html = "<html><body style='font-family:sans-serif; text-align:center;'>"
                      "<h2 style='color:green;'>Saved!</h2>"
                      "<p>File saved to SD: <b>" + savedPath + "</b></p>"
                      "<button onclick='history.back()'>Go Back</button>"
                      "</body></html>";
        server.send(200, "text/html", html);
    } else {
        server.send(500, "text/plain", "SD Save Failed (Check card/formatting)");
    }
}

// 5. Image Viewer (Helper)
static void handle_view() {
    String type = server.arg("type");
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool ok = false;

    if (type == "orig" && g_fb) {
        // Convert Original RGB -> JPEG
        ok = fmt2jpg(g_fb->buf, g_fb->len, g_fb->width, g_fb->height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    } 
    else if (type == "resampled" && g_resampled.data) {
        // Convert Resampled RGB -> JPEG
        ok = fmt2jpg(g_resampled.data, g_resampled.len, g_resampled.width, g_resampled.height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    }

    if (!ok) {
        server.send(404, "text/plain", "Image data missing");
        return;
    }

    server.setContentLength(jpg_len);
    server.send(200, "image/jpeg", "");
    WiFiClient client = server.client();
    client.write(jpg_buf, jpg_len);
    free(jpg_buf);
}

bool http_server_begin() {
  server.on("/", handle_root);
  server.on("/capture", handle_capture);
  server.on("/process", handle_process);
  server.on("/save", handle_save);
  server.on("/view", handle_view);
  
  server.begin();
  Serial.printf("HTTP server started on port %d\n", HTTP_PORT);
  return true;
}

void http_server_loop() {
  server.handleClient();
}