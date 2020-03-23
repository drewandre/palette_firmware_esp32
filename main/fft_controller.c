// #ifdef __cplusplus
// extern "C"
// {
// #endif

// #include <stdint.h>
// #include <stdbool.h>
// #include <math.h>
// #include <stdlib.h>
// #include <string.h>
// #include "esp_log.h"

// #include "esp_dsp.h"
// #include "fft_controller.h"

// #define BUF_SIZE 1024
// #define USE_WINDOW false

//   static uint8_t fft_buffer[BUF_SIZE];

//   static bool fft_running = false;
//   // static float x1[BUF_SIZE];
//   static float y_cf[BUF_SIZE * 2];
//   static float *y1_cf = &y_cf[0];

//   void deinit_fft()
//   {
//   }

//   void init_fft()
//   {
//     esp_err_t err = dsps_fft2r_init_fc32(NULL, BUF_SIZE);
//     if (err != ESP_OK)
//     {
//       ESP_LOGE(FFT_TAG, "Not possible to initialize FFT. Error = %i", err);
//       return;
//     }
//     // dsps_tone_gen_f32(x1, BUF_SIZE, 1.0, 0.16, 0);
//   }

//   void calculate_fft()
//   {
//     fft_running = true;
//     for (int i = 0; i < BUF_SIZE; i++)
//     {
//       // float f1 = fft_buffer[i] / 255;
//       // ESP_LOGI(FFT_TAG, "%f", f1);
// #if USE_WINDOW
//       y_cf[i * 2 + 0] = f1 * wind[i];
//       y_cf[i * 2 + 1] = x1[i] * wind[i];
// #else
//     y_cf[i * 2 + 0] = fft_buffer[i] / 255;
//     y_cf[i * 2 + 1] = 0;
// #endif
//     }

//     dsps_fft2r_fc32(y_cf, BUF_SIZE);
//     dsps_bit_rev_fc32(y_cf, BUF_SIZE);
//     dsps_cplx2reC_fc32(y_cf, BUF_SIZE);

//     for (int i = 0; i < BUF_SIZE / 2; i++)
//     {
//       y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / BUF_SIZE);
//     }

//     ESP_LOGW(FFT_TAG, "Signal y1_cf");
//     // dsps_view(y1_cf, BUF_SIZE / 2, 128, 30, -60, 60, '|');
//     fft_running = false;
//   }

//   void copy_a2dp_buffer_for_fft(const uint8_t *data, uint32_t len)
//   {
//     if (!fft_running)
//     {
//       memcpy(fft_buffer, data, BUF_SIZE);
//     }
//   }

// #ifdef __cplusplus
// }
// #endif