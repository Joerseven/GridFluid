#include "raylib.h"
#include "solver.h"
#include "raymath.h"
#include <algorithm>

constexpr int width = 800;
constexpr int height = 400;

constexpr float *GetPixelLocation(float *array, int x, int y) {
  return &array[width * x + y];
}

Color floatToColor(float number) {
  return { (unsigned char)(number * 255), (unsigned char)(0 * 255), (unsigned char)(0 * 255), (unsigned char)255};
}

std::ostream& operator<<(std::ostream& stream, const Color& color) {
  stream << (int)color.r << ", " << (int)color.g << ", " << (int)color.b << ", " << (int)color.a;
  return stream;
}

void add_liquid(int x, int y, float* density) {
  for (int i=0; i<width*height; i++) {
    Vector2 position = { (float)(i % width), (float)(i / width) };
    if (Vector2Distance(position, {(float)x,(float)y}) < 10) {
      density[i] += 1.0f;
      density[i] = std::min(density[i], 1.0f);
    }
  }
}

void update_velocity(float* vely, float dt) {
  for (int i=0; i<width*height; i++) {
    const float gravity = -9.81f;
  }
}

int main (int argc, char *argv[]) {

  float vely[width * height] = { 0.0 };
  float velx[width * height] = { 0.0 };
  float density[width * height] = { 0.0 };

  for (int i = 0; i < width * height; i++) {
    int row = i / width;
    int column = i % width;
    density[i] = 0.0;
  }

  auto s = Solver();

  InitWindow(800, 400, "Fluid Window!");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      add_liquid(GetMouseX(), GetMouseY(), density);
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    for (int i=0; i<width; i++) {
      for (int j=0; j<height; j++) {
        float color = *GetPixelLocation(density, j, i);
        DrawPixel(i, j, floatToColor(color));
      }
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
