#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TLV320DAC3100.h>
#include <driver/i2s.h>
#include <math.h>

Adafruit_TLV320DAC3100 codec;

// I2C pins from your schematic
#define I2C_SDA   8
#define I2C_SCL   9

// I2S pins from your schematic
#define I2S_BCLK  6
#define I2S_WS    7
#define I2S_DOUT  36

// New reset wiring
#define TLV_RESET 21

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100

void fail(const char *msg) {
  Serial.println(msg);
  while (1) {
    delay(10);
  }
}

void resetCodec() {
  pinMode(TLV_RESET, OUTPUT);
  digitalWrite(TLV_RESET, LOW);
  delay(20);
  digitalWrite(TLV_RESET, HIGH);
  delay(20);
}

void setupCodec() {
  Serial.println("Initializing codec...");
  if (!codec.begin()) {
    fail("Failed to initialize codec!");
  }

  delay(20);

  Serial.println("Configuring codec interface...");
  if (!codec.setCodecInterface(TLV320DAC3100_FORMAT_I2S,
                               TLV320DAC3100_DATA_LEN_16)) {
    fail("Failed to configure codec interface!");
  }

  Serial.println("Configuring codec clocks...");
  if (!codec.setCodecClockInput(TLV320DAC3100_CODEC_CLKIN_PLL) ||
      !codec.setPLLClockInput(TLV320DAC3100_PLL_CLKIN_BCLK)) {
    fail("Failed to configure codec clocks!");
  }

  if (!codec.setPLLValues(1, 2, 32, 0)) {
    fail("Failed to configure PLL values!");
  }

  if (!codec.setNDAC(true, 8) ||
      !codec.setMDAC(true, 2)) {
    fail("Failed to configure DAC dividers!");
  }

  if (!codec.powerPLL(true)) {
    fail("Failed to power PLL!");
  }

  Serial.println("Configuring DAC path...");
  if (!codec.setDACDataPath(true, true,
                            TLV320_DAC_PATH_NORMAL,
                            TLV320_DAC_PATH_NORMAL,
                            TLV320_VOLUME_STEP_1SAMPLE)) {
    fail("Failed to configure DAC data path!");
  }

  if (!codec.configureAnalogInputs(TLV320_DAC_ROUTE_MIXER,
                                   TLV320_DAC_ROUTE_MIXER,
                                   false, false, false, false)) {
    fail("Failed to configure DAC routing!");
  }

  Serial.println("Setting DAC volume...");
  if (!codec.setDACVolumeControl(false, false, TLV320_VOL_INDEPENDENT) ||
      !codec.setChannelVolume(false, 18) ||
      !codec.setChannelVolume(true, 18)) {
    fail("Failed to configure DAC volume!");
  }

  Serial.println("Configuring headphone output...");
  if (!codec.configureHeadphoneDriver(true, true,
                                      TLV320_HP_COMMON_1_35V,
                                      false) ||
      !codec.configureHPL_PGA(0, true) ||
      !codec.configureHPR_PGA(0, true) ||
      !codec.setHPLVolume(true, 6) ||
      !codec.setHPRVolume(true, 6)) {
    fail("Failed to configure headphone outputs!");
  }

  Serial.println("Codec configured");
}

void setupI2S() {
  Serial.println("Initializing I2S...");

  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate = SAMPLE_RATE;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = 0;
  i2s_config.dma_buf_count = 8;
  i2s_config.dma_buf_len = 256;
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;
  i2s_config.fixed_mclk = 0;

  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = I2S_BCLK;
  pin_config.ws_io_num = I2S_WS;
  pin_config.data_out_num = I2S_DOUT;
  pin_config.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
    fail("Failed to install I2S driver!");
  }

  if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK) {
    fail("Failed to set I2S pins!");
  }

  if (i2s_set_clk(I2S_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO) != ESP_OK) {
    fail("Failed to set I2S clock!");
  }

  Serial.println("I2S ready");
}

void playTone(float freq, uint32_t durationMs) {
  const int samplesPerChunk = 256;
  int16_t buffer[samplesPerChunk * 2];

  float phase = 0.0f;
  float phaseStep = 2.0f * PI * freq / SAMPLE_RATE;

  uint32_t totalSamples = (SAMPLE_RATE * durationMs) / 1000;
  uint32_t sent = 0;

  while (sent < totalSamples) {
    int count = samplesPerChunk;
    if (totalSamples - sent < (uint32_t)samplesPerChunk) {
      count = totalSamples - sent;
    }

    for (int i = 0; i < count; i++) {
      int16_t sample = (int16_t)(sin(phase) * 5000);
      buffer[2 * i] = sample;
      buffer[2 * i + 1] = sample;

      phase += phaseStep;
      if (phase >= 2.0f * PI) {
        phase -= 2.0f * PI;
      }
    }

    size_t bytesWritten = 0;
    if (i2s_write(I2S_PORT, buffer, count * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY) != ESP_OK) {
      Serial.println("I2S write failed");
      return;
    }

    sent += count;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("TLV320DAC3100 Headphone Test");

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(50);

  resetCodec();
  setupCodec();
  setupI2S();

  Serial.println("Setup complete");
}

void loop() {
  Serial.println("Playing 440 Hz...");
  playTone(440.0f, 1000);
  delay(500);

  Serial.println("Playing 880 Hz...");
  playTone(880.0f, 1000);
  delay(1000);
}