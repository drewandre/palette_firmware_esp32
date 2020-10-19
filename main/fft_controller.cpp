#include "audio_analysis_controller.hpp"

const char *AUDIO_ANALYSIS_TAG = "AUDIO_ANALYSIS";

SemaphoreHandle_t xSemaphore = NULL;
TaskHandle_t RunAudioAnalysisTaskHandle = 0;
static int AnalysisHasUpdatedData = 0;
static bool initialized = false;

int number_of_octave_bands = 1;
float octave_bands[96];

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
static float *filter_output;
static float *buffer;
static float *wind;
static float *y_cf;
static float *y1_cf;
float wind[N_SAMPLES_HALF];
float y_cf[N_SAMPLES * 2];
float *y1_cf = &y_cf[0];
float *fft_averages;
#endif

void init_audio_analysis_controller()
{
  if (initialized)
  {
    ESP_LOGW(AUDIO_ANALYSIS_TAG, "Audio analysis controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(AUDIO_ANALYSIS_TAG, "Initializing audio analysis controller (using FFT)");

  // Semaphore created to sync analysis buffer with A2DP audio buffer
  xSemaphore = xSemaphoreCreateMutex();
  if (xSemaphore != NULL)
  {
    xSemaphoreGive(xSemaphore);
  }

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
  filter_output = (float *)calloc(N_SAMPLES, sizeof(float));
  buffer = (float *)calloc(N_SAMPLES, sizeof(float));
  wind = (float *)calloc(N_SAMPLES_HALF, sizeof(float));
  y_cf = (float *)calloc(N_SAMPLES * 2, sizeof(float));
  y1_cf = &y_cf[0];
#endif

  esp_err_t result = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
  if (result != ESP_OK)
  {
    ESP_LOGE(AUDIO_ANALYSIS_TAG, "Not possible to initialize FFT. Error = %i", result);
    return;
  }
  dsps_wind_hann_f32(wind, N_SAMPLES);
  number_of_octave_bands = init_log_averages(15.625, NUM_FFT_OCTAVES_PER_BAND);
  fft_averages = (float *)calloc(number_of_octave_bands, sizeof(float));

  xTaskCreatePinnedToCore(run_audio_analysis_task, "RunAudioAnalysisTask", RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE, NULL, RUN_AUDIO_ANALYSIS_TASK_PRIORITY, &RunAudioAnalysisTaskHandle, RUN_AUDIO_ANALYSIS_TASK_CORE_ID);

  initialized = true;
  ESP_LOGI(AUDIO_ANALYSIS_TAG, "Audio analysis controller initialized");
}

int get_averages_size() {
  return number_of_octave_bands / NUM_FFT_OCTAVES_PER_BAND;
}

void run_audio_analysis_task(void *pvParameters) {
  for (;;) {
    if (xSemaphore) {
      // Obtain semaphore, portMAX_DELAY
      if (AnalysisHasUpdatedData == 1 && (pdTRUE == xSemaphoreTake(xSemaphore, 0))) {
        calculate_fft();
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
  return fft_averages;
}


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
  free(wind);
  free(y_cf);
  free(fft_averages);
#endif
  initialized = false;
  ESP_LOGW(AUDIO_ANALYSIS_TAG, "Audio analysis controller deinitialized");
}