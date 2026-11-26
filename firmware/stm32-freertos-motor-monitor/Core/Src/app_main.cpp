#include "app_main.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

// CubeMX'in main.c'de oluşturduğu SPI değişkeni. C dosyasında tanımlı olduğu için extern "C"
extern "C" {
extern SPI_HandleTypeDef hspi1;
}

namespace {

// ---------------------------------------------------------------- Blinker
class Blinker {
public:
  explicit Blinker(uint16_t pin) : pin_(pin) {}

  void start(const char* name, UBaseType_t priority) {
    xTaskCreate(&Blinker::taskEntry, name, 256, this, priority, nullptr);
  }

private:
  static void taskEntry(void* arg) { static_cast<Blinker*>(arg)->run(); }

  void run() {
    for (;;) {
      HAL_GPIO_TogglePin(GPIOD, pin_);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  uint16_t pin_;
};

// ------------------------------------------------------------ TiltMonitor
// LIS3DSH ivmeölçeri 10 ms'de bir okur, devrilmeyi debounce ile onaylar,
// arızayı kilitler ve LED'leri sürer.
class TiltMonitor {
public:
  explicit TiltMonitor(SPI_HandleTypeDef& spi) : spi_(spi) {}

  void start(UBaseType_t priority) {
    xTaskCreate(&TiltMonitor::taskEntry, "Tilt", 256, this, priority, &handle_);
  }

  // Başka bir task'tan çağrılır: "kart düzse arızayı temizle"
  void requestClear() {
    if (handle_ != nullptr) {
      xTaskNotifyGive(handle_);
    }
  }

private:
  // Sihirli sayılar yerine isimli sabitler
  static constexpr int16_t  kTiltLimit      = 11800;  // ~45 derece (1 g ~ 16700)
  static constexpr uint32_t kConfirmSamples = 50;     // 50 x 10 ms = 500 ms
  static constexpr uint8_t  kRegWhoAmI      = 0x0F;
  static constexpr uint8_t  kExpectedId     = 0x3F;   // LIS3DSH kimliği
  static constexpr uint8_t  kRegCtrl4       = 0x20;
  static constexpr uint8_t  kCtrl4Value     = 0x67;   // 100 Hz, X/Y/Z açık
  static constexpr uint8_t  kRegOutX        = 0x28;
  static constexpr uint8_t  kRegOutY        = 0x2A;

  static void taskEntry(void* arg) { static_cast<TiltMonitor*>(arg)->run(); }

  void run() {
    __HAL_SPI_ENABLE(&spi_);          // ilk aktarımdan önce SPI'ı aç (Ekim'deki 0xFF hatası)
    vTaskDelay(pdMS_TO_TICKS(10));

    if (readReg(kRegWhoAmI) != kExpectedId) {
      setLeds(false, true);           // sensör cevap vermiyor: kırmızı yak ve dur
      for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    writeReg(kRegCtrl4, kCtrl4Value);

    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
      const bool tilted = isTilted(readAxis(kRegOutX), readAxis(kRegOutY));

      // Debounce: kesintisiz eğimi say, bir kez bile düzse sıfırla
      if (tilted) {
        if (tiltCount_ < kConfirmSamples) { tiltCount_++; }
      } else {
        tiltCount_ = 0;
      }

      if (tiltCount_ >= kConfirmSamples) { latched_ = true; }

      // Temizleme isteği geldiyse ve kart düzse kilidi aç
      if (ulTaskNotifyTake(pdTRUE, 0) > 0 && !tilted) { latched_ = false; }

      setLeds(!latched_, latched_);
      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
    }
  }

  static bool isTilted(int16_t x, int16_t y) {
    return x > kTiltLimit || x < -kTiltLimit || y > kTiltLimit || y < -kTiltLimit;
  }

  uint8_t readReg(uint8_t reg) {
    uint8_t tx = reg | 0x80;          // en üst bit 1 = okuma
    uint8_t rx = 0;
    csLow();
    HAL_SPI_Transmit(&spi_, &tx, 1, 10);
    HAL_SPI_Receive(&spi_, &rx, 1, 10);
    csHigh();
    return rx;
  }

  void writeReg(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { static_cast<uint8_t>(reg & 0x7F), val };   // en üst bit 0 = yazma
    csLow();
    HAL_SPI_Transmit(&spi_, tx, 2, 10);
    csHigh();
  }

  int16_t readAxis(uint8_t regLow) {
    const uint8_t lo = readReg(regLow);
    const uint8_t hi = readReg(static_cast<uint8_t>(regLow + 1));
    return static_cast<int16_t>((hi << 8) | lo);
  }

  static void csLow()  { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET); }
  static void csHigh() { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET); }

  static void setLeds(bool green, bool red) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, green ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, red ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  SPI_HandleTypeDef& spi_;
  TaskHandle_t handle_ = nullptr;
  uint32_t tiltCount_ = 0;
  bool latched_ = false;
};

// Statik nesneler: new yok, program başlarken bir kez oluşturulur
Blinker     blueBlinker(GPIO_PIN_15);
TiltMonitor tiltMonitor(hspi1);

}  // namespace

extern "C" void app_main(void)
{
  tiltMonitor.start(4);
  blueBlinker.start("Blink", 1);
}

extern "C" void app_request_clear(void)
{
  tiltMonitor.requestClear();
}
