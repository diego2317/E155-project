/*
 * OV7670 Signal Diagnostic Tool (UPDATED A3)
 * Checks for signals on the new pin mapping.
 * VSYNC is now on Pin A3.
 */

#include "Arduino.h"
#include "esp_camera.h"

// ===================
//   Pin Definitions
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

// *** UPDATED PIN ***
#define VSYNC_GPIO_NUM   4   // Pin A3
#define HREF_GPIO_NUM    1   // Pin A0
#define PCLK_GPIO_NUM    2   // Pin A1

volatile unsigned long pclk_count = 0;
volatile unsigned long vsync_count = 0;
volatile unsigned long href_count = 0;

void IRAM_ATTR onPCLK() { pclk_count++; }
void IRAM_ATTR onVSYNC() { vsync_count++; }
void IRAM_ATTR onHREF() { href_count++; }

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Starting OV7670 Signal Checker...");

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
  config.xclk_freq_hz = 10000000; 
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_GRAYSCALE; 
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Init Failed: 0x%x\n", err);
  } else {
    Serial.println("Init Success. Listening for signals...");
  }

  // Attach Interrupts
  pinMode(PCLK_GPIO_NUM, INPUT);
  pinMode(VSYNC_GPIO_NUM, INPUT);
  pinMode(HREF_GPIO_NUM, INPUT);

  attachInterrupt(digitalPinToInterrupt(PCLK_GPIO_NUM), onPCLK, RISING);
  attachInterrupt(digitalPinToInterrupt(VSYNC_GPIO_NUM), onVSYNC, RISING);
  attachInterrupt(digitalPinToInterrupt(HREF_GPIO_NUM), onHREF, RISING);
}

void loop() {
  delay(1000);
  Serial.println("------ Signal Counts (per second) ------");
  Serial.print("PCLK: "); Serial.println(pclk_count);
  Serial.print("HREF: "); Serial.println(href_count);
  Serial.print("VSYNC: "); Serial.println(vsync_count);
  pclk_count = 0; href_count = 0; vsync_count = 0;
}