#include "Menus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
constexpr Color ink{0.075f, 0.20f, 0.22f};
constexpr Color paper{1.0f, 0.95f, 0.82f};
constexpr Color yellow{1.0f, 0.82f, 0.22f};
constexpr Color coral{1.0f, 0.40f, 0.27f};
constexpr Color muted{0.36f, 0.43f, 0.39f};
constexpr Color sky{0.98f, 0.57f, 0.39f};

// An original, small block alphabet. Each glyph is uploaded once as triangles;
// text uses the same fixed-function, VBO-backed path as the rest of the UI.
constexpr char glyphCharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,:/-+<>%";
constexpr unsigned char glyphRows[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, // A B
    {15,16,16,16,16,16,15}, {30,17,17,17,17,17,30}, // C D
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16}, // E F
    {15,16,16,23,17,17,15}, {17,17,17,31,17,17,17}, // G H
    {31,4,4,4,4,4,31},     {7,2,2,2,18,18,12},    // I J
    {17,18,20,24,20,18,17},{16,16,16,16,16,16,31}, // K L
    {17,27,21,21,17,17,17},{17,25,25,21,19,19,17}, // M N
    {14,17,17,17,17,17,14},{30,17,17,30,16,16,16}, // O P
    {14,17,17,17,21,18,13},{30,17,17,30,20,18,17}, // Q R
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},       // S T
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},  // U V
    {17,17,17,21,21,27,17},{17,17,10,4,10,17,17},  // W X
    {17,17,10,4,4,4,4},   {31,1,2,4,8,16,31},     // Y Z
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},     // 0 1
    {14,17,1,2,4,8,31},   {30,1,1,14,1,1,30},     // 2 3
    {2,6,10,18,31,2,2},   {31,16,16,30,1,1,30},   // 4 5
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},      // 6 7
    {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},  // 8 9
    {0,0,0,0,0,12,12},    {0,0,0,0,0,4,8},       // . ,
    {0,12,12,0,12,12,0},  {1,2,2,4,8,8,16},      // : /
    {0,0,0,31,0,0,0},     {0,4,4,31,4,4,0},      // - +
    {2,4,8,16,8,4,2},     {8,4,2,1,2,4,8},       // < >
    {25,25,2,4,8,19,19}                           // %
};
static_assert(sizeof(glyphRows) / sizeof(glyphRows[0]) == 45);
static_assert(sizeof(glyphCharacters) - 1 == 45);

struct Rect {
    float x, y, width, height;
};

// A uniform virtual canvas keeps both text and hit areas in proportion at
// different resolutions. Extra space on tall or wide displays stays usable.
struct Layout {
    float scale, width, height;

    Layout(int w, int h)
        : scale(std::min(std::max(w, 1) / 1600.0f,
                         std::max(h, 1) / 900.0f)),
          width(w / scale), height(h / scale) {}
};

bool inside(float x, float y, Rect r) {
    return x >= r.x && x <= r.x + r.width &&
           y >= r.y && y <= r.y + r.height;
}

Rect action(int index, const Layout& layout, AppState state) {
    switch (index) {
    case 0: // Main menu Start, followed by customization.
    case 3: // Customization Play.
        return {layout.width - 434, layout.height - 142, 370, 82};
    case 1: return {64, layout.height - 142, 310, 82}; // Sound.
    case 2: // Exit on the main menu, Back on submenus.
        return state == AppState::MainMenu
            ? Rect{layout.width - 180, 42, 116, 44}
            : Rect{64, layout.height - 142, 240, 82};
    default: return {64, 642, 230, 46}; // Mute.
    }
}

Rect colorOption(int index) { return {64.0f + index * 112.0f, 385, 90, 66}; }
Rect sizeOption(int index) { return {64.0f + index * 152.0f, 551, 138, 68}; }
Rect volumeSlider(int index) { return {64, 352.0f + index * 110.0f, 490, 34}; }

class Painter {
public:
    Painter(Renderer& renderer, const std::array<std::size_t, 45>& glyphs)
        : renderer_(renderer), glyphs_(glyphs) {}

    void rectangle(Rect r, Color color) const {
        renderer_.draw(MeshId::Quad,
            Matrix4::translation(r.x + r.width / 2, r.y + r.height / 2, 0) *
                Matrix4::scale(r.width, r.height, 1), color);
    }

