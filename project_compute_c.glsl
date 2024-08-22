#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 1) buffer vel_u_layout {
  float vel_u[];
};

layout(std430, binding = 2) buffer vel_v_layout {
  float vel_v[];
};

layout(std430, binding = 3) buffer vel_prev_v_layout {
  float p[];
};

layout(std430, binding = 4) buffer vel_prev_u_layout {
  float div[];
};

#define N 100
#define IX(i,j) ((i)+(N+2)*(j))

float project(vec4 value, ivec2 texel_coord) {

  float h = 1.0 / N;

  int x = texel_coord.x;
  int y = texel_coord.y;

  vel_u[IX(x, y)] = vel_u[IX(x, y)] - (0.5 * (p[IX(x + 1, y)] - p[IX(x - 1, y)]) / h);
  vel_v[IX(x, y)] = vel_v[IX(x, y)] - (0.5 * (p[IX(x, y + 1)] - p[IX(x, y - 1)]) / h);

  return vel_u[IX(x, y)];
}


void main() {

  vec4 value = vec4(0.0, 0.0, 0.0, 1.0);

  ivec2 texel_coord = ivec2(gl_GlobalInvocationID.xy);
  
  int arr_coord = IX(texel_coord.x, texel_coord.y);
  value.x = project(value, texel_coord);

  imageStore(img_output, texel_coord, value);
}




