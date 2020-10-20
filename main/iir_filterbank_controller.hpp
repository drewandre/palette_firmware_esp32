#ifndef __IIR_FILTERBANK_CONTROLLER_H__
#define __IIR_FILTERBANK_CONTROLLER_H__

#include <math.h>
#include <string.h>
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "free_rtos_controller.hpp"
#include "audio_analysis_controller.hpp"

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

void init_iir_filterbank_controller();

void deinit_iir_filterbank_controller();

float *get_iir_results();

int get_iir_num_filters();

#endif /* __IIR_FILTERBANK_CONTROLLER_H__*/