    void circle(float x, float y, float diameter, Color color) const {
        renderer_.draw(MeshId::Disc,
            Matrix4::translation(x, y, 0) *
                Matrix4::scale(diameter, diameter, 1), color);
    }

    void bar(float x, float y, float width, float height, float angle,
             Color color) const {
        renderer_.draw(MeshId::Quad,
            Matrix4::translation(x, y, 0) *
                Matrix4::rotation(angle, {0, 0, 1}) *
                Matrix4::scale(width, height, 1), color);
    }

    void text(float x, float y, const char* value, float cell, Color color) const {
        for (const char* c = value; *c; ++c, x += cell * 6) {
            if (*c == ' ') continue;
            const char* glyph = std::strchr(glyphCharacters, *c);
            if (!glyph) continue;
            renderer_.drawStaticMesh(glyphs_[glyph - glyphCharacters],
                Matrix4::translation(x, y, 0) * Matrix4::scale(cell, cell, 1),
                color);
        }
    }

    void centered(Rect r, const char* value, float cell, Color color) const {
        const float width = (std::strlen(value) * 6.0f - 1) * cell;
        text(r.x + (r.width - width) / 2, r.y + (r.height - 7 * cell) / 2,
             value, cell, color);
    }

    void rotor(float x, float y) const {
        bar(x, y, 37, 5, -15, ink);
        bar(x, y, 37, 5, 75, ink);
        circle(x, y, 12, yellow);
        circle(x, y, 5, ink);
    }

    void primary(Rect r, const char* title, const char* caption, float hover) const {
        rectangle({r.x + 6, r.y + 6, r.width, r.height}, ink);
        r.y -= hover * 4;
        rectangle(r, hover > 0.5f ? paper : yellow);
        text(r.x + 23, r.y + 18, title, 3.1f, ink);
        text(r.x + 23, r.y + 54, caption, 1.5f, ink);
        text(r.x + r.width - 47 + hover * 5, r.y + 24, ">", 4.5f, ink);
    }

    void back(Rect r, float hover) const {
        rectangle({r.x, r.y + r.height - 6, r.width, 3}, ink);
        text(r.x + hover * 5, r.y + 25, "< BACK TO MENU", 2.3f, ink);
    }

private:
    Renderer& renderer_;
    const std::array<std::size_t, 45>& glyphs_;
};

void beginCanvas(Renderer& renderer, const Layout& layout) {
    renderer.beginInterface(static_cast<int>(std::lround(layout.width)),
                            static_cast<int>(std::lround(layout.height)));
}

void drawSky(const Painter& p, const Layout& l) {
    p.rectangle({0, 0, l.width, l.height}, sky);
    p.circle(l.width * 0.76f, 210, 346, yellow);
    // Flat bands keep the sunset graphic, without gradients or lighting.
    p.rectangle({0, 480, l.width, l.height - 480}, {1.0f, 0.67f, 0.44f});
}

void drawWorld(Renderer& renderer, const City& city,
               const Helicopter& helicopter, const CustomizationSettings& settings,
               int width, int height, float elapsed) {
    const float roof = city.spawnSurfaceHeightAt({0, 0, 0});
    const float angle = 0.78f + 0.055f * std::sin(elapsed * 0.16f);
    const Vec3 eye{13.0f * std::sin(angle), roof + 6.7f,
                   13.0f * std::cos(angle)};
    const Matrix4 view = Matrix4::lookAt(eye, {0, roof + 1.8f, 0}, {0, 1, 0});
    // Shift the composition, leaving room for the menu at the left. This
    // presentation camera is independent of the player's orbit and zoom.
    const Matrix4 projection = Matrix4::translation(0.38f, 0.02f, 0) *
        Matrix4::perspective(60, static_cast<float>(width) / height, 0.1f, 1000);
    renderer.begin3D(view, projection);
    city.render(renderer);

    const float scale = settings.scale();
    const Matrix4 model = Matrix4::translation(0, roof + 0.95f * scale, 0) *
                          Matrix4::scale(scale, scale, scale);
    helicopter.draw(renderer, model, settings.bodyColor());
}

