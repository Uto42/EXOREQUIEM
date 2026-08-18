/*****************************************************************//**
 * @file    BlobShadow.cpp
 * @brief   ユニットの足元に投影される簡易的な円形影（丸影）の描画制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Effect/Visuals/BlobShadow.h"
#include "WICTextureLoader.h"

/**
 * @brief コンストラクタ
 */
BlobShadow::BlobShadow() 
{
}

/**
 * @brief 初期化処理
 * @param[in] device  ID3D11Device
 * @param[in] context ID3D11DeviceContext
 */
void BlobShadow::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // 板（1x1サイズ）の作成
    m_plane = DirectX::GeometricPrimitive::CreateBox(context, DirectX::XMFLOAT3(1.0f, 0.001f, 1.0f));

    // 影画像の読み込み
    DX::ThrowIfFailed(
        DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/Effects/effect_shadow.png",
            nullptr, m_texture.GetAddressOf())
    );

    // エフェクトの設定
    m_effect = std::make_unique<DirectX::BasicEffect>(device);
    m_effect->SetTextureEnabled(true);
    m_effect->SetTexture(m_texture.Get());
    m_effect->SetAlpha(1.0f);          // 影の濃さ
    m_effect->SetLightingEnabled(false); // 影自体は発光しない

    // 入力レイアウトの作成
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    device->CreateInputLayout(
        DirectX::VertexPositionNormalTexture::InputElements,
        DirectX::VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_inputLayout.ReleaseAndGetAddressOf()
    );


    // アルファブレンド（半透明）用
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // デプスステンシル（Zバッファ書き込み無効化）用
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 深度バッファを汚さない
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = -RASTER_DEPTH_BIAS;
    rasterDesc.SlopeScaledDepthBias = RASTER_SLOPE_SCALED_DEPTH_BIAS; // 斜面のちらつき防止
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;

    device->CreateRasterizerState(&rasterDesc, m_rasterizerState.GetAddressOf());
}

/**
 * @brief 影の描画実行
 * @param[in] context デバイスコンテキスト
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 * @param[in] position 影を落とすオブジェクトの座標
 * @param[in] baseScale 基本サイズ
 * @param[in] groundY 地面の高さ（Y座標）
 */
void BlobShadow::Render(ID3D11DeviceContext* context, const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& proj, const DirectX::SimpleMath::Vector3& position, 
    float baseScale, float groundY)
{
    // ステート変更（半透明ON, Z書き込みOFF）
    context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);

    // バイアス設定をONにする
    context->RSSetState(m_rasterizerState.Get());

    // 影の位置計算
    DirectX::SimpleMath::Vector3 drawPos = position;

    // 受け取った「地面の高さ」の上に置く
    drawPos.y = groundY + GROUND_HEIGHT_OFFSET;

    // サイズ計算 (プレイヤーの高さ - 地面の高さ)
    // 地面より下にめり込んでいる場合の対策で max(0)
    float dist = std::max(0.0f, position.y - groundY);

    float currentScale = baseScale / (1.0f + dist * HEIGHT_ATTENUATION_FACTOR);

    // 行列計算
    DirectX::SimpleMath::Matrix world = 
        DirectX::SimpleMath::Matrix::CreateScale(currentScale)
        * DirectX::SimpleMath::Matrix::CreateTranslation(drawPos);

    // エフェクト適用
    m_effect->SetWorld(world);
    m_effect->SetView(view);
    m_effect->SetProjection(proj);

    m_effect->Apply(context);

    // 描画
    m_plane->Draw(m_effect.get(), m_inputLayout.Get(), false, false, [=]
        {
            context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_depthState.Get(), 0);
            context->RSSetState(m_rasterizerState.Get());
        });

    // 後始末
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}