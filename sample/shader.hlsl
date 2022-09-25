cbuffer per_obj_cb : register(b0, space0) {
    float4x4 world;
    float4x4 joints[300];
    float4x4 inv_bind_mat[300];
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

SamplerState g_shadow_sampler : register(s0, space2);
Texture2D g_shadow_texture : register(t0, space3);

struct VSInput {
    [[vk::location(0)]] float4 pos : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] uint4x4 joint_idx : BLENDINDICES;
    [[vk::location(6)]] float4x4 weights : BLENDWEIGHT;
};

struct PSInput {
    float4 p : SV_POSITION;
    float4 v : POSITION0;
    float4 n : NORMAL;
    float4 light_space_p : POSITION1;
};

PSInput VSMain(VSInput input) {
    PSInput result;
    float4 v = float4(0.0, 0.0, 0.0, 0.0);
    float4 n4 = float4(input.normal, 0.0);
    float4 n = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
          int idx = input.joint_idx[i][j];
          v += mul(mul(input.pos, mul(inv_bind_mat[idx], joints[idx])), input.weights[i][j]);
          n += mul(mul(n4, mul(inv_bind_mat[idx], joints[idx])), input.weights[i][j]);
        }
    }
    result.p = mul(v, mul(mul(world, view), proj));
    result.v = v;
    result.n = n;
    float4x4 light_mvp = mul(world, mul(light_view, light_proj));
    result.light_space_p = mul(v, light_mvp);

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 light_dir = normalize(light_pos - input.v);
    float3 ambient = 0.1 * light_color;
    float3 diffuse = max(dot(input.n.xyz, light_dir), 0.0) * light_color;
    float3 eye_dir = normalize(eye_pos - input.v);
    float3 reflect_dir = reflect(-light_dir, input.n);
    float spec = pow(max(dot(eye_dir, reflect_dir), 0.0), 32);
    float3 specular = 0.5 * spec * light_color;
    float4 color = float4((ambient + diffuse + specular) * obj_color.xyz, 1.0);

    // Convert p -> uv
    /* input.light_space_p = input.light_space_p / input.light_space_p.w; */
    /* float2 shadow_uv; */
    /* shadow_uv.x = (input.light_space_p.x + 1.0) / 2.0; */
    /* shadow_uv.y = 1.0 - (input.light_space_p.y + 1.0) / 2.0; */
    /* if ((saturate(shadow_uv.x) == shadow_uv.x) && (saturate(shadow_uv.y) == shadow_uv.y) && (input.light_space_p.z > 0)) { */
    /*   float sample = g_shadow_texture.Sample(g_shadow_sampler, shadow_uv); */
    /*   if (input.light_space_p.z > sample + 0.0005) */
    /*     return float4(0.0f, 0.0f, 0.0f, 1.0f); */
    /* } */
    return color;
}
