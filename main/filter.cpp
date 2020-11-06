#include "filter.hpp"

const char *FILTER_TAG = "FILTER GEN";

Filter band1 = {
  .hz = 63,
  .type = BAND_PASS_FILTER,
  .smoothing = DEFAULT_IIR_SMOOTHING,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN - 2.0f,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band2 = {
  .hz = 180,
  .type = BAND_PASS_FILTER,
  .smoothing = DEFAULT_IIR_SMOOTHING,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN - 1.0f,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band3 = {
  .hz = 400,
  .type = BAND_PASS_FILTER,
  .smoothing = DEFAULT_IIR_SMOOTHING,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band4 = {
  .hz = 1000,
  .type = BAND_PASS_FILTER,
  .smoothing = DEFAULT_IIR_SMOOTHING,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN + 1.0f,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band5 = {
  .hz = 2500,
  .type = BAND_PASS_FILTER,
  .smoothing = DEFAULT_IIR_SMOOTHING,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN + 2.5f,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band6 = {
  .hz = 6250,
  .type = BAND_PASS_FILTER,
  .smoothing = 0.5f,
  .q_factor = DEFAULT_IIR_Q_FACTOR,
  .average = 0,
  .gain = IIR_BASE_GAIN + 5.0f,
  .delay_line = {0, 0},
  .coeffs = {}
};

Filter band7 = {
  .hz = 8000,
  .type = HIGH_PASS_FILTER,
  .smoothing = 0.6f,
  .q_factor = 1,
  .average = 0,
  .gain = IIR_BASE_GAIN + 5.5f,
  .delay_line = {0, 0},
  .coeffs = {}
};

FilterBank bank = {
  .filters = {
    band1,
    band2,
    band3,
    band4,
    band5,
    band6,
    band7,
  }
};

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

FilterBank *get_filterbank() {
  return &bank;
}

int get_number_of_filters() {
  return NUMBER_OF_FILTERS;
}

void generate_filterbank() {
  ESP_LOGI(FILTER_TAG, "Generating IIR filter bank coefficients");
  for (int i = 0; i < get_number_of_filters(); i++) {
    float freq = mapFloat(bank.filters[i].hz, 0.0f, (float)HALF_SAMPLING_RATE, 0.0f, 0.5f);
    filter_type type = bank.filters[i].type;
    
    if (type == LOW_PASS_FILTER) {
      if (dsps_biquad_gen_lpf_f32(bank.filters[i].coeffs, freq, bank.filters[i].q_factor) != ESP_OK)
      {
        ESP_LOGE(FILTER_TAG, "Unable to initialize lpf");
      }
    } else if (type == HIGH_PASS_FILTER) {
      if (dsps_biquad_gen_hpf_f32(bank.filters[i].coeffs, freq, bank.filters[i].q_factor) != ESP_OK)
      {
        ESP_LOGE(FILTER_TAG, "Unable to initialize hpf");
      }
    } else if (type == BAND_PASS_FILTER) {
      if (dsps_biquad_gen_bpf_f32(bank.filters[i].coeffs, freq, bank.filters[i].q_factor) != ESP_OK)
      {
        ESP_LOGE(FILTER_TAG, "Unable to initialize bpf");
      }
    }
  }
  ESP_LOGI(FILTER_TAG, "IIR filter bank coefficients generated");
}
