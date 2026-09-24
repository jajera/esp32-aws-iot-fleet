#include "board_sd.h"

#if DEVICE_HAS_SD

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "ff.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef SD_MOUNT_POINT
#define SD_MOUNT_POINT "/sdcard"
#endif

#ifndef SD_PIN_CLK
#define SD_PIN_CLK 14
#endif
#ifndef SD_PIN_CMD_MOSI
#define SD_PIN_CMD_MOSI 15
#endif
#ifndef SD_PIN_D0_MISO
#define SD_PIN_D0_MISO 2
#endif
#ifndef SD_PIN_CS
#define SD_PIN_CS 13
#endif

static const char *TAG = "sd";
static board_sd_status_t s_status;
static sdmmc_card_t *s_card;
static bool s_use_spi;

static void sd_pullups(void)
{
    const int pins[] = {SD_PIN_CLK, SD_PIN_CMD_MOSI, SD_PIN_D0_MISO, SD_PIN_CS};
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        gpio_set_pull_mode(pins[i], GPIO_PULLUP_ONLY);
    }
}

static void refresh_capacity(void)
{
    FATFS *fs = NULL;
    DWORD free_clusters = 0;
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK && fs) {
        s_status.total_bytes = (uint64_t)(fs->n_fatent - 2) * fs->csize * 512ULL;
        s_status.free_bytes = (uint64_t)free_clusters * fs->csize * 512ULL;
    }
}

static bool mount_sdspi(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 4000;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_CMD_MOSI,
        .miso_io_num = SD_PIN_D0_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t err = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = host.slot;

    err = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sdspi mount failed: %s", esp_err_to_name(err));
        return false;
    }
    s_use_spi = true;
    return true;
}

static bool mount_sdmmc(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_err_t err =
        esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sdmmc mount failed: %s", esp_err_to_name(err));
        return false;
    }
    s_use_spi = false;
    return true;
}

bool board_sd_init(void)
{
    memset(&s_status, 0, sizeof(s_status));
    s_card = NULL;
    sd_pullups();

    if (!mount_sdspi() && !mount_sdmmc()) {
        s_status.ok = false;
        return false;
    }

    refresh_capacity();
    s_status.ok = true;
    mkdir(SD_MOUNT_POINT "/fleet", 0755);
    sdmmc_card_print_info(stdout, s_card);
    ESP_LOGI(TAG, "mounted %s via %s total=%llu MiB free=%llu MiB", SD_MOUNT_POINT,
             s_use_spi ? "spi" : "sdmmc", (unsigned long long)(s_status.total_bytes / (1024 * 1024)),
             (unsigned long long)(s_status.free_bytes / (1024 * 1024)));
    return true;
}

bool board_sd_write_file(const char *path, const uint8_t *data, size_t len)
{
    if (!s_status.ok || !path || !data || len == 0) {
        s_status.fail_count++;
        return false;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen %s failed", path);
        s_status.fail_count++;
        return false;
    }
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    if (written != len) {
        ESP_LOGE(TAG, "fwrite short %u/%u", (unsigned)written, (unsigned)len);
        s_status.fail_count++;
        unlink(path);
        return false;
    }

    strncpy(s_status.last_path, path, sizeof(s_status.last_path) - 1);
    s_status.write_count++;
    refresh_capacity();
    ESP_LOGI(TAG, "wrote %u bytes -> %s", (unsigned)len, path);
    return true;
}

void board_sd_get_status(board_sd_status_t *out)
{
    if (out) {
        *out = s_status;
    }
}

#endif
