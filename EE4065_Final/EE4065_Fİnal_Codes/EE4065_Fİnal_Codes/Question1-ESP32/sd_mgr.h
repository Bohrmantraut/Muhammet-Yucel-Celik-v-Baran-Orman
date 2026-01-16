// --- sd_mgr.h ---
#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "img_proc.h"

bool sd_init();
bool sd_save_frame(camera_fb_t* fb, String &outPath);
bool sd_save_processed(raw_image_t* img, String &outPath);

// [NEW] Save raw RGB565 buffer directly
bool sd_save_raw(camera_fb_t* fb, String &outPath);