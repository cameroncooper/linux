#version 450
layout(location = 0) out vec4 color;
layout(push_constant) uniform Frame {
    uint index;
} frame;

void main() {
    uvec2 pixel = uvec2(gl_FragCoord.xy);
    uint fine = (pixel.x ^ pixel.y) & 1u;
    uint coarse = ((pixel.x / 17u) ^ (pixel.y / 31u)) & 1u;
    color = vec4(float(fine), float(coarse), float(frame.index & 1u), 1.0);
}
