# City Helicopter Simulator

The application is in its first implementation stage. See AGENTS.md for scope,
subsystem ownership, and collaboration rules.

## Build

The Makefile builds optimized C++17 code by default. Run the commands from the
project root so the program can load the city assets.

### Linux

Install a C++17 compiler, GNU Make, and development packages for FreeGLUT,
GLEW, OpenGL, and GLU. For example, on Debian or Ubuntu:

```sh
sudo apt install g++ make freeglut3-dev libglew-dev libgl1-mesa-dev libglu1-mesa-dev
make
./build/helicopter-simulator
```

### Windows (MSYS2 UCRT64)

Install [MSYS2](https://www.msys2.org/), open its **UCRT64** terminal, and
install the compiler, dependencies, GNU Make, and Python:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut mingw-w64-ucrt-x86_64-glew mingw-w64-ucrt-x86_64-python
make
./build/helicopter-simulator.exe
```

Use the UCRT64 terminal to run the program so the MSYS2 runtime DLLs are on
`PATH`. The Makefile relies on the Unix shell utilities provided by MSYS2; it
is not a native Command Prompt or Visual Studio build. Do not mix packages from
different MSYS2 environments.

The current application opens a full-screen window; press Escape to exit. Run
`make clean` to remove generated build files.

## Flight HUD

Flight uses a pilot-style overlay implemented with the legacy fixed-function
OpenGL path (C++17, FreeGLUT, GLEW, and OpenGL compatibility context). It
shows a heading strip plus simple height and speed tapes. Displayed values are
smoothed for readability only; this does not change helicopter movement.

The city is loaded from `assets/city/city.mesh` and `city.collision`, so run the program from the
project root. The asset contains the current fixed 3 × 3 city geometry and solid
colors, including its roads, buildings, parks and helipad. Its binary format is:
`CITYMESH` (8 bytes), version and batch count (little-endian uint32), followed
by batches of primitive and vertex count (uint32), RGBA (four IEEE-754 float32)
and XYZ positions (three float32 per vertex). Version 1 requires a little-endian
system. Renderer uploads these static batches once during initialization.
The collision asset contains building and rooftop axis-aligned boxes and ground
height. A mesh fingerprint detects a collision file from a different build.
The bounds are loaded but collision response is not implemented yet.

### Editing the city

Edit `assets/city/layout.json` to change starting-chunk building positions,
dimensions, styles and palette choices, or the central tower's height. Then run:

```sh
make city-asset
make
```

Run the platform-specific executable shown above. Set `PYTHON` when necessary
to choose a Python 3 interpreter, for example `make city-asset PYTHON=python`.

The generator validates building overlap, road/sidewalk clearance and the
reserved park and helipad areas before replacing the mesh and collision data. It requires Python 3,
the C++ compiler and the OpenGL development headers; Python is not needed at
runtime. The generator's geometry rules live in `tools/city/CityGenerator.cpp`.
The current chunk template requires a 360-unit chunk, 120-unit road spacing,
and 8-unit roads; the layout file records these fixed values. Generated chunks
are deterministic variations of the starting layout. Changing tower height also
requires updating the helicopter's starting altitude in `Helicopter.h`.

## Renderer interface

Application initializes Renderer after creating the OpenGL context and releases
it before destroying that context. Renderer uploads its cube, line, point and
quad meshes once during initialization.

For a 3D frame, call `beginFrame`, then `begin3D` with the camera's view and
projection matrices. Call `draw` with a `MeshId`, model matrix and color for
each object. The unit cube is centered at the origin. Matrices use column-major
storage; `translation * rotation * scale` applies scale first.

For interface elements, call `beginInterface(width, height)`, draw using model
transforms in pixel coordinates, then call `endInterface`. The origin is at the
top-left. The interface pass restores the previous matrices and depth setting.
The current city asset is a fixed scene; streaming chunks is future work.
