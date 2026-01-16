#pragma once
#include <Arduino.h>
#include "esp_camera.h"

// Custom structure to hold Raw RGB565 data
typedef struct {
    uint8_t* data;  // Pointer to the pixel array
    int width;
    int height;
    size_t len;     // Total size in bytes
} raw_image_t;

// Resamples an RGB565 image by a float factor (e.g., 1.5, 0.66)
// Returns true if successful, false if memory allocation fails.
bool resample_rgb565(camera_fb_t* fb, raw_image_t* out_img, float scale_factor);

// Helper to free the memory allocated by resample_rgb565
void free_raw_image(raw_image_t* img);