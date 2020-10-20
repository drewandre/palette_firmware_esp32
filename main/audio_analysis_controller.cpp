#include "audio_analysis_controller.hpp"

const char *AUDIO_TAG = "AUDIO_ANALYSIS";
static bool initialized = false;

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
static float *buffer;
#else
float buffer[N_SAMPLES];
#endif

static SemaphoreHandle_t xSemaphore = NULL;
TaskHandle_t RunAudioAnalysisTaskHandle = 0;
static int AnalysisHasUpdatedData = 0;

float *get_buffer() {
  return buffer;
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

void run_audio_analysis_task(void *pvParameters) {
  for (;;) {
    if (xSemaphore) {
      // Obtain semaphore, portMAX_DELAY
      if (AnalysisHasUpdatedData == 1 && (pdTRUE == xSemaphoreTake(xSemaphore, 0))) {
      #ifdef USE_FFT
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

float *get_audio_analysis_results() {
#ifdef USE_FFT
  return get_fft_results();
#else
  return get_iir_results();
#endif
}

void init_audio_analysis_controller() {
  if (initialized)
  {
    ESP_LOGW(AUDIO_TAG, "Audio analysis controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(AUDIO_TAG, "Initializing audio analysis controller");

#ifdef USE_FFT
  init_fft_controller();
#else
  init_iir_filterbank_controller();
#endif

  // Semaphore created to sync analysis buffer with A2DP audio buffer
  xSemaphore = xSemaphoreCreateMutex();
  if (xSemaphore != NULL)
  {
    xSemaphoreGive(xSemaphore);
  }

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
  buffer = (float *)calloc(N_SAMPLES, sizeof(float));
#endif

  xTaskCreatePinnedToCore(run_audio_analysis_task, "RunAudioAnalysisTask", RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE, NULL, RUN_AUDIO_ANALYSIS_TASK_PRIORITY, &RunAudioAnalysisTaskHandle, RUN_AUDIO_ANALYSIS_TASK_CORE_ID);

  initialized = true;
  ESP_LOGI(AUDIO_TAG, "Audio analysis controller initialized");
}

void deinit_audio_analysis_controller() {
  if (!initialized) {
    ESP_LOGW(AUDIO_TAG, "Audio analysis controller not yet initialized - skipping %s", __func__);
    return;
  }

#ifdef USE_FFT
  deinit_fft_controller();
#else
  deinit_iir_filterbank_controller();
#endif

#ifdef ALLOCATE_AUDIO_ANALYSIS_BUFFERS_DYNAMICALLY
  free(buffer);
#endif
  vTaskDelete(RunAudioAnalysisTaskHandle);
  initialized = false;
  ESP_LOGI(AUDIO_TAG, "Audio analysis controller deinitialized");
}