#version 330
in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture;

void main() {
    color = texture (ourTexture, TexCoord) * vec4(0.1, 0.4, 0.3, 0);
    //color = vec4(0.5, 0.5, 0, 0);
}
