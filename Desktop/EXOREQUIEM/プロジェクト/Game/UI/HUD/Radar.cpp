/*****************************************************************//**
 * @file    Radar.cpp
 * @brief   プレイヤー周辺の敵位置を表示するUIレーダーの管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/HUD/Radar.h"
#include <WICTextureLoader.h>

/**
 * @brief 初期化処理
 * @param device Direct3Dデバイス
 */
void Radar::Initialize(ID3D11Device* device)
{
    // 1x1の白いテクスチャをプログラムで生成
    uint32_t white = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA data = { &white, sizeof(uint32_t), 0 };
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = desc.Height = 1;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    device->CreateTexture2D(&desc, &data, tex.GetAddressOf());
    device->CreateShaderResourceView(tex.Get(), nullptr, m_whitePixel.GetAddressOf());

    // レーダー画像の読み込み
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_radar.png", nullptr, m_radarTexture.GetAddressOf());
}

/**
 * @brief 更新処理
 * @param dt 経過時間
 */
void Radar::Update(float dt)
{
    m_sweepAngle += dt * SWEEP_SPEED;
    if (m_sweepAngle >DirectX::XM_2PI) m_sweepAngle -= DirectX::XM_2PI;
}

/**
 * @brief 描画処理
 * @param batch スプライトバッチ
 * @param screenPos レーダーの中心位置（スクリーン座標）
 * @param scaledRadius レーダーの半径（スケールされた値）
 * @param playerPos プレイヤーのワールド座標
 * @param playerYaw プレイヤーの向き（ヨー角）
 * @param enemyPositions 敵のワールド座標のリスト
 */
void Radar::Render(DirectX::SpriteBatch* batch, const DirectX::SimpleMath::Vector2& screenPos, 
    float scaledRadius, const DirectX::SimpleMath::Vector3& playerPos, float playerYaw,
    const std::vector<DirectX::SimpleMath::Vector3>& enemyPositions)
{
    // --- レーダー外枠（背景）の描画 ---
    DrawRadarBackground(batch, screenPos, scaledRadius);

    // --- スイープ線 ---
    DrawSweepLine(batch, screenPos, scaledRadius);

    // --- 敵の描画 ---
    DrawEnemies(batch, screenPos, scaledRadius, playerPos, playerYaw, enemyPositions);

    // --- プレイヤー（自機）の描画 ---
    DrawPlayer(batch, screenPos, scaledRadius);
}

/**
 * @brief レーダー背景の描画
 * @param[in] batch スプライトバッチ
 * @param[in] screenPos レーダーの中心スクリーン座標
 * @param[in] scaledRadius スケール調整されたレーダーの半径
 */
void Radar::DrawRadarBackground(DirectX::SpriteBatch* batch, 
    const DirectX::SimpleMath::Vector2& screenPos, float scaledRadius)
{
    DirectX::SimpleMath::Vector2 origin(RADAR_TEX_CENTER, RADAR_TEX_CENTER); // 画像の中心
    // 半径(100)を画像サイズ(1050)で割ってスケールを出す
    float textureScale = (scaledRadius / RADAR_TEX_CENTER);

    batch->Draw(m_radarTexture.Get(), screenPos, nullptr, DirectX::Colors::White,
        0.0f, origin, textureScale, DirectX::SpriteEffects_None);
}

/**
 * @brief スイープ線（回転する線）の描画
 * @param[in] batch スプライトバッチ
 * @param[in] screenPos レーダーの中心スクリーン座標
 * @param[in] scaledRadius スケール調整されたレーダーの半径
 */
