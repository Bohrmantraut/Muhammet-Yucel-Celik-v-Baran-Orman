#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "img_proc.h" // Needed for raw_image_t

bool sd_init();

// Save camera frame (handles both JPEG and RGB565)
bool sd_save_frame(camera_fb_t* fb, String &outPath);

// Save our custom processed image (Raw RGB565 -> SD Card as JPEG)
bool sd_save_processed(raw_image_t* img, String &outPath);