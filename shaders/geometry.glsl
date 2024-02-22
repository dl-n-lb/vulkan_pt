struct vertex_t {
  vec3 pos; vec3 norm; vec2 uv;
};

struct triangle_t {
  vertex_t vertices[3];
};

triangle_t unpack_triangle(uint prim_index, uint vertex_stride) {
  triangle_t tri;
  const uint idx = prim_index * 3;

  geometry_node geom_node = geometry_nodes.nodes[gl_InstanceID + gl_GeometryIndexEXT];

  indices indices = indices(geom_node.index_buffer_address);
  vertices vertices = vertices(geom_node.vertex_buffer_address);
  // unpack vertices data
  for (uint i = 0; i < 3; ++i) {
    //IGNORE: offset *= 3 because 3 vertices in a triangle
    const uint offset = indices.i[idx + i] * vertex_stride;

    vec4 d0 = vertices.v[offset];
    vec4 d1 = vertices.v[offset + 1];

    vertex_t v;
    v.pos = d0.xyz;
    v.norm = vec3(d0.w, d1.x, d1.y);
    v.uv = d1.zw;
    tri.vertices[i] = v;
  }

  return tri;
}
