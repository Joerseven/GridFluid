#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 1) buffer src_layout {
  float src_density[];
};

layout(std430, binding = 2) buffer dest_layout {
  float dest_density[];
};

layout(std430, binding = 3) buffer uvel_layout {
  float vel_u[];
};

layout(std430, binding = 4) buffer vvel_layout {
  float vel_v[];
};

layout(location = 5) uniform float dt;
layout(location = 6) uniform float diff;


#define N 400
#define IX(i,j) ((i)+(N+2)*(j))

float advect(vec4 value, ivec2 texel_coord, float diff, float dt) {

  int arr_coord = IX(texel_coord.x, texel_coord.y);

  int i0, j0, i1, j1;
  float s0, t0, s1, t1, dt0;
  dt0 = dt * N;
  float x = texel_coord.x - dt0 * vel_u[arr_coord];
  float y = texel_coord.y - dt0 * vel_v[arr_coord];
  if (x < 0.5) x = 0.5;
  if (x > N + 0.5) x = N + 0.5;
  i0 = int(x);
  i1 = i0 + 1;
  if (y < 0.5) y = 0.5;
  if (y > N + 0.5) y = N + 0.5;
  j0 = int(y);
  j1 = j0 + 1;
  s1 = x - i0;
  s0 = 1 - s1;
  t1 = y - j0;
  t0 = 1 - t1;
  dest_density[arr_coord] = s0 * (t0 * src_density[IX(i0, j0)] + t1 * src_density[IX(i0, j1)]) + s1 * (t0 * src_density[IX(i1, j0)] + t1 * src_density[IX(i1, j1)]);
  return dest_density[arr_coord];
}


void main() {
  vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
  ivec2 texel_coord = ivec2(gl_GlobalInvocationID.xy);
  value.x = advect(value, texel_coord, diff, dt);
}




