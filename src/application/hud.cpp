#include "hud.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
// Classic phosphor green stays distinct from the blue sky and city palette.
// The near-black shadow keeps thin strokes readable over bright buildings.
constexpr float R = 0.66f, G = 1.00f, B = 0.38f, A = 0.98f;
constexpr float SR = .008f, SG = .016f, SB = .035f, SA = .94f;
struct Button {
  float x, y, width, height;
};
struct Display {
  bool ready = false;
  float heading = 0, altitude = 0, speed = 0, opacity = A;
} display;

Button menuButton(int height) { return {20, float(height - 68), 170, 44}; }
bool inside(int x, int y, const Button &b) {
  return x >= b.x && x <= b.x + b.width && y >= b.y && y <= b.y + b.height;
}
float wrap180(float v) {
  v = std::fmod(v + 180.f, 360.f);
  if (v < 0)
    v += 360.f;
  return v - 180.f;
}
float heading(float yaw) {
  float h = std::fmod(yaw + 90.f, 360.f);
  return h < 0 ? h + 360.f : h;
}
float speed(const Vec3 &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) * 3.6f;
}
float filter(float rate, float dt) {
  return 1.f - std::exp(-rate * std::max(dt, 0.f));
}

void projection(int w, int h) {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, w, h, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}
void endProjection() {
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}
void text(float x, float y, const char *value, void *font, float opacity = 1) {
  glColor4f(SR, SG, SB, SA * opacity);
  glRasterPos2f(x + 1, y + 1);
  for (const char *c = value; *c; ++c)
    glutBitmapCharacter(font, *c);
  glColor4f(R, G, B, A * opacity);
  glRasterPos2f(x, y);
  for (const char *c = value; *c; ++c)
    glutBitmapCharacter(font, *c);
}
void line(float x1, float y1, float x2, float y2, float opacity,
          float width = 1.75f) {
  glLineWidth(width);
  glColor4f(SR, SG, SB, SA * opacity);
  glBegin(GL_LINES);
  glVertex2f(x1 + 1, y1 + 1);
  glVertex2f(x2 + 1, y2 + 1);
  glEnd();
  glLineWidth(width);
  glColor4f(R, G, B, A * opacity);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}
void box(float x, float y, float w, float h, float opacity) {
  line(x, y, x + w, y, opacity);
  line(x + w, y, x + w, y + h, opacity);
  line(x + w, y + h, x, y + h, opacity);
  line(x, y + h, x, y, opacity);
}
void rectangle(Renderer &r, float x, float y, float w, float h, Color c) {
  r.draw(MeshId::Quad,
         Matrix4::translation(x + w / 2, y + h / 2, 0) *
             Matrix4::scale(w, h, 1),
         c);
}
const char *cardinal(int h) {
  switch ((h + 360) % 360) {
  case 0:
    return "N";
  case 90:
    return "E";
  case 180:
    return "S";
  case 270:
    return "W";
  default:
    return nullptr;
  }
}

void headingTape(int width) {
  constexpr float pxPerDegree = 5, halfWindow = 37.5f;
  const float cx = width * .5f, base = 76, left = cx - halfWindow * pxPerDegree,
              right = cx + halfWindow * pxPerDegree;
  line(left, base, right, base, display.opacity);
  const int first = int(std::floor((display.heading - halfWindow) / 5)) * 5,
            last = int(std::ceil((display.heading + halfWindow) / 5)) * 5;
  for (int tick = first; tick <= last; tick += 5) {
    float offset = wrap180(float(tick) - display.heading);
    if (std::abs(offset) > halfWindow)
      continue;
    float x = cx + offset * pxPerDegree;
    bool major = tick % 10 == 0;
    line(x, base, x, base + (major ? 14 : 8), display.opacity);
    if (major) {
      int normal = (tick % 360 + 360) % 360;
      const char *c = cardinal(normal);
      char label[4] = {};
      if (!c)
        std::snprintf(label, sizeof(label), "%02d", normal / 10);
      text(x - (c ? 4 : 8), base - 8, c ? c : label, GLUT_BITMAP_8_BY_13,
           display.opacity);
    }
  }
  line(cx - 7, base - 15, cx, base - 7, display.opacity);
  line(cx, base - 7, cx + 7, base - 15, display.opacity);
  box(cx - 22, 12, 44, 20, display.opacity);
  char value[8];
  std::snprintf(value, sizeof(value), "%03d",
                (int(std::lround(display.heading)) + 360) % 360);
  text(cx - 12, 27, value, GLUT_BITMAP_8_BY_13, display.opacity);
}

