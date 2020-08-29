#include "audio_analysis_controller.hpp"

const char *AUDIO_ANALYSIS_TAG = "AUDIO_ANALYSIS";

SemaphoreHandle_t xSemaphore = NULL;
TaskHandle_t RunAudioAnalysisTaskHandle = 0;
static int AnalysisHasUpdatedData = 0;
static bool initialized = false;

#ifdef AUDIO_ANALYSIS_USES_FFT
int number_of_octave_bands = 1;
float octave_bands[96];
#else
Filter sixty_three_hz = {
  .hz = 63,
  .amp_adjustment = 0.4000,
  .delay_line = {0, 0},
  .coefficients = {
    0.00005038862037565108,
    0.00010077724075130216,
    0.00005038862037565108,
    -1.985652113937946,
    0.9858536684194484,
  }
};

Filter eighty_hz = {
  .hz = 80,
  .amp_adjustment = 0.4000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0009489171832160505,
    0,
    -0.0009489171832160505,
    -1.9979723743507785,
    0.998102165633568
  }
};

Filter one_hundred_hz = {
  .hz = 100,
  .amp_adjustment = 0.4000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0011858507307301064,
    0,
    -0.0011858507307301064,
    -1.9974255489893915,
    0.9976282985385398,
  },
};

Filter one_hundred_twenty_five_hz = {
  .hz = 125,
  .amp_adjustment = 0.5000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0014818459335719117,
    0,
    -0.0014818459335719117,
    -1.9967196088573131,
    0.9970363081328562,
  },
};

Filter one_hundred_sixty_hz = {
  .hz = 160,
  .amp_adjustment = 0.7000,
  .delay_line = {0, 0},
  .coefficients = {
    0.011268651675437521,
    0,
    -0.011268651675437521,
    -1.9769489109406724,
    0.9774626966491249,
  }
};

Filter two_hundred_hz = {
  .hz = 200,
  .amp_adjustment = 0.7000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0023686524472432717,
    0,
    -0.0023686524472432717,
    -1.994452698405304,
    0.9952626951055136,
  }
};

Filter two_hundred_fifty_hz = {
  .hz = 250,
  .amp_adjustment = 0.7000,
  .delay_line = {0, 0},
  .coefficients = {
    0.002958838718667536,
    0,
    -0.002958838718667536,
    -1.9928174995860943,
    0.9940823225626647,
  }
};

Filter three_hundred_fifteen_hz = {
  .hz = 350,
  .amp_adjustment = 0.7000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0037248097826274498,
    0,
    -0.0037248097826274498,
    -1.990544014788569,
    0.992550380434745,
  }
};

Filter four_hundred_hz = {
  .hz = 400,
  .amp_adjustment = 0.7500,
  .delay_line = {0, 0},
  .coefficients = {
    0.027691113106086122,
    0,
    -0.027691113106086122,
    -1.9414606671809405,
    0.9446177737878276,
  }
};

Filter five_hundred_hz = {
  .hz = 500,
  .amp_adjustment = 0.8000,
  .delay_line = {0, 0},
  .coefficients = {
    0.005896499266353602,
    0,
    -0.005896499266353602,
    -1.98316421594659,
    0.9882070014672928,
  }
};

Filter six_hundred_thirty_hz = {
  .hz = 630,
  .amp_adjustment = 0.9000,
  .delay_line = {0, 0},
  .coefficients = {
    0.0074145561015553614,
    0,
    -0.0074145561015553614,
    -1.9771791734334088,
    0.9851708877968892,
  }
};

Filter eight_hundred_hz = {
  .hz = 800,
  .amp_adjustment = 0.9500,
  .delay_line = {0, 0},
  .coefficients = {
    0.009388851294371383,
    0,
    -0.009388851294371383,
    -1.968366603879957,
    0.9812222974112571,
  }
};

Filter one_thousand_hz = {
  .hz = 1000,
  .amp_adjustment = 1,
  .delay_line = {0, 0},
  .coefficients = {
    0.0662907071074853,
    0,
    -0.0662907071074853,
    -1.8484969161333196,
    0.8674185857850294,
  }
};

// Filter one_thousand_two_hundred_fifty_hz = {
//   .hz = 1250,
//   .amp_adjustment = 1,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.014548131651512725,
//     0,
//     -0.014548131651512725,
//     -1.9397299332189117,
//     0.9709037366969746,
//   }
// };

// Filter one_thousand_six_hundred_hz = {
//   .hz = 1600,
//   .amp_adjustment = 1,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.018484561850751473,
//     0,
//     -0.018484561850751473,
//     -1.9122455652599208,
//     0.9630308762984973,
//   }
// };

// Filter two_thousand_hz = {
//   .hz = 2000,
//   .amp_adjustment = 1,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.022889713376707196,
//     0,
//     -0.022889713376707196,
//     -1.8754170728941222,
//     0.9542205732465857,
//   }
// };

