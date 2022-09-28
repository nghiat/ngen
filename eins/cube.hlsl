cbuffer per_obj_cb : register(b0, space0) {
    float4x4 world;
};

cbuffer shared_cb : register(b0, space1) {
    float4x4 view;
    float4x4 proj;
    float4x4 light_view;
    float4x4 light_proj;
    float4 eye_pos;
    float4 obj_color;
    float4 light_pos;
    float4 light_color;
};

SamplerState g_sampler : register(s0, space2);
TextureCube g_texture : register(t0, space3);

struct PSInput {
    float4 p : SV_POSITION;
    float3 uv : TEXCOORD0;
};

PSInput VSMain(float3 v: V) {
  PSInput result;
  float4x4 mvp = mul(world, mul(view, proj));
  result.p = mul(float4(v, 1.0), mvp);
  result.uv = v;

  return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
  return g_texture.Sample(g_sampler, input.uv);
}
