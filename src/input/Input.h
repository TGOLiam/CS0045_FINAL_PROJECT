#pragma once

#include <GL/glut.h>
#include <array>

class Input
{
private:
    static Input* instance;

    std::array<bool, 256> keys;

    bool rightMouseDown;

    int lastMouseX;
    int lastMouseY;

    static void keyboardDown(unsigned char key, int x, int y);
    static void keyboardUp(unsigned char key, int x, int y);

    static void mouseButton(int button, int state, int x, int y);
    static void mouseMotion(int x, int y);
    static void mouseWheel(int wheel, int direction, int x, int y);

public:
    Input();

    void initialize();

    bool isKeyPressed(unsigned char key) const;
    bool isRightMouseDown() const;
};
