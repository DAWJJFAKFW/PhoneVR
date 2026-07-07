#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
};

struct DistortionUniforms {
    float halfWidth;
    float halfHeight;
    float ipd;
    float k1;
    float k2;
    float chromaAb;
    float eyeOffset;
    float padding;
};

vertex VertexOut vrVertexShader(constant float2 *vertices [[buffer(0)]],
                                uint vid [[vertex_id]]) {
    VertexOut out;
    out.position = float4(vertices[vid], 0.0, 1.0);
    out.texcoord = vertices[vid] * 0.5 + 0.5;
    return out;
}

fragment float4 vrFragmentShader(VertexOut in [[stage_in]],
                                 texture2d<float> sceneTex [[texture(0)]]) {
    constexpr sampler s(coord::normalized, filter::linear, address::clamp_to_edge);
    return sceneTex.sample(s, in.texcoord);
}

fragment float4 distortionFragmentShader(VertexOut in [[stage_in]],
                                          constant DistortionUniforms &uniforms [[buffer(0)]],
                                          texture2d<float> sceneTex [[texture(0)]]) {
    constexpr sampler s(coord::normalized, filter::linear, address::clamp_to_edge);

    float2 uv = in.texcoord;

    float2 centered = uv * 2.0 - 1.0;

    float r2 = centered.x * centered.x + centered.y * centered.y;
    float r4 = r2 * r2;

    float distortion = 1.0 + uniforms.k1 * r2 + uniforms.k2 * r4;

    float2 distorted = centered * distortion;

    float2 finalUv = distorted * 0.5 + 0.5;

    if (finalUv.x < 0.0 || finalUv.x > 1.0 || finalUv.y < 0.0 || finalUv.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    float4 color;
    color.r = sceneTex.sample(s, finalUv + float2(uniforms.chromaAb, 0.0)).r;
    color.g = sceneTex.sample(s, finalUv).g;
    color.b = sceneTex.sample(s, finalUv - float2(uniforms.chromaAb, 0.0)).b;
    color.a = 1.0;

    return color;
}
