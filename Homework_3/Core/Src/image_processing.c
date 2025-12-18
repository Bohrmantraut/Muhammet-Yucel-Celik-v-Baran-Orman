/*
 * image_processing.c
 *
 *  Created on: Nov 9, 2025
 *      Author: muham
 */

#include "image_processing.h"

// 1) Eşik hesaplayan fonksiyon
uint8_t compute_otsu_threshold(const uint8_t* image, uint32_t num_pixels)
{
    uint32_t hist[256] = {0};
    for (uint32_t i = 0; i < num_pixels; i++)
        hist[ image[i] ]++;

    uint32_t total = num_pixels;

    uint32_t sum_total = 0;
    for (uint32_t i = 0; i < 256; i++)
        sum_total += i * hist[i];

    uint32_t wB = 0;
    uint32_t sumB = 0;

    double max_between = 0.0;
    uint8_t threshold = 0;

    for (uint32_t t = 0; t < 256; t++)
    {
        wB += hist[t];
        if (wB == 0) continue;

        uint32_t wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];

        double mB = (double)sumB / wB;
        double mF = (double)(sum_total - sumB) / wF;

        double diff = mB - mF;
        double between = (double)wB * (double)wF * diff * diff;

        if (between > max_between)
        {
            max_between = between;
            threshold   = (uint8_t)t;
        }
    }

    return threshold;
}

// 2) 8-bit görüntüye Otsu uygulayan fonksiyon
void apply_otsu_threshold(const uint8_t* in,
                          uint8_t* out,
                          uint32_t num_pixels)
{
    uint8_t T = compute_otsu_threshold(in, num_pixels);

    for (uint32_t i = 0; i < num_pixels; i++)
        out[i] = (in[i] > T) ? 255 : 0;
}

/**
 * @brief  8-bit grayscale görüntü için Otsu eşik değerini hesaplar.
 */
uint8_t compute_otsu_threshold_gray(const uint8_t* image, uint32_t num_pixels)
{
    uint32_t hist[256] = {0};

    // Histogram
    for (uint32_t i = 0; i < num_pixels; i++)
        hist[ image[i] ]++;

    uint32_t total = num_pixels;

    uint32_t sum_total = 0;
    for (uint32_t i = 0; i < 256; i++)
        sum_total += i * hist[i];

    uint32_t wB = 0;
    uint32_t sumB = 0;

    double max_between = 0.0;
    uint8_t threshold = 0;

    for (uint32_t t = 0; t < 256; t++)
    {
        wB += hist[t];
        if (wB == 0) continue;

        uint32_t wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];

        double mB = (double)sumB / wB;
        double mF = (double)(sum_total - sumB) / wF;

        double diff = mB - mF;
        double between = (double)wB * (double)wF * diff * diff;

        if (between > max_between)
        {
            max_between = between;
            threshold   = (uint8_t)t;
        }
    }

    return threshold;
}

/**
 * @brief  Grayscale görüntüye Otsu threshold uygular (binary çıkış).
 */
void apply_otsu_threshold_gray(const uint8_t* in,
                               uint8_t* out,
                               uint32_t num_pixels)
{
    uint8_t T = compute_otsu_threshold_gray(in, num_pixels);

    for (uint32_t i = 0; i < num_pixels; i++)
        out[i] = (in[i] > T) ? 255 : 0;
}

/**
 * @brief  3x3 grayscale dilation uygular.
 *         Grayscale görüntülerde maksimum filtre olarak çalışır.
 *         Binary görüntülerde klasik binary dilation sağlar.
 *
 * @param  in      : Giriş görüntüsü (8-bit grayscale)
 * @param  out     : Çıkış görüntüsü (8-bit grayscale)
 * @param  width   : Görüntü genişliği
 * @param  height  : Görüntü yüksekliği
 */
void apply_dilation_gray_7x7(const uint8_t* in,
                             uint8_t* out,
                             uint32_t width,
                             uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t maxVal = 0;

            // 3x3 çekirdek taraması
            for (int32_t ky = -3; ky <= 3; ky++)
            {
                int32_t yy = y + ky;
                if (yy < 0 || yy >= (int32_t)height) continue;

                for (int32_t kx = -3; kx <= 3; kx++)
                {
                    int32_t xx = x + kx;
                    if (xx < 0 || xx >= (int32_t)width) continue;

                    uint8_t val = in[yy * width + xx];
                    if (val > maxVal)
                        maxVal = val;
                }
            }

            out[y * width + x] = maxVal;
        }
    }
}

