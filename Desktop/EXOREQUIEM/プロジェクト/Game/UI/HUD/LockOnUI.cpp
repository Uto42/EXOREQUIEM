/*****************************************************************//**
 * @file    LockOnUI.cpp
 * @brief   ロックオンサイトの3D座標から2Dスクリーン座標への変換および照準描画の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/HUD/LockOnUI.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include <WICTextureLoader.h>

/**
 * @brief コンストラクタ
 */
LockOnUI::LockOnUI()
    : m_rotationTimer(0.0f)
    , m_crosshairPosition(DirectX::SimpleMath::Vector2::Zero)
{
}

/**
 * @brief デストラクタ
 */
LockOnUI::~LockOnUI()
{
}

/**
 * @brief 初期化処理
 * @param[in] device ID3D11Device
 * @param[in] context ID3D11DeviceContext（未使用）
 */
void LockOnUI::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UNREFERENCED_PARAMETER(context);

    // 照準テクスチャの読み込み
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_crosshair.png", 
        nullptr, m_textureSoftLock.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_crosshair.png", 
        nullptr, m_textureHardLock.GetAddressOf());
}

/*
 * @brief 更新処理
 * @param[in] dt 経過時間
 */
void LockOnUI::Update(float dt)
{
    m_rotationTimer += ROTATION_SPEED * dt;
}

/**
 * @brief 描画処理
 * @param[in] spriteBatch 描画用スプライトバッチ
 * @param[in] lockOnSystem ロックオンシステム
 * @param[in] view ビュー行列
 * @param[in] proj 投影行列
 * @param[in] viewport ビューポート
 */
void LockOnUI::Render(DirectX::SpriteBatch* spriteBatch, LockOnSystem* lockOnSystem,
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, const D3D11_VIEWPORT& viewport)
{
    if (!lockOnSystem) return;

    // 画面の中央座標を計算（ターゲットがいない、または画面外のときはここに戻す）
    DirectX::SimpleMath::Vector3 screenPos(viewport.Width * HALF_RATIO, viewport.Height * HALF_RATIO, 0.0f);
    bool isHardLock = lockOnSystem->IsHardLockMode();
    bool hasValidTarget = false;

    // 現在のターゲットを取得
    Robot* target = lockOnSystem->GetCurrentTarget();
    
    float scaleMultiplier = 1.0f;

    // ターゲットが存在し、生存している場合のみ座標計算を行う
    if (target && target->IsActive())
    {
        DirectX::SimpleMath::Vector3 targetPos = target->GetBoundingSphere().Center;

        scaleMultiplier = target->GetRadius() / 2.0f;

        // 画面を覆い尽くすほど巨大になるのを防ぐため、最大値を制限（例: 最大5倍）
        scaleMultiplier = std::min(scaleMultiplier, 5.0f);

        DirectX::SimpleMath::Vector3 calculatedPos;
        // 画面内かつ前方にいる場合のみ、ターゲットの座標に書き換える
        if (CalculateScreenPosition(targetPos, view, proj, viewport, calculatedPos))
        {
            screenPos = calculatedPos;
            hasValidTarget = true;
        }
    }

    lockOnSystem->SetTargetOnScreen(hasValidTarget);

    // ターゲットが「存在しない」「画面外」「真後ろ」のいずれかであれば、中央に即時描画
    if (!hasValidTarget)
    {
        isHardLock = false; 
    }

    // 確定したスクリーン座標を保存
    m_crosshairPosition = DirectX::SimpleMath::Vector2(screenPos.x, screenPos.y);

	// ロックオンマーカーの描画
    DrawLockOnMarker(spriteBatch, screenPos, isHardLock, scaleMultiplier);
}

/**
 * @brief 3Dワールド座標から2Dスクリーン座標への変換および画面内判定
 * @param[in] targetPos オフセット加算済みのターゲットワールド座標
 * @param[in] view ビュー行列
 * @param[in] proj 投影行列
 * @param[in] viewport ビューポート
 * @param[out] outScreenPos 変換後のスクリーン座標
 * @return 描画範囲内であれば true
 */
bool LockOnUI::CalculateScreenPosition(const DirectX::SimpleMath::Vector3& targetPos, 
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, 
    const D3D11_VIEWPORT& viewport, DirectX::SimpleMath::Vector3& outScreenPos) const
{
    // スクリーン座標へプロジェクト
    outScreenPos = XMVector3Project(
        targetPos,
        viewport.TopLeftX, viewport.TopLeftY,
        viewport.Width, viewport.Height,
        viewport.MinDepth, viewport.MaxDepth,
        proj, view, DirectX::SimpleMath::Matrix::Identity
    );

    // 画面外および背面判定 (Zが0～1の範囲外なら描画しない)
    if (outScreenPos.z > 1.0f || outScreenPos.z < 0.0f) return false;

    // Z値による背面チェック
    if (outScreenPos.z < 0.0f || outScreenPos.z > 1.0f)
    {
        return false;
    }

    // X, Y の画面外チェック
    if (outScreenPos.x < 0.0f || outScreenPos.x > viewport.Width ||
        outScreenPos.y < 0.0f || outScreenPos.y > viewport.Height)
    {
        return false;
    }

    return true;
}

/**
 * @brief ロックオンマーカーの描画実行
 * @param[in] spriteBatch 描画用スプライトバッチ
 * @param[in] screenPos 描画するスクリーン座標
 * @param[in] isHardLock ハードロック中かどうか
 * @paraim[on] dt 経過時間
 */
void LockOnUI::DrawLockOnMarker(DirectX::SpriteBatch* spriteBatch,
    const DirectX::SimpleMath::Vector3& screenPos, bool isHardLock, float scaleMultiplier)
{
    UNREFERENCED_PARAMETER(scaleMultiplier);

    // ロックオンモードに応じた外見設定
    bool isHardLockMode = isHardLock;
    ID3D11ShaderResourceView* textureToDraw = nullptr;
    DirectX::SimpleMath::Color color = (DirectX::SimpleMath::Color)DirectX::Colors::White;
    float rotation = 0.0f;

    if (isHardLockMode)
    {
        // ハードロック：赤色で回転させる
        textureToDraw = m_textureHardLock.Get();
        color = DirectX::Colors::Red;
        rotation = m_rotationTimer;
    }
    else
    {
        // ソフトロック：白色で固定
        textureToDraw = m_textureSoftLock.Get();
        color = DirectX::Colors::White;
        rotation = 0.0f;
    }

    if (!textureToDraw) return;

    // テクスチャのサイズ取得
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    textureToDraw->GetResource(resource.GetAddressOf());
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
    resource.As(&texture2D);

    D3D11_TEXTURE2D_DESC desc;
    texture2D->GetDesc(&desc);
    DirectX::SimpleMath::Vector2 origin(static_cast<float>(desc.Width) 
        * HALF_RATIO, static_cast<float>(desc.Height) * HALF_RATIO);

    // スプライト描画
    spriteBatch->Draw(
        textureToDraw,
        DirectX::SimpleMath::Vector2(screenPos.x, screenPos.y),
        nullptr,
        color,
        rotation,
        origin,
        MARKER_SCALE,
        DirectX::SpriteEffects_None,
        0.0f
    );
}