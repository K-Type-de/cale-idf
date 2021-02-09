#include "gdeh0154d67.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/task.h"

// Partial Update Delay
#define GDEH0154D67_PU_DELAY 100


// Constructor
Gdeh0154d67::Gdeh0154d67(EpdSpi& dio): 
  Adafruit_GFX(GDEH0154D67_WIDTH, GDEH0154D67_HEIGHT),
  Epd(GDEH0154D67_WIDTH, GDEH0154D67_HEIGHT), IO(dio)
{
  printf("Gdeh0154d67() %d*%d\n",
  GDEH0154D67_WIDTH, GDEH0154D67_HEIGHT);  
}

void Gdeh0154d67::initFullUpdate(){
    _wakeUp(0x03);
    
    _PowerOn();
    if (debug_enabled) printf("initFullUpdate() LUT\n");
}

void Gdeh0154d67::initPartialUpdate(){
    _wakeUp(0x03);

    _PowerOn();

    if (debug_enabled) printf("initPartialUpdate() LUT\n");
}

//Initialize the display
void Gdeh0154d67::init(bool debug)
{
    debug_enabled = debug;
    if (debug_enabled) printf("Gdeh0154d67::init(%d)\n", debug);
    IO.init(4, debug); // 4MHz frequency

    printf("Free heap:%d\n",xPortGetFreeHeapSize());
    fillScreen(EPD_WHITE);
}

void Gdeh0154d67::fillScreen(uint16_t color)
{
  // 0xFF = 8 pixels black, 0x00 = 8 pix. white
  uint8_t data = (color == EPD_BLACK) ? GDEH0154D67_8PIX_BLACK : GDEH0154D67_8PIX_WHITE;
  for (uint16_t x = 0; x < sizeof(_buffer); x++)
  {
    _buffer[x] = data;
  }

  if (debug_enabled) printf("fillScreen(%d) _buffer len:%d\n",data,sizeof(_buffer));
}

uint16_t Gdeh0154d67::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye) {
  printf("_setPartialRamArea not used in Gdeh0154d67");
  return 0;
}
void Gdeh0154d67::_wakeUp(){
  printf("_wakeUp not used in Gdeh0154d67");
}

/**
 * @deprecated It seems there is no need to do this for now
 */
void Gdeh0154d67::_writeCommandData(const uint8_t cmd, const uint8_t* pCommandData, uint8_t datalen) {
  if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY)){
    _waitBusy("_waitBusy",100);
  }
  IO.cmd(cmd);
  for (int i=0;i<datalen;++i) {
      IO.data(*pCommandData++);
  }
}

void Gdeh0154d67::_wakeUp(uint8_t em){
  printf("wakeup() start commands\n");

  IO.cmd(0x12);
  _waitBusy("epd_wakeup_power:ON"); // Needed?
  IO.cmd(0x01); // Driver output control
  IO.data(0xC7);
  IO.data(0x00);
  IO.data(0x00);
  IO.cmd(0x3C); // BorderWavefrom
  IO.data(0x05);
  IO.cmd(0x18); // Read built-in temperature sensor
  IO.data(0x80);
  _setRamDataEntryMode(em);
}

void Gdeh0154d67::update()
{
  initFullUpdate();
  printf("BUFF Size:%d\n",sizeof(_buffer));

  IO.cmd(0x24);        // update current data
  for (uint16_t y = 0; y < GDEH0154D67_HEIGHT; y++)
  {
    for (uint16_t x = 0; x < GDEH0154D67_WIDTH / 8; x++)
    {
      uint16_t idx = y * (GDEH0154D67_WIDTH / 8) + x;
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      IO.data(~data);
    }
  }
  IO.cmd(0x22);
  IO.data(0xf7);
  IO.cmd(0x20);
  _waitBusy("_Update_Full", 1200);

  _sleep();
}

void Gdeh0154d67::_setRamDataEntryMode(uint8_t em)
{
  const uint16_t xPixelsPar = GDEH0154D67_WIDTH - 1;
  const uint16_t yPixelsPar = GDEH0154D67_HEIGHT - 1;
  em = gx_uint16_min(em, 0x03);
  IO.cmd(0x11);
  IO.data(em);
  switch (em)
  {
    case 0x00: // x decrease, y decrease
      _SetRamArea(xPixelsPar / 8, 0x00, yPixelsPar % 256, yPixelsPar / 256, 0x00, 0x00);  // X-source area,Y-gate area
      _SetRamPointer(xPixelsPar / 8, yPixelsPar % 256, yPixelsPar / 256); // set ram
      break;
    case 0x01: // x increase, y decrease : as in demo code
      _SetRamArea(0x00, xPixelsPar / 8, yPixelsPar % 256, yPixelsPar / 256, 0x00, 0x00);  // X-source area,Y-gate area
      _SetRamPointer(0x00, yPixelsPar % 256, yPixelsPar / 256); // set ram
      break;
    case 0x02: // x decrease, y increase
      _SetRamArea(xPixelsPar / 8, 0x00, 0x00, 0x00, yPixelsPar % 256, yPixelsPar / 256);  // X-source area,Y-gate area
      _SetRamPointer(xPixelsPar / 8, 0x00, 0x00); // set ram
      break;
    case 0x03: // x increase, y increase : normal mode
      _SetRamArea(0x00, xPixelsPar / 8, 0x00, 0x00, yPixelsPar % 256, yPixelsPar / 256);  // X-source area,Y-gate area
      _SetRamPointer(0x00, 0x00, 0x00); // set ram
      break;
  }
}

