#include "led_controller.h"

void run_animation_task(void *pvParameters)
{
  while (1)
  {
    switch (currentAnimation)
    {
    case 0:
      // noise1d();
      noise2d();
      // blur1d(leds, NUM_LEDS, 100);
      // fadeToBlackBy(leds, NUM_LEDS, 50);
      show_leds();
      break;
    case 1:
      // rainbow();
      // fadeToBlackBy(leds, NUM_LEDS, 75);
      break;
    case 2:
      // fadeToBlackBy(leds, NUM_LEDS, 50);
      // mapFFTMono();
      break;
    case 3:
      // horizontalFlex();
      // fadeToBlackBy(leds, NUM_LEDS, 25);
      break;
    case 4:
      // solidColor(true);
      // fadeToBlackBy(leds, NUM_LEDS, 45);
      // FillLEDsFromPaletteColors();
      // blur1d(leds, NUM_LEDS, 5);
      break;
    case 5:
      // mapFFTMonoSmoothHorizontal();
      // fadeToBlackBy(leds, NUM_LEDS, 50);
      break;
    case 6:
      // mapPalette();
      // FillLEDsFromPaletteColors();
      break;
    default:
      currentAnimation = 0;
      break;
    }
  }
}

void show_leds()
{
  // while (1)
  // {
  FastLED.show();
  // }
}

void noise2d()
{
  // if (MSGEQ7.read())
  // {
  //   transformMSGEQ7Values(true);
  // }
  fill_noise_8();
  map_noise_to_leds_using_palette();
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fill_noise_8()
{
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if (speed < 50)
  {
    dataSmoothing = 200 - (speed * 4);
  }

  for (int i = 0; i < MAX_DIMENSION; i++)
  {
    int ioffset = scale * i;
    for (int j = 0; j < MAX_DIMENSION; j++)
    {
      int joffset = scale * j;

      uint8_t data = inoise8(x + ioffset, y + joffset, z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing)
      {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }

  z += speed;
  // z += map(leftFreqValues[0], 0, 255, 5, 50);

  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
uint16_t XY(uint8_t x, uint8_t y)
{
  uint16_t i;
  if (kMatrixSerpentineLayout == false)
  {
    i = (y * kMatrixWidth) + x;
  }
  if (kMatrixSerpentineLayout == true)
  {
    if (y & 0x01)
    {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    }
    else
    {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  return i;
}

void map_noise_to_leds_using_palette()
{
  // static uint8_t ihue = 0;

  for (int i = 0; i < kMatrixWidth; i++)
  {
    for (int j = 0; j < kMatrixHeight; j++)
    {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri = noise[i][j];

      // // if this palette is a 'loop', add a slowly-changing base value
      // if (colorLoop)
      // {
      //   index += ihue;
      // }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      // if (bri > 127)
      // {
      //   bri = 255;
      // }
      // else
      // {
      //   bri = dim8_raw(bri * 2);
      // }

      CRGB color = ColorFromPalette(currentPalette, index, bri);
      leds[XY(i, j)] = color;
    }
  }

  // ihue += 1;
}

void test_cylon_speed(void *pvParameters)
{
  while (1)
  {
    fadeToBlackBy(leds, NUM_LEDS, 7);
    static int i = 0;
    if (i > NUM_LEDS - 1)
    {
      i = 0;
    }
    i++;
    leds[i] = ColorFromPalette(currentPalette, i * iHue, 255, currentBlending);

    FastLED.show();
  }
};

void init_leds()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 30000);
  xTaskCreatePinnedToCore(&run_animation_task, "runAnimationTask", 5000, NULL, 4, NULL, 1);
  // xTaskCreatePinnedToCore(&show_leds, "showLeds", 5000, NULL, 10, NULL, 1);
}