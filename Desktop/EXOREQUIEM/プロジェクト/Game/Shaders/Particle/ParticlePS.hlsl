#include "Particle.hlsli"

Texture2D tex : register(t0);
Texture2D tex2 : register(t1);
SamplerState samLinear : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	//	指定された画像から情報を取得
	float4 output = tex.Sample(samLinear, input.Tex);

	//	真っ白な板ポリゴン
	float4 outputw = float4(1, 1, 1, 1);
	
	//	C++から指定された色割合を考慮して、表示色を決定
	return output * input.Color;
}