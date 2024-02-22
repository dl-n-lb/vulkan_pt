// from the sascha willems vulkan examples (im lazy)
uint tea(uint val0, uint val1)
{
    uint sum = 0;
    uint v0 = val0;
    uint v1 = val1;
    for (uint n = 0; n < 16; n++)
    {
        sum += 0x9E3779B9;
        v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
        v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
    }
    return v0;
}

uint lcg(inout uint previous)
{
    const uint multiplier = 1664525u;
    const uint increment = 1013904223u;
    previous   = (multiplier * previous + increment);
    return previous & 0x00FFFFFF;
}

uint get_seed(ivec2 val) {
  return tea(val.x, val.y);
}

float rand(inout uint seed) {
  return float(lcg(seed)) / float(0x01000000);
}

vec3 cosine_sample_hemisphere(uint seed)
{
	float r1 = rand(seed);
	float r2 = rand(seed);

	vec3 dir;
	float r = sqrt(r1);
	float phi = 2.0 * 3.141592653589793238 * r2;

	dir.x = r * cos(phi);
	dir.y = r * sin(phi);
	dir.z = sqrt(max(0.0, 1.0 - dir.x*dir.x - dir.y*dir.y));

	return dir;
}

// copied from https://www.shadertoy.com/view/XlGcRh
uint wang(inout uint v) {
    v = (v ^ 61u) ^ (v >> 16u);
    v *= 9u;
    v ^= v >> 4u;
    v *= 0x27d4eb2du;
    v ^= v >> 15u;
    return v;
}