void drawHeader(const Painter& p, const Layout& l, AppState state, float hover) {
    p.rotor(83, 63);
    const char* section = state == AppState::MainMenu ? "CITY HELICOPTER SIMULATOR"
        : state == AppState::Customization ? "FLIGHT DECK / CUSTOMIZATION"
        : "FLIGHT DECK / SOUND SETTINGS";
    p.text(119, 56, section, 1.8f, ink);
    p.rectangle({64, 103, l.width - 128, 2}, ink);
    if (state == AppState::MainMenu) {
        const Rect exit = action(2, l, state);
        if (hover > 0.01f) p.rectangle(exit, paper);
        p.centered(exit, "EXIT", 2.1f, ink);
    } else {
        p.text(l.width - 256, 56, state == AppState::Customization
            ? "01 / PREFLIGHT" : "AUDIO CONTROL", 1.8f, ink);
    }
}

void drawFooter(const Painter& p, const Layout& l) {
    p.rectangle({0, l.height - 176, l.width, 176}, paper);
    p.rectangle({0, l.height - 176, l.width, 3}, ink);
}

void drawMainMenu(const Painter& p, const Layout& l, const std::array<float, 5>& hover) {
    // Offset solid lettering is deliberately closer to a screen-printed poster
    // than a conventional panel of game buttons.
    p.text(70, 178, "CITY", 18, paper);
    p.text(64, 172, "CITY", 18, ink);
    p.text(69, 330, "HELICOPTER", 8.5f, paper);
    p.text(64, 325, "HELICOPTER", 8.5f, ink);
    p.rectangle({64, 407, 232, 33}, yellow);
    p.text(77, 416, "SIMULATOR", 2.2f, ink);

    drawFooter(p, l);

    const Rect sound = action(1, l, AppState::MainMenu);
    if (hover[1] > 0.01f)
        p.rectangle({sound.x - 10, sound.y + 4, sound.width, 64}, yellow);
    p.text(sound.x + hover[1] * 5, sound.y + 15, "SOUND SETTINGS", 2.6f, ink);
    p.text(sound.x + hover[1] * 5, sound.y + 48, "FIND YOUR FREQUENCY", 1.5f, muted);
    p.primary(action(0, l, AppState::MainMenu), "GAME START",
              "CUSTOMIZE + TAKE OFF", hover[0]);
}

void drawCustomization(const Painter& p, const Layout& l,
                       const CustomizationSettings& settings,
                       float mouseX, float mouseY,
                       const std::array<float, 5>& hover) {
    p.rectangle({32, 136, 550, l.height - 338}, paper);
    p.text(64, 210, "CUSTOMIZE", 8.0f, ink);
    p.text(64, 348, "01 / BODY COLOR", 2.1f, ink);
    constexpr const char* colorNames[] = {"RED", "BLUE", "GREEN", "WHITE"};
    for (int i = 0; i < 4; ++i) {
        const Rect r = colorOption(i);
        CustomizationSettings option = settings;
        option.color = static_cast<HelicopterColor>(i);
        const bool selected = static_cast<int>(settings.color) == i;
        p.rectangle({r.x - 4, r.y - 4, r.width + 8, r.height + 8},
                    selected ? ink : inside(mouseX, mouseY, r) ? yellow : paper);
        p.rectangle(r, option.bodyColor());
        p.centered({r.x, r.y + r.height + 14, r.width, 15}, colorNames[i], 1.6f, ink);
    }

    p.text(64, 513, "02 / AIRCRAFT SIZE", 2.1f, ink);
    constexpr const char* sizeNames[] = {"SMALL", "MEDIUM", "LARGE"};
    constexpr const char* accelerationNames[] = {"FAST", "NORMAL", "SLOW"};
    for (int i = 0; i < 3; ++i) {
        const Rect r = sizeOption(i);
        const bool selected = static_cast<int>(settings.size) == i;
        p.rectangle(r, selected ? ink : inside(mouseX, mouseY, r) ? yellow
                                                                   : Color{0.92f, 0.87f, 0.73f});
        p.centered({r.x, r.y + 12, r.width, 17}, sizeNames[i], 1.9f,
                   selected ? paper : ink);
        p.centered({r.x, r.y + 40, r.width, 14}, accelerationNames[i], 1.5f,
                   selected ? yellow : muted);
    }
    p.text(64, 651, "SCROLL TO CHANGE SIZE", 1.7f, muted);
    p.rectangle({l.width - 415, l.height - 270, 351, 58}, ink);
    p.text(l.width - 397, l.height - 257, "YOUR HELICOPTER", 2.0f, paper);
    p.text(l.width - 397, l.height - 234, "LIVE PAINT + SIZE PREVIEW", 1.3f, yellow);

    drawFooter(p, l);
    p.back(action(2, l, AppState::Customization), hover[2]);
    p.primary(action(3, l, AppState::Customization), "PLAY", "TAKE TO THE SKY", hover[3]);
}

