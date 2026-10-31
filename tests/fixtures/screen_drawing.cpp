// Exercise the actual native renderer's activation/reset/cache boundary.
#include <screen_drawing.hpp>
#include <cstdio>
#include <cstdlib>
#define require(value) do { if (!(value)) { std::fprintf(stderr, "line %d\n", __LINE__); std::abort(); } } while (0)

static opxy::u32 token = 1;
opxy::u32 sdk_screen_activation(opxy::u32 page, opxy::u32) { return page == 84 ? token : 0; }
void sdk_display_note_activity(opxy::u32, opxy::draw::Input*) {}
static opxy::u8 pitches[128], releases[128];
void sdk_note_pitches(opxy::u32, opxy::u8* held, opxy::u8* released) {
    for (unsigned i = 0; i < 128; ++i) { held[i] = pitches[i]; released[i] = releases[i]; }
}
struct Machine { static const char* const ids[2]; };
const char* const Machine::ids[2] = {"node", "palette"};

int main() {
    using namespace opxy;
    using Renderer = screen::Drawing<Machine>;
    Renderer::Object object = {};
    object.base.page = 84;
    screen::Controls controls = {}; controls.valid = true;
    object.keys[0] = 77;
    Renderer::animate(&object, controls);
    require(binding::resets == 1 && binding::renders == 1 && object.keys[0] == 0);
    require(screen::display_state.ticks == 1);
    // Same page/track must preserve state and the applied geometry key.
    object.keys[0] = 77;
    Renderer::animate(&object, controls);
    require(binding::resets == 1 && object.keys[0] == 77 && screen::display_state.ticks == 2);
    token = 2;
    Renderer::animate(&object, controls);
    require(binding::resets == 2 && object.keys[0] == 0 && screen::display_state.ticks == 1);
    // Invalid controls defer activation until a frame can render.
    object.keys[0] = 77; token = 3; controls.valid = false;
    Renderer::animate(&object, controls);
    require(binding::resets == 2 && binding::renders == 3 && object.keys[0] == 77);
    controls.valid = true;
    Renderer::animate(&object, controls);
    require(binding::resets == 3 && object.keys[0] == 0 && screen::display_state.ticks == 1);
    token = 0; object.keys[0] = 77;
    Renderer::animate(&object, controls);
    require(binding::resets == 3 && object.keys[0] == 77);
    controls.pad = 53;
    const auto baseline = screen::display_note_serial(controls);
    pitches[61] = 1; ++screen::fixture_note_starts;
    const auto other = screen::display_note_serial(controls);
    require(other == baseline+(OPXY_SCREEN_DRUM ? 0 : 1));
    pitches[60] = 1; ++screen::fixture_note_starts;
    require(screen::display_note_serial(controls) == other+1);
    pitches[60] = 0; ++releases[60];
    require(screen::display_note_serial(controls) == other+1);
    pitches[60] = 1; ++screen::fixture_note_starts;
    require(screen::display_note_serial(controls) == other+2);
    std::puts("native display activation resets state and geometry caches only on a new active token");
}
