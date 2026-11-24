#include "dmx_output.h"
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <nvs_flash.h>
#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>
#include <setup_payload/OnboardingCodesUtil.h>

using namespace esp_matter;
using namespace chip::app::Clusters;
namespace {
constexpr const char *TAG = "studio_matter";
uint16_t light_endpoint = 0;
bool ready = false;

esp_matter_attr_val_t read(uint32_t cluster_id, uint32_t attribute_id) {
    auto *attribute = attribute::get(light_endpoint, cluster_id, attribute_id);
    esp_matter_attr_val_t value = esp_matter_invalid(nullptr);
    ESP_ERROR_CHECK(attribute ? attribute::get_val(attribute, &value) : ESP_ERR_NOT_FOUND);
    return value;
}
void publish_state() {
    studio::LightState state;
    state.on = read(OnOff::Id, OnOff::Attributes::OnOff::Id).val.b;
    const auto level = read(LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id).val.u8;
    state.level = level == 255 ? 0 : level; // nullable level -> blackout
    state.hue = read(ColorControl::Id, ColorControl::Attributes::CurrentHue::Id).val.u8;
    state.saturation = read(ColorControl::Id, ColorControl::Attributes::CurrentSaturation::Id).val.u8;
    state.x = read(ColorControl::Id, ColorControl::Attributes::CurrentX::Id).val.u16;
    state.y = read(ColorControl::Id, ColorControl::Attributes::CurrentY::Id).val.u16;
    state.mireds = read(ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id).val.u16;
    state.mode = static_cast<studio::ColorMode>(read(ColorControl::Id, ColorControl::Attributes::ColorMode::Id).val.u8);
    dmx_output_set_state(state);
}
esp_err_t changed(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                  uint32_t, esp_matter_attr_val_t *, void *) {
    // Read committed attributes on the Matter thread. A separate task owns DMX timing.
    if (ready && type == attribute::POST_UPDATE && endpoint_id == light_endpoint &&
        (cluster_id == OnOff::Id || cluster_id == LevelControl::Id || cluster_id == ColorControl::Id)) {
        publish_state();
    }
    return ESP_OK;
}
esp_err_t identify(identification::callback_type_t type, uint16_t endpoint_id,
                   uint8_t, uint8_t, void *) {
    if (endpoint_id == light_endpoint && type != identification::EFFECT)
        dmx_output_identify(type == identification::START);
    return ESP_OK;
}
void device_event(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t) {
    using namespace chip::DeviceLayer::DeviceEventType;
    if (event->Type == kCommissioningComplete) ESP_LOGI(TAG, "Matter commissioning complete");
    if (event->Type == kInterfaceIpAddressChanged) ESP_LOGI(TAG, "Network address changed");
    if (event->Type == kFabricRemoved && chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
        auto &manager = chip::Server::GetInstance().GetCommissioningWindowManager();
        if (!manager.IsCommissioningWindowOpen()) {
            const auto err = manager.OpenBasicCommissioningWindow(chip::System::Clock::Seconds16(300),
                chip::CommissioningWindowAdvertisement::kDnssdOnly);
            if (err != CHIP_NO_ERROR) ESP_LOGE(TAG, "Could not reopen commissioning");
        }
    }
}
}

extern "C" void app_main() {
    // Do not automatically erase NVS on errors: it contains Matter fabrics and Wi-Fi credentials.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(dmx_output_init());

    node::config_t node_config;
    auto *node = node::create(&node_config, changed, identify);
    ESP_ERROR_CHECK(node ? ESP_OK : ESP_FAIL);

    endpoint::extended_color_light::config_t config;
    config.on_off.on_off = false;
    config.on_off_lighting.start_up_on_off = static_cast<uint8_t>(0); // power restoration always starts off
    config.level_control.current_level = 128;
    config.level_control.on_level = nullptr; // preserve previous level when turning on
    config.level_control_lighting.start_up_current_level = nullptr;
    config.color_control.color_mode = 0;
    config.color_control.enhanced_color_mode = 0;
    config.color_control_xy.current_x = 20495;
    config.color_control_xy.current_y = 21561;
    config.color_control_color_temperature.color_temp_physical_min_mireds = 154;
    config.color_control_color_temperature.color_temp_physical_max_mireds = 500;
    config.color_control_color_temperature.color_temperature_mireds = 250;
    config.color_control_color_temperature.couple_color_temp_to_level_min_mireds = 154;
    config.color_control_color_temperature.startup_color_temperature_mireds = nullptr;

    auto *endpoint = endpoint::extended_color_light::create(node, &config, ENDPOINT_FLAG_NONE, nullptr);
    ESP_ERROR_CHECK(endpoint ? ESP_OK : ESP_FAIL);
    light_endpoint = endpoint::get_id(endpoint);
    auto *color = cluster::get(endpoint, ColorControl::Id);
    cluster::color_control::feature::hue_saturation::config_t hsv;
    ESP_ERROR_CHECK(cluster::color_control::feature::hue_saturation::add(color, &hsv));
    ESP_ERROR_CHECK(esp_matter::start(device_event));

    // Matter starts a separate task. All data-model reads are scheduled on its thread.
    chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {
        ready = true;
        publish_state();
        PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
    }, 0);
#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
#endif
    ESP_LOGI(TAG, "Standard Matter RGB lamp, endpoint %u. ADJ 64P LED PRO must use 3-channel DMX mode.",
             light_endpoint);
}
