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

layout(rgba8, binding = 7) uniform image2D border_input;

//void set_boundaries(ivec2 texel_coord) {
//
//
//
//  int arr_coord = IX(texel_coord.x, texel_coord.y);
//  int arr_coord_yu = IX(texel_coord.x, texel_coord.y + 1);
//  int arr_coord_yd = IX(texel_coord.x, texel_coord.y - 1);
//  int arr_coord_xr = IX(texel_coord.x + 1, texel_coord.y);
//  int arr_coord_xl = IX(texel_coord.x - 1, texel_coord.y);
//
//  if (imageLoad(border_input, arr_coord_yu, value).a > 0)
//}


#define N 400
#define IX(i,j) ((i)+(N+2)*(j))

void border(ivec2 texel_coord) {

  ivec2 d = texel_coord + ivec2(0, -1);
  ivec2 u = texel_coord + ivec2(0, 1);
  ivec2 l = texel_coord + ivec2(1, 0);
  ivec2 r = texel_coord + ivec2(-1, 0);


  float down = imageLoad(border_input, d).a;
  float up = imageLoad(border_input, u).a;
  float left = imageLoad(border_input, l).a;
  float right = imageLoad(border_input, r).a;

  if (down > 0.8) dest_density[IX(d.x, d.y)] = dest_density[IX(texel_coord.x, texel_coord.y)];
  if (up > 0.8) dest_density[IX(u.x, u.y)] = dest_density[IX(texel_coord.x, texel_coord.y)];
  if (left > 0.8) dest_density[IX(l.x, l.y)] = dest_density[IX(texel_coord.x, texel_coord.y)];
  if (right > 0.8) dest_density[IX(r.x, r.y)] = dest_density[IX(texel_coord.x, texel_coord.y)];
}

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

  texel_coord += ivec2(1);
  

  int arr_coord = IX(texel_coord.x, texel_coord.y);

  value.x = diffuse(value, texel_coord, diff, dt);
  border(texel_coord);

  imageStore(img_output, texel_coord, value);
}




