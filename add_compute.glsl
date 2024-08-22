#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 1) buffer src_layout {
  float src_density[];
};

layout(std430, binding = 2) buffer dest_layout {
  float dest_density[];
};

layout(std430, binding = 3) buffer array_three {
  float arr3[];
};

layout(std430, binding = 4) buffer array_four {
  float arr4[];
};

layout(location = 5) uniform float dt;
layout(location = 6) uniform float diff;


#define N 200
#define IX(i,j) ((i)+(N+2)*(j))

float add_source(vec4 value, ivec2 texel_coord, float dt) {
  int arr_coord = IX(texel_coord.x, texel_coord.y);
  dest_density[arr_coord] = dest_density[arr_coord] + src_density[arr_coord] * dt;
  return dest_density[arr_coord];
}

void main() {

  vec4 value = vec4(0.0, 0.0, 0.0, 1.0);

  ivec2 texel_coord = ivec2(gl_GlobalInvocationID.xy);
  texel_coord += ivec2(1);

  int arr_coord = IX(texel_coord.x, texel_coord.y);
  value.x = add_source(value, texel_coord, dt);
}




