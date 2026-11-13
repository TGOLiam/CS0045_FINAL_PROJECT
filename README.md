# City Helicopter Simulator

The application is in its first implementation stage. See AGENTS.md for scope,
subsystem ownership, and collaboration rules.

## Build

On Linux, install a C++17 compiler, GNU Make, and development packages for
FreeGLUT, GLEW, OpenGL, and GLU. For example, on Debian or Ubuntu:

```sh
sudo apt install g++ make freeglut3-dev libglew-dev libgl1-mesa-dev libglu1-mesa-dev
make
./build/helicopter-simulator
```

The current entry point exits immediately; window creation comes next. Run
`make clean` to remove generated build files.
