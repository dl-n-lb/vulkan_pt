#include "structs.glsl"

triangle_t unpack_triangle(geometry_node node, uint primitive_index) {
  indices indices = indices(node.index_buffer_address);
  vertices vertices = vertices(node.vertex_buffer_address);

  triangle_t tri;
  for (uint i = 0; i < 3; ++i) {
    const uint offset = indices.i[3 * primitive_index + i];

    vertex_t v = vertices.v[offset];
    tri.vertices[i] = v;
  }
  
  return tri;
}
