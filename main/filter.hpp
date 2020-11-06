#ifndef __FILTER_H__
#define __FILTER_H__

#include "stdint.h"
#include "esp_dsp.h"
#include "iir_filterbank_controller.hpp"

#define NUMBER_OF_FILTERS 7
#define DEFAULT_IIR_SMOOTHING 0.3f
#define DEFAULT_IIR_Q_FACTOR 1
#define IIR_BASE_GAIN 4.0f

typedef enum
{
  LOW_PASS_FILTER,
  HIGH_PASS_FILTER,
  BAND_PASS_FILTER,
} filter_type;

struct Filter
{
  int hz;
  filter_type type;
  float smoothing;
  float floor;
  float q_factor;
  float average;
  float gain;
  float delay_line[2];
  float coeffs[5];
};

struct FilterBank
{
  Filter filters[NUMBER_OF_FILTERS];
};

void generate_filterbank();

FilterBank *get_filterbank();

int get_number_of_filters();

#endif