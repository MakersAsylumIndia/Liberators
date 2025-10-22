#include <driver/i2s.h>
#include <math.h>

#define I2S_NUM      I2S_NUM_0
#define I2S_BCK_PIN  26
#define I2S_WS_PIN   25
#define I2S_DATA_PIN 22

void setup() {
  Serial.begin(115200);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
}

void loop() {
  const int N = 512;
  static int16_t buf[N * 2];
  float freq = 440.0;  // A4
  for (int i = 0; i < N; i++) {
    float t = (float)i / 44100.0;
    int16_t s = (int16_t)(sin(2 * M_PI * freq * t) * 15000);
    buf[2*i]   = s;
    buf[2*i+1] = s;
  }
  size_t bytesWritten;
  i2s_write(I2S_NUM, buf, sizeof(buf), &bytesWritten, portMAX_DELAY);
}
