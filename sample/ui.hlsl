struct PSInput {
    float4 p : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D<float> g_texture : register(t0, space0);
SamplerState g_sampler : register(s0, space1);

PSInput VSMain(float2 v: V, float2 uv: UV) {
    PSInput result;
    result.p = float4(v.x, v.y, 0.0, 1.0);
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
  float x = g_texture.Sample(g_sampler, input.uv);
  return float4(x, x, x, 1.0);
}