void Gdeh0154d67::_SetRamArea(uint8_t Xstart, uint8_t Xend, uint8_t Ystart, uint8_t Ystart1, uint8_t Yend, uint8_t Yend1)
{
  IO.cmd(0x44);
  IO.data(Xstart);
  IO.data(Xend);
  IO.cmd(0x45);
  IO.data(Ystart);
  IO.data(Ystart1);
  IO.data(Yend);
  IO.data(Yend1);
}

void Gdeh0154d67::_SetRamPointer(uint8_t addrX, uint8_t addrY, uint8_t addrY1)
{
  IO.cmd(0x4e);
  IO.data(addrX);
  IO.cmd(0x4f);
  IO.data(addrY);
  IO.data(addrY1);
}

void Gdeh0154d67::_PowerOn(void)
{
  IO.cmd(0x22);
  IO.data(0xc0);
  IO.cmd(0x20);
  _waitBusy("_PowerOn");
}

void Gdeh0154d67::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (using_rotation) _rotate(x, y, w, h);
  if (x >= GDEH0154D67_WIDTH) return;
  if (y >= GDEH0154D67_HEIGHT) return;
  uint16_t xe = gx_uint16_min(GDEH0154D67_WIDTH, x + w) - 1;
  uint16_t ye = gx_uint16_min(GDEH0154D67_HEIGHT, y + h) - 1;
  uint16_t xs_d8 = x / 8;
  uint16_t xe_d8 = xe / 8;
  initPartialUpdate();
  _SetRamArea(xs_d8, xe_d8, y % 256, y / 256, ye % 256, ye / 256); // X-source area,Y-gate area
  _SetRamPointer(xs_d8, y % 256, y / 256); // set ram
  _waitBusy("partialUpdate1", 100); // needed ?
  IO.cmd(0x24);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_d8; x1 <= xe_d8; x1++)
    {
      uint16_t idx = y1 * (GDEH0154D67_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      IO.data(~data);
    }
  }
  
  IO.cmd(0x22);
  IO.data(0xff);
  IO.cmd(0x20);
  _waitBusy("partialUpdate2", 300);

  vTaskDelay(GDEH0154D67_PU_DELAY/portTICK_RATE_MS); 

  // update erase buffer
  _SetRamArea(xs_d8, xe_d8, y % 256, y / 256, ye % 256, ye / 256); // X-source area,Y-gate area
  _SetRamPointer(xs_d8, y % 256, y / 256); // set ram
  _waitBusy("partialUpdate3", 100); // needed ?
  IO.cmd(0x24);

  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_d8; x1 <= xe_d8; x1++)
    {
      uint16_t idx = y1 * (GDEH0154D67_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      IO.data(~data);
    }
  }
  vTaskDelay(GDEH0154D67_PU_DELAY/portTICK_RATE_MS); 
}

void Gdeh0154d67::_waitBusy(const char* message, uint16_t busy_time){
  if (debug_enabled) {
    ESP_LOGI(TAG, "_waitBusy for %s", message);
  }
  int64_t time_since_boot = esp_timer_get_time();
  // On high is busy
  if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 1) {
  while (1){
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 0) break;
    vTaskDelay(1);
    if (esp_timer_get_time()-time_since_boot>7000000)
    {
      if (debug_enabled) ESP_LOGI(TAG, "Busy Timeout");
      break;
    }
  }
  } else {
    vTaskDelay(busy_time/portTICK_RATE_MS); 
  }
}

void Gdeh0154d67::_waitBusy(const char* message){
  if (debug_enabled) {
    ESP_LOGI(TAG, "_waitBusy for %s", message);
  }
  int64_t time_since_boot = esp_timer_get_time();

  while (1){
    // On low is not busy anymore
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 0) break;
    vTaskDelay(1);
    if (esp_timer_get_time()-time_since_boot>7000000)
    {
      if (debug_enabled) ESP_LOGI(TAG, "Busy Timeout");
      break;
    }
  }
}

void Gdeh0154d67::_sleep(){
  IO.cmd(0x22); // power off display
  IO.data(0xc3);
  IO.cmd(0x20);
  _waitBusy("power_off");
}

void Gdeh0154d67::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = GDEH0154D67_WIDTH - x - w - 1;
      break;
    case 2:
      x = GDEH0154D67_WIDTH - x - w - 1;
      y = GDEH0154D67_HEIGHT - y - h - 1;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = GDEH0154D67_HEIGHT - y - h - 1;
      break;
  }
}


void Gdeh0154d67::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = GDEH0154D67_WIDTH - x - 1;
      break;
    case 2:
      x = GDEH0154D67_WIDTH - x - 1;
      y = GDEH0154D67_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = GDEH0154D67_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * GDEH0154D67_WIDTH / 8;

  // This is the trick to draw colors right. Genious Jean-Marc
  if (color) {
    _buffer[i] = (_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
    } else {
    _buffer[i] = (_buffer[i] | (1 << (7 - x % 8)));
    }
}