// Filter two_thousand_five_hundred_hz = {
//   .hz = 2500,
//   .amp_adjustment = 1.3000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.14846713280622098,
//     0,
//     -0.14846713280622098,
//     -1.5961682440350178,
//     0.7030657343875581,
//   }
// };

// Filter three_thousand_one_hundred_fifty_hz = {
//   .hz = 3150,
//   .amp_adjustment = 1.3000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.0348952707151781,
//     0,
//     -0.0348952707151781,
//     -1.7390586307020333,
//     0.9302094585696438,
//   }
// };

// Filter four_thousand_hz = {
//   .hz = 4000,
//   .amp_adjustment = 1.3000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.04302791656865104,
//     0,
//     -0.04302791656865104,
//     -1.611451193746246,
//     0.913944166862698,
//   }
// };

// Filter five_thousand_hz = {
//   .hz = 5000,
//   .amp_adjustment = 1.3000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.05165600383969903,
//     0,
//     -0.05165600383969903,
//     -1.4354302233774527,
//     0.8966879923206019,
//   }
// };

// Filter six_thousand_three_hundred_hz = {
//   .hz = 6300,
//   .amp_adjustment = 1.3000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.06116740652858825,
//     0,
//     -0.06116740652858825,
//     -1.170705095364023,
//     0.8776651869428236,
//   }
// };

// Filter six_thousand_two_hundred_fifty_hz = {
//   .hz = 6250,
//   .amp_adjustment = 2.200,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.27989430651657543,
//     0,
//     -0.27989430651657543,
//     -0.9059556689058577,
//     0.44021138696684914,
//   }
// };

// Filter eight_thousand_hz = {
//   .hz = 8000,
//   .amp_adjustment = 2.200,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.07038378636879018,
//     0,
//     -0.07038378636879018,
//     -0.7767315145056187,
//     0.8592324272624198,
//   }
// };

// hpf
// Filter ten_thousand_hz = {
//   .hz = 10000,
//   .amp_adjustment = 4.0000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.3831994032538892,
//     -0.7663988065077784,
//     0.3831994032538892,
//     -0.1947165117031973,
//     0.33808110131235947,
//   }
// };

// bpf
// Filter ten_thousand_hz = {
//   .hz = 10000,
//   .amp_adjustment = 4.0000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.07616662976058487,
//     0,
//     -0.07616662976058487,
//     -0.2688710139790474,
//     0.8476667404788303,
//   }
// };

// // hpf
// Filter sixteen_thousand_hz = {
//   .hz = 16000,
//   .amp_adjustment = 4.0000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.16414761995898747,
//     -0.32829523991797493,
//     0.16414761995898747,
//     1.2244151876269722,
//     0.8810056674629219,
//   }
// };

// Filter twelve_thousand_five_hundred_hz = {
//   .hz = 12500,
//   .amp_adjustment = 4.0000,
//   .delay_line = {0, 0},
//   .coefficients = {
//     0.07535824455206167,
//     0,
//     -0.07535824455206167,
//     0.3857761669901616,
//     0.8492835108958766,
//   }
// };

FilterBank bank = {
  .number_of_filters = NUM_AUDIO_ANALYSIS_BANDS,
  .averages = {},
  .smoothing_factor = SMOOTHING_FACTOR,
  .filters = {
    sixty_three_hz,
    eighty_hz,
    one_hundred_hz,
    one_hundred_twenty_five_hz,
    one_hundred_sixty_hz,
    two_hundred_hz,
    two_hundred_fifty_hz,
    three_hundred_fifteen_hz,
    four_hundred_hz,
    five_hundred_hz,
    six_hundred_thirty_hz,
    eight_hundred_hz,
    one_thousand_hz,
    // one_thousand_two_hundred_fifty_hz,
    // one_thousand_six_hundred_hz,
    // two_thousand_hz,
    // two_thousand_five_hundred_hz,
    // three_thousand_one_hundred_fifty_hz,
    // four_thousand_hz,
    // five_thousand_hz,
    // six_thousand_three_hundred_hz,
    // // six_thousand_two_hundred_fifty_hz,
    // eight_thousand_hz,
    // ten_thousand_hz,
    // twelve_thousand_five_hundred_hz,
    // sixteen_thousand_hz,
  }
};
#endif

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
static float *filter_output;
static float *buffer;
#ifdef AUDIO_ANALYSIS_USES_FFT
static float *wind;
static float *y_cf;
static float *y1_cf;
#endif
#else
float filter_output[N_SAMPLES];
float buffer[N_SAMPLES];
#ifdef AUDIO_ANALYSIS_USES_FFT
float wind[N_SAMPLES_HALF];
float y_cf[N_SAMPLES * 2];
float *y1_cf = &y_cf[0];
#endif
#endif

