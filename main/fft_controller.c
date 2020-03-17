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

#include "esp_dsp.h"
#include "fft_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

  // Input test array
  static float x1[N_SAMPLES];
  // Window coefficients
  static float wind[N_SAMPLES];
  // working complex array
  static float y_cf[N_SAMPLES * 2];
  // Pointers to result arrays
  static float *y1_cf = &y_cf[0];

  void init_fft()
  {
    esp_err_t ret = dsps_fft2r_init_sc16(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK)
    {
      ESP_LOGE(FFT_TAG, "Not possible to initialize FFT. Error = %i", ret);
      return;
    }

    // Generate hann window
    dsps_wind_hann_f32(wind, N_SAMPLES);

    // // Generate input signal for x1 A=1 , F=0.1
    // dsps_tone_gen_f32(x1, N_SAMPLES, 1.0, 0.16, 0);

    // // // Convert two input vectors to one complex vector
    // for (int i = 0; i < N_SAMPLES; i++)
    // {
    //   y_cf[i * 2 + 0] = x1[i] * wind[i];
    // }

    // // // FFT
    // // unsigned int start_b = xthal_get_ccount();
    // dsps_fft2r_fc32(y_cf, N_SAMPLES);
    // // unsigned int end_b = xthal_get_ccount();
    // // // Bit reverse
    // dsps_bit_rev_fc32(y_cf, N_SAMPLES);
    // // // Convert one complex vector to two complex vectors
    // dsps_cplx2reC_fc32(y_cf, N_SAMPLES);

    // for (int i = 0; i < N_SAMPLES / 2; i++)
    // {
    //   y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N_SAMPLES);
    // }

    // dsps_view(y1_cf, N_SAMPLES / 2, 64, 10, -60, 40, '|');
  }

  /* calculate fft */
  void calculate_fft(int16_t *data, uint32_t len)
  {
    // dsps_fft2r_fc32(y_cf, N_SAMPLES);
    dsps_fft2r_sc16(data, N_SAMPLES);
    dsps_bit_rev_fc32(data, N_SAMPLES);
    dsps_cplx2reC_fc32(data, N_SAMPLES);

    for (int i = 0; i < N_SAMPLES / 2; i++)
    {
      data[i] = 10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1]) / N_SAMPLES);
    }

    dsps_view(data, N_SAMPLES / 2, 64, 10, -60, 40, '|');
  }

#ifdef __cplusplus
}
#endif