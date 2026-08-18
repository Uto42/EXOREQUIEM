/*****************************************************************//**
 * @file    Bullet.h
 * @brief   マシンガンの弾丸発射とリロード制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Projectile/ProjectileBase.h"

class StageManager;

class Bullet : public ProjectileBase
{
public:
	// コンストラクタ
	Bullet();
	// デストラクタ
	virtual ~Bullet() = default;
	// 初期化処理
	void Initialize(const DirectX::SimpleMath::Vector3& startPosition,
		const DirectX::SimpleMath::Quaternion& startRotation, const Robot* owner);
	// 更新処理
	void Update(float dt) override;
	// 描画処理
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj) override;

	// 当たり判定済みかの取得
	bool IsHit() const { return m_isHit; }
	// 当たり判定済みフラグの設定
	void SetHit(bool hit) { m_isHit = hit; }

	// 当たり判定（球）の取得
	DirectX::BoundingSphere GetCollider() const override;

private:
	// ステージ（壁・地面）との衝突判定
	void CheckWorldCollision(float dt);

private:
	// --- 調整用定数パラメータ ---
	static constexpr float BULLET_SPEED = 200.0f;                   //< 弾速
	static constexpr float MAX_LIFE_TIME = 2.0f;                    //< 消滅までの時間
	static constexpr float BULLET_DAMAGE = 5.0f;                    //< ダメージ
	static constexpr float COLLISION_RADIUS = 0.5f;                 //< 当たり判定の半径
	static constexpr float MODEL_DEFAULT_SCALE = 0.5f;              //< 描画時のモデルの基本スケール
	static constexpr float MODEL_YAW_OFFSET_DEG = 0.0f;             //< モデル本来の向きを補正する初期回転角度

	static constexpr float GROUND_HEIGHT_THRESHOLD = 0.5f;		    //< 地面との衝突判定の高さしきい値
	static constexpr float ZERO_DIVISION_EPSILON = 0.001f;		    //< ゼロ除算防止用の極小値

	// --- 状態値・パラメータ ---
	bool m_isHit = false;                                           //< 当たり判定済みフラグ
};