#pragma once
#include "esp_camera.h"

bool camera_init_jpeg();
camera_fb_t* camera_capture();
void camera_release(camera_fb_t* fb);
