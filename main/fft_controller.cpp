// #include "fft_controller.hpp"

// const char *FFT_TAG = "FFT";
// static bool initialized = false;

// int number_of_octave_bands = 1;
// float octave_bands[96];

// #ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
// static float *wind;
// static float *y_cf;
// static float *y1_cf;
// #else
// float wind[N_SAMPLES_HALF];
// float y_cf[N_SAMPLES * 2];
// float *y1_cf = &y_cf[0];
// float fft_buffer[CONFIG_DSP_MAX_FFT_SIZE];
// #endif

// float *fft_averages;

// void init_fft_controller()
// {
//   if (initialized)
//   {
//     ESP_LOGW(FFT_TAG, "FFT controller has already been initialized. Skipping %s", __func__);
//     return;
//   }
//   ESP_LOGI(FFT_TAG, "Initializing FFT controller");

// #ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
//   wind = (float *)calloc(N_SAMPLES_HALF, sizeof(float));
//   y_cf = (float *)calloc(N_SAMPLES * 2, sizeof(float));
//   y1_cf = &y_cf[0];
//   // esp_err_t result = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
// #else
//   // esp_err_t result = dsps_fft2r_init_fc32(fft_buffer, CONFIG_DSP_MAX_FFT_SIZE);
// #endif
//   esp_err_t result = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

//   if (result != ESP_OK)
//   {
//     ESP_LOGE(FFT_TAG, "Not possible to initialize FFT. Error = %i", result);
//     return;
//   }
//   dsps_wind_hann_f32(wind, N_SAMPLES);
//   number_of_octave_bands = get_octave_bands(3, 15.625);
//   // number_of_octave_bands = init_log_averages(15.625, NUM_FFT_OCTAVES_PER_BAND);
//   fft_averages = (float *)calloc(number_of_octave_bands, sizeof(float));

//   initialized = true;
//   ESP_LOGI(FFT_TAG, "FFT controller initialized");
// }

// int get_averages_size() {
//   return number_of_octave_bands / NUM_FFT_OCTAVES_PER_BAND;
// }

// float *get_fft_results() {
//   return y1_cf;
// }

// int get_octave_bands(float N = 3, float fCtr0 = 15.625) {
//   int octave_bands_length = 0;
//   octave_bands[0] = fCtr0 / pow(2, 1 / (2 * N));
//   octave_bands[1] = fCtr0;
//   octave_bands[2] = fCtr0 * pow(2, 1 / (2 * N));
//   ESP_LOGI(FFT_TAG, "FFT octave band calibration");
//   ESP_LOGI(FFT_TAG, "%-12f %-12f %-12f", octave_bands[0], octave_bands[1], octave_bands[2]);
//   int i = 3;
//   while (octave_bands[i - 1] < HALF_SAMPLING_RATE) {
//     octave_bands[i] = octave_bands[i - 1];
//     octave_bands[i + 1] = octave_bands[i - 2] * pow(2, 1 / N);
//     octave_bands[i + 2] = octave_bands[i + 1] * pow(2, 1 / (2 * N));
//     ESP_LOGI(FFT_TAG, "%-12f %-12f %-12f", octave_bands[i], octave_bands[i + 1], octave_bands[i + 2]);
//     i += 3;
//   }
//   octave_bands_length = i;
//   ESP_LOGI(FFT_TAG, "Number of octave bands: %d", octave_bands_length/3);
//   return octave_bands_length/3;
// }

// int init_log_averages(int minBandwidth, int bandsPerOctave) {
//   float n = (float)HALF_SAMPLING_RATE;
//   int octaves = 1;
//   while ((n /= 2) > minBandwidth)
//   {
//     octaves++;
//   }
//   // avgPerOctave = bandsPerOctave;
//   printf("octaves = %d, total octaves = %i, frequency resolution = %f, nyquist = %i\n", octaves, octaves * bandsPerOctave, FREQ_RESOLUTION, HALF_SAMPLING_RATE);
//   return octaves * bandsPerOctave;
// }

// int freqToIndex(float freq) {
//   // special case: freq is lower than the bandwidth of spectrum[0]
//   if (freq < (HALF_FREQ_RESOLUTION)) {
//     return 0;
//   }
//   // special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
//   if (freq > (HALF_SAMPLING_RATE) - (HALF_FREQ_RESOLUTION)) {
//     return N_SAMPLES_HALF - 1;
//   }
//   // all other cases
//   float fraction = freq / (float)SAMPLING_RATE;
//   int i = round((float)N_SAMPLES * fraction);
//   return i;
// }

// float calcAvg(float lowFreq, float hiFreq) {
//   int lowBound = freqToIndex(lowFreq);
//   int hiBound = freqToIndex(hiFreq);
//   float avg = 0;
//   for (int i = lowBound; i <= hiBound; i++)
//   {
//     // float a = apply_a_weighting((float)i);
//     // avg += (a * y1_cf[i]);
//     avg += y1_cf[i];
// // #ifdef DEBUG_FFT
// //   printf("lowBound: %i (%fhz), hiBound: %i (%fhz), i: %i, a: %f, y1_cf[i]: %f, avg: %f\n", lowBound, lowFreq, hiBound, hiFreq, i, a, y1_cf[i], avg);
// // #endif
//     // avg += y1_cf[i];
//   }
//   avg /= (hiBound - lowBound + 1);
// // #ifdef DEBUG_FFT
// //   printf("low: %i (%fhz), high: %i (%fhz), avg: %f\n", lowBound, lowFreq, hiBound, hiFreq, avg);
// // #endif
//   return avg;
// }

// void calculate_fft() {
//   if (!initialized)
//   {
//     ESP_LOGE(FFT_TAG, "FFT controller has not yet been allocated. Please call init_fft_controller() before calculate_fft(). Exiting calculate_fft().");
//     return; // FFT memory has not yet been allocated
//   }

//   float *buffer = get_buffer();

//   for (int i = 0; i < N_SAMPLES; i++) {
//     y_cf[i * 2 + 0] = buffer[i] * wind[i];
//     y_cf[i * 2 + 1] = 0;
//   }

//   dsps_fft2r_fc32(y_cf, N_SAMPLES);
//   dsps_bit_rev_fc32(y_cf, N_SAMPLES);

//   for (int i = 0; i < N_SAMPLES_HALF; i++) {
//     // y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N_SAMPLES);
//     y1_cf[i] = (y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N_SAMPLES;
//   }
  
//   // average_into_octave_bands();
// }

// void average_into_octave_bands() {
//   int s = 0;
//   for (int i = 0; i < number_of_octave_bands; i++)
//   {
//     float lowFreq = octave_bands[(i * NUM_FFT_OCTAVES_PER_BAND)];
//     float hiFreq = octave_bands[(i * NUM_FFT_OCTAVES_PER_BAND) + 2];
//     fft_averages[s] = calcAvg(lowFreq, hiFreq);
//     // printf("%2.0f-%2.0fhz: %2.2f ", lowFreq, hiFreq, fft_averages[s]);
//     s++;
//   }
//   // printf("\n");
// }

// void deinit_fft_controller()
// {
//   if (!initialized) {
//     ESP_LOGW(FFT_TAG, "FFT controller not yet initialized - skipping %s", __func__);
//     return;
//   }
  
// #ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
//   free(wind);
//   free(y_cf);
//   free(fft_averages);
// #endif

//   initialized = false;
//   ESP_LOGW(FFT_TAG, "FFT controller deinitialized");
// }