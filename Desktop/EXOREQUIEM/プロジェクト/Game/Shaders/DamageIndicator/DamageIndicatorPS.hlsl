// DamageIndicatorPS.hlsl

cbuffer ConstantBuffer : register(b0)
{
    float2 Direction;   // 攻撃方向 (画面上の2Dベクトル)
    float Timer;        // 経過時間 (0.0 -> 1.0)
    float AspectRatio;  // アスペクト比
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target
{
    // UVを -1.0 ～ 1.0 に変換
    float2 uv = input.UV * 2.0f - 1.0f;
    
    // アスペクト比補正
    uv.x *= AspectRatio;
    
    // --- ドーナツ型を作る ---
    float dist = length(uv);
    
    // 半径 0.5 付近のみ描画
    float circleAlpha = smoothstep(0.44f, 0.475f, dist) - smoothstep(0.525f, 0.56f, dist);
    
    // --- 方向を切り取る ---
    float2 pixelDir = normalize(uv);
    
    // 内積で向きの一致度を計算
    float angleMatch = dot(pixelDir, Direction);
    
    // 角度制限: cos(30度) ≒ 0.866
    float arcAlpha = smoothstep(0.85f, 0.88f, angleMatch);
    
    // --- フェードアウト ---
    // Timerが増えるほど透明に
    float fade = 1.0f - saturate(Timer);
    
    // 最終的なアルファ値
    float finalAlpha = circleAlpha * arcAlpha * fade;
    
    // 透明部分は描画しない
    if (finalAlpha <= 0.01f)
        discard;
    
    // 赤～オレンジ色で発光
    return float4(1.0f, 0.2f, 0.0f, finalAlpha);
}