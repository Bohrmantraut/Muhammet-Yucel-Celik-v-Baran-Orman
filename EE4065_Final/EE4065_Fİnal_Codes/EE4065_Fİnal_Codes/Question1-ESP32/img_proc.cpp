#include "img_proc.h"

// Helper: Fast RGB565 to 8-bit Grayscale conversion
static inline uint8_t rgb565_to_gray(uint16_t pixel) {
    // 1. Extract the raw components
    // R is 5 bits (0-31), G is 6 bits (0-63), B is 5 bits (0-31)
    uint8_t r5 = (pixel >> 11) & 0x1F;
    uint8_t g6 = (pixel >> 5) & 0x3F;
    uint8_t b5 = pixel & 0x1F;
    
    // 2. Scale them to 8-bit (0-255)
    // r5 * 8, g6 * 4, b5 * 8 is a fast approximation
    uint16_t r8 = (r5 * 527 + 23) >> 6; 
    uint16_t g8 = (g6 * 259 + 33) >> 6; 
    uint16_t b8 = (b5 * 527 + 23) >> 6; 

    // 3. Calculate Grayscale (Luminosity Method)
    // 0.299*R + 0.587*G + 0.114*B
    return (r8 * 77 + g8 * 150 + b8 * 29) >> 8; 
}

bool find_object_by_size(camera_fb_t* fb, raw_image_t* out_img, int target_pixels) {
    if (!fb || !out_img) return false;

    // 1. Initialize Histogram
    int histogram[256] = {0};
    uint16_t* src_buf = (uint16_t*)fb->buf;
    size_t pixel_count = fb->width * fb->height;

    // 2. Build Histogram (One pass)
    for (size_t i = 0; i < pixel_count; i++) {
        uint8_t gray = rgb565_to_gray(src_buf[i]);
        histogram[gray]++;
    }

    // 3. Find Threshold T (Reverse Cumulative Sum)
    int count = 0;
    int threshold = 255;
    for (threshold = 255; threshold >= 0; threshold--) {
        count += histogram[threshold];
        if (count >= target_pixels) {
            break; 
        }
    }
    if (threshold < 0) threshold = 0;

    Serial.printf("[Q1] Target: %d px, Found: %d px, Threshold: %d\n", target_pixels, count, threshold);

    // 4. Generate Binary Output
    size_t new_len = fb->width * fb->height * 2; 
    uint8_t* new_data = (uint8_t*)ps_malloc(new_len);
    if (!new_data) return false;

    uint16_t* dst_buf = (uint16_t*)new_data;

    // 5. Apply Threshold
    for (size_t i = 0; i < pixel_count; i++) {
        uint8_t gray = rgb565_to_gray(src_buf[i]);
        if (gray >= threshold) {
            dst_buf[i] = 0xFFFF; // White
        } else {
            dst_buf[i] = 0x0000; // Black
        }
    }

    // 6. Fill Struct
    out_img->data = new_data;
    out_img->width = fb->width;
    out_img->height = fb->height;
    out_img->len = new_len;

    return true;
}

void free_raw_image(raw_image_t* img) {
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}