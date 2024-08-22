#include "raylib.h"
#include "rlgl.h"
#include "solver.h"
#include "raymath.h"
#include "glad.h"

#include <algorithm>
#include <array>
#include <memory>

constexpr int N = 400;
constexpr int fl_array_size = (N + 2) * (N + 2);
constexpr int scale_factor = 3;

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

void add_source(const float_array_ptr& dest, const float_array_ptr& src, float dt) {

  //dest = current
  // src = previous
  for (int i = 0; i < fl_array_size; i++) {
    (*dest)[i] += dt * (*src)[i];
  }
}

void set_boundary(int width, int b, const float_array_ptr& data) {
  for (int i = 1; i <= width; i++) {
    (*data)[fl_index(0, i)] = b == 1 ? -(*data)[fl_index(1, i)] : (*data)[fl_index(1, i)];
    (*data)[fl_index(width + 1, i)] = b == 1 ? -(*data)[fl_index(width, i)] : (*data)[fl_index(width, i)];
    (*data)[fl_index(i, 0)] = b == 2 ? -(*data)[fl_index(i, 1)] : (*data)[fl_index(i, 1)];
    (*data)[fl_index(i, width + 1)] = b == 2 ? -(*data)[fl_index(i, width)] : (*data)[fl_index(i, width)];
  }
  (*data)[fl_index(0, 0)] = 0.5f * ((*data)[fl_index(1, 0)] + (*data)[fl_index(0, 1)]);
  (*data)[fl_index(0, width + 1)] = 0.5f * ((*data)[fl_index(1, width + 1)] + (*data)[fl_index(0, width)]);
  (*data)[fl_index(width + 1, 0)] = 0.5f * ((*data)[fl_index(width, 0)] + (*data)[fl_index(width + 1, 1)]);
  (*data)[fl_index(width + 1, width + 1)] = 0.5f * ((*data)[fl_index(1, 0)] + (*data)[fl_index(0, 1)]);
}

void diffuse(int b, const float_array_ptr& dest, const float_array_ptr& src, float diff, float dt) {

  //dest = previous
  //src = current

  float a = dt * diff * N * N;

  for (int k = 0;k < 20;k++) {
    for (int i = 1;i <= N;i++) {
      for (int j = 1; j <= N; j++) {
        (*dest)[fl_index(i, j)] = ((*src)[fl_index(i, j)] + a * ((*dest)[fl_index(i - 1, j)] + (*dest)[fl_index(i + 1, j)] + (*dest)[fl_index(i, j - 1)] + (*dest)[fl_index(i, j + 1)])) / (1 + 4 * a);
      }
    }
    set_boundary(N, b, dest);
  }
}

void advection(int b, const float_array_ptr& den, const float_array_ptr& prev_den, const float_array_ptr& u, const float_array_ptr& v, float dt) {
  int i0, j0, i1, j1;
  float s0, t0, s1, t1, dt0;
  dt0 = dt * N;
  for (int i = 1; i <= N; i++) {
    for (int j = 1; j <= N; j++) {
      float x = i - dt0 * (*u)[fl_index(i, j)];
      float y = j - dt0 * (*v)[fl_index(i, j)];
      if (x < 0.5) x = 0.5; 
      if (x > N + 0.5) x = N + 0.5;
      i0 = (int)x;
      i1 = i0 + 1;
      if (y < 0.5) y = 0.5;
      if (y > N + 0.5) y = N + 0.5;
      j0 = (int)y;
      j1 = j0 + 1;
      s1 = x - i0;
      s0 = 1 - s1;
      t1 = y - j0;
      t0 = 1 - t1;
      (*den)[fl_index(i, j)] = s0 * (t0 * (*prev_den)[fl_index(i0, j0)] + t1 * (*prev_den)[fl_index(i0, j1)]) + s1 * (t0 * (*prev_den)[fl_index(i1, j0)] + t1 * (*prev_den)[fl_index(i1, j1)]);
    }
  }
  set_boundary(N, b, den);
}