void drawSound(const Painter& p, const Layout& l, const Audio& audio,
                float mouseX, float mouseY, int activeSlider,
                const std::array<float, 5>& hover) {
    p.rectangle({32, 136, 574, l.height - 338}, paper);
    p.text(64, 177, "SOUND", 10.2f, ink);
    p.text(64, 274, "SET YOUR FREQUENCY.", 2.0f, muted);
    const float values[] = {audio.getMasterVolume(), audio.getUIVolume(),
                             audio.getHelicopterVolume()};
    constexpr const char* labels[] = {"MASTER VOLUME", "MENU EFFECTS", "HELICOPTER"};
    for (int i = 0; i < 3; ++i) {
        const Rect r = volumeSlider(i);
        const bool highlighted = activeSlider == i || inside(mouseX, mouseY, r);
        p.text(r.x, r.y - 30, labels[i], 2.0f, ink);
        char percentage[8];
        std::snprintf(percentage, sizeof(percentage), "%d%%",
                       static_cast<int>(values[i] * 100 + 0.5f));
        p.text(r.x + r.width - 56, r.y - 30, percentage, 2.0f, ink);
        p.rectangle({r.x, r.y + 10, r.width, 12}, {0.85f, 0.81f, 0.69f});
        if (values[i] > 0)
            p.rectangle({r.x, r.y + 10, r.width * values[i], 12}, ink);
        p.rectangle({r.x + values[i] * r.width - 8, r.y + 1, 17, 31}, ink);
        p.rectangle({r.x + values[i] * r.width - 5, r.y + 4, 11, 25},
                    highlighted ? coral : yellow);
    }
    const Rect mute = action(4, l, AppState::SoundSettings);
    p.rectangle(mute, audio.isMuted() ? coral : hover[4] > 0.1f ? yellow : ink);
    p.centered(mute, audio.isMuted() ? "UNMUTE AUDIO" : "MUTE AUDIO", 2.0f,
               audio.isMuted() || hover[4] > 0.1f ? ink : paper);
    p.text(320, 659, audio.isMuted() ? "AUDIO OFF" : "AUDIO ON", 1.5f, muted);

    drawFooter(p, l);
    p.back(action(2, l, AppState::SoundSettings), hover[2]);
    p.text(432, l.height - 125, "MAKE YOURSELF AT HOME.", 2.0f, ink);
    p.text(432, l.height - 94, "VOLUME CHANGES APPLY INSTANTLY.", 1.5f, muted);
}

void setVolume(Audio& audio, int index, float value) {
    if (index == 0) audio.setMasterVolume(value);
    else if (index == 1) audio.setUIVolume(value);
    else audio.setHelicopterVolume(value);
}
} // namespace

bool Menus::initialize(Renderer& renderer) {
    for (std::size_t glyph = 0; glyph < glyphMeshes_.size(); ++glyph) {
        std::vector<float> vertices;
        for (int y = 0; y < 7; ++y) {
            // Merge adjacent cells into a single quad for each horizontal run.
            for (int x = 0; x < 5;) {
                if (!(glyphRows[glyph][y] & (1 << (4 - x)))) {
                    ++x;
                    continue;
                }
                const float left = static_cast<float>(x);
                while (x < 5 && (glyphRows[glyph][y] & (1 << (4 - x)))) ++x;
                const float right = static_cast<float>(x);
                const float top = static_cast<float>(y);
                const float bottom = top + 1;
                vertices.insert(vertices.end(), {
                    left,top,0, right,top,0, right,bottom,0,
                    left,top,0, right,bottom,0, left,bottom,0});
            }
        }
        if (!renderer.uploadStaticMesh(vertices, GL_TRIANGLES, glyphMeshes_[glyph]))
            return false;
    }
    return true;
}

