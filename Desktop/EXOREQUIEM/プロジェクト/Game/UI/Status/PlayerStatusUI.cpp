/*****************************************************************//**
 * @file    PlayerStatusUI.cpp
 * @brief   プレイヤーのHP・エネルギー残量のゲージ表示
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/Status/PlayerStatusUI.h"
#include "Game/Systems/Input/IManipulator.h"
#include "Game/GameObjects/Player/Player.h"
#include <WICTextureLoader.h>

/**
 * @brief コンストラクタ
 */
PlayerStatusUI::PlayerStatusUI()
{
}

/**
 * @brief デストラクタ
 */
PlayerStatusUI::~PlayerStatusUI()
{
}

/**
 * @brief 初期化処理
 * @param[in] device Direct3Dデバイス
 */
void PlayerStatusUI::Initialize(ID3D11Device* device)
{
    // 画像読み込み
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_frame.png",
        nullptr, m_frame.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_hp.png",
        nullptr, m_hpBar.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_energy.png",
        nullptr, m_energyBar.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_background.png",
        nullptr, m_back.GetAddressOf());
}

/**
 * @brief 座標計算処理
 * @param[in] viewport ビューポート
 * @param[in] anchor 基準点
 * @param[in] size UIのサイズ
 * @param[in] offset 基準点からのオフセット
 * @return 算出された描画座標
 */
DirectX::SimpleMath::Vector2 PlayerStatusUI::CalculateDrawPos(const D3D11_VIEWPORT& viewport,
    Anchor anchor, const DirectX::SimpleMath::Vector2& size, const DirectX::SimpleMath::Vector2& offset)
{
    DirectX::SimpleMath::Vector2 finalPos = DirectX::SimpleMath::Vector2::Zero;

    switch (anchor)
    {
    case Anchor::TopLeft:
        // 左上基準 (0, 0) + オフセット
        finalPos.x = 0.0f;
        finalPos.y = 0.0f;
        break;

    case Anchor::TopRight:
        // 右上基準 (画面幅 - バー幅, 0) + オフセット
        finalPos.x = viewport.Width - size.x;
        finalPos.y = 0.0f;
        break;

    case Anchor::BottomCenter:
        // 下中央基準 (画面幅半分 - バー半分, 画面高さ) + オフセット
        finalPos.x = (viewport.Width * HALF_RATIO) - (size.x * HALF_RATIO);
        finalPos.y = viewport.Height;
        break;
    }

    // 最後に微調整用の値を足す
    return finalPos + offset;
}

/**
 * @brief 描画処理
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] player プレイヤーへのポインタ
 * @param[in] viewport 現在のビューポート
 */
void PlayerStatusUI::Render(DirectX::SpriteBatch* spriteBatch,
    const Player* player, const D3D11_VIEWPORT& viewport)
{
    if (!player) return;

    // --- 画面サイズに合わせた倍率を計算 ---
    float guiScale = viewport.Height / BASE_SCREEN_HEIGHT;

    // --- 各バーの描画 ---
    DrawHPBar(spriteBatch, player, viewport, guiScale);
    DrawEnergyBar(spriteBatch, player, viewport, guiScale);
}

/**
 * @brief プレイヤーのHPバーを描画
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] player プレイヤーへのポインタ
 * @param[in] viewport 現在のビューポート
 * @param[in] guiScale 画面解像度に応じたスケール
 */
void PlayerStatusUI::DrawHPBar(DirectX::SpriteBatch* spriteBatch,
    const Player* player, const D3D11_VIEWPORT& viewport, float guiScale)
{
    DirectX::SimpleMath::Vector2 hpSize(HP_BAR_BASE_WIDTH * guiScale, HP_BAR_BASE_HEIGHT * guiScale);
    DirectX::SimpleMath::Vector2 hpOffset(viewport.Width * HP_OFFSET_X_RATIO, viewport.Height * HP_OFFSET_Y_RATIO);
    DirectX::SimpleMath::Vector2 hpPos = CalculateDrawPos(viewport, Anchor::TopLeft, hpSize, hpOffset);

    float hpRatio = std::clamp(player->GetHealthRatio(), 0.0f, 1.0f);

    // 共通関数へ委譲
    DrawGauge(spriteBatch, m_hpBar.Get(), hpPos, hpSize, hpRatio);
}

/**
 * @brief プレイヤーのエネルギーバーを描画
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] player プレイヤーへのポインタ
 * @param[in] viewport 現在のビューポート
 * @param[in] guiScale 画面解像度に応じたスケール
 */
void PlayerStatusUI::DrawEnergyBar(DirectX::SpriteBatch* spriteBatch,
    const Player* player, const D3D11_VIEWPORT& viewport, float guiScale)
{
    DirectX::SimpleMath::Vector2 energySize(ENG_BAR_BASE_WIDTH * guiScale, ENG_BAR_BASE_HEIGHT * guiScale);
    DirectX::SimpleMath::Vector2 energyPos = 
        CalculateDrawPos(
            viewport, 
            Anchor::BottomCenter, 
            energySize, 
            DirectX::SimpleMath::Vector2(0.0f, ENG_OFFSET_Y * guiScale));

    float energyRatio = std::clamp(player->GetEnergyRatio(), 0.0f, 1.0f);

    // 共通関数へ委譲
    DrawGauge(spriteBatch, m_energyBar.Get(), energyPos, energySize, energyRatio);
}

/**
 * @brief 共通のゲージ描画処理
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] barTexture 中身のバーとして使うテクスチャ(HP or Energy)
 * @param[in] pos 描画開始座標
 * @param[in] size 描画する全体サイズ
 * @param[in] ratio ゲージの残量割合(0.0f~1.0f)
 */
void PlayerStatusUI::DrawGauge(DirectX::SpriteBatch* spriteBatch, ID3D11ShaderResourceView* barTexture,
    const DirectX::SimpleMath::Vector2& pos, const DirectX::SimpleMath::Vector2& size, float ratio)
{
    // テクスチャサイズに対する描画比率スケールを計算
    DirectX::SimpleMath::Vector2 drawScale = 
        DirectX::SimpleMath::Vector2(size.x / GAUGE_TEX_WIDTH, size.y / GAUGE_TEX_HEIGHT);

    // 背景の描画
    if (m_back)
    {
        spriteBatch->Draw(
            m_back.Get(), 
            pos, 
            nullptr, 
            DirectX::Colors::White,
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero, drawScale);
    }

    // バー本体の描画
    if (barTexture)
    {
        RECT sourceRect = {};
        sourceRect.left = 0;
        sourceRect.top = 0;
        sourceRect.bottom = static_cast<long>(GAUGE_TEX_HEIGHT);
        sourceRect.right = static_cast<long>(GAUGE_TEX_WIDTH * ratio);

        spriteBatch->Draw(
            barTexture, 
            pos, 
            &sourceRect,
            DirectX::Colors::White, 
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero, drawScale);
    }

    // 外枠フレームの描画
    if (m_frame)
    {
        spriteBatch->Draw(
            m_frame.Get(),
            pos,
            nullptr,
            DirectX::Colors::White, 
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero, drawScale);
    }
}