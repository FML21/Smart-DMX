#include "../main/light_model.h"
#include <cassert>
#include <iostream>
int main() {
    studio::LightState state;
    state.level = 254;
    state.saturation = 254;
    assert((studio::render(state) == studio::RGB{0, 0, 0}));
    state.on = true;
    assert((studio::render(state) == studio::RGB{255, 0, 0}));
    state.hue = 85;
    auto rgb = studio::render(state);
    assert(rgb[1] == 255 && rgb[0] < 3 && rgb[2] < 3);
    state.hue = 169;
    rgb = studio::render(state);
    assert(rgb[2] == 255 && rgb[0] < 3 && rgb[1] < 3);
    state.hue = 254;
    assert((studio::render(state) == studio::RGB{255, 0, 0}));
    state.saturation = 0;
    state.level = 127;
    assert((studio::render(state) == studio::RGB{128, 128, 128}));
    state.level = 0;
    assert((studio::render(state) == studio::RGB{0, 0, 0}));
    state.level = 254;
    state.mode = studio::ColorMode::XY;
    state.x = 41943; state.y = 21627; // sRGB red
    rgb = studio::render(state);
    assert(rgb[0] > 250 && rgb[1] < 5 && rgb[2] < 5);
    state.y = 0;
    assert((studio::render(state) == studio::RGB{0, 0, 0}));
    state.x = 60000; state.y = 60000;
    assert((studio::render(state) == studio::RGB{0, 0, 0}));
    state.mode = studio::ColorMode::Temperature;
    state.mireds = 500;
    rgb = studio::render(state);
    assert(rgb[0] > rgb[1] && rgb[1] > rgb[2]);
    state.mireds = 154;
    rgb = studio::render(state);
    assert(rgb[0] > 230 && rgb[1] > 230 && rgb[2] > 230);
    state.mode = studio::ColorMode::HueSaturation;
    studio::Frame frame;
    assert(studio::make_frame(state, 510, frame));
    assert(frame[0] == 0 && frame[509] == 0 && frame[510] == 255 && frame[512] == 255);
    assert(!studio::make_frame(state, 511, frame));
    for (auto value : frame) assert(value == 0);
    assert(!studio::make_frame(state, 0, frame));
    state.on = false;
    assert(studio::make_frame(state, 1, frame));
    for (auto value : frame) assert(value == 0);
    // Exhaustive hue/level sweep: dimming must remain monotonic, full saturation bounded.
    state.on = true; state.saturation = 254;
    for (int h = 0; h <= 254; ++h) {
        state.hue = h;
        studio::RGB previous{};
        for (int level = 0; level <= 254; ++level) {
            state.level = level;
            const auto current = studio::render(state);
            for (int c = 0; c < 3; ++c) assert(current[c] >= previous[c]);
            previous = current;
        }
    }
    std::cout << "Matter RGB/XY/temperature mapping and DMX bounds passed\n";
}
