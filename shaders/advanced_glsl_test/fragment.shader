#version 330 core
out vec4 FragColor;

uniform int uSubSenceIndex;

uniform int uSplitScreen;
uniform float uSplitBase;
uniform int uFrontFacing;
uniform vec3 uObjectColor;

void main()
{
    if (uSubSenceIndex == 1 && uSplitScreen == 1) {
        if (gl_FragCoord.x < uSplitBase) {
            FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
    else if (uSubSenceIndex == 1 && uFrontFacing == 1) {
        if (gl_FrontFacing == false) {
            FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
    else if (uSubSenceIndex == 2) {
        FragColor = vec4(uObjectColor, 1.0f);
    }
    else {
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}

