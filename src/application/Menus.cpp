#include "Menus.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cstdio>

namespace
{
    struct Button
    {
        float x, y, width, height;
        const char* label;
    };

    // --------------------------------------------------
    // GENERAL UI HELPERS
    // --------------------------------------------------

    bool inside(int mouseX, int mouseY, const Button& button)
    {
        return mouseX >= button.x &&
               mouseX <= button.x + button.width &&
               mouseY >= button.y &&
               mouseY <= button.y + button.height;
    }

    void drawRectangle(
        Renderer& renderer,
        float x,
        float y,
        float width,
        float height,
        Color color)
    {
        renderer.draw(
            MeshId::Quad,
            Matrix4::translation(
                x + width / 2.0f,
                y + height / 2.0f,
                0.0f
            ) * Matrix4::scale(width, height, 1.0f),
            color
        );
    }

    void drawText(
        float x,
        float y,
        const char* text,
        void* font = GLUT_BITMAP_HELVETICA_18)
    {
        GLint previousMatrixMode;

        const GLint windowWidth = glutGet(GLUT_WINDOW_WIDTH);
        const GLint windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

        glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(
            0,
            windowWidth,
            windowHeight,
            0,
            -1,
            1
        );

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glColor3f(0.92f, 0.96f, 1.0f);
        glRasterPos2f(x, y);

        for (const char* character = text; *character; ++character)
        {
            glutBitmapCharacter(font, *character);
        }

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(previousMatrixMode);
        glPopAttrib();
    }

    void drawButton(
        Renderer& renderer,
        const Button& button,
        bool selected)
    {
        const Color color = selected
            ? Color{0.12f, 0.48f, 0.72f}
            : Color{0.12f, 0.20f, 0.30f};

        drawRectangle(
            renderer,
            button.x,
            button.y,
            button.width,
            button.height,
            color
        );

        drawText(
            button.x + 18.0f,
            button.y + button.height * 0.65f,
            button.label
        );
    }

    // --------------------------------------------------
    // MAIN MENU BUTTONS
    // --------------------------------------------------

    Button mainButton(int index, int width, int height)
    {
        const float panelWidth = width * 0.82f;
        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = height * 0.11f;

        return {
            panelX + 45.0f,
            panelY + 150.0f + index * 78.0f,
            panelWidth * 0.43f,
            54.0f,

            index == 0 ? "GAME START" :
            index == 1 ? "CUSTOMIZATION" :
            index == 2 ? "SOUND SETTINGS" : "EXIT"
        };
    }

    // --------------------------------------------------
    // CUSTOMIZATION BUTTONS
    // --------------------------------------------------

    Button sizeButton(int index, int width, int height)
    {
        const float panelWidth = width * 0.72f;
        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - height * 0.70f) / 2.0f;

