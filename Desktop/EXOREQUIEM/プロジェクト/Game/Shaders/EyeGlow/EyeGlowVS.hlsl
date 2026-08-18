struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

// 頂点バッファを使わず、VertexID(0, 1, 2)だけで画面全体を覆う三角形を生成する
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    
    // UV座標の生成 (0.0 ~ 2.0 の範囲で巨大な三角形を作る)
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    
    // クリップ空間（SV_Position）の座標に変換 (-1.0 ~ 1.0)
    output.position = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}