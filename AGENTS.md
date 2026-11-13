# Project Guide

## Purpose and scope

Build a small C++ OpenGL helicopter simulator set above a city with a simple grid street layout.

- Run in full-screen mode.
- Move the helicopter forward/backward and left/right at a fixed altitude.
- Provide a third-person camera with orbit and distance-based zoom.
- Let players select helicopter color and size in customization before pressing Play.
- Allow size changes only in customization, never during flight.
- Loop helicopter engine and rotor sounds during flight.
- Use the flow: Main Menu -> Customization -> Play -> Flight.
- Use solid, unlit colors. Do not introduce lighting, shaders, textures, or extra features without agreement.
- Start with a finite city. Procedural building variation and infinite generation are not confirmed scope.

## OpenGL requirements

- Use the legacy fixed-function pipeline in a compatible context. Do not use GLSL shaders or a core-only context.
- Demonstrate vertex arrays and VBOs. Vertex arrays do not mean VAOs; VAOs are not required.
- Configure fixed-function vertex arrays and draw reusable VBO-backed meshes.
- Use triangles for surfaces, lines for road markings, and points for visible navigation lights.
- Enable depth testing for 3D rendering; keep lighting disabled.
- Use perspective projection for flight and the helicopter preview, and orthographic projection for interface elements.
- Keep independent object transforms. Balance matrix stack pushes/pops and restore render state after temporary changes.
- Upload static geometry once and reuse it instead of rebuilding buffers every frame.

## Assignment completion

The assignment requires at least three major objects and three primitive types, distinguishable colors, vertex arrays, VBOs, and three transformations on different objects:

1. Automatic translation at timed intervals.
2. Rotation triggered by a mouse click.
3. Scaling controlled by the mouse scroll wheel.

The helicopter preview provides scroll-controlled scaling. The objects for automatic translation and click-triggered rotation are still undecided. Do not silently choose them or claim the assignment is complete without them. Manual flight and camera orbit do not replace these requirements.

## Subsystem ownership

| Subsystem | Owner | Responsibilities |
| --- | --- | --- |
| Application / Menus | Member 1 | Window, main loop, application states, main menu, transitions. |
| Renderer | Member 1 | Fixed-function drawing, VBO-backed meshes, vertex arrays, colors, depth testing, matrix application. |
| Camera | Member 1 | Tracking, orbit, zoom limits, view and perspective matrices. |
| Customization | Member 2 | Color and size settings, preview, customization controls. |
| Audio | Member 2 | Sound resources, looping playback, cleanup. |
| City | Member 2 | Streets, buildings, positions, dimensions, colors. |
| Helicopter | Member 2 | Model parts, fixed-altitude movement, local transforms, rotor animation. |
| Input | Member 2 | Keyboard/mouse state, movement and orbit input, state-dependent scrolling. |

Ownership coordinates work; it does not prohibit necessary cross-subsystem fixes. Coordinate before changing interfaces or editing files the other member is actively changing. Do not overwrite another member's work.

## Structure

```text
README.md
.gitignore
assets/
  audio/
  fonts/
src/
  main.cpp
  application/     # Application, AppState, Menus
  renderer/        # Renderer, Mesh, PrimitiveMeshes
  camera/          # Camera
  input/           # Input
  customization/   # CustomizationSettings, CustomizationMenu
  city/            # City
  helicopter/      # Helicopter
  audio/           # Audio
docs/
  controls.md
build/             # Generated; ignore in Git
```

Keep related .h and .cpp files together. Do not add shader directories or a Shader class. Do not use CMake. Agree on the build method and windowing, math, UI, and audio dependencies before introducing them; document the actual choices and build steps in README.md.

## Data and interface rules

- Application owns subsystem lifetimes and the current state: MainMenu, Customization, or Flight.
- CustomizationSettings is the single source of truth for helicopter color and scale. Flight reads these settings without modifying them.
- City owns building and street data; Helicopter owns its position and motion; Camera owns orbit angles and distance.
- Renderer owns OpenGL resources. Release those resources before destroying the OpenGL context.
- Input collects events; Application routes actions according to the active state. Input must not directly modify rendering resources.
- Camera reads the helicopter position as its target.
- City and Helicopter supply model transforms and colors to shared renderer helpers, such as draw(mesh, modelMatrix, color).
- Reuse helicopter drawing code for customization preview and flight.
- Avoid mutable globals and duplicate authoritative copies of data.
- Update simulation before rendering. Rendering should not advance movement or animation.

## Controls

- Customization scroll: change helicopter size within limits.
- Customization color option: change helicopter color.
- Flight W/S: forward/backward; A/D: left/right.
- Flight right mouse drag: orbit camera.
- Flight scroll: change camera distance only.
- Initially use fixed world directions for movement; keep altitude constant.
- Apply deltaTime to movement and animation; normalize diagonal movement.
- Clamp camera distance and avoid orbit singularities. Start with a 60-degree perspective field of view.
- Document final bindings in docs/controls.md.

## Execution stages

1. Member 1 builds the application and renderer; Member 2 builds input. Verify a rendered cube and input events.
2. Member 1 supplies meshes and camera; Member 2 builds city and helicopter. Verify the complete scene.
3. Member 1 implements camera tracking/orbit/zoom; Member 2 connects flight movement, rotor animation, and input. Verify fixed-altitude flight.
4. Member 1 connects menus and state transitions; Member 2 implements customization and preview. Verify settings persist into flight and size cannot change there.
5. Member 2 adds audio; both members integrate the remaining assignment interactions and verify the complete program.

## Collaboration and verification

- Keep changes focused and integrate small, working commits from separate branches.
- Agree on shared interfaces early; avoid unnecessary architecture and unrelated refactoring.
- Preserve existing user changes. Never discard work to resolve a conflict without agreement.
- Keep generated builds and temporary files out of version control.
- Compile after meaningful changes using the agreed build method. Document the exact build commands and any platform-specific setup in README.md.
- Manually verify full-screen startup, menu transitions, preview, color/size persistence, movement, camera controls, sound lifecycle, and transform independence.
- Check all assignment interactions and primitive types before submission.
- Report what changed, what was verified, and any remaining issues. Do not claim checks passed unless they were performed.
