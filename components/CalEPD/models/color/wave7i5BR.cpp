// Black & Red version for Waveshare 7.5 inches epaper
#include "wave7i5BR.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/task.h"

#define SUBBITPACKEDARRAY_NO_EXCEPTIONS
#include <subbitpackedarray.h>


// Constructor
Wave7I5RB::Wave7I5RB(EpdSpi& dio): 
  Adafruit_GFX(WAVE7I5_WIDTH, WAVE7I5_HEIGHT),
  Epd(WAVE7I5_WIDTH, WAVE7I5_HEIGHT), IO(dio),
  _buffer{new Wave7i5PixelBuffer{}}
{
  printf("Wave7I5RB() constructor injects IO and extends Adafruit_GFX(%d,%d) Pix Buffer[%d]\n",
  (int)WAVE7I5_WIDTH, (int)WAVE7I5_HEIGHT, (int)this->_buffer->getByteSize());
}

void Wave7I5RB::initFullUpdate(){
  printf("Not implemented for this Epd\n");
}

void Wave7I5RB::initPartialUpdate(){
  printf("Not implemented for this Epd\n");
 }

// Initialize the display
void Wave7I5RB::init(bool debug)
{
    printf("\nAvailable heap at beginning of Epd init:%d\n", (int)xPortGetFreeHeapSize());

    debug_enabled = debug;
    if (debug_enabled) printf("Wave7I5RB::init(debug:%d)\n", debug);
    //Initialize SPI at 4MHz frequency. true for debug (Increasing up to 6 still works but no noticiable speed change)
    IO.init(4, debug_enabled);

    fillScreen(EPD_WHITE);

    printf("\nAvailable heap after Epd init:%d\n", (int)xPortGetFreeHeapSize());
}

void Wave7I5RB::fillScreen(uint16_t color)
{

  WbrColor wbrColor = color == EPD_WHITE ? WbrColor::WHITE : color == EPD_BLACK ? WbrColor::BLACK : WbrColor::RED;

  for (uint32_t i = 0; i < WAVE7I5_PIXEL_NUM; i++)
  {
    this->_buffer->set(i, static_cast<uint16_t>(wbrColor));
  }
}

void Wave7I5RB::_powerOn(){
  // Power on
  IO.cmd(0x12);
  vTaskDelay(pdMS_TO_TICKS(100));
  _waitBusy("display refresh");
}

void Wave7I5RB::_wakeUp(){
  _reset();

  // Wait for the electronic paper IC to release the idle signal
  _waitBusy("epd_wakeup reset");

  IO.cmd(0x01); //POWER SETTING
  IO.data(0x07);
  IO.data(0x07); //VGH=20V,VGL=-20V
  IO.data(0x3F); //VDH=15V
  IO.data(0x3F); //VDL=-15V

  IO.cmd(0x04); //POWER ON

  vTaskDelay(100 / portTICK_PERIOD_MS);
  _waitBusy("epd_power on");

  IO.cmd(0x00);    //PANNEL SETTING
  IO.data(0x0F);  //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

  IO.cmd(0x61);   //tres
  IO.data(0x03);  //source 800
  IO.data(0x20);
  IO.data(0x01);  //gate 480
  IO.data(0xE0);

  IO.cmd(0X15);
  IO.data(0x00);

  IO.cmd(0X50);   //VCOM AND DATA INTERVAL SETTING
  IO.data(0x11);
  IO.data(0x07);

  IO.cmd(0X60);   //TCON SETTING
  IO.data(0x22);

  IO.cmd(0x65);   // Resolution setting
  IO.data(0x00);
  IO.data(0x00);  //800*480
  IO.data(0x00);
  IO.data(0x00);

}

