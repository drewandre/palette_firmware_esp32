#include "iir_filterbank_controller.hpp"

const char *IIR_TAG = "IIR";

static SemaphoreHandle_t xSemaphore = NULL;
TaskHandle_t RunAudioAnalysisTaskHandle = 0;
static int AnalysisHasUpdatedData = 0;

float y_cf[A2DP_BUFFER_LENGTH];
float filter_output[N_SAMPLES];
float buffer[N_SAMPLES];

int updateLedPeriodCount = 0;

void init_iir_controller()
{
  // Semaphore created to sync analysis buffer with A2DP audio buffer
  xSemaphore = xSemaphoreCreateMutex();
  if (xSemaphore != NULL)
  {
    xSemaphoreGive(xSemaphore);
  }
  else {
    ESP_LOGE(IIR_TAG, "Error initializing IIR filterbank semaphore");
    return;
  }

  generate_filterbank();

  xTaskCreate(calculate_iir, "calculate_iir", 4096, NULL, 9, NULL);
}

int get_num_iir_filters() {
  return NUMBER_OF_FILTERS;
}

int checkHaveCharRepeat(const uint8_t *buffer, int len)
{
  char prevTemp = 0.0;
  int filterCount = 0;
  int thresholdCount = len / 16;

  // for check data is valid
  prevTemp = buffer[0];
  for (int i = 1; i < len; i += 3)
  {
    if (prevTemp == buffer[i])
    {
      filterCount++;
    }

    if (filterCount > thresholdCount)
    {
      return -1;
    }
    prevTemp = buffer[i];
  }

  return 0;
}

void copy_a2dp_buffer_to_audio_analysis_buffer(const uint8_t *data, uint32_t len)
{
  // short temp_left = 0;
  // short temp_right = 0;
  // static int samplePeriod = 0;
  // float *y_cf_addr = NULL;

  if (len != A2DP_BUFFER_LENGTH)
  {
    // samplePeriod = 0;
    ESP_LOGE(IIR_TAG, "len != 2048 (len = %i)", len);
    return;
  }

  // if (checkHaveCharRepeat(data, len) != 0)
  // {
  //   ESP_LOGE(IIR_TAG, "checkHaveCharRepeat");
  //   // samplePeriod = 0;
  //   return;
  // }

  if (xSemaphore)
  {
    if (AnalysisHasUpdatedData == 0 && (pdTRUE == xSemaphoreTake(xSemaphore, 0)))
    {
      int t = 0;
      int16_t sample_l_int = 0;
      int16_t sample_r_int = 0;
      float sample_l_float = 0.0f;
      float sample_r_float = 0.0f;
      float in = 0.0f;

      // y_cf_addr = (float *)(&buffer[samplePeriod * (N_SAMPLES_HALF)]);
      // for (int i = 0; i < N_SAMPLES / 4; i++)
      for (int i = 0; i < A2DP_BUFFER_LENGTH; i += 4)
      {
        // temp_left = (short)(data[i * 4 + 1] << 8 | data[i * 4 + 0]);
        // temp_right = (short)(data[i * 4 + 3] << 8 | data[i * 4 + 2]);
        // y_cf_addr[i] = (((float)((temp_left + temp_right) / 32768.0f)) / 2.0f);

        sample_l_int = (int16_t)((*(data + i + 1) << 8) | *(data + i));
        sample_r_int = (int16_t)((*(data + i + 3) << 8) | *(data + i + 2));
        sample_l_float = (float)sample_l_int / 0x8000;
        sample_r_float = (float)sample_r_int / 0x8000;
        in = (sample_l_float + sample_r_float) / 2.0f;
        buffer[t] = in;
        t++;
      }
      // total raw data is N_SAMPLES
      // if (++samplePeriod == 4)
      // {
      AnalysisHasUpdatedData = 1;
      // samplePeriod = 0;
      // }

      xSemaphoreGive(xSemaphore);
    }
  }
}

void calculate_iir(void *pvParams)
{
  FilterBank *filter_bank = get_filterbank();
  while (1)
  {
    if (xSemaphore)
    {
      // obtain semaphore, portMAX_DELAY
      if (AnalysisHasUpdatedData == 1 && pdTRUE == xSemaphoreTake(xSemaphore, 0))
      {
        for (int bank = 0; bank < get_number_of_filters(); bank++) {
          float avg = 0;
          float x = 0;
          dsps_biquad_f32(
            buffer,
            filter_output,
            N_SAMPLES,
            filter_bank->filters[bank].coeffs,
            filter_bank->filters[bank].delay_line
          );
          for (int i = 0; i < N_SAMPLES; i++)
          {
            x = fabs(filter_output[i]) * filter_bank->filters[bank].gain;
            avg += x;
          }
          avg /= (float)N_SAMPLES;
          if (avg > 1)
          {
            avg = 1;
          } else if (avg < 0.01) {
            avg = 0;
          }
          filter_bank->filters[bank].average = filter_bank->filters[bank].average + (avg - filter_bank->filters[bank].average) * (float)filter_bank->filters[bank].smoothing;
          // printf("%f ", filter_bank->filters[bank].average);
        }
        // printf("\n");

        AnalysisHasUpdatedData = 0;
        updateLedPeriodCount++;
        xSemaphoreGive(xSemaphore);
      }
      else
      {
        // if (updateLedPeriodCount > LED_UPDATE_PERIOD)
        // {
        updateLedPeriodCount = 0;
        // run_led_audio_animation(averages, 7);
        // }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void deinit_iir_controller()
{
}