#pragma once

#include "AppState.h"

class Application {
public:
    int run(int argc, char** argv);

private:
    int windowId_ = 0;
};
