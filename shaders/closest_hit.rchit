#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2    : require
#extension GL_EXT_scalar_block_layout  : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT ray_payload payload;

hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;
// TODO: light bvh?
//layout(binding = 1, set = 0) uniform accelerationStructureEXT light_tlas;

struct geometry_node {
  uint64_t vertex_buffer_address;
  uint64_t index_buffer_address;
  uint material_index;
};

layout(binding = 2, set = 0) buffer geometry_nodes_t {
  geometry_node nodes[];
} geometry_nodes;

struct material_t {
  vec3 col;
  uint texture_index;
  uint normal_map_index;
};

layout(binding = 3, set = 0) buffer materials_t {
  material_t mats[];
} materials;

layout(binding = 4, set = 0) uniform sampler2D textures[];

material_t get_material(geometry_node geom) {
  uint index = geom.material_index;
  return materials.mats[index];
}

#include "buffer_references.glsl"
#include "random.glsl"
#include "geometry.glsl"

void main() {
  payload.depth += 1;
  triangle_t tri = unpack_triangle(gl_PrimitiveID, 2); // vertex is 2 vec4s
  // TODO: fix this
  vertex_t v0 = tri.vertices[0];
  vertex_t v1 = tri.vertices[1];
  vertex_t v2 = tri.vertices[2];

  uint geom_index = gl_InstanceID + gl_GeometryIndexEXT;

  // get the properties of the current point on the triangle
  vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  // normal calculation now based on a texture read as well
  vec3 norm = v0.norm * bary.x + v1.norm * bary.y + v2.norm * bary.z;
  vec2 uv = v0.uv * bary.x + v1.uv * bary.y + v2.uv * bary.z;

  // create an onb for the triangle to calculate the normal map
  vec3 up = (abs(norm.z) < 0.9) ? vec3(0, 0, 1) : vec3(1, 0, 0);
  vec3 tx = normalize(cross(up, norm));
  vec3 ty = cross(norm, tx);
  mat3 frame = mat3(tx, ty, norm);
  
  // TODO: if statements...
  // read normal from normal map
  material_t material = get_material(geometry_nodes.nodes[geom_index]);
  
  vec3 n_col;
  if (material.normal_map_index == 0xFFFFFFFF) {
    n_col = vec3(0, 0, 1);
  } else {
    n_col = 2.0*texture(textures[nonuniformEXT(material.normal_map_index)], uv).rgb - 1.0;
    //n_col = normalize(n_col); // maybe not needed ?
  }
  
  norm = n_col * transpose(frame); // acts as an inverse

  // new onb given the normal map (is there a better way)
  // we don't need to recalculate up (probably)? 
  up = (abs(norm.z) < 0.99) ? vec3(0, 0, 1) : vec3(1, 0, 0);
  tx = normalize(cross(up, norm));
  ty = cross(norm, tx);
  frame = mat3(tx, ty, norm);
  
  
  vec3 material_colour;
  // TODO: we don't like if statements here
  if (material.texture_index == 0xFFFFFFFF) {
    material_colour = material.col;
  } else {
    material_colour = texture(textures[nonuniformEXT(material.texture_index)], uv).rgb;
  }

  // light source (TEMP)
  // TODO: as above...
  if (geometry_nodes.nodes[geom_index].material_index == 1) {
    //payload.attenuated_colour *= material_colour;
    payload.attenuated_colour *= vec3(20);
    payload.stop = true;
    payload.hit_light = true;
  } else {
    // pick the direction of a light to head towards
    payload.rd = frame * cosine_sample_hemisphere(payload.seed);
    //payload.rd = gl_WorldRayDirectionEXT + 2 * interp_normal; // reflected direction
    payload.ro = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    //payload.ro += 0.01 * payload.rd;

    payload.attenuated_colour *= material_colour;
  }
}
