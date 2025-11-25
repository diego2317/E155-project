/*
 * Arduino Nano ESP32 - OV7670 Serial Stream (USB)
 * * * * NO WIFI VERSION
 * This script sends JPEG images directly over the USB Serial connection.
 * You must run the accompanying Python script on your computer to view the video.
 *
 * * * * PIN MAPPING (Nano ESP32):
 * 3.3V -> 3.3V
 * GND  -> GND
 * SIOC -> A5  (GPIO 12)
 * SIOD -> A4  (GPIO 11)
 * VSYNC-> D13 (GPIO 48)
 * HREF -> A0  (GPIO 1)
 * PCLK -> A1  (GPIO 2)
 * XCLK -> A2  (GPIO 3)
 * D7   -> D10 (GPIO 21)
 * D6   -> D9  (GPIO 18)
 * D5   -> D8  (GPIO 17)
 * D4   -> D7  (GPIO 10)
 * D3   -> D6  (GPIO 9)
 * D2   -> D5  (GPIO 8)
 * D1   -> D4  (GPIO 7)
 * D0   -> D3  (GPIO 6)
 * RESET-> 3.3V
 * PWDN -> GND
 */

#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ===================
//   Pin Definitions (Raw GPIOs)
// ===================
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    3   // Pin A2
#define SIOD_GPIO_NUM    11  // Pin A4
#define SIOC_GPIO_NUM    12  // Pin A5

#define Y9_GPIO_NUM      21  // Pin D10
#define Y8_GPIO_NUM      18  // Pin D9
#define Y7_GPIO_NUM      17  // Pin D8
#define Y6_GPIO_NUM      10  // Pin D7
#define Y5_GPIO_NUM      9   // Pin D6
#define Y4_GPIO_NUM      8   // Pin D5
#define Y3_GPIO_NUM      7   // Pin D4
#define Y2_GPIO_NUM      6   // Pin D3

#define VSYNC_GPIO_NUM   4  // Pin D13
#define HREF_GPIO_NUM    1   // Pin A0
#define PCLK_GPIO_NUM    2   // Pin A1

#define THRESHOLD_VAL 127

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  // Use a high baud rate for video
  Serial.begin(2000000); 
  // Wait for serial connection
  while(!Serial);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  // 10MHz is a good balance for USB streaming
  config.xclk_freq_hz = 10000000; 
  
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_YUV422; 
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM; // Back to PSRAM (safer default on S3)
  config.jpeg_quality = 12;
  config.fb_count = 2;

  // Camera Init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("ERROR: Camera init failed with error 0x%x\n", err);
    return;
  }

  // sensor_t * s = esp_camera_sensor_get();
  // if (s != NULL) {
  //   s->set_aec2(s, 0);      // Disable Auto Exposure
  //   s->set_ae_level(s, 0); // Manual Exposure Level
  // }
}

void loop() {
  // 1. Acquire Frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    // If capture fails, send a text error so Python sees it
    delay(10);
    return;
  }

  // --- THRESHOLDING ---
  size_t len = fb->len;
  uint8_t *buf = fb->buf;
  for (size_t i = 0; i < len; i++) {
    if (buf[i] > THRESHOLD_VAL) buf[i] = 255;
    else buf[i] = 0;
  }

  // 2. Convert to JPEG
  uint8_t * jpg_buf = NULL;
  size_t jpg_len = 0;
  bool jpeg_converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format, 31, &jpg_buf, &jpg_len);
  
  // Free the raw YUV buffer immediately
  esp_camera_fb_return(fb);

  if (jpeg_converted) {
    // 3. Send Protocol Header: "IMG:<length>\n"
    Serial.printf("IMG:%d\n", jpg_len);
    
    // 4. Send Raw Data
    Serial.write(jpg_buf, jpg_len);
    
    free(jpg_buf);
  } else {
    Serial.println("ERROR: JPEG compression failed");
  }
}