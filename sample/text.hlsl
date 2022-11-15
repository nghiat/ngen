cbuffer cb : register(b0, space0) {
  float width;
  float height;
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
  float2 xy = float2(input.pos.x/width, input.pos.y/height);
  xy = 2*xy - float2(1.0, 1.0);
  ps_input.p = float4(xy, 0.0, 1.0);
  ps_input.uv = input.uv;
  return ps_input;
}

float4 PSMain(PSInput input) : SV_TARGET {
  float r = g_texture.Sample(g_sampler, input.uv);
  return float4(0.0, 0.0, 0.0, r);
}
