#version 460 core

in vec3 vertex_color;

out vec4 color;

void main() {
    color = vec4(vertex_color.r, vertex_color.g, vertex_color.b, 1.0);
}