/**
 * @brief  3x3 grayscale erosion uygular.
 *         Grayscale görüntülerde minimum filtre olarak çalışır.
 *         Binary görüntülerde klasik binary erosion sağlar.
 *
 * @param  in      : Giriş görüntüsü (8-bit grayscale)
 * @param  out     : Çıkış görüntüsü (8-bit grayscale)
 * @param  width   : Görüntü genişliği
 * @param  height  : Görüntü yüksekliği
 */
void apply_erosion_gray_7x7(const uint8_t* in,
                            uint8_t* out,
                            uint32_t width,
                            uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t minVal = 255;

            // 3x3 komşuluk taraması
            for (int32_t ky = -2; ky <= 2; ky++)
            {
                int32_t yy = y + ky;
                if (yy < 0 || yy >= (int32_t)height) continue;

                for (int32_t kx = -2; kx <= 2; kx++)
                {
                    int32_t xx = x + kx;
                    if (xx < 0 || xx >= (int32_t)width) continue;

                    uint8_t val = in[yy * width + xx];
                    if (val < minVal)
                        minVal = val;
                }
            }

            out[y * width + x] = minVal;
        }
    }
}

/**
 * @brief  3x3 grayscale opening uygular: Erosion -> Dilation
 *         Küçük parlak gürültüyü temizler, parlak çıkıntıları kırpar.
 *
 * @param  in      : Giriş görüntüsü (8-bit grayscale)
 * @param  out     : Çıkış görüntüsü (8-bit grayscale)
 * @param  width   : Görüntü genişliği
 * @param  height  : Görüntü yüksekliği
 */
void apply_opening_gray_3x3(const uint8_t* in,
                            uint8_t* out,
                            uint32_t width,
                            uint32_t height)
{
    // 128x128 için temp buffer (width*height kadar olmalı)
    // Eğer ileride çözünürlük değişirse bu kısmı dinamik veya global yaparız.
    static uint8_t temp[128 * 128];

    // Güvenlik: boyut kontrolü (istersen kaldırabilirsin)
    if (width * height > sizeof(temp))
        return;

    // 1) Erosion
    apply_erosion_gray_3x3(in, temp, width, height);

    // 2) Dilation
    apply_dilation_gray_3x3(temp, out, width, height);
}

/**
 * @brief  3x3 grayscale closing uygular: Dilation -> Erosion
 *         Küçük koyu boşlukları doldurur, ince karanlık çatlakları kapatır.
 *
 * @param  in      : Giriş görüntüsü (8-bit grayscale)
 * @param  out     : Çıkış görüntüsü (8-bit grayscale)
 * @param  width   : Görüntü genişliği
 * @param  height  : Görüntü yüksekliği
 */
void apply_closing_gray_3x3(const uint8_t* in,
                            uint8_t* out,
                            uint32_t width,
                            uint32_t height)
{
    // 128x128 için geçici buffer
    static uint8_t temp[128 * 128];

    // Güvenlik kontrolü (istersen kaldırabilirsin)
    if (width * height > sizeof(temp))
        return;

    // 1) Dilation
    apply_dilation_gray_3x3(in, temp, width, height);

    // 2) Erosion
    apply_erosion_gray_3x3(temp, out, width, height);
}



/**
 * @brief  RGB565 renkli görüntüye kanal kanal Otsu threshold uygular.
 *         Her kanal ayrı ayrı binarize edilir (R,G,B = 0 veya 255).
 * @param  pRGB565     : RGB565 piksel buffer'ı (in-place güncellenir)
 * @param  num_pixels  : piksel sayısı
 */
void Apply_Otsu_On_ColorImage_RGB565(uint16_t* pRGB565,
                                     uint32_t num_pixels,
                                     uint8_t* tempImg1,
                                     uint8_t* tempImg2)
{
    const char channels[3] = { 'R', 'G', 'B' };

    for (int c = 0; c < 3; c++)
    {
        // 1) İlgili kanalı tempImg1 buffer'ına çıkar
        Extract_Channel_From_RGB565(pRGB565,
                                    tempImg1,
                                    num_pixels,
                                    channels[c]);

        // 2) Otsu ile binary yap (Sonuç tempImg2'ye)
        apply_otsu_threshold(tempImg1,
                             tempImg2,
                             num_pixels);

        // 3) Binary kanalı RGB565 görüntüye geri yaz
        Write_Channel_To_RGB565(pRGB565,
                                tempImg2,
                                num_pixels,
                                channels[c]);
    }
}

