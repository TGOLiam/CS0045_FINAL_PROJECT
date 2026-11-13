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

The current application opens a full-screen window; press Escape to exit. Run
`make clean` to remove generated build files.

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
The six-colored cube in Application is temporary Stage 1 test scene code.
