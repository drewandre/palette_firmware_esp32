#ifndef __FFT_CONTROLLER_H__
#define __FFT_CONTROLLER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_dsp.h"
#include <stdint.h>

#define FFT_TAG "FFT"
#define N_SAMPLES 1024

  /**
 * @brief init fft
 */
  void init_fft();

  /**
 * @brief calculate fft
 */
  void calculate_fft(int16_t *data, uint32_t len);

#endif /* __FFT_CONTROLLER_H__*/

#ifdef __cplusplus
}
#endif