// #ifndef __AUDIO_ANALYSIS_CONTROLLER_H__
// #define __AUDIO_ANALYSIS_CONTROLLER_H__

// // #define DEBUG_AUDIO_ANALYSIS
// #define USE_FFT
// #define ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY

// #define SAMPLING_RATE (44100)
// #define HALF_SAMPLING_RATE (22050)
// #define A2DP_BUFFER_LENGTH (4096)
// #define N_SAMPLES (2048) // 1024
// #define N_SAMPLES_HALF (1024) // 512
// #define SMOOTHING_FACTOR (0.6000)
// #define NUM_FFT_OCTAVES_PER_BAND (3)

// const float freq_resolution = (float)SAMPLING_RATE / N_SAMPLES;
// const float half_freq_resolution = freq_resolution * 0.5;
// #define FREQ_RESOLUTION freq_resolution
// #define HALF_FREQ_RESOLUTION half_freq_resolution

// #include <math.h>
// #include <string.h>
// #include "esp_dsp.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/task.h"
// #include "free_rtos_controller.hpp"
// #include "fft_controller.hpp"
// #include "iir_filterbank_controller.hpp"

// void init_audio_analysis_controller();

// void deinit_audio_analysis_controller();

// float *get_audio_analysis_results();

// float *get_buffer();

// int checkHaveCharRepeat(const uint8_t *buffer, int len);

// void copy_a2dp_buffer_to_audio_analysis_buffer(const uint8_t *buffer, uint32_t len);

// #endif /* __AUDIO_ANALYSIS_CONTROLLER_H__*/