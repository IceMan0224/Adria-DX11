
#include "../Globals/GlobalsPS.hlsli"
#include "../Util/DitherUtil.hlsli"
#include "../Util/ShadowUtil.hlsli"


Texture2D        normalMetallicTx       : register(t0);
Texture2D        diffuseRoughnessTx     : register(t1);
Texture2D<float> DepthTx                : register(t2);

Texture2D<float>       ShadowMap       : register(t4);
TextureCube<float>     ShadowCubeMap   : register(t5);
Texture2DArray<float>  ShadowCascadeMaps : register(t6);


//https://panoskarabelas.com/posts/screen_space_shadows/
static const uint  SSCS_MAX_STEPS = 16; // Max ray steps, affects quality and performance.
//static const float SSCS_RAY_MAX_DISTANCE = 0.05f; // Max shadow length, longer shadows are less accurate.
//static const float SSCS_THICKNESS = 0.5f; // Depth testing thickness.
//static const float SSCS_MAX_DISTANCE = 200.0f;

float SSCS(float3 pos_vs)
{
    float3 ray_pos = pos_vs;
    float2 ray_uv = 0.0f;

    float4 ray_projected = mul(float4(ray_pos, 1.0f), projection);
    ray_projected.xy /= ray_projected.w;
    ray_uv = ray_projected.xy * float2(0.5f, -0.5f) + 0.5f;

    float depth = DepthTx.Sample(point_clamp_sampler, ray_uv);
    float linear_depth = ConvertZToLinearDepth(depth);

    const float SSCS_STEP_LENGTH = current_light.sscsMaxRayDistance / (float) SSCS_MAX_STEPS;

    if (linear_depth > current_light.sscsMaxDepthDistance)
        return 1.0f;

    float3 ray_direction = normalize(-current_light.direction.xyz);
    float3 ray_step = ray_direction * SSCS_STEP_LENGTH;
    //ray_position += ray_step * dither(uv);

    float occlusion = 0.0f;
    [unroll(SSCS_MAX_STEPS)]
    for (uint i = 0; i < SSCS_MAX_STEPS; i++)
    {
        // Step the ray
        ray_pos += ray_step;

        ray_projected = mul(float4(ray_pos, 1.0), projection);
        ray_projected.xy /= ray_projected.w;
        ray_uv = ray_projected.xy * float2(0.5f, -0.5f) + 0.5f;

        [branch]
        if (IsSaturated(ray_uv))
        {
            depth = DepthTx.Sample(point_clamp_sampler, ray_uv);
            //pute the difference between the ray's and the camera's depth
            linear_depth = ConvertZToLinearDepth(depth);
            float depth_delta = ray_projected.z - linear_depth;

            // Check if the camera can't "see" the ray (ray depth must be larger than the camera depth, so positive depth_delta)
            if (depth_delta > 0 && (depth_delta < current_light.sscsThickness))
            {
                // Mark as occluded
                occlusion = 1.0f;
                // screen edge fade:
                float2 fade = max(12 * abs(ray_uv - 0.5) - 5, 0);
                occlusion *= saturate(1 - dot(fade, fade));

                break;
            }
        }
    }

    return 1.0f - occlusion;
}


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float4 main(VSToPS pin) : SV_TARGET
{

    //unpack gbuffer
    float4 NormalMetallic = normalMetallicTx.Sample(linear_wrap_sampler, pin.Tex);
    float3 Normal = 2 * NormalMetallic.rgb - 1.0;
    float metallic = NormalMetallic.a;
    float depth = DepthTx.Sample(linear_wrap_sampler, pin.Tex);
    float3 Position = GetPositionVS(pin.Tex, depth);
    float4 AlbedoRoughness = diffuseRoughnessTx.Sample(linear_wrap_sampler, pin.Tex);
    float3 V = normalize(0.0f.xxx - Position);
    float roughness = AlbedoRoughness.a;
    

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    switch (current_light.type)
    {
        case DIRECTIONAL_LIGHT:
            Lo = DoDirectionalLightPBR(current_light, Position, Normal, V, AlbedoRoughness.rgb, metallic, roughness);
            break;
        case POINT_LIGHT:
            Lo = DoPointLightPBR(current_light, Position, Normal, V, AlbedoRoughness.rgb, metallic, roughness);
            break;
        case SPOT_LIGHT:
            Lo = DoSpotLightPBR(current_light, Position, Normal, V, AlbedoRoughness.rgb, metallic, roughness);
            break;
        default:
            return float4(1, 0, 0, 1);
    }

    if (current_light.castsShadows)
    {

        float shadow_factor = 1.0f;
        if (current_light.type == POINT_LIGHT)
        {
            const float zf = current_light.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            
            float3 light_to_pixelWS = mul(float4(Position - current_light.position.xyz, 0.0f), inverse_view);

            const float3 m = abs(light_to_pixelWS).xyz;
            const float major = max(m.x, max(m.y, m.z));
            float fragment_depth = (c1 * major + c0) / major;
            shadow_factor = ShadowCubeMap.SampleCmpLevelZero(shadow_sampler, normalize(light_to_pixelWS.xyz), fragment_depth);
        }
        else if (current_light.type == DIRECTIONAL_LIGHT && current_light.useCascades)
        {
            float view_depth = Position.z;
            for (uint i = 0; i < 4; ++i)
            {
                matrix light_space_matrix = shadow_matrices[i];
                if (view_depth < splits[i])
                {
                    float4 pos_shadow_map = mul(float4(Position, 1.0), light_space_matrix);
                    float3 UVD = pos_shadow_map.xyz / pos_shadow_map.w;
                    UVD.xy = 0.5 * UVD.xy + 0.5;
                    UVD.y = 1.0 - UVD.y;
                    shadow_factor = CSMCalcShadowFactor_PCF3x3(shadow_sampler, ShadowCascadeMaps, i, UVD, shadow_map_size, softness);
                    break;
                }
            }    
        }
        else
        {
            float4 pos_shadow_map = mul(float4(Position, 1.0), shadow_matrices[0]);
            float3 UVD = pos_shadow_map.xyz / pos_shadow_map.w;
            UVD.xy = 0.5 * UVD.xy + 0.5;
            UVD.y = 1.0 - UVD.y; 
            shadow_factor = CalcShadowFactor_PCF3x3(shadow_sampler, ShadowMap, UVD, shadow_map_size, softness);
        }
        Lo = Lo * shadow_factor;
    }
    
    if (current_light.screenSpaceShadows)
        Lo = Lo * SSCS(Position);

    return float4(Lo, 1.0f);
}


