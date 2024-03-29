#version 460 core

uniform float pos_offset;

layout(location=0) in vec3 p;
layout(location=1) in vec3 c;

out vec3 vertex_color;

void main() {
    vertex_color = c;
    gl_Position = vec4(p.x + pos_offset, p.y, p.z, 1.0);
}