void Menus::render(Renderer& renderer, const City& city, AppState state,
                    int width, int height, const CustomizationSettings& settings,
                    const Audio& audio) const {
    if (state == AppState::Playing) return;
    const Layout layout(width, height);
    const Painter painter(renderer, glyphMeshes_);

    beginCanvas(renderer, layout);
    drawSky(painter, layout);
    renderer.endInterface();

    drawWorld(renderer, city, displayHelicopter_, settings, width, height, elapsed_);

    beginCanvas(renderer, layout);
    drawHeader(painter, layout, state, hover_[2]);
    if (state == AppState::MainMenu) drawMainMenu(painter, layout, hover_);
    else if (state == AppState::Customization)
        drawCustomization(painter, layout, settings, mouseX_, mouseY_, hover_);
    else drawSound(painter, layout, audio, mouseX_, mouseY_, activeSlider_, hover_);
    renderer.endInterface();
}

void Menus::update(Input& input, AppState& state, int width, int height,
                    bool& quitRequested, CustomizationSettings& settings,
                    Audio& audio, float deltaTime) {
    const Layout layout(width, height);
    elapsed_ += deltaTime;
    int x, y;
    input.getMousePosition(x, y);
    mouseX_ = x / layout.scale;
    mouseY_ = y / layout.scale;
    const float blend = 1 - std::exp(-14 * deltaTime);
    for (std::size_t i = 0; i < hover_.size(); ++i) {
        const float target = inside(mouseX_, mouseY_, action(static_cast<int>(i), layout, state))
            ? 1.0f : 0.0f;
        hover_[i] += (target - hover_[i]) * blend;
    }

    // Consume menu gestures here so they cannot become camera movement or zoom
    // when the player subsequently presses Play.
    int dragX, dragY;
    input.consumeMouseDelta(dragX, dragY);
    const int wheel = input.consumeWheelDelta();
    if (state == AppState::Customization && wheel != 0) {
        settings.size = static_cast<HelicopterSize>(
            std::clamp(static_cast<int>(settings.size) + wheel, 0, 2));
    }

    const bool clicked = input.isLeftMouseClicked();
    if (clicked) input.consumeLeftMouseClick();
    if (!input.isLeftMouseDown() || state != AppState::SoundSettings)
        activeSlider_ = -1;

    if (state == AppState::SoundSettings) {
        if (clicked) {
            activeSlider_ = -1;
            for (int i = 0; i < 3; ++i) {
                const Rect r = volumeSlider(i);
                if (inside(mouseX_, mouseY_, {r.x - 12, r.y - 12, r.width + 24, r.height + 24})) {
                    activeSlider_ = i;
                    break;
                }
            }
        }
        if (activeSlider_ >= 0 && (clicked || input.isLeftMouseDown())) {
            const Rect r = volumeSlider(activeSlider_);
            setVolume(audio, activeSlider_, std::clamp((mouseX_ - r.x) / r.width, 0.0f, 1.0f));
            if (!input.isLeftMouseDown()) activeSlider_ = -1;
            return;
        }
    }
    if (!clicked) return;

    bool handled = true;
    if (inside(mouseX_, mouseY_, action(2, layout, state))) {
        if (state == AppState::MainMenu) quitRequested = true;
        else state = AppState::MainMenu;
    } else if (state == AppState::MainMenu) {
        if (inside(mouseX_, mouseY_, action(0, layout, state)))
            state = AppState::Customization;
        else if (inside(mouseX_, mouseY_, action(1, layout, state)))
            state = AppState::SoundSettings;
        else handled = false;
    } else if (state == AppState::Customization) {
        if (inside(mouseX_, mouseY_, action(3, layout, state))) {
            state = AppState::Playing;
        } else {
            handled = false;
            for (int i = 0; i < 4; ++i) {
                if (!inside(mouseX_, mouseY_, colorOption(i))) continue;
                settings.color = static_cast<HelicopterColor>(i);
                handled = true;
            }
            for (int i = 0; i < 3; ++i) {
                if (!inside(mouseX_, mouseY_, sizeOption(i))) continue;
                settings.size = static_cast<HelicopterSize>(i);
                handled = true;
            }
        }
    } else if (state == AppState::SoundSettings &&
                inside(mouseX_, mouseY_, action(4, layout, state))) {
        audio.setMuted(!audio.isMuted());
    } else handled = false;
    if (handled) audio.playUIClick();
}
