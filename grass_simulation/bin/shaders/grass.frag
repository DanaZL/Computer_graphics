#version 330

in vec2 TexCoord;
in vec4 Point;

out vec4 color;

uniform sampler2D ourTexture;

void main() {
    color = texture (ourTexture, TexCoord) * vec4(1 * sqrt(sqrt(sqrt(Point.y))), 1 * sqrt(sqrt(Point.y)), 1 * sqrt(sqrt(Point.y)), 0);
    // color = vec4(0.2, 0.6, 0.3, 0);
}
