#define M_pi_f 3.14159265358979323846f

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
Texture2D<float4> g_albedo_texture : register(t0, space3);
Texture2D<float4> g_normal_texture : register(t1, space3);
Texture2D<float> g_metallic_texture : register(t2, space3);
Texture2D<float> g_roughness_texture : register(t3, space3);

struct PSInput {
  float4 p : SV_POSITION;
  float3 world_pos : POSITION0;
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL0;
};

float3 get_normal_from_map(PSInput input) {
  float3 tangent_normal = g_normal_texture.Sample(g_sampler, input.uv).xyz * 2.0 - 1.0;

  float3 q1 = ddx(input.world_pos);
  float3 q2 = ddy(input.world_pos);
  float2 st1 = ddx(input.uv);
  float2 st2 = ddy(input.uv);

  float3 n = normalize(input.normal);
  float3 t = normalize(q1*st2.x - q2*st1.x);
  float3 b = -normalize(cross(n, t));
  float3x3 tbn = float3x3(t, b, n);

  return normalize(mul(tangent_normal, tbn));
}

float distribution_ggx(float3 n, float3 h, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float n_dot_h = max(dot(n, h), 0.0);
    float n_dot_h_2 = n_dot_h*n_dot_h;

    float nom = a2;
    float denom = (n_dot_h_2 * (a2 - 1.0) + 1.0);
    denom = M_pi_f * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float n_dot_v, float roughness)
{
  float r = (roughness + 1.0);
  float k = (r*r) / 8.0;

  float nom   = n_dot_v;
  float denom = n_dot_v * (1.0 - k) + k;

  return nom / denom;
}

float geometry_smith(float3 n, float3 v, float3 l, float roughness)
{
  float n_dot_v = max(dot(n, v), 0.0);
  float n_dot_l = max(dot(n, l), 0.0);
  float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
  float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);

  return ggx1 * ggx2;
}

float3 fresnel_schlick(float cos_theta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

PSInput VSMain(float3 v: V, float2 uv: UV, float3 normal: NORMAL) {
  PSInput result;
  float4x4 mvp = mul(world, mul(view, proj));
  result.p = mul(float4(v, 1.0), mvp);
  result.world_pos = v;
  result.uv = uv;
  result.normal = normal;
  return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
  float3 albedo = pow(g_albedo_texture.Sample(g_sampler, input.uv).rgb, float3(2.2, 2.2, 2.2));
  float metallic = g_metallic_texture.Sample(g_sampler, input.uv).r;
  float roughness = g_roughness_texture.Sample(g_sampler, input.uv).r;

  float3 n = get_normal_from_map(input);
  float3 v = normalize(eye_pos - input.world_pos);

  // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
  float3 f0 = float3(0.04, 0.04, 0.04);
  f0 = lerp(f0, albedo, metallic);

  // reflectance equation
  float3 lo = float3(0.0, 0.0, 0.0);
  /* for(int i = 0; i < 4; ++i) */
  /* { */
      // calculate per-light radiance
      float3 l = normalize(light_pos - input.world_pos);
      float3 h = normalize(v + l);
      /* float distance = length(light_pos - input.world_pos); */
      /* float attenuation = 1.0 / (distance * distance); */
      float3 radiance = light_color;

      // Cook-Torrance BRDF
      float ndf = distribution_ggx(n, h, roughness);
      float g = geometry_smith(n, v, l, roughness);
      float3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);

      float3 numerator = ndf * g * f;
      float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
      float3 specular = numerator / denominator;

      // kS is equal to Fresnel
      float3 k_s = f;
      // for energy conservation, the diffuse and specular light can't
      // be above 1.0 (unless the surface emits light); to preserve this
      // relationship the diffuse component (kD) should equal 1.0 - kS.
      float3 k_d = float3(1.0, 1.0, 1.0) - k_s;
      // multiply kD by the inverse metalness such that only non-metals
      // have diffuse lighting, or a linear blend if partly metal (pure metals
      // have no diffuse light).
      k_d *= 1.0 - metallic;

      // scale light by NdotL
      float n_dot_l = max(dot(n, l), 0.0);

      // add to outgoing radiance Lo
      lo += (k_d * albedo / M_pi_f + specular) * radiance * n_dot_l;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  /* } */

  // ambient lighting (note that the next IBL tutorial will replace
  // this ambient lighting with environment lighting).
  float3 ambient = float3(0.03, 0.03, 0.03) * albedo;

  float3 color = ambient + lo;

  // HDR tonemapping
  color = color / (color + float3(1.0, 1.0, 1.0));
  /* // gamma correct */
  color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));

  return float4(color, 1.0);
}
