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

#define FFT_BUF_TYPE float

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

#define N_SAMPLES 2048
  uint8_t fft_buffer[N_SAMPLES * 2];

  int N = N_SAMPLES;
  // Input test array
  float *x1;
  // Window coefficients
  float *wind;
  // working complex array
  float *y_cf;
  // Pointers to result arrays
  float *y1_cf;

  uint8_t fft_initialized = 0;
  int fft_size = 0;

  bool fft_running = false;

  FFT_BUF_TYPE map(FFT_BUF_TYPE x, FFT_BUF_TYPE in_min, FFT_BUF_TYPE in_max, FFT_BUF_TYPE out_min, FFT_BUF_TYPE out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  void deinit_fft()
  {
    free(x1);
    free(wind);
    free(y_cf);
    fft_initialized = 0;
  }

  void init_fft()
  {
    if (fft_initialized)
    {
      ESP_LOGW(FFT_TAG, "FFT has already been initialized. Skipping init_fft();");
      return;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK)
    {
      ESP_LOGE(FFT_TAG, "Not possible to initialize FFT. Error = %i", ret);
      return;
    }

    x1 = (float *)calloc(N, sizeof(float));
    wind = (float *)calloc(N, sizeof(float));
    y_cf = (float *)calloc(N * 2, sizeof(float));
    y1_cf = &y_cf[0];

    dsps_wind_hann_f32(wind, N);
    dsps_tone_gen_f32(x1, N, 1.0, 0.16, 0);

    ESP_LOGI(FFT_TAG, "FFT Initialized");
    fft_initialized = 1;
  }

  void calculate_fft(void *pvParameters)
  {
    while (1)
    {
      if (!fft_initialized)
      {
        ESP_LOGE(FFT_TAG, "FFT has not yet been allocated. Please call init_fft() before calculate_fft(). Exiting calculate_fft().");
        return; // FFT memory has not yet been allocated
      }

      fft_running = true;

      ESP_LOGI(FFT_TAG, "\nRunning FFT");

      for (int i = 0; i < N; i++)
      {
        x1[i] = map(((FFT_BUF_TYPE)fft_buffer[i * 2]) / 255.0, 0.0, 1.0, -1.0, 1.0);
      }

      // Convert two input vectors to one complex vector
      for (int i = 0; i < N; i++)
      {
        y_cf[i * 2 + 0] = x1[i] * wind[i];
        y_cf[i * 2 + 1] = 0;
      }

      unsigned int start_b = xthal_get_ccount();
      dsps_fft2r_fc32(y_cf, N);
      unsigned int end_b = xthal_get_ccount();
      // Bit reverse
      dsps_bit_rev_fc32(y_cf, N);
      // Convert one complex vector to two complex vectors
      dsps_cplx2reC_fc32(y_cf, N);

      for (int i = 0; i < N / 2; i++)
      {
        y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N);
      }

      dsps_view(y1_cf, N / 2, 128, 20, -60, 40, '|');
      ESP_LOGI(FFT_TAG, "FFT for %i complex points take %i cycles", N, end_b - start_b);

      vTaskDelay(15000 / portTICK_PERIOD_MS);

      fft_running = false;
    }
  }

  void copy_a2dp_buffer_to_fft_buffer(const uint8_t *data, uint32_t len)
  {
    if (!fft_running)
    {
      memcpy(fft_buffer, data, len);
    }
  }

#ifdef __cplusplus
}
#endif