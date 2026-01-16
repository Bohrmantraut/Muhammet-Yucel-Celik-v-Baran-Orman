#include "img_proc.h"



bool resample_rgb565(camera_fb_t* fb, raw_image_t* out_img, float scale_factor) {

    if (!fb || !out_img || scale_factor <= 0) return false;



    // 1. Calculate New Dimensions

    int new_w = (int)(fb->width * scale_factor);

    int new_h = (int)(fb->height * scale_factor);

   

    // RGB565 = 2 bytes per pixel

    size_t new_len = new_w * new_h * 2;



    // 2. Safety Check: Cap at 2MB to prevent crashes

    if (new_len > 2 * 1024 * 1024) {

        Serial.printf("Error: Output image too large (%u bytes)\n", new_len);

        return false;

    }



    // 3. Allocate PSRAM

    // We cast to uint8_t* for storage, but we will process as uint16_t*

    uint8_t* new_data = (uint8_t*)ps_malloc(new_len);

    if (!new_data) {

        Serial.println("Error: Failed to allocate PSRAM");

        return false;

    }



    // 4. Backward Mapping (Nearest Neighbor)

    float ratio = 1.0f / scale_factor;

   

    // Cast buffers to uint16_t so we copy full 2-byte pixels at once

    uint16_t* src_buf = (uint16_t*)fb->buf;

    uint16_t* dst_buf = (uint16_t*)new_data;



    for (int y_dst = 0; y_dst < new_h; y_dst++) {

        // Find which source row corresponds to this destination row

        int y_src = (int)(y_dst * ratio);

        if (y_src >= fb->height) y_src = fb->height - 1;



        for (int x_dst = 0; x_dst < new_w; x_dst++) {

            // Find which source column corresponds to this destination column

            int x_src = (int)(x_dst * ratio);

            if (x_src >= fb->width) x_src = fb->width - 1;



            // Calculate linear indices

            int src_index = (y_src * fb->width) + x_src;

            int dst_index = (y_dst * new_w) + x_dst;



            // Copy the pixel

            dst_buf[dst_index] = src_buf[src_index];

        }

    }



    // 5. Fill the output structure

    out_img->data = new_data;

    out_img->width = new_w;

    out_img->height = new_h;

    out_img->len = new_len;



    return true;

}



void free_raw_image(raw_image_t* img) {

    if (img && img->data) {

        free(img->data); // free() handles ps_malloc pointers correctly

        img->data = NULL;

        img->width = 0;

        img->height = 0;

    }

} 