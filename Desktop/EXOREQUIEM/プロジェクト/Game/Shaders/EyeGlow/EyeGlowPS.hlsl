Texture2D<float4> BaseTexture : register(t0);
SamplerState TextureSampler : register(s0);

cbuffer EyeGlowBuffer : register(b0)
{
    float2 g_eyeUV;
    float g_uvRadius;
    float g_time;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0; 
};

float4 main(PSInput input) : SV_Target
{
    // ”wŒi‰æ‘œ‚ÌƒTƒ“ƒvƒŠƒ“ƒO
    float4 base = BaseTexture.Sample(TextureSampler, input.texCoord);

    float currentIntensity = 0.0f;
    float flareIntensity = 0.0f;

    if (g_time >= 1.0f)
    {
        float flash = saturate(1.0f - (g_time - 1.0f) * 4.0f);
        float flicker = 0.8f + 0.15f * sin(g_time * 20.0f) + 0.1f * sin(g_time * 45.0f);
        
        currentIntensity = flicker + (flash * 5.0f);
        flareIntensity = flash * 2.5f;
    }

    float dist = distance(input.texCoord, g_eyeUV);
    float eyeMask = saturate(1.0f - (dist / g_uvRadius));
    eyeMask = pow(eyeMask, 2.0f);

    float diffY = abs(input.texCoord.y - g_eyeUV.y);
    float flareMask = saturate(1.0f - (diffY / (3.0f / 720.0f)));
    flareMask = pow(flareMask, 4.0f);

    float diffX = abs(input.texCoord.x - g_eyeUV.x);
    float flareXAttenuation = saturate(1.0f - (diffX / (400.0f / 1280.0f)));
    flareXAttenuation = pow(flareXAttenuation, 2.0f);

    float finalFlare = flareMask * flareXAttenuation * flareIntensity;

    float3 glowColor = float3(0.0f, 1.0f, 0.3f);
    float3 totalGlow = (eyeMask * currentIntensity) + finalFlare;

    // RGB‚É”­Œõ‚ğ‰ÁZ‚µ‚Äo—Í
    return float4(base.rgb + (glowColor * totalGlow), base.a);
}