#ifdef AUDIO_ANALYSIS_USES_FFT
float *fft_averages;
#endif

void init_audio_analysis_controller()
{
  if (initialized)
  {
    ESP_LOGW(AUDIO_ANALYSIS_TAG, "Audio analysis controller has already been initialized. Skipping %s", __func__);
    return;
  }
#ifdef AUDIO_ANALYSIS_USES_FFT
  ESP_LOGI(AUDIO_ANALYSIS_TAG, "Initializing audio analysis controller (using FFT)");
#else
  ESP_LOGI(AUDIO_ANALYSIS_TAG, "Initializing audio analysis controller (using IIR filter bank)");
#endif

  // Semaphore created to sync analysis buffer with A2DP audio buffer
  xSemaphore = xSemaphoreCreateMutex();
  if (xSemaphore != NULL)
  {
    xSemaphoreGive(xSemaphore);
  }

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
  filter_output = (float *)calloc(N_SAMPLES, sizeof(float));
  buffer = (float *)calloc(N_SAMPLES, sizeof(float));
#ifdef AUDIO_ANALYSIS_USES_FFT
  wind = (float *)calloc(N_SAMPLES_HALF, sizeof(float));
  y_cf = (float *)calloc(N_SAMPLES * 2, sizeof(float));
  y1_cf = &y_cf[0];
#endif
#endif

#ifdef AUDIO_ANALYSIS_USES_FFT
  esp_err_t result = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
  if (result != ESP_OK)
  {
    ESP_LOGE(AUDIO_ANALYSIS_TAG, "Not possible to initialize FFT. Error = %i", result);
    return;
  }
  dsps_wind_hann_f32(wind, N_SAMPLES);
  number_of_octave_bands = init_log_averages(15.625, NUM_FFT_OCTAVES_PER_BAND);
  fft_averages = (float *)calloc(number_of_octave_bands, sizeof(float));
#endif

  xTaskCreatePinnedToCore(run_audio_analysis_task, "RunAudioAnalysisTask", RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE, NULL, RUN_AUDIO_ANALYSIS_TASK_PRIORITY, &RunAudioAnalysisTaskHandle, RUN_AUDIO_ANALYSIS_TASK_CORE_ID);

  initialized = true;
  ESP_LOGI(AUDIO_ANALYSIS_TAG, "Audio analysis controller initialized");
}

int get_averages_size() {
#ifdef AUDIO_ANALYSIS_USES_FFT
  return number_of_octave_bands / NUM_FFT_OCTAVES_PER_BAND;
#else
  return NUM_AUDIO_ANALYSIS_BANDS;
#endif
}

