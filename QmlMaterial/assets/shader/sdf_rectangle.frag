#version 440

#extension GL_GOOGLE_include_directive : enable

#include "sdf.glsl"

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    // tl,tr,bl,br
    vec4 radius;
    // item_size, rect_size
    vec4 size;
    vec4 color;

    float smoothing;
    float offset;
};

void main() {
    vec2 p = qt_TexCoord0 - vec2(0.5);
    p *= size.xy;
    // br,tr,bl,tl
    float sdf = sdf_rounded_rectangle(p, size.zw / 2.0, vec4(radius.w, radius.y, radius.z, radius.x));

    fragColor   = color;
    fragColor.a = sdf_alpha_uv(sdf, p, smoothing, offset) * qt_Opacity;
    // premultiplied
    fragColor.xyz *= fragColor.a;
}