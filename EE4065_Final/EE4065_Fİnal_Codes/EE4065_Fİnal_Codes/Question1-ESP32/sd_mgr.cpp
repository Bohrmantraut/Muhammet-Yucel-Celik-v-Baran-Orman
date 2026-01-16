#include "sd_mgr.h"
#include "config.h"
#include "FS.h"
#include "SD_MMC.h"
#include "img_converters.h"

static uint32_t g_counter = 0;

bool sd_init() {
  if (!SD_MMC.begin("/sdcard", SD_1BIT_MODE, false, SD_FREQ_HZ)) return false;
  return (SD_MMC.cardType() != CARD_NONE);
}

static bool save_buffer_to_sd(uint8_t* buf, size_t len, String &outPath) {
    char name[32];
    snprintf(name, sizeof(name), "/IMG_%06lu.jpg", (unsigned long)g_counter++);
    outPath = String(name);
    File f = SD_MMC.open(outPath.c_str(), FILE_WRITE);
    if (!f) return false;
    size_t written = f.write(buf, len);
    f.close();
    return (written == len);
}

bool sd_save_frame(camera_fb_t* fb, String &outPath) {
    if (!fb) return false;
    if (fb->format == PIXFORMAT_JPEG) return save_buffer_to_sd(fb->buf, fb->len, outPath);
    
    // RGB565 -> JPEG
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool ok = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    if (!ok) return false;
    bool saved = save_buffer_to_sd(jpg_buf, jpg_len, outPath);
    free(jpg_buf);
    return saved;
}

bool sd_save_processed(raw_image_t* img, String &outPath) {
    if (!img || !img->data) return false;
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool ok = fmt2jpg(img->data, img->len, img->width, img->height, PIXFORMAT_RGB565, 30, &jpg_buf, &jpg_len);
    if (!ok) return false;
    bool saved = save_buffer_to_sd(jpg_buf, jpg_len, outPath);
    free(jpg_buf);
    return saved;
}

bool sd_save_raw(camera_fb_t* fb, String &outPath) {
    if (!fb) return false;

    // Generate filename with .bin extension
    // We use a static counter just like the others
    static uint32_t raw_counter = 0;
    char name[32];
    snprintf(name, sizeof(name), "/RAW_%06lu.bin", (unsigned long)raw_counter++);
    outPath = String(name);

    File f = SD_MMC.open(outPath.c_str(), FILE_WRITE);
    if (!f) return false;

    // Write the raw buffer directly (fastest method)
    // fb->len is the total bytes (width * height * 2)
    size_t written = f.write(fb->buf, fb->len);
    f.close();

    return (written == fb->len);
}