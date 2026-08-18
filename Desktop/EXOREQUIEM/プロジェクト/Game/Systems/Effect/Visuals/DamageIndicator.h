/*****************************************************************//**
 * @file    DamageIndicator.h
 * @brief   攻撃を受けた方向を画面上に表示するダメージインディケーターの制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <d3d11.h>

class DamageIndicator
{
private:
    // ヒット情報の構造体
    struct HitData
    {
        DirectX::SimpleMath::Vector2 direction;                     //< 画面上の向き
        float timer = 0.0f;                                         //< 経過時間 (0.0 -> 1.0)
    };

    // シェーダーに送る定数バッファ
    struct ConstantBufferData
    {
        DirectX::SimpleMath::Vector2 direction;                     //< 表示方向
        float timer;                                                //< アニメーション時間
        float aspectRatio;                                          //< 画面アスペクト比
    };

public:
    // コンストラクタ
    DamageIndicator();
    // デストラクタ
    ~DamageIndicator();
    // 初期化処理
    void Initialize(ID3D11Device* device);
    // 更新処理
    void Update(float dt);
    // 描画処理
    void Render(ID3D11DeviceContext* context);

    // 攻撃を受けたときに呼ぶ関数
    void AddHit(const DirectX::SimpleMath::Vector3& playerPos, const DirectX::SimpleMath::Vector3& camForward,
        const DirectX::SimpleMath::Vector3& camRight, const DirectX::SimpleMath::Vector3& enemyPos);

private:
	// --- 調整用定数パラメータ ---
	static constexpr float MAX_LIFETIME = 1.0f;             //< ヒット表示の最大寿命（秒）

	static constexpr float FADE_SPEED_MULTIPLIER = 1.5f;    //< フェードアウトの速度倍率（1.5倍速で消える）

	static constexpr UINT  INDICATOR_VERTEX_COUNT = 3;      //< インディケーターを構成する頂点数

    // --- メンバ変数 ---
    std::vector<HitData>  m_hits;                                   //< 現在表示中のダメージ情報リスト

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;                //< 頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_ps;                //< ピクセルシェーダー
    Microsoft::WRL::ComPtr<ID3D11Buffer>       m_constantBuffer;    //< 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11BlendState>   m_additiveBlend;     //< 加算合成用ステート
};