/**
 * @brief  8-bit kanal düzlemini, RGB565 görüntüde ilgili kanalın yerine yazar.
 *         Diğer iki kanal korunur.
 * @param  dst      : RGB565 görüntü (in-place güncellenecek)
 * @param  src      : 8-bit kanal verisi (num_pixels adet)
 * @param  num_pixels : piksel sayısı
 * @param  channel  : 'R', 'G' veya 'B'
 */
void Write_Channel_To_RGB565(uint16_t* dst,
                             const uint8_t* src,
                             uint32_t num_pixels,
                             char channel)
{
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint16_t pixel = dst[i];

        // Mevcut kanalları 8-bit'e genişlet
        uint8_t R8 = ((pixel >> 11) & 0x1F) << 3;
        uint8_t G8 = ((pixel >> 5)  & 0x3F) << 2;
        uint8_t B8 = ( pixel        & 0x1F) << 3;

        // Hangi kanalı güncelleyeceğiz?
        uint8_t newR = R8;
        uint8_t newG = G8;
        uint8_t newB = B8;

        switch (channel)
        {
            case 'R':
                newR = src[i];
                break;
            case 'G':
                newG = src[i];
                break;
            case 'B':
                newB = src[i];
                break;
            default:
                break;
        }

        // Tekrar 5-6-5 formatına indirgeme
        uint16_t R5 = newR >> 3;
        uint16_t G6 = newG >> 2;
        uint16_t B5 = newB >> 3;

        dst[i] = (R5 << 11) | (G6 << 5) | B5;
    }
}


/**
 * @brief  RGB565 görüntüden tek bir renk kanalını (R/G/B) 8-bit düzleme çıkarır.
 * @param  src      : RGB565 kaynak görüntü (num_pixels adet 16-bit piksel)
 * @param  dst      : 8-bit hedef buffer (num_pixels adet)
 * @param  num_pixels : piksel sayısı
 * @param  channel  : 'R', 'G' veya 'B'
 */
void Extract_Channel_From_RGB565(const uint16_t* src,
                                 uint8_t* dst,
                                 uint32_t num_pixels,
                                 char channel)
{
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint16_t pixel = src[i];

        uint8_t R8 = ((pixel >> 11) & 0x1F) << 3; // R5 -> R8
        uint8_t G8 = ((pixel >> 5)  & 0x3F) << 2; // G6 -> G8
        uint8_t B8 = ( pixel        & 0x1F) << 3; // B5 -> B8

        switch (channel)
        {
            case 'R':
                dst[i] = R8;
                break;
            case 'G':
                dst[i] = G8;
                break;
            case 'B':
                dst[i] = B8;
                break;
            default:
                dst[i] = 0;
                break;
        }
    }
}


/**
 * @brief  8-bit Gri tonlamadan RGB565'e çevirir. (1 Byte -> 2 Byte)
 * (Seri porttan göndermek için gereklidir.)
 */
void Convert_Grayscale8bit_To_RGB565(const uint8_t* pIn8bit, uint16_t* pOut16bit, uint32_t num_pixels)
{
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint8_t gray8bit = pIn8bit[i];

        // 8-bit Gri değeri R, G, B bileşenleri olarak kabul edilir.
        uint16_t R5 = gray8bit >> 3; // 8 bit -> 5 bit R
        uint16_t G6 = gray8bit >> 2; // 8 bit -> 6 bit G
        uint16_t B5 = gray8bit >> 3; // 8 bit -> 5 bit B

        // Yeni RGB565 Gri Pikseli Oluştur
        pOut16bit[i] = (R5 << 11) | (G6 << 5) | B5;
    }
}

/**
 * @brief  RGB565'ten 8-bit Gri tonlamaya çevirir. (2 Byte -> 1 Byte)
 */
void Convert_RGB565_To_Grayscale8bit(const uint16_t* pIn16bit, uint8_t* pOut8bit, uint32_t num_pixels)
{
    // Ağırlıklı Ortalama (ITU-R BT.601) kullanılır, 8-bit R/G/B varsayılarak.
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint16_t pixel = pIn16bit[i];

        // 16-bit'ten 8-bit R, G, B bileşenlerini çıkar
        uint8_t R8 = ((pixel >> 11) & 0x1F) << 3; // R5 -> R8
        uint8_t G8 = ((pixel >> 5) & 0x3F) << 2;  // G6 -> G8
        uint8_t B8 = (pixel & 0x1F) << 3;         // B5 -> B8

        // Gri Tonlama Hesaplama (77*R + 150*G + 29*B) >> 8
        uint32_t gray8bit = (77 * R8 + 150 * G8 + 29 * B8) >> 8;

        pOut8bit[i] = (uint8_t)gray8bit;
    }
}

