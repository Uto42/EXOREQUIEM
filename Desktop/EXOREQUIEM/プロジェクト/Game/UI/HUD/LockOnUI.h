/*****************************************************************//**
 * @file    LockOnUI.h
 * @brief   ロックオンサイトの3D座標から2Dスクリーン座標への変換および照準描画
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class LockOnSystem;
class Enemy;

/**
 * @class LockOnUI
 * @brief ロックオン状態を画面に表示するUIクラス
 */
class LockOnUI
{
public:
    // コンストラクタ
    LockOnUI();
    // デストラクタ
    ~LockOnUI();
    // 初期化処理
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    // 更新処理
    void Update(float dt);
    // 描画実行
    void Render(
        DirectX::SpriteBatch* spriteBatch,
        LockOnSystem* lockOnSystem,
        const DirectX::SimpleMath::Matrix& view, 
        const DirectX::SimpleMath::Matrix& proj,
        const D3D11_VIEWPORT& viewport);

    // 確定したクロスヘアのスクリーン座標を取得
    DirectX::SimpleMath::Vector2 GetCrosshairPosition() const { return m_crosshairPosition; }

private:
    // 3D座標からスクリーン座標への変換と画面内判定
    bool CalculateScreenPosition(const DirectX::SimpleMath::Vector3& targetPos, 
        const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, 
        const D3D11_VIEWPORT& viewport, DirectX::SimpleMath::Vector3& outScreenPos) const;

    // ロックオンマーカーのスプライト描画
    void DrawLockOnMarker(DirectX::SpriteBatch* spriteBatch,
        const DirectX::SimpleMath::Vector3& screenPos, bool isHardLock, float scaleMultiplier);

private:
    // --- 描画・配置用の調整定数 ---
    static constexpr float TARGET_Y_OFFSET = 1.5f;       //< ターゲットモデルの中心からの上方オフセット
    static constexpr float MARKER_SCALE = 0.25f;         //< ロックオンマーカーの基本描画スケール
    static constexpr float ROTATION_SPEED = 2.0f;        //< ハードロック時の回転速度
	static constexpr float HALF_RATIO = 0.5f;            //< 画面中心や半分を計算するための割合

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureSoftLock; //< ソフトロック用テクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureHardLock; //< ハードロック用テクスチャ

    float m_rotationTimer;                                              //< 演出用回転タイマー

    DirectX::SimpleMath::Vector2 m_crosshairPosition;                   //< クロスヘアの位置
};