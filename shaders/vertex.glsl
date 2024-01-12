#version 460 core

in vec4 p;

void main() {
    gl_Position = vec4(p.x, p.y, p.z, 1.0);
}
