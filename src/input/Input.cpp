#include "Input.h"

#include <GL/freeglut.h>

Input* Input::instance = nullptr;

Input::Input()
    : keys{}, rightMouseDown(false), lastMouseX(0), lastMouseY(0),
      dragX(0), dragY(0), wheelDelta(0)
{
}

void Input::initialize()
{
    instance = this;
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMouseWheelFunc(mouseWheel);
}

void Input::shutdown()
{
    // The window may already be gone after a window-manager close.
    if (instance == this) instance = nullptr;
}

void Input::keyboardDown(unsigned char key, int, int)
{
    if (instance) instance->keys[key] = true;
}

void Input::keyboardUp(unsigned char key, int, int)
{
    if (instance) instance->keys[key] = false;
}

void Input::mouseButton(int button, int state, int x, int y)
{
    if (!instance || button != GLUT_RIGHT_BUTTON) return;
    instance->rightMouseDown = state == GLUT_DOWN;
    instance->lastMouseX = x;
    instance->lastMouseY = y;
}

void Input::mouseMotion(int x, int y)
{
    if (!instance || !instance->rightMouseDown) return;
    instance->dragX += x - instance->lastMouseX;
    instance->dragY += y - instance->lastMouseY;
    instance->lastMouseX = x;
    instance->lastMouseY = y;
}

void Input::mouseWheel(int, int direction, int, int)
{
    if (instance) instance->wheelDelta += direction;
}

bool Input::isKeyPressed(unsigned char key) const { return keys[key]; }
bool Input::isRightMouseDown() const { return rightMouseDown; }

void Input::consumeMouseDelta(int& x, int& y)
{
    x = dragX;
    y = dragY;
    dragX = 0;
    dragY = 0;
}

int Input::consumeWheelDelta()
{
    const int delta = wheelDelta;
    wheelDelta = 0;
    return delta;
}
