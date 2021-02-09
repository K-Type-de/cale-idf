#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include <stdint.h>
#include <math.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include <string>
#include <epd.h>
#include <Adafruit_GFX.h>
#include <epdspi.h>
#include <gdew_colors.h>

// Controller: IL3829  Note: Used by Waveshare in https://www.waveshare.com/wiki/1.54inch_e-Paper_Module
#define GDEH0154D67_WIDTH 200
#define GDEH0154D67_HEIGHT 200
#define GDEH0154D67_BUFFER_SIZE (uint32_t(GDEH0154D67_WIDTH) * uint32_t(GDEH0154D67_HEIGHT) / 8)
// 1 byte of this color in the buffer
#define GDEH0154D67_8PIX_BLACK 0xFF
#define GDEH0154D67_8PIX_WHITE 0x00

class Gdeh0154d67 : public Epd
{
  public:
    Gdeh0154d67(EpdSpi& IO);
    uint8_t colors_supported = 1;
    
    void drawPixel(int16_t x, int16_t y, uint16_t color);  // Override GFX own drawPixel method
    
    // EPD tests 
    void init(bool debug);
    void initFullUpdate();
    void initPartialUpdate();

    void fillScreen(uint16_t color);
    void update();
    // Partial update of rectangle from buffer to screen, does not power off
    void updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation = true);

  private:
    EpdSpi& IO;
    uint8_t _buffer[GDEH0154D67_BUFFER_SIZE];
    bool color = false;
    bool _initial = true;
    bool _debug_buffer = false;
    void _PowerOn();
    void _writeCommandData(const uint8_t cmd, const uint8_t* pCommandData, uint8_t datalen); // Waits for busy on each command
    void _setRamDataEntryMode(uint8_t em);
    void _SetRamArea(uint8_t Xstart, uint8_t Xend, uint8_t Ystart, uint8_t Ystart1, uint8_t Yend, uint8_t Yend1);
    void _SetRamPointer(uint8_t addrX, uint8_t addrY, uint8_t addrY1);

    uint16_t _setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye);
    void _wakeUp();

    void _wakeUp(uint8_t em);
    void _sleep();
    void _waitBusy(const char* message, uint16_t busy_time);
    void _waitBusy(const char* message);
    void _rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h);
};