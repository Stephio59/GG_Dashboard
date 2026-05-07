/*
 * sd_lvgl_fs.cpp - Driver LVGL filesystem pour SD_MMC + decodeur BMP
 */
#include "sd_lvgl_fs.h"
#include "sd_card.h"
#include "FS.h"
#include "SD_MMC.h"

// ═══════════════════════════════════════════════════════════════════
// PARTIE 1 : DRIVER FILESYSTEM LVGL
// ═══════════════════════════════════════════════════════════════════

// Wrapper pour stocker un File ouvert
typedef struct {
    File f;
} sd_file_handle_t;

static void* _sd_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    if (!sdCard.mounted) return nullptr;
    
    const char* file_mode = "r";
    if (mode == LV_FS_MODE_WR) file_mode = "w";
    else if (mode == (LV_FS_MODE_RD | LV_FS_MODE_WR)) file_mode = "r+";
    
    sd_file_handle_t* h = new sd_file_handle_t();
    h->f = SD_MMC.open(path, file_mode);
    if (!h->f) {
        delete h;
        return nullptr;
    }
    return (void*)h;
}

static lv_fs_res_t _sd_close(lv_fs_drv_t* drv, void* file_p) {
    sd_file_handle_t* h = (sd_file_handle_t*)file_p;
    if (h) {
        h->f.close();
        delete h;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t _sd_read(lv_fs_drv_t* drv, void* file_p, void* buf,
                             uint32_t btr, uint32_t* br) {
    sd_file_handle_t* h = (sd_file_handle_t*)file_p;
    if (!h) return LV_FS_RES_INV_PARAM;
    *br = h->f.read((uint8_t*)buf, btr);
    return LV_FS_RES_OK;
}

static lv_fs_res_t _sd_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos,
                             lv_fs_whence_t whence) {
    sd_file_handle_t* h = (sd_file_handle_t*)file_p;
    if (!h) return LV_FS_RES_INV_PARAM;
    
    SeekMode m = SeekSet;
    if (whence == LV_FS_SEEK_CUR) m = SeekCur;
    else if (whence == LV_FS_SEEK_END) m = SeekEnd;
    
    return h->f.seek(pos, m) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t _sd_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    sd_file_handle_t* h = (sd_file_handle_t*)file_p;
    if (!h) return LV_FS_RES_INV_PARAM;
    *pos_p = h->f.position();
    return LV_FS_RES_OK;
}

static lv_fs_drv_t fs_drv;

void sd_lvgl_fs_init() {
    if (!sdCard.mounted) {
        Serial0.println("[SD-LVGL] SD non montee, driver non installe");
        return;
    }
    
    lv_fs_drv_init(&fs_drv);
    fs_drv.letter = 'S';
    fs_drv.cache_size = 0;
    fs_drv.open_cb  = _sd_open;
    fs_drv.close_cb = _sd_close;
    fs_drv.read_cb  = _sd_read;
    fs_drv.seek_cb  = _sd_seek;
    fs_drv.tell_cb  = _sd_tell;
    
    lv_fs_drv_register(&fs_drv);
    Serial0.println("[SD-LVGL] Driver filesystem 'S:' enregistre");
}

// ═══════════════════════════════════════════════════════════════════
// PARTIE 2 : DECODEUR BMP -> lv_img_dsc_t (RGB565 + ALPHA via chroma key)
// ═══════════════════════════════════════════════════════════════════
// Format BMP gere : 24-bit non compresse (BI_RGB)
// Magenta pur (#FF00FF / RGB 255,0,255) = pixel transparent
// Conversion vers LV_IMG_CF_TRUE_COLOR_ALPHA (RGB565 + 1 byte alpha)

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} bmp_header_t;
#pragma pack(pop)

static inline uint16_t rgb888_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

