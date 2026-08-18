#include "Particle.hlsli"

// 定数定義
static const int vnum = 4;
static const float4 offset_array[vnum] =
{
    float4(-0.5f, 0.5f, 0.0f, 0.0f),
	float4(0.5f, 0.5f, 0.0f, 0.0f),
	float4(-0.5f, -0.5f, 0.0f, 0.0f),
	float4(0.5f, -0.5f, 0.0f, 0.0f),
};

[maxvertexcount(vnum)]
void main(point PS_INPUT input[1], inout TriangleStream<PS_INPUT> output)
{
    // カメラの「右」と「上」のベクトルを取得 (ビュー行列の行から取得)
    float3 cameraRight = normalize(float3(matView._11, matView._21, matView._31));
    float3 cameraUp = normalize(float3(matView._12, matView._22, matView._32));

    for (int i = 0; i < vnum; i++)
    {
        PS_INPUT element;
        float size = input[0].Tex.x;

        // ビルボード計算: カメラの右・上方向に展開
        float3 offset = (cameraRight * offset_array[i].x + cameraUp * offset_array[i].y) * size;
        float4 worldPos = float4(input[0].Pos.xyz + offset, 1.0f);

        // 座標変換
        element.Pos = mul(worldPos, matView);
        element.Pos = mul(element.Pos, matProj);

        // 色とUV
        element.Color = input[0].Color;
        element.Tex.x = offset_array[i].x + 0.5f;
        element.Tex.y = -offset_array[i].y + 0.5f;

        output.Append(element);
    }
    output.RestartStrip();
}