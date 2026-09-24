# ideaspark-oled-02 — Ideaspark ESP32 + 1.14″ ST7789 TFT

| Field | Value |
|-------|--------|
| **Make / product** | Ideaspark ESP32-WROOM with **1.14″ colour TFT** (not the 0.96″ OLED sibling) |
| **Chip** | ESP32-WROOM / D0WD |
| **USB** | CH340 → `/dev/ttyUSB0` |
| **Display** | ST7789 SPI 240×135 (landscape) |
| **SPI pins** | MOSI **23**, SCLK **18**, CS **15**, DC **2**, RST **4**, BL **32** |
| **PIO env** | `ideaspark-oled-02` |
| **Thing** | `ideaspark-oled-02` |
| **Model telemetry** | `ideaspark-tft-1.14` |
| **MQTT topics** | `fleet/ideaspark-oled-02/…` |

Sibling: [`ideaspark-oled.md`](ideaspark-oled.md) (0.96″ SSD1306 I2C).

## Onboard

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2
pio run -e ideaspark-oled-02 -t upload
pio device monitor -e ideaspark-oled-02
```

If colours look inverted or the image is shifted, tweak `esp_lcd_panel_set_gap` / `invert_color` / `mirror` in `src/status_display_st7789.c`.

Status UI uses banded SPI blits (~7.5 KiB peak) rather than a full-frame RGB565 buffer.
