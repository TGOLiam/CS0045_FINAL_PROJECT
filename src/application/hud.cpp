#include "HUD.h"

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>

namespace
{
    struct Button {
        float x;
        float y;
        float width;
        float height;
    };

    Button mainMenuButton(int height)
    {
        return {20.0f, height - 68.0f, 170.0f, 44.0f};
    }

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

    void drawText(float x, float y, const char* text)
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

        for (const char* character = text; *character; ++character) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *character);
        }

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(previousMatrixMode);
        glPopAttrib();
    }

    float getSpeed(const Vec3& velocity)
    {
        const float speed = std::sqrt(
            velocity.x * velocity.x +
            velocity.y * velocity.y +
            velocity.z * velocity.z
        );

        // Converts meters per second to kilometers per hour.
        return speed * 3.6f;
    }

    float getCompassHeading(float yawDegrees)
    {
        // At zero yaw, the helicopter faces East (+X).
        float heading = yawDegrees + 90.0f;

        while (heading < 0.0f)
            heading += 360.0f;

        while (heading >= 360.0f)
            heading -= 360.0f;

        return heading;
    }

    const char* getDirection(float heading)
    {
        if (heading >= 315.0f || heading < 45.0f)
            return "N";

        if (heading < 135.0f)
            return "E";

        if (heading < 225.0f)
            return "S";

        return "W";
    }

    void drawMainMenuButton(Renderer& renderer, int height)
    {
        const Button button = mainMenuButton(height);

        drawRectangle(
            renderer,
            button.x,
            button.y,
            button.width,
            button.height,
            {0.12f, 0.30f, 0.45f}
        );

        drawText(
            button.x + 18.0f,
            button.y + 28.0f,
            "MAIN MENU"
        );
    }
}

void HUD::render(
    Renderer& renderer,
    const Helicopter& helicopter,
    int width,
    int height,
    float groundHeight)
{
    renderer.beginInterface(width, height);

    const Vec3 position = helicopter.position();
    const Vec3 velocity = helicopter.velocity();

    const float heading =
        getCompassHeading(helicopter.headingDegrees());

    const float altitude = position.y - groundHeight;
    const float speed = getSpeed(velocity);

    char headingText[64];
    char altitudeText[64];
    char speedText[64];

    std::snprintf(
        headingText,
        sizeof(headingText),
        "HEADING: %.0f DEG  %s",
        heading,
        getDirection(heading)
    );

    std::snprintf(
        altitudeText,
        sizeof(altitudeText),
        "ALTITUDE: %.1f M",
        altitude
    );

    std::snprintf(
        speedText,
        sizeof(speedText),
        "SPEED: %.1f KM/H",
        speed
    );

    // Flight information panel.
    drawRectangle(
        renderer,
        20.0f,
        20.0f,
        260.0f,
        130.0f,
        {0.035f, 0.075f, 0.13f, 0.88f}
    );

    drawText(40.0f, 50.0f, "FLIGHT INFORMATION");
    drawText(40.0f, 82.0f, headingText);
    drawText(40.0f, 110.0f, altitudeText);
    drawText(40.0f, 138.0f, speedText);

    // Controls panel.
    const float controlsX = width - 265.0f;

    drawRectangle(
        renderer,
        controlsX,
        20.0f,
        245.0f,
        190.0f,
        {0.035f, 0.075f, 0.13f, 0.88f}
    );

    drawText(controlsX + 20.0f, 50.0f, "FLIGHT CONTROLS");
    drawText(controlsX + 20.0f, 82.0f, "W / S  - Forward / Back");
    drawText(controlsX + 20.0f, 110.0f, "A / D  - Left / Right");
    drawText(controlsX + 20.0f, 138.0f, "SPACE  - Ascend");
    drawText(controlsX + 20.0f, 166.0f, "CTRL   - Descend");
    drawText(controlsX + 20.0f, 194.0f, "Q / E  - Turn");

    // Main menu button.
    drawMainMenuButton(renderer, height);

    renderer.endInterface();
}

bool HUD::handleInput(
    Input& input,
    int width,
    int height)
{
    if (!input.isLeftMouseClicked())
        return false;

    int mouseX;
    int mouseY;

    input.getMousePosition(mouseX, mouseY);

    if (inside(mouseX, mouseY, mainMenuButton(height)))
    {
        input.consumeLeftMouseClick();
        return true;
    }

    return false;
}