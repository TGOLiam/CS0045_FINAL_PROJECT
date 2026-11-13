#include "Application.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

int Application::run(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion(2, 1);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    windowId_ = glutCreateWindow("City Helicopter Simulator");
    if (windowId_ <= 0) {
        std::cerr << "Could not create the OpenGL window.\n";
        return 1;
    }

    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "Could not initialize GLEW: "
                  << glewGetErrorString(glewStatus) << '\n';
        glutDestroyWindow(windowId_);
        windowId_ = 0;
        return 1;
    }

    // Temporary Stage 1 callbacks until the frame loop and Input are integrated.
    glutDisplayFunc([]() {
        glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
    });
    glutKeyboardFunc([](unsigned char key, int, int) {
        if (key == 27) glutLeaveMainLoop();
    });

    glutFullScreen();
    glutMainLoop();
    if (glutGetWindow() != 0) {
        glutDestroyWindow(windowId_);
        windowId_ = 0;
    }
    return 0;
}
