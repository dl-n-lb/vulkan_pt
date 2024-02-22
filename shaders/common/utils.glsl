
// barycentric coords helpers
vec3 make_bary(vec2 attribs) {
  return vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
}

vec2 bary_interp(vec2 a, vec2 b, vec2 c, vec3 bary) {
  return a * bary.x + b * bary.y + c * bary.z;
}

vec3 bary_interp(vec3 a, vec3 b, vec3 c, vec3 bary) {
  return a * bary.x + b * bary.y + c * bary.z;
}

mat3 tbn(vec3 n) {
  // conditional required to avoid degenerate cases
  vec3 up = (abs(n.z) < 0.9) ? vec3(0, 0, 1) : vec3(1, 0, 0);
  vec3 t = cross(up, n);
  vec3 b = cross(n, t);
  return mat3(t, b, n);
}