        return {
            panelX + 40.0f,
            panelY + 115.0f + index * 58.0f,
            220.0f,
            48.0f,

            index == 0 ? "SMALL - 1.0x" :
            index == 1 ? "MEDIUM - 1.25x" :
                         "LARGE - 1.5x"
        };
    }

    Button colorButton(int index, int width, int height)
    {
        const float panelWidth = width * 0.72f;
        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - height * 0.70f) / 2.0f;

        return {
            panelX + 40.0f,
            panelY + 365.0f + index * 42.0f,
            220.0f,
            36.0f,

            index == 0 ? "RED" :
            index == 1 ? "BLUE" :
            index == 2 ? "GREEN" : "WHITE"
        };
    }

    // --------------------------------------------------
    // BACK BUTTON
    // --------------------------------------------------

    Button backButton(int width, int height)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        return {
            panelX + 40.0f,
            panelY + panelHeight - 65.0f,
            220.0f,
            45.0f,
            "BACK"
        };
    }

    // --------------------------------------------------
    // SOUND SETTINGS BUTTONS
    // --------------------------------------------------

    Button volumeSlider(int index, int width, int height)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        return {
            panelX + 40.0f,
            panelY + 155.0f + index * 95.0f,
            panelWidth - 80.0f,
            22.0f,
            ""
        };
    }

    Button muteButton(int width, int height)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        return {
            panelX + 40.0f,
            panelY + panelHeight - 125.0f,
            220.0f,
            45.0f,
            "MUTE"
        };
    }

    float getSliderValue(int mouseX, const Button& slider)
    {
        const float value =
            static_cast<float>(mouseX - slider.x) / slider.width;

        return std::clamp(value, 0.0f, 1.0f);
    }

    void drawVolumeSlider(
        Renderer& renderer,
        int width,
        int height,
        int index,
        const char* label,
        float volume)
    {
        const Button slider = volumeSlider(index, width, height);

        char percentage[16];

        std::snprintf(
            percentage,
            sizeof(percentage),
            "%d%%",
            static_cast<int>(volume * 100.0f + 0.5f)
        );

        // Slider label.
        drawText(
            slider.x,
            slider.y - 15.0f,
            label
        );

        // Volume percentage.
        drawText(
            slider.x + slider.width - 45.0f,
            slider.y - 15.0f,
            percentage
        );

        // Slider background.
        drawRectangle(
            renderer,
            slider.x,
            slider.y,
            slider.width,
            slider.height,
            {0.12f, 0.20f, 0.30f}
        );

        // Filled volume portion.
        if (volume > 0.0f)
        {
            drawRectangle(
                renderer,
                slider.x,
                slider.y,
                slider.width * volume,
                slider.height,
                {0.12f, 0.48f, 0.72f}
            );
        }
    }

    // --------------------------------------------------
    // MAIN MENU DISPLAY
    // --------------------------------------------------

    void drawMainMenu(
        Renderer& renderer,
        int width,
        int height)
    {
        const float panelWidth = width * 0.82f;
        const float panelHeight = height * 0.78f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = height * 0.11f;

        drawRectangle(
            renderer,
            panelX,
            panelY,
            panelWidth,
            panelHeight,
            {0.035f, 0.075f, 0.13f}
        );

        drawText(
            panelX + 45.0f,
            panelY + 55.0f,
            "THAT HELICOPTER"
        );

        drawText(
            panelX + 45.0f,
            panelY + 88.0f,
            "CITY HELICOPTER SIMULATOR"
        );

        const float previewX = panelX + panelWidth * 0.53f;
        const float previewY = panelY + 145.0f;
        const float previewWidth = panelWidth * 0.43f;
        const float previewHeight = panelHeight - 195.0f;

        drawRectangle(
            renderer,
            previewX,
            previewY,
            previewWidth,
            previewHeight,
            {0.55f, 0.79f, 0.91f}
        );

        drawText(
            previewX + 20.0f,
            previewY + 30.0f,
            "HELICOPTER PREVIEW"
        );

        for (int i = 0; i < 4; ++i)
        {
            drawButton(
                renderer,
                mainButton(i, width, height),
                false
            );
        }

        drawText(
            panelX + 45.0f,
            panelY + panelHeight - 25.0f,
            "WASD: MOVE | RIGHT MOUSE: CAMERA"
        );
    }

    // --------------------------------------------------
    // CUSTOMIZATION DISPLAY
    // --------------------------------------------------

    void drawCustomizationMenu(
        Renderer& renderer,
        int width,
        int height,
        const CustomizationSettings& customization)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        drawRectangle(
            renderer,
            panelX,
            panelY,
            panelWidth,
            panelHeight,
            {0.035f, 0.075f, 0.13f}
        );

        drawText(
            panelX + 40.0f,
            panelY + 65.0f,
            "HELICOPTER CUSTOMIZATION"
        );

        drawText(
            panelX + 40.0f,
            panelY + 95.0f,
            "SELECT HELICOPTER SIZE:"
        );

        // Size buttons.
        for (int i = 0; i < 3; ++i)
        {
            drawButton(
                renderer,
                sizeButton(i, width, height),
                static_cast<int>(customization.size) == i
            );
        }

        const char* selectedSize =
            customization.size == HelicopterSize::Small
                ? "SMALL"
                : customization.size == HelicopterSize::Medium
                    ? "MEDIUM"
                    : "LARGE";

        drawText(
            panelX + 40.0f,
            panelY + 350.0f,
            "SELECTED SIZE:"
        );

        drawText(
            panelX + 190.0f,
            panelY + 350.0f,
            selectedSize
        );

        drawText(
            panelX + 40.0f,
            panelY + 390.0f,
            "BODY COLOR:"
        );

        // Color buttons.
        for (int i = 0; i < 4; ++i)
        {
            drawButton(
                renderer,
                colorButton(i, width, height),
                static_cast<int>(customization.color) == i
            );
        }

        drawButton(
            renderer,
            backButton(width, height),
            false
        );

        // Preview background.
        const float previewX = panelX + panelWidth * 0.53f;
        const float previewY = panelY + 105.0f;
        const float previewWidth = panelWidth * 0.43f;
        const float previewHeight = panelHeight - 145.0f;

        drawRectangle(
            renderer,
            previewX,
            previewY,
            previewWidth,
            previewHeight,
            {0.55f, 0.79f, 0.91f}
        );

        drawText(
            previewX + 20.0f,
            previewY + 30.0f,
            "LIVE HELICOPTER PREVIEW"
        );
    }

    // --------------------------------------------------
    // SOUND SETTINGS DISPLAY
    // --------------------------------------------------

    void drawSoundMenu(
        Renderer& renderer,
        int width,
        int height,
        Audio& audio)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        drawRectangle(
            renderer,
            panelX,
            panelY,
            panelWidth,
            panelHeight,
            {0.035f, 0.075f, 0.13f}
        );

        drawText(
            panelX + 40.0f,
            panelY + 65.0f,
            "SOUND SETTINGS"
        );

        // Master volume.
        drawVolumeSlider(
            renderer,
            width,
            height,
            0,
            "MASTER VOLUME",
            audio.getMasterVolume()
        );

        // UI volume.
        drawVolumeSlider(
            renderer,
            width,
            height,
            1,
            "UI SOUND VOLUME",
            audio.getUIVolume()
        );

        // Helicopter volume.
        drawVolumeSlider(
            renderer,
            width,
            height,
            2,
            "HELICOPTER VOLUME",
            audio.getHelicopterVolume()
        );

        // Mute button.
        Button mute = muteButton(width, height);

        mute.label = audio.isMuted() ? "UNMUTE" : "MUTE";

        drawButton(
            renderer,
            mute,
            audio.isMuted()
        );

        // Back button.
        drawButton(
            renderer,
            backButton(width, height),
            false
        );
    }

    // --------------------------------------------------
    // 3D HELICOPTER PREVIEW
    // --------------------------------------------------

    void drawHelicopterPreview(
        Renderer& renderer,
        Helicopter& helicopter,
        int width,
        int height,
        float previewX,
        float previewY,
        float previewWidth,
        float previewHeight,
        const CustomizationSettings& customization)
    {
        const GLint headerSpace = 45;

        const GLint viewportX =
            static_cast<GLint>(previewX);

        const GLint viewportY =
            static_cast<GLint>(
                height - previewY - previewHeight
            );

        const GLint viewportWidth =
            static_cast<GLint>(previewWidth);

        const GLint viewportHeight =
            static_cast<GLint>(previewHeight - headerSpace);

        if (viewportWidth <= 0 || viewportHeight <= 0)
            return;

        // Restrict rendering to the preview area.
        glEnable(GL_SCISSOR_TEST);

        glScissor(
            viewportX,
            viewportY,
            viewportWidth,
            viewportHeight
        );

        glViewport(
            viewportX,
            viewportY,
            viewportWidth,
            viewportHeight
        );

        glClearColor(
            0.55f,
            0.79f,
            0.91f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        const Matrix4 view = Matrix4::lookAt(
            {6.0f, 3.5f, 7.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        );

        const Matrix4 projection = Matrix4::perspective(
            45.0f,
            static_cast<float>(viewportWidth) /
                static_cast<float>(viewportHeight),
            0.1f,
            100.0f
        );

        renderer.begin3D(view, projection);

        const float previewScale = customization.scale();

        const Matrix4 model = Matrix4::scale(
            previewScale,
            previewScale,
            previewScale
        );

        helicopter.draw(
            renderer,
            model,
            customization.bodyColor()
        );

        glDisable(GL_SCISSOR_TEST);

        glViewport(
            0,
            0,
            width,
            height
        );
    }
}

// ==================================================
// MENU RENDERING
// ==================================================

void Menus::render(
    Renderer& renderer,
    Helicopter& helicopter,
    AppState state,
    int width,
    int height,
    const CustomizationSettings& customization,
    Audio& audio)
{
    if (state == AppState::Playing)
        return;

    // MAIN MENU
    if (state == AppState::MainMenu)
    {
        const float panelWidth = width * 0.82f;
        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = height * 0.11f;
        const float panelHeight = height * 0.78f;

        const float previewX = panelX + panelWidth * 0.53f;
        const float previewY = panelY + 145.0f;
        const float previewWidth = panelWidth * 0.43f;
        const float previewHeight = panelHeight - 195.0f;

        renderer.beginInterface(width, height);

        drawMainMenu(
            renderer,
            width,
            height
        );

        renderer.endInterface();

        drawHelicopterPreview(
            renderer,
            helicopter,
            width,
            height,
            previewX,
            previewY,
            previewWidth,
            previewHeight,
            customization
        );
    }

    // CUSTOMIZATION MENU
    else if (state == AppState::Customization)
    {
        const float panelWidth = width * 0.72f;
        const float panelHeight = height * 0.70f;

        const float panelX = (width - panelWidth) / 2.0f;
        const float panelY = (height - panelHeight) / 2.0f;

        const float previewX = panelX + panelWidth * 0.53f;
        const float previewY = panelY + 105.0f;
        const float previewWidth = panelWidth * 0.43f;
        const float previewHeight = panelHeight - 145.0f;

        renderer.beginInterface(width, height);

        drawCustomizationMenu(
            renderer,
            width,
            height,
            customization
        );

        renderer.endInterface();

        drawHelicopterPreview(
            renderer,
            helicopter,
            width,
            height,
            previewX,
            previewY,
            previewWidth,
            previewHeight,
            customization
        );
    }

    // SOUND SETTINGS MENU
    else if (state == AppState::SoundSettings)
    {
        renderer.beginInterface(width, height);

        drawSoundMenu(
            renderer,
            width,
            height,
            audio
        );

        renderer.endInterface();
    }
}

// ==================================================
// MENU INPUT
// ==================================================

void Menus::handleInput(
    Input& input,
    AppState& state,
    int width,
    int height,
    bool& quitRequested,
    CustomizationSettings& customization,
    Audio& audio)
{
    if (!input.isLeftMouseClicked())
        return;

    int mouseX;
    int mouseY;

    input.getMousePosition(mouseX, mouseY);

    input.consumeLeftMouseClick();

    // --------------------------------------------------
    // MAIN MENU INPUT
    // --------------------------------------------------

    if (state == AppState::MainMenu)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (!inside(
                mouseX,
                mouseY,
                mainButton(i, width, height)
            ))
            {
                continue;
            }

            switch (i)
            {
                case 0:
                    state = AppState::Playing;
                    break;

                case 1:
                    state = AppState::Customization;
                    break;

                case 2:
                    state = AppState::SoundSettings;
                    break;

                case 3:
                    quitRequested = true;
                    break;
            }

            return;
        }
    }

    // --------------------------------------------------
    // OTHER MENU INPUT
    // --------------------------------------------------

    else
    {
        // CUSTOMIZATION INPUT
        if (state == AppState::Customization)
        {
            // Size selection.
            for (int i = 0; i < 3; ++i)
            {
                if (inside(
                    mouseX,
                    mouseY,
                    sizeButton(i, width, height)
                ))
                {
                    customization.size =
                        static_cast<HelicopterSize>(i);

                    return;
                }
            }

            // Body color selection.
            for (int i = 0; i < 4; ++i)
            {
                if (inside(
                    mouseX,
                    mouseY,
                    colorButton(i, width, height)
                ))
                {
                    customization.color =
                        static_cast<HelicopterColor>(i);

                    return;
                }
            }
        }

        // SOUND SETTINGS INPUT
        if (state == AppState::SoundSettings)
        {
            // Master volume.
            Button masterSlider =
                volumeSlider(0, width, height);

            if (inside(mouseX, mouseY, masterSlider))
            {
                audio.setMasterVolume(
                    getSliderValue(mouseX, masterSlider)
                );

                return;
            }

            // UI sound volume.
            Button uiSlider =
                volumeSlider(1, width, height);

            if (inside(mouseX, mouseY, uiSlider))
            {
                audio.setUIVolume(
                    getSliderValue(mouseX, uiSlider)
                );

                return;
            }

            // Helicopter volume.
            Button helicopterSlider =
                volumeSlider(2, width, height);

            if (inside(mouseX, mouseY, helicopterSlider))
            {
                audio.setHelicopterVolume(
                    getSliderValue(mouseX, helicopterSlider)
                );

                return;
            }

            // Mute / Unmute.
            if (inside(
                mouseX,
                mouseY,
                muteButton(width, height)
            ))
            {
                audio.setMuted(!audio.isMuted());

                return;
            }
        }

        // BACK BUTTON
        if (inside(
            mouseX,
            mouseY,
            backButton(width, height)
        ))
        {
            state = AppState::MainMenu;
            return;
        }
    }
}
