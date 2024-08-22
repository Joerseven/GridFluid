#include "raylib.h"
#include "rlgl.h"
#include "solver.h"
#include "raymath.h"
#include "glad.h"

#include <algorithm>
#include <array>
#include <memory>

constexpr int N = 200;
constexpr int fl_array_size = (N + 2) * (N + 2);
constexpr int scale_factor = 4;

typedef std::unique_ptr<std::array<float, fl_array_size>> float_array_ptr;


constexpr inline int fl_index(int x, int y) {
  return ((x) + (N + 2) * (y));
}

Color floatToColor(float number) {
  if (number > 1) number = 1;
  return { (unsigned char)(0 * 255), (unsigned char)(0 * 255), (unsigned char)(number * 255), (unsigned char)255};
}

std::ostream& operator<<(std::ostream& stream, const Color& color) {
  stream << (int)color.r << ", " << (int)color.g << ", " << (int)color.b << ", " << (int)color.a;
  return stream;
}

void add_liquid_point(const float_array_ptr& dest, int x, int y, int radius) {
  for (int i = x - radius; i < x + radius; i++) {
    for (int j = y - radius; j < y + radius; j++) {
      (*dest)[fl_index(i, j)] += 50.0f;
    }
  }
}

void add_force(const float_array_ptr& dest, int x, int y, int magnitude) {
  (*dest)[fl_index(x, y)] += magnitude;
}

unsigned int load_compute_shader(const char* filename) {
  char* file = LoadFileText(filename);
  unsigned int shader = rlCompileShader(file, RL_COMPUTE_SHADER);
  unsigned int program = rlLoadComputeShaderProgram(shader);
  UnloadFileText(file);
  return program;
}

