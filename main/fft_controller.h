#ifndef __FFT_CONTROLLER_H__
#define __FFT_CONTROLLER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define FFT_TAG "FFT"

  void init_fft();
  void deinit_fft();
  float get_fft_bins();
  float find_e(int bands, int bins);
  float read_fft(int16_t binFirst, int16_t binLast);
  void calculate_fft();
  void copy_a2dp_buffer_to_fft_buffer(const uint8_t *data, uint32_t len);
  void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType);

#endif /* __FFT_CONTROLLER_H__*/

#ifdef __cplusplus
}
#endif