cbuffer cb : register(b0, space0) {
    float4x4 world;
};

SamplerState g_sampler : register(s0, space1);
Texture2D g_texture : register(t0, space2);

struct VSInput {
    float2 pos : POSITION;
    float2 uv : UV;
};

struct PSInput {
    float4 p : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input) {
  PSInput ps_input;
  ps_input.p = mul(float4(input.pos, 0.0, 1.0), world);
  ps_input.uv = input.uv;
  return ps_input;
}

float4 PSMain(PSInput input) : SV_TARGET {
  return g_texture.Sample(g_sampler, input.uv);
}