void run_audio_analysis_task(void *pvParameters) {
  for (;;) {
    if (xSemaphore) {
      // Obtain semaphore, portMAX_DELAY
      if (AnalysisHasUpdatedData == 1 && (pdTRUE == xSemaphoreTake(xSemaphore, 0))) {
      #ifdef AUDIO_ANALYSIS_USES_FFT
        calculate_fft();
      #else
        calculate_iir_filter_bank();
      #endif
        AnalysisHasUpdatedData = 0;
        xSemaphoreGive(xSemaphore);
      }
    }
    vTaskDelay(RUN_AUDIO_ANALYSIS_TASK_DELAY / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void copy_a2dp_buffer_to_audio_analysis_buffer(const uint8_t *data, uint32_t len)
{
  if (initialized && xSemaphore) {
    if (AnalysisHasUpdatedData == 0 && (pdTRUE == xSemaphoreTake(xSemaphore, 0))) {
      int t = 0;
      int16_t sample_l_int = 0;
      int16_t sample_r_int = 0;
      float sample_l_float = 0.0f;
      float sample_r_float = 0.0f;
      float in = 0.0f;

      for (uint32_t i = 0; i < A2DP_BUFFER_LENGTH; i += 4) {
        sample_l_int = (int16_t)((*(data + i + 1) << 8) | *(data + i));
        sample_r_int = (int16_t)((*(data + i + 3) << 8) | *(data + i + 2));
        sample_l_float = (float)sample_l_int / 0x8000;
        sample_r_float = (float)sample_r_int / 0x8000;
        in = (sample_l_float + sample_r_float) * 0.5;
        buffer[t] = in;
        t++;
      }
      AnalysisHasUpdatedData = 1;
      xSemaphoreGive(xSemaphore);
    }
  }
}

float *get_audio_analysis_results() {
#ifdef AUDIO_ANALYSIS_USES_FFT
  return fft_averages;
#else
  return bank.averages;
#endif
}

#ifdef AUDIO_ANALYSIS_USES_FFT

int init_log_averages(int minBandwidth, int bandsPerOctave) {
  float n = (float)HALF_SAMPLING_RATE;
  int octaves = 1;
  while ((n /= 2) > minBandwidth)
  {
    octaves++;
  }
  // avgPerOctave = bandsPerOctave;
  printf("octaves = %d, total octaves = %i, frequency resolution = %f, nyquist = %i\n", octaves, octaves * bandsPerOctave, FREQ_RESOLUTION, HALF_SAMPLING_RATE);
  return octaves * bandsPerOctave;
}

int freqToIndex(float freq) {
  // special case: freq is lower than the bandwidth of spectrum[0]
  if (freq < (HALF_FREQ_RESOLUTION)) {
    return 0;
  }
  // special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
  if (freq > (HALF_SAMPLING_RATE) - (HALF_FREQ_RESOLUTION)) {
    return N_SAMPLES_HALF - 1;
  }
  // all other cases
  float fraction = freq / (float)SAMPLING_RATE;
  int i = round((float)N_SAMPLES * fraction);
  return i;
}

float calcAvg(float lowFreq, float hiFreq) {
  int lowBound = freqToIndex(lowFreq);
  int hiBound = freqToIndex(hiFreq);
  float avg = 0;
  for (int i = lowBound; i <= hiBound; i++)
  {
    // float a = apply_a_weighting((float)i);
    // avg += (a * y1_cf[i]);
    avg += y1_cf[i];
// #ifdef DEBUG_FFT
//   printf("lowBound: %i (%fhz), hiBound: %i (%fhz), i: %i, a: %f, y1_cf[i]: %f, avg: %f\n", lowBound, lowFreq, hiBound, hiFreq, i, a, y1_cf[i], avg);
// #endif
    // avg += y1_cf[i];
  }
  avg /= (hiBound - lowBound + 1);
// #ifdef DEBUG_FFT
//   printf("low: %i (%fhz), high: %i (%fhz), avg: %f\n", lowBound, lowFreq, hiBound, hiFreq, avg);
// #endif
  return avg;
}

void calculate_fft() {
  if (!initialized)
  {
    ESP_LOGE(AUDIO_ANALYSIS_TAG, "Audio analysis controller has not yet been allocated. Please call init_audio_analysis_controller() before calculate_fft(). Exiting calculate_fft().");
    return; // FFT memory has not yet been allocated
  }

  for (int i = 0; i < N_SAMPLES; i++) {
    y_cf[i * 2 + 0] = buffer[i] * wind[i];
    y_cf[i * 2 + 1] = 0;
  }

  dsps_fft2r_fc32(y_cf, N_SAMPLES);
  dsps_bit_rev_fc32(y_cf, N_SAMPLES);

  for (int i = 0; i < N_SAMPLES_HALF; i++) {
    y1_cf[i] = (y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N_SAMPLES;
  }
  
  average_into_octave_bands();
}

void average_into_octave_bands() {
  int s = 0;
  for (int i = 0; i < number_of_octave_bands; i++)
  {
    float lowFreq = octave_bands[(i * NUM_FFT_OCTAVES_PER_BAND)];
    float hiFreq = octave_bands[(i * NUM_FFT_OCTAVES_PER_BAND) + 2];
    fft_averages[s] = calcAvg(lowFreq, hiFreq);
    // printf("%2.0f-%2.0fhz: %2.2f ", lowFreq, hiFreq, fft_averages[s]);
    s++;
  }
  // printf("\n");
}

#else

void calculate_iir_filter_bank() {
  float avg, x;
  for (int i = 0; i < bank.number_of_filters; i++) {
    avg = 0.0f;

    dsps_biquad_f32(buffer, filter_output, N_SAMPLES, bank.filters[i].coefficients, bank.filters[i].delay_line);

    for (int t = 0; t < N_SAMPLES; t++)
    {
      x = fabs(filter_output[t]) * 4.0f;
      avg += x;
    }
    avg /= (float)N_SAMPLES;

    avg *= bank.filters[i].amp_adjustment;

    if (avg > 1) {
      avg = 1;
    }

    bank.averages[i] = (bank.smoothing_factor * bank.averages[i]) + ((1 - bank.smoothing_factor) * avg);
  }
}

#endif

void deinit_audio_analysis_controller()
{
  if (!initialized) {
    ESP_LOGW(AUDIO_ANALYSIS_TAG, "Audio analysis controller not yet initialized - skipping %s", __func__);
    return;
  }
  vTaskDelete(RunAudioAnalysisTaskHandle);
#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
  free(buffer);
  free(filter_output);
#ifdef AUDIO_ANALYSIS_USES_FFT
  free(wind);
  free(y_cf);
#endif

#ifdef AUDIO_ANALYSIS_USES_FFT
  free(fft_averages);
#endif

#endif
  initialized = false;
  ESP_LOGW(AUDIO_ANALYSIS_TAG, "Audio analysis controller deinitialized");
}