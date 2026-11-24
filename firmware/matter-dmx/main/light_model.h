#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace studio {
enum class ColorMode : uint8_t { HueSaturation = 0, XY = 1, Temperature = 2 };
struct LightState {
    bool on = false;
    uint8_t level = 128; // Matter range 0..254
    uint8_t hue = 0;
    uint8_t saturation = 0;
    uint16_t x = 20495; // approximately D65
    uint16_t y = 21561;
    uint16_t mireds = 250;
    ColorMode mode = ColorMode::HueSaturation;
};
using RGB = std::array<uint8_t, 3>;
using Frame = std::array<uint8_t, 513>; // start code + 512 slots

inline float clamp(float value) { return std::clamp(value, 0.0f, 1.0f); }
inline uint8_t byte(float value) {
    return static_cast<uint8_t>(std::lround(clamp(value) * 255.0f));
}
inline std::array<float, 3> hue_rgb(uint8_t hue, uint8_t saturation) {
    const float h = std::min<int>(hue, 254) / 254.0f * 6.0f;
    const float s = std::min<int>(saturation, 254) / 254.0f;
    const float x = s * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
    const float m = 1.0f - s;
    std::array<float, 3> rgb{};
    switch (static_cast<int>(h) % 6) {
    case 0: rgb = {s, x, 0}; break;
    case 1: rgb = {x, s, 0}; break;
    case 2: rgb = {0, s, x}; break;
    case 3: rgb = {0, x, s}; break;
    case 4: rgb = {x, 0, s}; break;
    default: rgb = {s, 0, x}; break;
    }
    for (auto &channel : rgb) channel += m;
    return rgb;
}
inline std::array<float, 3> xy_rgb(uint16_t raw_x, uint16_t raw_y) {
    const float x = raw_x / 65536.0f;
    const float y = raw_y / 65536.0f;
    if (y <= 0 || x + y > 1.0f) return {0, 0, 0};
    const float X = x / y;
    const float Z = (1.0f - x - y) / y;
    // CIE XYZ to linear sRGB; fixture gamut/calibration is not yet measured.
    std::array<float, 3> rgb{
        3.2406f * X - 1.5372f - 0.4986f * Z,
        -0.9689f * X + 1.8758f + 0.0415f * Z,
        0.0557f * X - 0.2040f + 1.0570f * Z
    };
    for (auto &c : rgb) c = std::max(c, 0.0f);
    const float peak = std::max({rgb[0], rgb[1], rgb[2], 1.0f});
    for (auto &c : rgb) {
        c /= peak;
        c = c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
    }
    return rgb;
}
inline std::array<float, 3> temperature_rgb(uint16_t mireds) {
    // Approximate Planckian locus in CIE xy, 2000..6500 K.
    const double t = std::clamp(1000000.0 / std::max<int>(mireds, 1), 2000.0, 6500.0);
    const double x = t <= 4000
        ? -0.2661239e9 / (t*t*t) - 0.2343580e6 / (t*t) + 0.8776956e3 / t + 0.179910
        : -3.0258469e9 / (t*t*t) + 2.1070379e6 / (t*t) + 0.2226347e3 / t + 0.240390;
    const double y = t <= 2222
        ? -1.1063814*x*x*x - 1.34811020*x*x + 2.18555832*x - 0.20219683
        : t <= 4000
        ? -0.9549476*x*x*x - 1.37418593*x*x + 2.09137015*x - 0.16748867
        : 3.0817580*x*x*x - 5.87338670*x*x + 3.75112997*x - 0.37001483;
    return xy_rgb(static_cast<uint16_t>(x * 65536), static_cast<uint16_t>(y * 65536));
}
inline RGB render(const LightState &state) {
    if (!state.on || state.level == 0) return {0, 0, 0};
    std::array<float, 3> rgb;
    switch (state.mode) {
    case ColorMode::HueSaturation: rgb = hue_rgb(state.hue, state.saturation); break;
    case ColorMode::XY: rgb = xy_rgb(state.x, state.y); break;
    case ColorMode::Temperature: rgb = temperature_rgb(state.mireds); break;
    default: return {0, 0, 0};
    }
    const float level = std::min<int>(state.level, 254) / 254.0f;
    return {byte(rgb[0] * level), byte(rgb[1] * level), byte(rgb[2] * level)};
}
inline bool make_frame(const LightState &state, uint16_t address, Frame &frame) {
    frame.fill(0);
    if (address < 1 || address > 510) return false;
    const auto rgb = render(state);
    std::copy(rgb.begin(), rgb.end(), frame.begin() + address);
    return true;
}
} // namespace studio
