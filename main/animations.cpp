#include "animations.hpp"

void map_filterbank() {
  int number_of_audio_analysis_bands = get_num_iir_filters();
  float segment_length = (float)(NUM_LEDS) / (float)(number_of_audio_analysis_bands);
  float color_multiplier = 255.0f / (float)number_of_audio_analysis_bands;
  int overwriteAnimation = 0;
  int i = 0;

  float hue_multiplier = 255.0f / (float)NUM_LEDS;
  float band_multiplier = (float)255.0f / (number_of_audio_analysis_bands);
  FilterBank *filter_bank = get_filterbank();

  for (int band = 0; band < number_of_audio_analysis_bands; band++) {
    for (int i = band * segment_length; i < (band + 1) * segment_length; i++) {
      leds[i] = CHSV(band * band_multiplier, 255, filter_bank->filters[band].average * 255);
    }
    // printf("%f\t", filter_bank->filters[band].average * 255);
    // float current_brightness = filter_bank->filters[band].average * 255.0f;
    // float next_brightness = band == number_of_audio_analysis_bands - 1 ? current_brightness : filter_bank->filters[band + 1].average * 255.0f;
    // float current_hue = band * color_multiplier;
    // if (current_hue > 255) {
    //   current_hue = 255;
    // }
    // float next_hue = band == number_of_audio_analysis_bands - 1 ? current_hue : (band + 1) * color_multiplier;
    // if (next_hue > 255) {
    //   next_hue = 255;
    // }
    // fill_gradient(
    //   tempLeds,
    //   band * segment_length,
    //   CHSV(current_hue, 255, current_brightness),
    //   (band + 1) * segment_length,
    //   CHSV(next_hue, 255, next_brightness),
    //   SHORTEST_HUES
    // );
  }
  // printf("\n");
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   leds[i] += tempLeds[i];
  // }

  // blur1d(leds, NUM_LEDS, 50);
  // fastled_show_esp32();
  // fadeToBlackBy(leds, NUM_LEDS, 75);
}

void map_palette()
{
  static uint8_t startIndex = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, i * hue_multiplier, 255, LINEARBLEND);
  }
}