void project(const float_array_ptr& u, const float_array_ptr& v, const float_array_ptr& p, const float_array_ptr& div) {
  float h = 1.0 / N;
  for (int i = 1; i <= N; i++) {
    for (int j = 1; j <= N;j++) {
      (*div)[fl_index(i, j)] = -0.5 * h * ((*u)[fl_index(i + 1, j)] - (*u)[fl_index(i - 1, j)] + (*v)[fl_index(i, j + 1)] - (*v)[fl_index(i, j - 1)]);
      (*p)[fl_index(i, j)] = 0;
    }
  }
  set_boundary(N, 0, div);
  set_boundary(N, 0, p);

  for (int k = 0; k < 20; k++) {
    for (int i = 1; i <= N;i++) {
      for (int j = 1; j <= N;j++) {
        (*p)[fl_index(i, j)] = ((*div)[fl_index(i, j)] + (*p)[fl_index(i - 1, j)] + (*p)[fl_index(i + 1, j)] + (*p)[fl_index(i, j - 1)] + (*p)[fl_index(i, j + 1)]) / 4.0;
      }
    }
    set_boundary(N, 0, p);
  }

  for (int i = 1; i <= N; i++) {
    for (int j = 1;j <= N;j++) {
      (*u)[fl_index(i, j)] -= 0.5 * ((*p)[fl_index(i + 1, j)] - (*p)[fl_index(i - 1, j)]) / h;
      (*v)[fl_index(i, j)] -= 0.5 * ((*p)[fl_index(i, j + 1)] - (*p)[fl_index(i, j - 1)]) / h;
    }
  }

  set_boundary(N, 1, u);
  set_boundary(N, 2, v);
}

void density_step(const float_array_ptr& dest, const float_array_ptr& src, const float_array_ptr& u, const float_array_ptr& v, float diff, float dt, unsigned int diffuse_shader, int diffuse_buffer) {
  add_source(dest, src, dt);
  std::swap(*dest, *src);
  diffuse(0, dest, src, diff, dt);
  std::swap(*dest, *src);
  //advection(0, dest, src, u, v, dt);
}

void velocity_step(const float_array_ptr& u, const float_array_ptr& v, const float_array_ptr& prev_u, const float_array_ptr& prev_v, float visc, float dt, unsigned int diffuse_shader, int diffuse_buffer) {
  add_source(u, prev_u, dt);
  add_source(v, prev_v, dt);
  std::swap(*prev_u, *u);
  diffuse(1, u, prev_u, visc, dt);
  std::swap(*prev_v, *v);
  diffuse(2, v, prev_v, visc, dt);
  project(u, v, prev_u, prev_v);
  std::swap(*prev_u, *u);
  std::swap(*prev_v, *v);
  advection(1, u, prev_u, prev_u, prev_v, dt);
  advection(2, v, prev_v, prev_u, prev_v, dt);
  project(u, v, prev_u, prev_v);
}