void Wave7I5RB::update()
{
  uint64_t startTime = esp_timer_get_time();
  _wakeUp();
  
  printf("\nSending BLACK buffer via SPI...\n");

  IO.cmd(0x10); // Black buffer

  uint32_t sent = 0;
  uint8_t bit;

  if (this->_spi_optimized)
  {
    uint16_t lineBytes = WAVE7I5_WIDTH/8;
    uint8_t linebuf[lineBytes] = {0};

    uint16_t byteCount = 0;
    uint8_t bitCount = 0;
    uint8_t data = 0;
    for(uint16_t state : *this->_buffer)
    {
      bit = static_cast<WbrColor>(state) == WbrColor::BLACK ? 0 : 1; //Inverse
      data |= (bit << (7 - bitCount++));

      if(bitCount == 8)
      {
        bitCount = 0;
        linebuf[byteCount++] = data;
        data = 0;

        if(byteCount == lineBytes)
        {
          IO.data(linebuf, sizeof(linebuf));
          sent+= sizeof(linebuf);
          byteCount = 0;
        }
      }
    }
  }
  else
  {
    uint8_t count = 0;
    uint8_t sendByte = 0;
    for(uint16_t state : *this->_buffer)
    {
      bit = static_cast<WbrColor>(state) == WbrColor::BLACK ? 0 : 1; //Inverse

      sendByte |= (bit << (7 - count++));
      if(count == 8)
      {
        IO.data(sendByte);
        sent++;
        count = 0;
        sendByte = 0;
      }
    }
  }

  printf("Sent %lu bytes\n", sent);


  IO.cmd(0x92);

  printf("\nSending RED buffer via SPI...\n");
  IO.cmd(0x13); // Red buffer

  sent = 0;
  
  if (this->_spi_optimized)
  {
    uint16_t lineBytes = WAVE7I5_WIDTH/8;
    uint8_t linebuf[lineBytes] = {0};

    uint16_t byteCount = 0;
    uint8_t bitCount = 0;
    uint8_t data = 0;
    for(uint16_t state : *this->_buffer)
    {
      bit = static_cast<WbrColor>(state) == WbrColor::RED ? 1 : 0;
      data |= (bit << (7 - bitCount++));

      if(bitCount == 8)
      {
        bitCount = 0;
        linebuf[byteCount++] = data;
        data = 0;

        if(byteCount == lineBytes)
        {
          IO.data(linebuf, sizeof(linebuf));
          sent+= sizeof(linebuf);
          byteCount = 0;
        }
      }
    }
  }
  else
  {
    uint8_t count = 0;
    uint8_t sendByte = 0;
    for(uint16_t state : *this->_buffer)
    {
      bit = static_cast<WbrColor>(state) == WbrColor::RED ? 1 : 0;

      sendByte |= (bit << (7 - count++));
      if(count == 8)
      {
        IO.data(sendByte);
        sent++;
        count = 0;
        sendByte = 0;
      }
    }
  }

  printf("Sent %lu bytes\n", sent);

  _waitBusy("sent data");

  uint64_t endTime = esp_timer_get_time();
  
  _powerOn();
  uint64_t powerOnTime = esp_timer_get_time();
  printf("\nAvailable heap after Epd update: %d bytes\nSTATS (ms)\n%llu _wakeUp settings+send Buffer\n%llu _powerOn\n%llu total time in millis\n",
  (int) xPortGetFreeHeapSize(), (endTime-startTime)/1000, (powerOnTime-endTime)/1000, (powerOnTime-startTime)/1000);

  _sleep();
}

uint16_t Wave7I5RB::_setPartialRamArea(uint16_t, uint16_t, uint16_t, uint16_t)
{
  printf("_setPartialRamArea not implemented in this Epd\n");
  return 0;
}

void Wave7I5RB::_waitBusy(const char* message)
{
  if (debug_enabled) {
    ESP_LOGI(TAG, "_waitBusy for %s", message);
  }
  int64_t time_since_boot = esp_timer_get_time();

  while (1){
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 1) break;
    vTaskDelay(1);
    if (esp_timer_get_time()-time_since_boot>2000000)
    {
      if (debug_enabled) ESP_LOGI(TAG, "Busy Timeout");
      break;
    }
  }
}


void Wave7I5RB::_sleep()
{
  IO.cmd(0x02);
  _waitBusy("poweroff");

  IO.cmd(0x07); // deep sleep
  IO.data(0xA5);
}

void Wave7I5RB::_reset()
{
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 0);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

void Wave7I5RB::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = WAVE7I5_WIDTH - x - w - 1;
      break;
    case 2:
      x = WAVE7I5_WIDTH - x - w - 1;
      y = WAVE7I5_HEIGHT - y - h - 1;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = WAVE7I5_HEIGHT - y - h - 1;
      break;
  }
}

void Wave7I5RB::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = WAVE7I5_WIDTH - x - 1;
      break;
    case 2:
      x = WAVE7I5_WIDTH - x - 1;
      y = WAVE7I5_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = WAVE7I5_HEIGHT - y - 1;
      break;
  }
  uint32_t pixel = x + y * WAVE7I5_WIDTH;
  WbrColor wbrColor = color == EPD_WHITE ? WbrColor::WHITE : color == EPD_BLACK ? WbrColor::BLACK : WbrColor::RED;

  this->_buffer->set(pixel, static_cast<uint16_t>(wbrColor));

}

