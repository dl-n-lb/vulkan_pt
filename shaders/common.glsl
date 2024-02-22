struct ray_payload {
  vec3 ro;
  vec3 rd;
  vec3 attenuated_colour;
  uint depth;
  uint seed;
  bool stop;
  bool hit_light;
};