void addGlitter(fract8 chanceOfGlitter) 
{
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void rainbow()
{
  // static int initial_hue = 0;
  // initial_hue-=(animationVal2*0.1);
  // fill_rainbow(leds, NUM_LEDS, initial_hue, float(animationVal1)*0.05);
  // // map_temp_leds_to_leds();
  static int initial_hue = 0;
  initial_hue++;
  // initial_hue -= (animationVal2 * 0.1);
  // fill_rainbow(leds, NUM_LEDS, initial_hue, float(animationVal1) * 0.05);
  fill_rainbow(leds, NUM_LEDS, initial_hue, float(animationVal1) * 0.05);
  // map_temp_leds_to_leds();
}

void noise_2d()
{
  fill_noise_8();
  map_noise_to_leds_using_palette();
}

void noise_1d()
{
  fill_noise_8();
  map_noise_to_leds_using_palette();
}

void noise()
{
  uint16_t scale = animationVal2* 4;                             // the "zoom factor" for the noise
  for (uint16_t i = 0; i < NUM_LEDS; i++) {

    uint16_t shift_x = beatsin8(5);                           // the x position of the noise field swings @ 17 bpm
    uint16_t shift_y = millis() / 100;                        // the y position becomes slowly incremented

    uint16_t real_x = (i + shift_x)*scale;                    // the x position of the noise field swings @ 17 bpm
    uint16_t real_y = (i + shift_y)*scale;                    // the y position becomes slowly incremented
    uint32_t real_z = millis() * 20;                          // the z position becomes quickly incremented
    
    uint8_t noise = inoise16(real_x, real_y, real_z) >> 8;   // get the noise data and scale it down

    uint8_t index = sin8(noise*3);                           // map LED color based on noise data
    uint8_t bri   = noise;

    leds[i] = ColorFromPalette(currentPalette, index, bri, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
}

// // Fill the x/y array of 8-bit noise values using the inoise8 function.
// void fill_noise_8()
// {
//   // If we're runing at a low "speed", some 8-bit artifacts become visible
//   // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
//   // The amount of data smoothing we're doing depends on "speed" (animationVal2).
//   uint8_t dataSmoothing = 0;
//   if (animationVal2 < 50)
//   {
//     dataSmoothing = 200 - (animationVal2 * 4);
//   }

//   for (int i = 0; i < MAX_DIMENSION; i++)
//   {
//     int ioffset = animationVal1 * i;
//     for (int j = 0; j < MAX_DIMENSION; j++)
//     {
//       int joffset = animationVal1 * j;

//       uint8_t data = inoise8(x + ioffset, y + joffset, z);

//       // The range of the inoise8 function is roughly 16-238.
//       // These two operations expand those values out to roughly 0..255
//       // You can comment them out if you want the raw noise data.
//       data = qsub8(data, 16);
//       data = qadd8(data, scale8(data, 39));

//       if (dataSmoothing)
//       {
//         uint8_t olddata = noise[i][j];
//         uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
//         data = newdata;
//       }

//       noise[i][j] = data;
//     }
//   }

//   z += animationVal2;
//   // apply slow drift to X and Y, just for visual variation.
//   x += animationVal2 / 8;
//   y -= animationVal2 / 16;
// }

//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
// uint16_t XY(uint8_t x, uint8_t y)
// {
//   uint16_t i;
//   if (kMatrixSerpentineLayout == false)
//   {
//     i = (y * kMatrixWidth) + x;
//   }
//   if (kMatrixSerpentineLayout == true)
//   {
//     if (y & 0x01)
//     {
//       // Odd rows run backwards
//       uint8_t reverseX = (kMatrixWidth - 1) - x;
//       i = (y * kMatrixWidth) + reverseX;
//     }
//     else
//     {
//       // Even rows run forwards
//       i = (y * kMatrixWidth) + x;
//     }
//   }
//   return i;
// }

void map_noise_to_leds_using_palette()
{
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   uint8_t index = noise[i];
  //   uint8_t bri = noise[i];
  // }
  // static uint8_t ihue = 0;
  // for (int i = 0; i < kMatrixWidth; i++)
  // {
  //   for (int j = 0; j < kMatrixHeight; j++)
  //   {
  //     // We use the value at the (i,j) coordinate in the noise
  //     // array for our brightness, and the flipped value from (j,i)
  //     // for our pixel's index into the color palette.

  //     uint8_t index = noise[j][i];
  //     uint8_t bri = noise[i][j];

  //     // // if this palette is a 'loop', add a slowly-changing base value
  //     // if (colorLoop)
  //     // {
  //     //   index += ihue;

  //     // }

  //     // brighten up, as the color palette itself often contains the
  //     // light/dark dynamic range desired
  //     if (bri > 127)
  //     {
  //       bri = 255;
  //     }
  //     else
  //     {
  //       bri = dim8_raw(bri * 2);
  //     }

      // CRGB color = ColorFromPalette(currentPalette, index, bri);
      // leds[i] = color;
  //     leds[XY(i, j)] = color;
  //   }
  // }
  // ihue += 1;
}

void test_cylon_speed()
{
  fadeToBlackBy(leds, NUM_LEDS, 7);
  static int i = 0;
  if (i > NUM_LEDS - 1)
  {
    i = 0;
  }
  i++;
  leds[i] += ColorFromPalette(currentPalette, i * hue_multiplier, 255, currentBlending);
};

void show_led_ota_percentage(float percentage) {
  int amount = NUM_LEDS * percentage;
  printf("Received progress %i\n", amount);
  fill_solid(leds, amount, CRGB::White);
  fastled_show_esp32();
}

void blackout() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
}