void add_liquid_point(const float_array_ptr& dest, int x, int y, int radius) {
  for (int i = x - radius; i < x + radius; i++) {
    for (int j = y - radius; j < y + radius; j++) {
      (*dest)[fl_index(i, j)] += 100.0f;
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

  // Bind texture

  rlBindImageTexture(compute_texture.id, 0, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);

  // Create buffers

  unsigned int fluid_density_current = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_density_previous = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_u = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_v = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_up = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);
  unsigned int fluid_velocity_vp = rlLoadShaderBuffer(fl_array_size * sizeof(float), NULL, RL_DYNAMIC_COPY);



  Vector2 first_pressed = Vector2{};

  // Load Diffuse Compute


  rlUpdateShaderBuffer(fluid_density_previous, density->data(), fl_array_size * sizeof(float), 0);

  unsigned int diffuse_compute_program = load_compute_shader("fluid_compute.glsl");
  unsigned int advect_compute_program = load_compute_shader("advection_compute.glsl");
  unsigned int add_compute_program = load_compute_shader("add_compute.glsl");
  unsigned int project_compute_program_a = load_compute_shader("project_compute_a.glsl");
  unsigned int project_compute_program_b = load_compute_shader("project_compute_b.glsl");
  unsigned int project_compute_program_c = load_compute_shader("project_compute_c.glsl");


  while (!WindowShouldClose()) {

    float dt = GetFrameTime();

    prev_velx->fill(0.0);
    prev_vely->fill(0.0);
    prev_density->fill(0.0);
    rlUpdateShaderBuffer(fluid_density_previous, prev_density->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_up, prev_velx->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_vp, prev_vely->data(), fl_array_size * sizeof(float), 0);


    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      add_liquid_point(prev_density, N/2, N/2, 2);
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

    float diff = 0.0001f;
    float visc = 0.0f;
    
    //velocity_step(velx, vely, prev_velx, prev_vely, 0, dt, diffuse_compute_program, fluid_sbo);
    //density_step(density, prev_density, velx, vely, 0.001f, dt, diffuse_compute_program, fluid_sbo);

    rlUpdateShaderBuffer(fluid_density_previous, prev_density->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_up, prev_velx->data(), fl_array_size * sizeof(float), 0);
    rlUpdateShaderBuffer(fluid_velocity_vp, prev_vely->data(), fl_array_size * sizeof(float), 0);

    // Velocity Step

    // Add sources

    rlEnableShader(add_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_u, 2);
    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rlEnableShader(add_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 1);
    rlBindShaderBuffer(fluid_velocity_v, 2);
    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Diffuse

    rlEnableShader(diffuse_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(4, &visc, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_u, 1);
    rlBindShaderBuffer(fluid_velocity_up, 2);

    for (int i = 0; i < 20; i++) {
      rlComputeShaderDispatch(N + 2, N + 2, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rlEnableShader(diffuse_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(4, &visc, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_v, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 2);

    for (int i = 0; i < 20; i++) {
      rlComputeShaderDispatch(N + 2, N + 2, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Project

    rlEnableShader(project_compute_program_a);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 2);
    rlBindShaderBuffer(fluid_velocity_u, 3);
    rlBindShaderBuffer(fluid_velocity_v, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rlEnableShader(project_compute_program_b);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 2);
    rlBindShaderBuffer(fluid_velocity_u, 3);
    rlBindShaderBuffer(fluid_velocity_v, 4);

    for (int i = 0; i < 20; i++) {
      rlComputeShaderDispatch(N + 2, N + 2, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rlEnableShader(project_compute_program_c);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 2);
    rlBindShaderBuffer(fluid_velocity_u, 3);
    rlBindShaderBuffer(fluid_velocity_v, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Advect

    rlEnableShader(advect_compute_program);

    rlSetUniform(5, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(6, &visc, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_u, 2);
    rlBindShaderBuffer(fluid_velocity_up, 3);
    rlBindShaderBuffer(fluid_velocity_vp, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rlEnableShader(advect_compute_program);

    rlSetUniform(5, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(6, &visc, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 1);
    rlBindShaderBuffer(fluid_velocity_v, 2);
    rlBindShaderBuffer(fluid_velocity_up, 3);
    rlBindShaderBuffer(fluid_velocity_vp, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Project

    rlEnableShader(project_compute_program_a);
    rlBindShaderBuffer(fluid_velocity_u, 1);
    rlBindShaderBuffer(fluid_velocity_v, 2);
    rlBindShaderBuffer(fluid_velocity_up, 3);
    rlBindShaderBuffer(fluid_velocity_vp, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rlEnableShader(project_compute_program_b);
    rlBindShaderBuffer(fluid_velocity_u, 1);
    rlBindShaderBuffer(fluid_velocity_v, 2);
    rlBindShaderBuffer(fluid_velocity_up, 3);
    rlBindShaderBuffer(fluid_velocity_vp, 4);

    for (int i = 0; i < 20; i++) {
      rlComputeShaderDispatch(N + 2, N + 2, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rlEnableShader(project_compute_program_c);
    rlBindShaderBuffer(fluid_velocity_up, 1);
    rlBindShaderBuffer(fluid_velocity_vp, 2);
    rlBindShaderBuffer(fluid_velocity_u, 3);
    rlBindShaderBuffer(fluid_velocity_v, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Density Step


    // Add Source

    rlEnableShader(add_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_density_previous, 1);
    rlBindShaderBuffer(fluid_density_current, 2);
    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    

    //Diffuse

    rlEnableShader(diffuse_compute_program);
    rlSetUniform(3, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(4, &diff, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_density_current, 1);
    rlBindShaderBuffer(fluid_density_previous, 2);

    for (int i = 0; i < 20; i++) {
      rlComputeShaderDispatch(N + 2, N + 2, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Advect

    rlEnableShader(advect_compute_program);

    rlSetUniform(5, &dt, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(6, &diff, RL_SHADER_UNIFORM_FLOAT, 1);
    rlBindShaderBuffer(fluid_density_previous, 1);
    rlBindShaderBuffer(fluid_density_current, 2);
    rlBindShaderBuffer(fluid_velocity_u, 3);
    rlBindShaderBuffer(fluid_velocity_v, 4);

    rlComputeShaderDispatch(N + 2, N + 2, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    DrawTexturePro(compute_texture, { 0,0,N + 2,N + 2 }, { 0,0,(N + 2) * scale_factor, (N + 2) * scale_factor }, { 0,0 }, 0, RAYWHITE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}


