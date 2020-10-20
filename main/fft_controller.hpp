#ifndef __FFT_CONTROLLER_H__
#define __FFT_CONTROLLER_H__

#include <math.h>
#include <string.h>
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "free_rtos_controller.hpp"
#include "audio_analysis_controller.hpp"

int init_log_averages(int minBandwidth, int bandsPerOctave);

int freqToIndex(float freq);

float calcAvg(float lowFreq, float hiFreq);

void calculate_fft();

void average_into_octave_bands();

void init_fft_controller();

void deinit_fft_controller();

float *get_fft_results();

int get_averages_size();

#endif /* __FFT_CONTROLLER_H__*/