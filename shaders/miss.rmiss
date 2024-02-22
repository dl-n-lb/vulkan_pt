#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT ray_payload payload;

layout(binding = 5, set = 0) uniform sampler2D hdri;

#define PI 3.141592653589
#define _2PI (2.0 * PI)
#define INV_PI (1.0 / PI)
#define INV_2PI (1.0 / _2PI)

void main() {
  vec2 uv = vec2((PI + atan(payload.rd.z, payload.rd.x)) * INV_2PI,
		 acos(payload.rd.y) * INV_PI);
  
  payload.stop = true;
  payload.hit_light = true;
  payload.attenuated_colour *= pow(texture(hdri, uv).xyz, vec3(0.5));
}