void Radar::DrawSweepLine(DirectX::SpriteBatch* batch,
    const DirectX::SimpleMath::Vector2& screenPos, float scaledRadius)
{
    // 1セグメントあたりの角度
    float angleStep = TRAIL_ANGLE_WIDTH / TRAIL_SEGMENTS;

    // 過去のセグメントから順番に、隙間を埋めるように面として描画する
    for (int i = TRAIL_SEGMENTS - 1; i >= 0; --i)
    {
        // 描画するセグメントの角度
        float angleA = m_sweepAngle - (i * angleStep);
        float angleB = m_sweepAngle - ((i + 1) * angleStep);
        float middleAngle = (angleA + angleB) * HALF_RATIO;

        // アルファ減衰率：現在に近いほど濃く、過去にいくほど 0.0 に近づく
        float progress = 1.0f - (static_cast<float>(i) / TRAIL_SEGMENTS);
        float alphaFactor = progress * progress;

        float segmentWidth = scaledRadius * angleStep * 1.05f;

        // 白ピクセルを「長く引き伸ばした板」として扱い、それを敷き詰めて扇型を作る
        batch->Draw(
            m_whitePixel.Get(),
            screenPos,
            nullptr,
            DirectX::Colors::Lime * (SWEEP_ALPHA_MAX * alphaFactor),
            middleAngle,
            DirectX::SimpleMath::Vector2(HALF_RATIO, 0.0f),
            DirectX::SimpleMath::Vector2(segmentWidth, scaledRadius * SWEEP_LENGTH_RATIO),
            DirectX::SpriteEffects_None
        );
    }

    // 先頭の最も明るいメイン線を引く
    batch->Draw(
        m_whitePixel.Get(),
        screenPos,
        nullptr,
        DirectX::Colors::Lime,
        m_sweepAngle,
        DirectX::SimpleMath::Vector2(HALF_RATIO, 0.0f),
        DirectX::SimpleMath::Vector2(3.0f * (scaledRadius / RADAR_RADIUS), scaledRadius * SWEEP_LENGTH_RATIO),
        DirectX::SpriteEffects_None
    );
}

/**
 * @brief 敵位置を示すドットの描画
 * @param[in] batch スプライトバッチ
 * @param[in] screenPos レーダーの中心スクリーン座標
 * @param[in] scaledRadius スケール調整されたレーダーの半径
 * @param[in] playerPos プレイヤーのワールド座標
 * @param[in] playerYaw プレイヤーのY軸回転角度(Yaw)
 * @param[in] enemyPositions 敵のワールド座標リスト
 */
void Radar::DrawEnemies( DirectX::SpriteBatch* batch, const DirectX::SimpleMath::Vector2& screenPos, 
    float scaledRadius, const DirectX::SimpleMath::Vector3& playerPos, float playerYaw, 
    const std::vector<DirectX::SimpleMath::Vector3>& enemyPositions)
{
    // 現在の解像度半径に基づいたドットの動的スケール
    float dotScale = ENEMY_DOT_SIZE * (scaledRadius / RADAR_RADIUS);

    for (const auto& ePos : enemyPositions)
    {
        DirectX::SimpleMath::Vector3 diff = ePos - playerPos;
        float dist = DirectX::SimpleMath::Vector2(diff.x, diff.z).Length();

        // 索敵範囲内の敵のみ描画
        if (dist <= DETECT_RANGE)
        {
            float displayRadius = scaledRadius * ENEMY_DISPLAY_LIMIT;
            float r = (dist / DETECT_RANGE) * displayRadius;
            float angle = atan2f(diff.x, diff.z) - playerYaw;

            DirectX::SimpleMath::Vector2 drawPos;
            drawPos.x = screenPos.x - r * sinf(angle);
            drawPos.y = screenPos.y - r * cosf(angle);

            // 敵点（ドット）の描画
            batch->Draw(
                m_whitePixel.Get(),
                drawPos,
                nullptr,
                DirectX::Colors::Red,
                0.0f,
                DirectX::SimpleMath::Vector2(HALF_RATIO, HALF_RATIO),
                dotScale,
                DirectX::SpriteEffects_None
            );
        }
    }
}

/**
 * @brief プレイヤー（自機）を示すドットの描画
 * @param[in] batch     スプライトバッチ
 * @param[in] screenPos レーダーの中心スクリーン座標
 */
void Radar::DrawPlayer(DirectX::SpriteBatch* batch, 
    const DirectX::SimpleMath::Vector2& screenPos, float scaledRadius)
{
    // 現在の解像度半径に基づいたドットの動的スケール
    float dotScale = PLAYER_DOT_SIZE * (scaledRadius / RADAR_RADIUS);

    // 自機点（ドット）の描画
    batch->Draw(
        m_whitePixel.Get(),
        screenPos,
        nullptr,
        DirectX::Colors::White,
        0.0f,
        DirectX::SimpleMath::Vector2(HALF_RATIO, HALF_RATIO),
        dotScale,
        DirectX::SpriteEffects_None
    );
}