#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#ifndef GLSL_STRUCTS
#define GLSL_STRUCTS
struct geometry_node {
  uint64_t vertex_buffer_address;
  uint64_t index_buffer_address;
  uint material_index;
};

struct material_t {
  vec3 col;
  uint texture_index;
  uint normal_map_index;
};

struct vertex_t {
  vec3 pos; vec3 norm; vec2 uv;
};

struct triangle_t {
  vertex_t vertices[3];
};

struct ray_payload {
  vec3 ro;
  vec3 rd;
  vec3 attenuated_colour;
  uint depth;
  uint seed;
  bool stop;
  bool hit_light;
};

#endif // GLSL_STRUCTS
