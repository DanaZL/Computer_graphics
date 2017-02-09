#version 330

in vec4 point;
in vec2 position;
in vec4 variance;
in vec4 init_variance;
in float last_variance;

in vec2 textPoints;

out vec2 TexCoord;
out vec4 Point;
uniform mat4 camera;


void main() {
    mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[1][1] = 0.1 * init_variance.y;
    scaleMatrix[0][0] = 0.01 * init_variance.y;
    scaleMatrix[2][2] = 0.01 * init_variance.y;

    mat4 positionMatrix = mat4(1.0);

    positionMatrix[3][0] = position.x;
    positionMatrix[3][2] = position.y;

    mat4 M = mat4(1.0);

    M[0][0] = cos(init_variance.x);
    M[2][0] = -1 * init_variance.x;
    M[0][2] = sin(init_variance.x);
    M[2][2] = cos(init_variance.x);
    float delta_y = 0;

    vec4 real_point = M * point;

    vec4 scaled_point = positionMatrix * scaleMatrix * real_point;

    vec4 real_variance = variance * real_point.y * real_point.y;

    if (((real_variance.x >= last_variance) && (real_variance.x > 0)) || ((real_variance.x <= last_variance) && (real_variance.x < 0))) {
    	delta_y = -1 * real_point.y + sqrt(real_point.y * real_point.y - real_variance.x * real_variance.x);
    } else {
    	delta_y = real_point.y - sqrt(real_point.y * real_point.y - real_variance.x * real_variance.x);
    }
    // float delta_y = -0.5 * abs(variance.x);

    vec4 init_delta = vec4(init_variance[2], 0, init_variance[3], 0.0) * real_point.y * real_point.y;

    vec4 res_point = (scaled_point + real_variance + init_delta + vec4(0, 10 * delta_y, 0, 0));

    vec2 real_textPoints;
    real_textPoints.x = 0.01 * init_variance.y * textPoints.x;
    real_textPoints.y = 0.1 * init_variance.y * textPoints.y;
    TexCoord = real_textPoints;
    gl_Position = camera * res_point;
    Point = point;
}