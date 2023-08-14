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
// Note in S3 rtc_wdt has errors: https://github.com/espressif/esp-idf/issues/8038
#if defined CONFIG_IDF_TARGET_ESP32 && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "soc/rtc_wdt.h"       // Watchdog control
#endif
#include <gdew_colors.h>
#include <esp_timer.h>

#define WAVE7I5_WIDTH 800
#define WAVE7I5_HEIGHT 480

enum class WbrColor: uint8_t
{
  WHITE = 0,
  BLACK,
  RED
};

// Total number of pixels
#define WAVE7I5_PIXEL_NUM (uint32_t(WAVE7I5_WIDTH) * uint32_t(WAVE7I5_HEIGHT))

#define COLORS_SUPPORTED (3)

namespace kt
{
  template <uint16_t num_states, std::size_t num_values>
  class SubBitPackedArray;
}
typedef kt::SubBitPackedArray<COLORS_SUPPORTED, WAVE7I5_PIXEL_NUM> Wave7i5PixelBuffer;

class Wave7I5RB : public Epd
{
  public:

    static const uint8_t colors_supported = COLORS_SUPPORTED;
   
    Wave7I5RB(EpdSpi& IO);
    
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;  // Override GFX own drawPixel method
    
    uint16_t _setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye);
    // EPD tests 
    void init(bool debug = false) override;
    void update() override;
    void initFullUpdate();
    void initPartialUpdate();
    void updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation);
    void fillScreen(uint16_t color);

  private:
    EpdSpi& IO;

    Wave7i5PixelBuffer *_buffer;

    const bool _spi_optimized = true;

    bool _initial = true;
    
    void _wakeUp() override;
    void _sleep() override;
    void _waitBusy(const char* message) override;
    void _powerOn();
    void _reset();
    void _rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h);
};