lv_img_dsc_t* sd_load_bmp(const char* path) {
    if (!sdCard.mounted) {
        Serial0.printf("[SD-BMP] SD non montee\n");
        return nullptr;
    }
    
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial0.printf("[SD-BMP] Echec ouverture %s\n", path);
        return nullptr;
    }
    
    bmp_header_t hdr;
    if (f.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) {
        Serial0.printf("[SD-BMP] Header trop court %s\n", path);
        f.close();
        return nullptr;
    }
    
    if (hdr.bfType != 0x4D42) {
        Serial0.printf("[SD-BMP] Pas un BMP %s (magic=%04X)\n", path, hdr.bfType);
        f.close();
        return nullptr;
    }
    
    if (hdr.biBitCount != 24 || hdr.biCompression != 0) {
        Serial0.printf("[SD-BMP] Format non supporte %s (bits=%d comp=%d)\n",
                       path, hdr.biBitCount, hdr.biCompression);
        f.close();
        return nullptr;
    }
    
    int width = hdr.biWidth;
    int height = abs(hdr.biHeight);
    bool top_down = (hdr.biHeight < 0);
    
    int row_size = ((width * 3 + 3) / 4) * 4;
    
    // Format LVGL TRUE_COLOR_ALPHA = 3 bytes/pixel (2 RGB565 + 1 alpha)
    size_t img_data_size = width * height * 3;
    
    uint8_t* pixel_buf = (uint8_t*)heap_caps_malloc(img_data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pixel_buf) {
        pixel_buf = (uint8_t*)malloc(img_data_size);
    }
    if (!pixel_buf) {
        Serial0.printf("[SD-BMP] Allocation %u bytes echouee\n", (unsigned)img_data_size);
        f.close();
        return nullptr;
    }
    
    lv_img_dsc_t* dsc = (lv_img_dsc_t*)heap_caps_malloc(sizeof(lv_img_dsc_t),
                                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!dsc) dsc = (lv_img_dsc_t*)malloc(sizeof(lv_img_dsc_t));
    if (!dsc) {
        free(pixel_buf);
        f.close();
        return nullptr;
    }
    
    f.seek(hdr.bfOffBits);
    
    uint8_t* row_buf = (uint8_t*)malloc(row_size);
    if (!row_buf) {
        free(pixel_buf);
        free(dsc);
        f.close();
        return nullptr;
    }
    
    int magenta_count = 0;
    
    // Lire et convertir ligne par ligne
    for (int y = 0; y < height; y++) {
        int dst_y = top_down ? y : (height - 1 - y);
        
        if (f.read(row_buf, row_size) != row_size) {
            Serial0.printf("[SD-BMP] Lecture ligne %d echouee\n", y);
            free(row_buf);
            free(pixel_buf);
            free(dsc);
            f.close();
            return nullptr;
        }
        
        // 3 bytes par pixel : RGB565 (2 bytes) + alpha (1 byte)
        uint8_t* dst_line = pixel_buf + (dst_y * width * 3);
        for (int x = 0; x < width; x++) {
            uint8_t b = row_buf[x*3 + 0];
            uint8_t g = row_buf[x*3 + 1];
            uint8_t r = row_buf[x*3 + 2];
            
            // Chroma key : magenta (FF, 00, FF) = transparent
            uint8_t alpha = 0xFF;
            if (r >= 248 && g <= 8 && b >= 248) {
                alpha = 0x00;
                magenta_count++;
            }
            
            uint16_t rgb565 = rgb888_to_565(r, g, b);
            dst_line[x*3 + 0] = rgb565 & 0xFF;       // RGB565 LSB
            dst_line[x*3 + 1] = (rgb565 >> 8) & 0xFF; // RGB565 MSB
            dst_line[x*3 + 2] = alpha;
        }
    }
    
    free(row_buf);
    f.close();
    
    dsc->header.always_zero = 0;
    dsc->header.w = width;
    dsc->header.h = height;
    dsc->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    dsc->data_size = img_data_size;
    dsc->data = pixel_buf;
    
    Serial0.printf("[SD-BMP] %s charge: %dx%d (%u KB, %d transparents)\n",
                   path, width, height, (unsigned)(img_data_size/1024), magenta_count);
    return dsc;
}

void sd_free_bmp(lv_img_dsc_t* img) {
    if (!img) return;
    if (img->data) free((void*)img->data);
    free(img);
}
