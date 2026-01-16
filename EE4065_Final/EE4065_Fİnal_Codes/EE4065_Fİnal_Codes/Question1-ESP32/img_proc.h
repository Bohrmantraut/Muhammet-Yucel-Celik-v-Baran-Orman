#pragma once
#include <Arduino.h>
#include "esp_camera.h"

// Structure to hold processed RGB565 images
typedef struct {
    uint8_t* data;  // Pointer to the pixel array
    int width;
    int height;
    size_t len;     // Total size in bytes
} raw_image_t;

// [Q1] Size-Based Thresholding
// Finds a threshold T such that approx 'target_pixels' are white.
bool find_object_by_size(camera_fb_t* fb, raw_image_t* out_img, int target_pixels);

// Helper to free memory
void free_raw_image(raw_image_t* img);