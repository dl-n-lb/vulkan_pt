#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2    : require
#extension GL_EXT_scalar_block_layout  : require

#include "common/structs.glsl"
#include "common/random.glsl"
#include "common/utils.glsl"
#include "buffer_references.glsl"
#include "common/geometry.glsl"

layout(location = 0) rayPayloadInEXT ray_payload payload;
hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(binding = 2, set = 0) buffer geometry_nodes_t {
  geometry_node nodes[];
} geometry_nodes;

layout(binding = 3, set = 0) buffer materials_t {
  material_t mats[];
} materials;

layout(binding = 4, set = 0) uniform sampler2D textures[];

// NOTE: binding = 5 is the HDRI image

struct light_info {
  uint idx;
  uint tri_count;
};

// lights are indices into the geometry nodes
layout(binding = 6, set = 0) buffer light_nodes_t {
  light_info nodes[]; // nodes[0].idx the size of the array
} light_nodes;
// TODO: light bvh?
//layout(binding = 6, set = 0) uniform accelerationStructureEXT light_tlas;

material_t get_material(geometry_node geom) {
  uint index = geom.material_index;
  return materials.mats[index];
}

vec2 uniform_sample_triangle(triangle_t tri) {
  float r1 = rand(payload.seed);
  float r2 = rand(payload.seed);
  float r = sqrt(r2);
  float u = 1.0 - r;
  float v = r1 * r;
  return vec2(u, v);
}

vec3 get_position_triangle(triangle_t tri, vec3 bary) {
  return bary_interp(tri.vertices[0].pos, tri.vertices[1].pos, tri.vertices[2].pos, bary);
}

float get_triangle_area(triangle_t tri) {
  vec3 e1 = tri.vertices[1].pos - tri.vertices[0].pos;
  vec3 e2 = tri.vertices[2].pos - tri.vertices[0].pos;
  return 0.5 * length(cross(e1, e2));
}

#define PI 3.141592653589
#define _2PI (2.0 * PI)
#define INV_PI (1.0 / PI)
#define INV_2PI (1.0 / _2PI)

void try_sample_light() {
  uint lights_count = light_nodes.nodes[0].idx - 1;
  light_info light = light_nodes.nodes[(lcg(payload.seed) % lights_count) + 1];

  geometry_node light_node = geometry_nodes.nodes[light.idx];

  // get a random triangle in the mesh
  triangle_t tri = unpack_triangle(light_node, lcg(payload.seed) % light.tri_count);

  vec3 bary = make_bary(uniform_sample_triangle(tri));
  vec3 pos = get_position_triangle(tri, bary);
  
  vec3 rd = pos - payload.ro;
  payload.rd = normalize(rd);
  float dist_sq = dot(rd, rd);

  // approximation of the triangle normal...
  //vec3 e1 = tri.vertices[1].pos - tri.vertices[0].pos;
  //vec3 e2 = tri.vertices[2].pos - tri.vertices[0].pos;
  //vec3 tri_norm = normalize(cross(e1, e2));
  vec3 tri_norm = tri.vertices[0].norm;
  
  // calculate the pdf of this direction being sampled
  // this might be wrong
  payload.attenuated_colour *= get_triangle_area(tri) * abs(dot(tri_norm, payload.rd));
//   * (1.0/dist_sq); // * length(rd) /
  //   (get_triangle_area(tri) * abs(dot(tri_norm, rd)));
}

void main() {
  payload.depth += 1;
  uint geom_index = gl_InstanceID + gl_GeometryIndexEXT;
  geometry_node geom_node = geometry_nodes.nodes[geom_index];
  // triangle_t tri = unpack_triangle(gl_PrimitiveID, 2); // vertex is 2 vec4s
  triangle_t tri = unpack_triangle(geom_node, gl_PrimitiveID);
  // TODO: fix this
  vertex_t v0 = tri.vertices[0];
  vertex_t v1 = tri.vertices[1];
  vertex_t v2 = tri.vertices[2];

  // get the properties of the current point on the triangle
  vec3 bary = make_bary(attribs);
  // get the normal to convert the normal map to the correct space
  vec3 norm = bary_interp(v0.norm, v1.norm, v2.norm, bary);
  vec2 uv = bary_interp(v0.uv, v1.uv, v2.uv, bary);

  // create a tbn matrix for the triangle to calculate the normal map
  mat3 frame = tbn(norm);
  
  material_t material = get_material(geom_node);
  
  // TODO: if statements...
  // read normal from normal map
  vec3 n_col;
  if (material.normal_map_index == 0xFFFFFFFF) {
    n_col = vec3(0, 0, 1);
  } else {
    n_col = 2.0*texture(textures[nonuniformEXT(material.normal_map_index)], uv).rgb - 1.0;
    //n_col = normalize(n_col); // maybe not needed ?
  }
  
  // the inverse of a tbn matrix is its transpose (this is faster)
  norm = n_col * transpose(frame); 

  // new onb given the normal map (is there a better way)
  // we don't need to recalculate up (probably)? 
  //up = (abs(norm.z) < 0.99) ? vec3(0, 0, 1) : vec3(1, 0, 0);
  //tx = normalize(cross(up, norm));
  //ty = cross(norm, tx);
  //frame = mat3(tx, ty, norm);
  frame = tbn(norm);
  
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
    payload.ro = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    if (rand(payload.seed) > 0.0) {
      try_sample_light();
    } else {
      // random direction
      payload.rd = frame * cosine_sample_hemisphere(payload.seed);
    }

    payload.attenuated_colour *= material_colour;
  }
}
