#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT ray_payload payload;

void main() {

  payload.stop = true;
  payload.attenuated_colour *= vec3(0);
  /*
  if (payload.depth == 0) {
    payload.stop = true;
    payload.attenuated_colour = vec3(0);
  }
  vec3 light_position = vec3(20, 20, 20);
  float light_dist = length(light_position - payload.ro)/10;
  vec3 light_color = vec3(5) * 1.0 / (light_dist * light_dist);
  float light_contrib = max(dot(normalize(light_position), gl_WorldRayDirectionEXT), 0.0001);

  payload.attenuated_colour *= vec3(light_color * light_contrib);
  payload.stop = true;
  */
}