void run_compute_shader(unsigned int shader_id, unsigned int buffer1, unsigned int buffer2, unsigned int buffer3, unsigned int buffer4, float* float1, float* float2, int boundary, int iterations) {

  rlEnableShader(shader_id);

  rlBindShaderBuffer(buffer1, 1);
  rlBindShaderBuffer(buffer2, 2);
  rlBindShaderBuffer(buffer3, 3);
  rlBindShaderBuffer(buffer4, 4);

  rlSetUniform(5, float1, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(6, float2, RL_SHADER_UNIFORM_FLOAT, 1);
  
  for (int i = 0; i < iterations; i++) {
    rlComputeShaderDispatch(N, N, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}

int main (int argc, char *argv[]) {

  float_array_ptr vely = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr velx = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_velx = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_vely = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr density = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr prev_density = std::make_unique<std::array<float, fl_array_size>>();
  float_array_ptr contents = std::make_unique<std::array<float, fl_array_size>>();
  contents->fill(0.0);
  density->fill(0.0);
  prev_velx->fill(0.0);
  prev_vely->fill(0.0);

  auto s = Solver();

  InitWindow((N+2) * scale_factor, (N+2) * scale_factor, "Fluid Window!");
  SetTargetFPS(60);
  
  Texture2D density_texture = {
    rlLoadTexture(density->data(), N + 2, N + 2, PIXELFORMAT_UNCOMPRESSED_R32, 1),
    N + 2,
    N + 2,
    1,
    PIXELFORMAT_UNCOMPRESSED_R32
  };

  int ist;
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &ist);

  Texture2D compute_texture = {
    rlLoadTexture(nullptr, (N + 2), (N + 2), PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1),
    N+2,
    N+2,
    1,
    RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32
  };

  Texture2D boundaries = LoadTexture("map.png");

  // Bind texture

  rlBindImageTexture(compute_texture.id, 0, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
  rlBindImageTexture(boundaries.id, 7, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, true);

  // Create buffers

  unsigned int fluid_density_current = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_density_previous = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_u = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_v = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_up = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_vp = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);



  Vector2 first_pressed = Vector2{};

  // Load Diffuse Compute


  rlUpdateShaderBuffer(fluid_density_previous, density->data(), fl_array_size * sizeof(float), 0
  );

  unsigned int diffuse_compute_program = load_compute_shader("fluid_compute.glsl");
  unsigned int advect_compute_program = load_compute_shader("advection_compute.glsl");
  unsigned int add_compute_program = load_compute_shader("add_compute.glsl");
  unsigned int project_compute_program_a = load_compute_shader("project_compute_a.glsl");
  unsigned int project_compute_program_b = load_compute_shader("project_compute_b.glsl");
  unsigned int project_compute_program_c = load_compute_shader("project_compute_c.glsl");

  float diff = 0.0003f;
  float visc = 0.0f;


  while (!WindowShouldClose()) {

    float dt = GetFrameTime();

    prev_velx->fill(0.0);
    prev_vely->fill(0.0);
    prev_density->fill(0.0);
    rlUpdateShaderBuffer(fluid_density_previous, prev_density->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_up, prev_velx->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_vp, prev_vely->data(), fl_array_size * sizeof(float), 0);


    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      add_liquid_point(prev_density, GetMousePosition().x / scale_factor, GetMousePosition().y / scale_factor, 1);
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      first_pressed = GetMousePosition();
      std::cout << first_pressed.x << first_pressed.y << std::endl;
    }

    if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
      Vector2 force = Vector2Scale(Vector2Subtract(first_pressed, GetMousePosition()), -80);
      std::cout << force.x << force.y << std::endl;
      add_force(prev_velx, first_pressed.x / scale_factor, first_pressed.y / scale_factor, force.x);
      add_force(prev_vely, first_pressed.x / scale_factor, first_pressed.y / scale_factor, force.y);
    }

    rlUpdateShaderBuffer(fluid_density_previous, prev_density->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_up, prev_velx->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_vp, prev_vely->data(), fl_array_size * sizeof(float), 0);

    // Velocity Step

    // Add sources

    run_compute_shader(add_compute_program,
      fluid_velocity_up,
      fluid_velocity_u,
      0,
      0,
      &dt,
      nullptr, 0, 1);

    run_compute_shader(add_compute_program,
      fluid_velocity_vp,
      fluid_velocity_v,
      0,
      0,
      &dt,
      nullptr, 0, 1);

    // Diffuse

    run_compute_shader(diffuse_compute_program,
      fluid_velocity_u,
      fluid_velocity_up,
      0,
      0,
      &dt,
      &visc, 1, 20);

    run_compute_shader(diffuse_compute_program,
      fluid_velocity_v,
      fluid_velocity_vp,
      0,
      0,
      &dt,
      &visc, 2, 20);

    // Project

    run_compute_shader(project_compute_program_a,
      fluid_velocity_up,
      fluid_velocity_vp,
      fluid_velocity_u,
      fluid_velocity_v,
      nullptr,
      nullptr, 0, 1);

    run_compute_shader(project_compute_program_b,
      fluid_velocity_up,
      fluid_velocity_vp,
      fluid_velocity_u,
      fluid_velocity_v,
      nullptr,
      nullptr, 0, 20);

    run_compute_shader(project_compute_program_c,
      fluid_velocity_up,
      fluid_velocity_vp,
      fluid_velocity_u,
      fluid_velocity_v,
      nullptr,
      nullptr, 0, 1);

    // Advect

    run_compute_shader(advect_compute_program,
      fluid_velocity_up,
      fluid_velocity_u,
      fluid_velocity_up,
      fluid_velocity_vp,
      &dt,
      &visc, 1, 1);

    run_compute_shader(advect_compute_program,
      fluid_velocity_vp,
      fluid_velocity_v,
      fluid_velocity_up,
      fluid_velocity_vp,
      &dt,
      &visc, 2, 1);

    // Project

    run_compute_shader(project_compute_program_a,
      fluid_velocity_u,
      fluid_velocity_v,
      fluid_velocity_up,
      fluid_velocity_vp,
      nullptr,
      nullptr, 0, 1);

    run_compute_shader(project_compute_program_b,
      fluid_velocity_u,
      fluid_velocity_v,
      fluid_velocity_up,
      fluid_velocity_vp,
      nullptr,
      nullptr, 0, 20);

    run_compute_shader(project_compute_program_c,
      fluid_velocity_up,
      fluid_velocity_vp,
      fluid_velocity_u,
      fluid_velocity_v,
      nullptr,
      nullptr, 0, 1);

    // Density Step


    // Add Source

    run_compute_shader(add_compute_program,
      fluid_density_previous,
      fluid_density_current,
      0,
      0,
      &dt,
      nullptr, 0, 1);

    //Diffuse

    run_compute_shader(diffuse_compute_program,
      fluid_density_current,
      fluid_density_previous,
      0,
      0,
      &dt,
      &diff, 0, 20);

    // Advect

    run_compute_shader(advect_compute_program,
      fluid_density_previous,
      fluid_density_current,
      fluid_velocity_u,
      fluid_velocity_v,
      &dt,
      &diff, 0, 1);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    DrawTexturePro(compute_texture, { 0,0,N + 2,N + 2 }, { 0,0,(N + 2) * scale_factor, (N + 2) * scale_factor }, { 0,0 }, 0, RAYWHITE);

    DrawText(TextFormat("FPS: %i", (int)(1.0f / dt)), 40, 40, 20, GREEN);
    DrawText(TextFormat("Left click to add dye"), 40, 70, 20, GREEN);
    DrawText(TextFormat("Right click, hold and release to add forces at a point"), 40, 100, 20, GREEN);


    EndDrawing();
  }

  CloseWindow();
  return 0;
}


