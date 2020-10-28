#ifndef __IIR_CONTROLLER_H__
#define __IIR_CONTROLLER_H__

#define LED_UPDATE_PERIOD (1) // ms
#define THRESHOLD_HIGH (0.1f)
#define THRESHOLD_LOW (0.00001f)
#define SLOW_SPEED (2)

#include "stdint.h"
#include <math.h>
#include <string.h>
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "iir_filterbank_controller.hpp"
#include "led_controller.hpp"
#include "filter.hpp"

// #define DEBUG_AUDIO_ANALYSIS
#define ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY

#define SAMPLING_RATE (44100)
#define HALF_SAMPLING_RATE (22050)
#define A2DP_BUFFER_LENGTH (4096)
#define N_SAMPLES (2048) // 1024
#define N_SAMPLES_HALF (1024) // 512
#define SMOOTHING_FACTOR (0.6000)
#define NUM_FFT_OCTAVES_PER_BAND (3)

const float freq_resolution = (float)SAMPLING_RATE / N_SAMPLES;
const float half_freq_resolution = freq_resolution * 0.5;
#define FREQ_RESOLUTION freq_resolution
#define HALF_FREQ_RESOLUTION half_freq_resolution

void init_iir_controller();

void calculate_iir(void *pvParams);

int get_num_iir_filters();

int checkHaveCharRepeat(const uint8_t *buffer, int len);

float *get_buffer();

void copy_a2dp_buffer_to_audio_analysis_buffer(const uint8_t *data, uint32_t len);

void deinit_iir_controller();

#endif
