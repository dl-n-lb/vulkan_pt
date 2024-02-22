layout (push_constant) uniform buffer_references_t {
  uint64_t vertices;
  uint64_t indices;
  uint64_t buffer_address;
} buffer_references;


layout (buffer_reference, scalar) buffer vertices { vec4 v[]; };
layout (buffer_reference, scalar) buffer indices  { uint i[]; };
layout (buffer_reference, scalar) buffer data     { vec4 f[]; };
