#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fft_controller.h"
#include "esp_dsp.h"

#define A2DP_BUFFER_LENGTH 4096
#define N_SAMPLES 2048
#define N_BANDS 32
#define MAX_BIN 512
#define SAMPLING_RATE 44100
#define N_FFT_SAMPLES 1024

  static int freq_resolution = SAMPLING_RATE / N_FFT_SAMPLES;

  static int16_t start_bin = 0;
  static int16_t end_bin = 0;

  static int16_t fft_bins[N_BANDS];
  static float output[N_BANDS];

  static uint8_t *fft_buffer;
  // Input test array
  static float *x1;
  // Window coefficients
  static float *wind;
  // working complex array
  static float *y_cf;
  // Pointers to result arrays
  static float *y1_cf;

  static uint8_t fft_initialized = 0;
  static int fft_size = 0;

  static bool fft_running = false;

  float find_e(int bands, int bins)
  {
    float increment = 0.1, eTest, n;
    int b, count, d;

    // Find E through brute force calculations
    for (eTest = 1; eTest < bins; eTest += increment)
    {
      count = 0;

      // Calculate full log values
      for (b = 0; b < bands; b++)
      {
        n = pow(eTest, b);
        d = (int)(n + 0.5);
        count += d;
      }

      if (count > bins)
      {                     // We calculated over our last bin
        eTest -= increment; // Revert back to previous calculation increment
        increment /= 10.0;  // Get a finer detailed calculation & increment a
                            // decimal point lower
      }
      else if (count == bins)
      {
        return eTest; // E found, return calculated E
      }

      if (increment < 0.0000001)
      {
        // Ran out of calculations. Return previous E.
        // Last bin will be lower than (bins-1)
        return eTest - increment;
      }
    }
    return 0; // Return error 0
  }

  float get_fft_bins()
  {
    static float e, n;
    static uint16_t b, bands, bins, count = 0, d;

    bands = N_BANDS; // Frequency bands; (Adjust to desiredvalue)
    bins = MAX_BIN;
    e = find_e(bands, bins); // Find calculated E value

    ESP_LOGI(FFT_TAG, "E: %4.4f", e); // Print calculated E value

    if (e)
    { // If a value was returned continue
      for (b = 0; b < bands; b++)
      { // Test and print the bins from the
        // calculated
        n = pow(e, b);
        d = (int)(n + 0.5);
        fft_bins[b] = count;
        ESP_LOGI(FFT_TAG, "Bin %2d: [%3d %3d] [%5dhz - %5dhz]", b, count, count + d - 1, count * freq_resolution, (count + d - 1) * freq_resolution); // Print low bin and high bin
        count += d - 1;
        ++count;
      }
    }
    else
    {
      ESP_LOGE(FFT_TAG, "Error calculating FFT bins");
      return e;
    }

    return e;
  }

  void init_fft()
  {
    if (fft_initialized)
    {
      ESP_LOGW(FFT_TAG, "FFT has already been initialized. Skipping init_fft();");
      return;
    }

    get_fft_bins();

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, N_SAMPLES);
    if (ret != ESP_OK)
    {
      ESP_LOGE(FFT_TAG, "Not possible to initialize FFT. Error = %i", ret);
      return;
    }

    fft_buffer = (uint8_t *)calloc(A2DP_BUFFER_LENGTH, sizeof(uint8_t));
    x1 = (float *)calloc(N_SAMPLES, sizeof(float));
    wind = (float *)calloc(N_SAMPLES * 0.25, sizeof(float));
    y_cf = (float *)calloc(N_SAMPLES * 2, sizeof(float));
    y1_cf = &y_cf[0];

    dsps_wind_hann_f32(wind, N_SAMPLES * 0.25);

    fft_size = (N_SAMPLES * sizeof(float)) + (N_SAMPLES * 0.25 * sizeof(float)) + (N_SAMPLES * 2 * sizeof(float)) + (A2DP_BUFFER_LENGTH * sizeof(uint8_t));

    ESP_LOGI(FFT_TAG, "Allocated %d bytes for FFT analysis", fft_size);
    // xTaskCreatePinnedToCore(&calculate_fft, "calculate_fft", 2000, NULL, 2, NULL, 1);

    fft_initialized = 1;
  }

  float read_fft(int16_t binFirst, int16_t binLast)
  {
    int16_t length = binLast - binFirst;
    if (binFirst > binLast)
    {
      int16_t tmp = binLast;
      binLast = binFirst;
      binFirst = tmp;
    }
    if (binFirst > MAX_BIN)
    {
      return 0.0;
    }
    if (binLast > MAX_BIN)
    {
      binLast = MAX_BIN;
    }
    float sum = 0;
    if (length != 0)
    {
      do
      {
        sum += y_cf[binFirst++];
      } while (binFirst <= binLast);
    }
    else
    {
      return y_cf[binFirst];
    }
    // return (float)sum * (1.0 / 16384.0);
    return sum / length;
  }

  void calculate_fft()
  {
    // while (1)
    // {
    if (!fft_initialized)
    {
      ESP_LOGE(FFT_TAG, "FFT has not yet been allocated. Please call init_fft() before calculate_fft(). Exiting calculate_fft().");
      return; // FFT memory has not yet been allocated
    }

    int t = 0;
    int16_t sample_l_int = 0;
    int16_t sample_r_int = 0;
    float sample_l_float = 0.0f;
    float sample_r_float = 0.0f;
    float in = 0.0f;

    for (uint32_t i = 0; i < N_SAMPLES; i += 4)
    {
      sample_l_int = (int16_t)((*(fft_buffer + i + 1) << 8) | *(fft_buffer + i));
      sample_r_int = (int16_t)((*(fft_buffer + i + 3) << 8) | *(fft_buffer + i + 2));
      sample_l_float = (float)sample_l_int / 0x8000;
      sample_r_float = (float)sample_r_int / 0x8000;
      in = (sample_l_float + sample_r_float) / 2.0f;
      x1[t] = in * wind[t];
      t++;
    }

    // Convert two input vectors to one complex vector
    for (int i = 0; i < N_SAMPLES; i++)
    {
      y_cf[i * 2 + 0] = x1[i];
      y_cf[i * 2 + 1] = 0;
    }

    dsps_fft2r_fc32(y_cf, N_SAMPLES);
    dsps_bit_rev_fc32(y_cf, N_SAMPLES);

    for (int i = 0; i < N_SAMPLES / 2; i++)
    {
      y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N_SAMPLES);
    }

    // for (int i = 0; i < N_BANDS; i++)
    // {
    //   start_bin = fft_bins[i];
    //   if (i < N_BANDS - 1)
    //   {
    //     end_bin = fft_bins[i + 1] - 1;
    //   }
    //   else
    //   {
    //     end_bin = MAX_BIN;
    //   }

    //   output[i] = read_fft(start_bin, end_bin);
    //   ESP_LOGI(FFT_TAG, "[%3d %3d]: %10f", start_bin, end_bin, output[i]);
    // }
    // ESP_LOG_BUFFER_CHAR(FFT_TAG, output, N_BANDS);

    // dsps_view(y1_cf, N_SAMP  LES / 2, 128, 20, -60, 40, '.');
    // ESP_LOGI(FFT_TAG, "FFT for %i complex points take %i cycles", N_SAMPLES, end_b - start_b);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    fft_running = false;
    // }
  }

  void copy_a2dp_buffer_to_fft_buffer(const uint8_t *data, uint32_t len)
  {
    if (fft_initialized)
    {
      memcpy(fft_buffer, data, A2DP_BUFFER_LENGTH);
    }
  }

  void deinit_fft()
  {
    free(x1);
    free(wind);
    free(y_cf);
    free(fft_buffer);
    fft_initialized = 0;
  }

#ifdef __cplusplus
}
#endif