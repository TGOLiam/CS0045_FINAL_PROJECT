#include "Camera.h"

#include <GL/glew.h>
#include <GL/glu.h>

void Camera::apply(int width, int height) const
{
    // Temporary perspective and fixed viewpoint for inspecting the test cube.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<double>(width) / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4.0, 3.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}
