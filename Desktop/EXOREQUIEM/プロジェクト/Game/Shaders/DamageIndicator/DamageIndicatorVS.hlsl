// DamageIndicatorVS.hlsl

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// 頂点データ入力なしで描画するテクニック
VS_OUTPUT main(uint id : SV_VertexID)
{
    VS_OUTPUT output;
    
    // 画面全体を覆う三角形の座標とUVを計算
    float2 tex = float2((id << 1) & 2, id & 2);
    
    output.UV = tex;
    output.Pos = float4(tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}