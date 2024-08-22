#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 1) buffer src_layout {
  float src_density[];
};

layout(std430, binding = 2) buffer dest_layout {
  float dest_density[];
};

layout(location = 3) uniform float dt;
layout(location = 4) uniform float diff;


#define N 400
#define IX(i,j) ((i)+(N+2)*(j))

float diffuse(vec4 value, ivec2 texel_coord, float diff, float dt) {

  float a = dt * diff * N * N;

  int arr_coord = IX(texel_coord.x, texel_coord.y);
  int arr_coord_yu = IX(texel_coord.x, texel_coord.y + 1);
  int arr_coord_yd = IX(texel_coord.x, texel_coord.y - 1);
  int arr_coord_xr = IX(texel_coord.x + 1, texel_coord.y);
  int arr_coord_xl = IX(texel_coord.x - 1, texel_coord.y);

  dest_density[arr_coord] = (src_density[arr_coord] + a * (dest_density[arr_coord_yu] + dest_density[arr_coord_yd] + dest_density[arr_coord_xr] + dest_density[arr_coord_xl])) / (1 + 4 * a);

  return dest_density[arr_coord];
}


void main() {

  vec4 value = vec4(0.0, 0.0, 0.0, 1.0);

  ivec2 texel_coord = ivec2(gl_GlobalInvocationID.xy);
  

  int arr_coord = IX(texel_coord.x, texel_coord.y);
  value.x = diffuse(value, texel_coord, diff, dt);
  //value.x = dest_density[arr_coord];

  imageStore(img_output, texel_coord, value);
}




