# Planned flight physics

Status: acceleration-based flight integration is implemented; collision response
and camera obstruction handling are not implemented yet.

Altitude control replaces the original fixed-altitude flight requirement. A new
Physics subsystem will support simple acceleration-based flight with momentum
and drift. This revises the fixed-altitude scope described in AGENTS.md and the
older project plans.

## Flight behavior

- Store velocity separately from player input.
- Directional input applies acceleration rather than directly setting position.
- Releasing input stops player acceleration while existing velocity persists.
- Drag gradually reduces velocity, allowing the helicopter to drift.
- Opposite input brakes the current motion before reversing direction.
- Allow combined movement, such as sideways drift while accelerating forward.
- Support vertical acceleration and momentum for altitude control.
- Rudder uses angular acceleration and drag, so turns continue briefly after
  release and opposite rudder brakes before reversing the turn.
- Tune horizontal and vertical acceleration, drag, and speed limits separately.
- Visual tilt can follow requested acceleration, returning toward level while
  the helicopter continues drifting.

This is a simple flight model, not a full aerodynamic simulation. Update physics
using delta time before rendering; rendering must not advance the simulation.

## Proposed responsibilities

- Input collects directional and altitude controls.
- Application routes controls and coordinates simulation updates.
- Physics calculates acceleration, velocity, and resulting position.
- Helicopter owns its motion state, rotor animation, and visual tilt.
- Camera follows the updated helicopter position.

Helicopter owns a `MotionState` containing position and velocity. The Physics
foundation defines axis-aligned bounds, an overlap query, and a scale-aware
helicopter box. City exposes static building and rooftop boxes plus ground
height from its generated collision asset. Swept collision and camera obstruction are future work. The
conservative box includes rotor sweep and tilt; before collision response is
enabled, the rooftop spawn must be raised to avoid overlapping the roof.
Avoid duplicate authoritative copies of position or velocity.

## Decisions still pending

- Flight tuning and feel need interactive review. Space ascends, Ctrl descends,
  and Q/E turn the helicopter left/right. WASD follows the helicopter's heading.
- Altitude limits and ground contact behavior.
- Building collisions are planned using the exported city bounds.
- Physics file layout and integration details.

Flight currently uses 120 Hz fixed simulation steps. Existing city collision
bounds are loaded but do not yet restrict movement.

Size-based speed tuning uses Medium as the reference: Small is 1.5× Medium,
Medium is 2× the original flight tuning, and Large is 0.5× Medium. Both the
acceleration and maximum horizontal/vertical speeds use these multipliers;
drag stays the same. Size is selected before flight.
