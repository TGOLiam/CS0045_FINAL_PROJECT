# **Technical Execution Plan**

## **1\. Application structure**

Use one main loop and three application states:

```
enum class AppState { MainMenu, Customization, Flight };
```

Each frame:

```
input.poll();
app.update(deltaTime);
app.render();
```

`Application` owns the window, current state, renderer, input, customization settings, flight scene, and audio.

## **2\. Subsystems**

| Subsystem | Owns and implements |
| ----- | ----- |
| **Application / Menus** | Full-screen window, main loop, menu buttons, state transitions. |
| **Renderer** | Shaders, VAOs, VBOs, reusable meshes, and drawing functions. |
| **City** | Road and building data: positions, dimensions, colors. |
| **Helicopter** | Position, movement speed, fixed altitude, and part transforms. |
| **Camera** | Orbit angles, distance, perspective projection, and view matrix. |
| **Input** | Keyboard/button states, mouse movement, and scroll events. |
| **Customization** | Helicopter color and scale, shared by preview and flight. |
| **Audio** | Sound resources and looping flight playback. |

A `FlightScene` groups the city, helicopter, and camera.

## **3\. Rendering foundation**

Implement a shared interface:

```
renderer.beginFrame(view, projection);
renderer.draw(mesh, modelMatrix, color);
```

*   
  Upload mesh geometry once using VBOs and VAOs.  
* Use solid-color shaders; no lighting.  
* Enable depth testing for 3D objects.  
* Use perspective projection for flight and the helicopter preview.  
* Use orthographic projection for menus.  
* Give every object its own model matrix.  
* Use triangles for surfaces, lines for road markings, and points for navigation lights.

Scene subsystems compose objects using these helpers. The renderer owns OpenGL resources; scene objects own their data.

## **4\. City and helicopter**

**City**

* Create a finite grid of roads and building blocks as the initial implementation.  
* Represent buildings using reusable cube geometry.  
* Store building positions, dimensions, and colors.  
* Keep city construction separate from rendering.

**Dynamic city actors**

* Cars and pedestrians live in `src/city/CityActors`, updated with
  `City::update(deltaTime)` from the application loop and rendered through
  the shared unit primitives; they are not baked into the static city asset.
* Cars wander the road grid: lane centers sit 2 m right of each centerline,
  nodes are the 120 m intersections (lines at 120 m spacing offset 60 m),
  exits are weighted (straight 5 : turn 2 : 2), boundary nodes turn inward.
* Pedestrians walk the block sidewalk rings (5.5 m in from each road
  centerline), bouncing at ring ends; density comes from `ActorConfig`
  (`walkersPerBlock`, default 6 per ring) and `carsPerChunk` (default 12,
  matched to the reference placements; values above 12 add seeded random
  lane starts). Pass the config to `City::initialize`.
* Movement runs on fixed 1/120 s steps so the seeded wander is
  deterministic; actor collisions are not simulated (they are below the
  flight corridor).

**Helicopter**

* Build the body, tail, landing gear, and rotors from simple meshes.  
* Combine the helicopter’s overall transform with each part’s local transform.  
* Apply the color and size selected before flight.  
* Keep its size fixed throughout flight.

## **5\. Controls**

| State | Input | Action |
| ----- | ----- | ----- |
| Customization | Scroll | Increase or decrease helicopter size |
| Customization | Click color option | Change helicopter color |
| Flight | W / S | Move forward / backward |
| Flight | A / D | Move left / right |
| Flight | Right mouse drag | Orbit camera |
| Flight | Scroll | Zoom camera |

*   
  Route input according to the current application state.  
* Start with movement relative to fixed world directions.  
* Multiply movement speed by `deltaTime`.  
* Keep helicopter altitude constant.  
* Clamp customization size and camera distance to usable ranges.  
* Keep the camera focused on the helicopter.  
* Start with a perspective field of view of 60°.  
* Flight input must not modify the selected helicopter size.

## **6\. Menus and customization**

**Main Menu → Customization → Play → Flight**

* Main menu: start and exit.  
* Customization: color choices, scroll-controlled size adjustment, helicopter preview, and Play button.  
* Store color and size in customization settings.  
* On Play, use those settings for the flight helicopter.  
* Size adjustments are available only in customization.

## **7\. Audio**

* Load engine and rotor sounds once.  
* Start looping playback when entering flight.  
* Stop playback when leaving flight.  
* Release audio resources on shutdown.

## **8\. Implementation order**

1. Full-screen window, main loop, and application states.  
2. Renderer with a colored cube and depth testing.  
3. City grid and helicopter model.  
4. Keyboard movement, camera orbit, and camera zoom.  
5. Customization menu with preview, color selection, and scroll-controlled sizing.  
6. Connect Play to flight, preserving the selected appearance.  
7. Helicopter audio.  
8. Assignment interactions and final verification.

## **Completion checks**

* Application compiles and runs full-screen.  
* Color and size selections carry into flight.  
* Helicopter size can change only in customization.  
* Scrolling during flight changes camera distance only.  
* Movement stays at a fixed altitude.  
* Object transformations remain independent.  
* Audio starts and stops with flight.  
* All three primitive types and required object interactions are demonstrated.

## **Project Structure**

HelicopterSimulator/  
├── CMakeLists.txt  
├── README.md  
├── .gitignore  
├── assets/  
│   ├── audio/  
│   │   ├── engine.wav  
│   │   └── rotor.wav  
│   └── fonts/  
├── src/  
│   ├── main.cpp  
│   ├── application/                 \# Member 1  
│   │   ├── Application.h  
│   │   ├── Application.cpp  
│   │   ├── AppState.h  
│   │   ├── Menus.h  
│   │   └── Menus.cpp  
│   ├── renderer/                    \# Member 1  
│   │   ├── Renderer.h  
│   │   ├── Renderer.cpp  
│   │   ├── Mesh.h  
│   │   ├── Mesh.cpp  
│   │   ├── PrimitiveMeshes.h  
│   │   └── PrimitiveMeshes.cpp  
│   ├── camera/                      \# Member 1  
│   │   ├── Camera.h  
│   │   └── Camera.cpp  
│   ├── input/                       \# Member 2  
│   │   ├── Input.h  
│   │   └── Input.cpp  
│   ├── customization/               \# Member 2  
│   │   ├── CustomizationSettings.h  
│   │   ├── CustomizationMenu.h  
│   │   └── CustomizationMenu.cpp  
│   ├── city/                        \# Member 2  
│   │   ├── City.h  
│   │   └── City.cpp  
│   ├── helicopter/                  \# Member 2  
│   │   ├── Helicopter.h  
│   │   └── Helicopter.cpp  
│   └── audio/                       \# Member 2  
│       ├── Audio.h  
│       └── Audio.cpp  
├── docs/  
│   └── controls.md  
└── build/                           \# Generated; excluded from Git