cbuffer cb : register(b0, space0) {
    float4x4 world;
};

struct VSInput {
    float2 pos : POSITION;
};

struct PSInput {
    float4 p : SV_POSITION;
};

PSInput VSMain(VSInput input) {
  PSInput ps_input;
  ps_input.p = mul(float4(input.pos, 0.0, 1.0), world);
  return ps_input;
}

float4 PSMain(PSInput input) : SV_TARGET {
  return float4(1.0, 0.0, 0.0, 1.0);
}
