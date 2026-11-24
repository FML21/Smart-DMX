#include "dmx_output.h"
#include "sdkconfig.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <atomic>

namespace {
QueueHandle_t state_queue = nullptr;
std::atomic<bool> identifying{false};
constexpr const char *TAG = "dmx";
#if CONFIG_STUDIO_DMX_OUTPUT
constexpr uart_port_t kUart = UART_NUM_2;
static_assert(CONFIG_STUDIO_DMX_TX_GPIO != CONFIG_STUDIO_DMX_ENABLE_GPIO, "TX and enable must differ");
#endif

void output_task(void *) {
    studio::LightState state;
    studio::Frame frame{};
    studio::RGB previous{0, 0, 0};
    bool first = true;
    auto wake = xTaskGetTickCount();
    while (true) {
        studio::LightState next;
        if (xQueueReceive(state_queue, &next, 0) == pdTRUE) state = next;
        auto displayed = state;
        if (identifying.load()) {
            // A gentle 1 Hz identification pulse, independent of stored lamp state.
            displayed.on = ((xTaskGetTickCount() / pdMS_TO_TICKS(500)) % 2) == 0;
            displayed.level = 64;
            displayed.saturation = 0;
            displayed.mode = studio::ColorMode::HueSaturation;
        }
        studio::make_frame(displayed, CONFIG_STUDIO_DMX_START_ADDRESS, frame);
        const auto rgb = studio::render(displayed);
        if (first || rgb != previous) {
            ESP_LOGI(TAG, "RGB %u %u %u (%s)", rgb[0], rgb[1], rgb[2],
#if CONFIG_STUDIO_DMX_OUTPUT
                     "UART"
#else
                     "dry-run"
#endif
            );
            previous = rgb;
            first = false;
        }
#if CONFIG_STUDIO_DMX_OUTPUT
        // TX is idle and no bytes are queued. Invert idle high to generate BREAK.
        ESP_ERROR_CHECK(uart_set_line_inverse(kUart, UART_SIGNAL_TXD_INV));
        esp_rom_delay_us(120);
        ESP_ERROR_CHECK(uart_set_line_inverse(kUart, 0));
        esp_rom_delay_us(16); // mark-after-break
        const int written = uart_write_bytes(kUart, frame.data(), frame.size());
        if (written != static_cast<int>(frame.size())) {
            ESP_LOGE(TAG, "Incomplete DMX write; disabling transmitter");
            gpio_set_level(static_cast<gpio_num_t>(CONFIG_STUDIO_DMX_ENABLE_GPIO), 0);
            vTaskDelete(nullptr);
        }
        ESP_ERROR_CHECK(uart_wait_tx_done(kUart, pdMS_TO_TICKS(100)));
#endif
        // 513 bytes at 250000 baud / 8N2 takes 22.572 ms; target period 30 ms.
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(30));
    }
}
}
esp_err_t dmx_output_init() {
    state_queue = xQueueCreate(1, sizeof(studio::LightState));
    if (!state_queue) return ESP_ERR_NO_MEM;
#if CONFIG_STUDIO_DMX_OUTPUT
    gpio_config_t enable{};
    enable.pin_bit_mask = 1ULL << CONFIG_STUDIO_DMX_ENABLE_GPIO;
    enable.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&enable));
    ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(CONFIG_STUDIO_DMX_ENABLE_GPIO), 0));
    uart_config_t uart{};
    uart.baud_rate = 250000;
    uart.data_bits = UART_DATA_8_BITS;
    uart.parity = UART_PARITY_DISABLE;
    uart.stop_bits = UART_STOP_BITS_2;
    uart.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart.source_clk = UART_SCLK_APB;
    ESP_ERROR_CHECK(uart_param_config(kUart, &uart));
    ESP_ERROR_CHECK(uart_set_pin(kUart, CONFIG_STUDIO_DMX_TX_GPIO, UART_PIN_NO_CHANGE,
                                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(kUart, 1024, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(CONFIG_STUDIO_DMX_ENABLE_GPIO), 1));
#else
    ESP_LOGW(TAG, "Dry-run: no UART or GPIO pins driven. Enable physical output only after wiring verification.");
#endif
    return xTaskCreate(output_task, "dmx_output", 4096, nullptr, 5, nullptr) == pdPASS
        ? ESP_OK : ESP_ERR_NO_MEM;
}
void dmx_output_set_state(const studio::LightState &state) {
    configASSERT(state_queue);
    xQueueOverwrite(state_queue, &state);
}
void dmx_output_identify(bool enabled) { identifying.store(enabled); }
