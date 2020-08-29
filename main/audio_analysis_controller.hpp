#ifndef __AUDIO_ANALYSIS_CONTROLLER_H__
#define __AUDIO_ANALYSIS_CONTROLLER_H__

#include <math.h>
#include <string.h>
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "free_rtos_controller.hpp"

// #define DEBUG_AUDIO_ANALYSIS
#define ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY

// if false, use IIR filter bank
// #define AUDIO_ANALYSIS_USES_FFT

#define SAMPLING_RATE (44100)
#define HALF_SAMPLING_RATE (22050)
#define A2DP_BUFFER_LENGTH (4096)
#define N_SAMPLES (1024)
#define N_SAMPLES_HALF (512)
#define NUM_AUDIO_ANALYSIS_BANDS (13)
#define SMOOTHING_FACTOR (0.6000)

#ifdef AUDIO_ANALYSIS_USES_FFT

#define NUM_FFT_OCTAVES_PER_BAND 3

const float freq_resolution = (float)SAMPLING_RATE / N_SAMPLES;
const float half_freq_resolution = freq_resolution * 0.5;
#define FREQ_RESOLUTION freq_resolution
#define HALF_FREQ_RESOLUTION half_freq_resolution

int init_log_averages(int minBandwidth, int bandsPerOctave);

int freqToIndex(float freq);

float calcAvg(float lowFreq, float hiFreq);

void calculate_fft();

void average_into_octave_bands();

#else

struct Filter
{
  int hz;
  float amp_adjustment;
  float delay_line[5];
  float coefficients[5];
};

struct FilterBank
{
  int number_of_filters;
  float averages[NUM_AUDIO_ANALYSIS_BANDS];
  float smoothing_factor;
  Filter filters[NUM_AUDIO_ANALYSIS_BANDS];
};

extern FilterBank filterBank[NUM_AUDIO_ANALYSIS_BANDS];

void calculate_iir_filter_bank();

#endif

void init_audio_analysis_controller();

void deinit_audio_analysis_controller();

void copy_a2dp_buffer_to_audio_analysis_buffer(const uint8_t *buffer, uint32_t len);

void run_audio_analysis_task(void *pvParameters);

float *get_audio_analysis_results();

int get_averages_size();

#endif /* __AUDIO_ANALYSIS_CONTROLLER_H__*/