void verticalTape(float x, int height, float value, float pxPerUnit,
                  bool altitude) {
  const float cy = height * .5f, half = height * .175f, top = cy - half,
              bottom = cy + half, step = altitude ? 5.f : 10.f;
  const int first =
                int(std::floor((value - half / pxPerUnit) / step)) * int(step),
            last =
                int(std::ceil((value + half / pxPerUnit) / step)) * int(step);
  line(x, top, x, bottom, display.opacity);
  for (int tick = first; tick <= last; tick += int(step)) {
    float y = cy - (float(tick) - value) * pxPerUnit;
    if (y < top || y > bottom)
      continue;
    bool major = tick % int(step * 2) == 0;
    float len = major ? 13 : 7;
    line(x - len, y, x, y, display.opacity);
    if (major) {
      char label[12];
      std::snprintf(label, sizeof(label), "%d", tick);
      text(x - len - 8 * std::strlen(label), y + 5, label, GLUT_BITMAP_8_BY_13,
           display.opacity);
    }
  }
  float bw = altitude ? 72 : 64;
  box(x - bw / 2, cy - 12, bw, 24, display.opacity);
  char readout[16];
  if (altitude) {
    std::snprintf(readout, sizeof(readout), "%03d",
                  std::max(0, int(std::lround(value))));
  } else {
    std::snprintf(readout, sizeof(readout), "%02d",
                  std::max(0, int(std::lround(value))));
  }
  text(x - (altitude ? 12.0f : 8.0f), cy + 5, readout, GLUT_BITMAP_8_BY_13,
       display.opacity);
  text(x - (altitude ? 32.0f : 20.0f), top - 14, altitude ? "HEIGHT" : "SPEED",
       GLUT_BITMAP_8_BY_13, display.opacity);
}

void pilotHud(int width, int height) {
  GLint mode;
  glGetIntegerv(GL_MATRIX_MODE, &mode);
  glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LINE_BIT |
               GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  projection(width, height);
  headingTape(width);
  float cx = width * .5f, altitudeX = cx - width * .22f,
        speedX = cx + width * .22f;
  verticalTape(altitudeX, height, display.altitude, 5, true);
  verticalTape(speedX, height, display.speed, 2.1f, false);

  endProjection();
  glMatrixMode(GLenum(mode));
  glPopAttrib();
}
void hudTextPass(int width, int height, float controlsX) {
  GLint mode;
  glGetIntegerv(GL_MATRIX_MODE, &mode);
  glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  projection(width, height);
  text(controlsX + 20, 50, "FLIGHT CONTROLS", GLUT_BITMAP_HELVETICA_18);
  text(controlsX + 20, 82, "W / S  - Forward / Back", GLUT_BITMAP_HELVETICA_18);
  text(controlsX + 20, 110, "A / D  - Left / Right", GLUT_BITMAP_HELVETICA_18);
  text(controlsX + 20, 138, "SPACE  - Ascend", GLUT_BITMAP_HELVETICA_18);
  text(controlsX + 20, 166, "CTRL   - Descend", GLUT_BITMAP_HELVETICA_18);
  text(controlsX + 20, 194, "Q / E  - Turn", GLUT_BITMAP_HELVETICA_18);
  Button b = menuButton(height);
  text(b.x + 18, b.y + 28, "MAIN MENU", GLUT_BITMAP_HELVETICA_18);
  endProjection();
  glMatrixMode(GLenum(mode));
  glPopAttrib();
}
} // namespace

void HUD::reset() { display = {}; }
void HUD::update(const Helicopter &helicopter, float groundHeight, float dt) {
  const float rawHeading = heading(helicopter.headingDegrees()),
              rawAltitude = helicopter.position().y - groundHeight,
              rawSpeed = speed(helicopter.velocity());
  if (!display.ready) {
    display.ready = true;
    display.heading = rawHeading;
    display.altitude = rawAltitude;
    display.speed = rawSpeed;
    display.opacity = rawSpeed < .4f ? .4f : A;
    return;
  }
  float alpha = filter(8, dt);
  display.heading += wrap180(rawHeading - display.heading) * alpha;
  if (display.heading < 0)
    display.heading += 360;
  if (display.heading >= 360)
    display.heading -= 360;
  display.altitude += (rawAltitude - display.altitude) * alpha;
  display.speed += (rawSpeed - display.speed) * alpha;
  float target = rawSpeed < .4f ? .4f : A;
  display.opacity += (target - display.opacity) * filter(5, dt);
}
void HUD::render(Renderer &renderer, const Helicopter &, int width, int height,
                 float) {
  renderer.beginInterface(width, height);
  pilotHud(width, height);
  float controlsX = width - 265;
  rectangle(renderer, controlsX, 20, 245, 190, {.035f, .075f, .13f, .88f});
  Button b = menuButton(height);
  rectangle(renderer, b.x, b.y, b.width, b.height, {.12f, .30f, .45f});
  hudTextPass(width, height, controlsX);
  renderer.endInterface();
}
bool HUD::handleInput(Input &input, int, int height) {
  if (!input.isLeftMouseClicked())
    return false;
  int x, y;
  input.getMousePosition(x, y);
  if (!inside(x, y, menuButton(height)))
    return false;
  input.consumeLeftMouseClick();
  return true;
}
