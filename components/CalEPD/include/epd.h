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
#include <Adafruit_GFX.h>
#include <espspi.h>

// the only colors supported by any of these displays; mapping of other colors is class specific
#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */

typedef struct {
    uint8_t cmd;
    uint8_t data[42];
    uint8_t databytes; //No of data in data; 0xFF = end of cmds.
} epd_init_42;

typedef struct {
    uint8_t cmd;
    uint8_t data[44];
    uint8_t databytes;
} epd_init_44;

typedef struct {
    uint8_t cmd;
    uint8_t data[1];
} epd_init_1;

typedef struct {
    uint8_t cmd;
    uint8_t data[2];
} epd_init_2;

typedef struct {
    uint8_t cmd;
    uint8_t data[3];
} epd_init_3;

typedef struct {
    uint8_t cmd;
    uint8_t data[5];
} epd_power_5;

// Note: GxGDEW0213I5F is our test display that will be the default initializing this class
class Epd : public virtual Adafruit_GFX
{
  public:
    const char* TAG = "Epd driver";
    
    Epd(int16_t w, int16_t h) : Adafruit_GFX(w,h) {};

    /* @iotPanic
    Help needed:
       Can you please check the OOP consistency of this Classes?

     * Basically I want this Epd to provide general methods
       that need to be implemented like this ones
     * Another ones like write, print, println should be inherited
       so we avoid rewriting them on every MODELX class
    */
    // Every display model should implement this public methods
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;  // Override GFX own drawPixel method
    virtual void init(bool debug) = 0;
    virtual void initFullUpdate() = 0;
    virtual void initPartialUpdate() = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void update() = 0; 

    // Both partial updates DO NOT work as expected, turning all screen black or making strange effects
    // partial update of rectangle from buffer to screen, does not power off
    //virtual void updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation = true);
    // partial update of rectangle at (xs,ys) from buffer to screen at (xd,yd), does not power off
    //virtual void updateToWindow(uint16_t xs, uint16_t ys, uint16_t xd, uint16_t yd, uint16_t w, uint16_t h, bool using_rotation = true);

    // This are common methods every MODELX will inherit
    // hook to Adafruit_GFX::write
    size_t write(uint8_t);
    void print(const std::string& text);
    void println(const std::string& text);

  // Methods that should be accesible by inheriting this abstract class
  protected: 
    // This should be inherited from this abstract class so we don't repeat in every model
    static inline uint16_t gx_uint16_min(uint16_t a, uint16_t b) {return (a < b ? a : b);};
    static inline uint16_t gx_uint16_max(uint16_t a, uint16_t b) {return (a > b ? a : b);};
    bool _using_partial_mode = false;
    bool debug_enabled = true;
    // Very smart template from GxEPD to swap x,y:
    template <typename T> static inline void
    swap(T& a, T& b)
    {
      T t = a;
      a = b;
      b = t;
    }

  private:
    // Every display model should implement this private methods
    virtual uint16_t _setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye) = 0;
    virtual void _wakeUp() = 0;
    virtual void _sleep() = 0;
    virtual void _waitBusy(const char* message) = 0;
    virtual void _rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h) = 0;
 
    // Command & data structs should be implemented by every MODELX display
};