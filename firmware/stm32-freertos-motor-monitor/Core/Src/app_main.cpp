#include "app_main.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

// İlk C++ sınıfımız: bir LED'i kendi task'ında yakıp söndürür
class Blinker {
public:
  explicit Blinker(uint16_t pin) : pin_(pin) {}

  void start(const char* name, UBaseType_t priority) {
    // FreeRTOS C ile yazıldığı için task fonksiyonu sınıf metodu olamaz.
    // Köprü: statik bir fonksiyon veriyoruz, nesnenin adresini (this) parametre olarak geçiyoruz.
    xTaskCreate(&Blinker::taskEntry, name, 256, this, priority, nullptr);
  }

private:
  static void taskEntry(void* arg) {
    static_cast<Blinker*>(arg)->run();   // adresi tekrar nesneye çevir ve metodu çağır
  }

  void run() {
    for (;;) {
      HAL_GPIO_TogglePin(GPIOD, pin_);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  uint16_t pin_;
};

// Statik nesne: new yok, program başlarken bir kez oluşturulur
Blinker blueBlinker(GPIO_PIN_15);

}  // namespace

extern "C" void app_main(void)
{
  blueBlinker.start("Blink", 1);
}
