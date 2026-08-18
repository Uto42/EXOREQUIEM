/*****************************************************************//**
 * @file    Shotgun.h
 * @brief   ショットガンの残弾管理、リロード制御、および3方向発射ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Weapon/WeaponBase.h"

class Robot;
class ProjectileManager;
class LockOnSystem;

class Shotgun : public WeaponBase
{
public:
	// コンストラクタ
	Shotgun();
	// デストラクタ
	virtual ~Shotgun() = default;

	// 発射試行
	virtual void TryFire(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot,
		const DirectX::SimpleMath::Vector3& aimPosOrDir,
		const Robot* owner,
		const Robot* target,
		ProjectileManager* projectileManager,
		LockOnSystem* lockOnSystem,
		const Robot* manualTarget) override;

private:
	// 最終的な発射方向の計算（偏差射撃）
	DirectX::SimpleMath::Vector3 CalculateAimDirection(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Vector3& baseAimDir,
		const Robot* targetRobot) const;

	// 銃口位置の計算
	DirectX::SimpleMath::Vector3 CalculateGunPosition(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Vector3& finalAimDir,
		const Robot* owner) const;

	// 銃口位置を基準とした最終的な偏差射撃方向の再計算
	DirectX::SimpleMath::Vector3 CalculateFinalForward(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Vector3& gunPos,
		const DirectX::SimpleMath::Vector3& baseAimDir,
		const Robot* targetRobot,
		const Robot* owner) const;

	// 散弾の展開と発射実行
	void ExecuteSpreadFire(
		const DirectX::SimpleMath::Vector3& gunPos,
		const DirectX::SimpleMath::Vector3& forward,
		const Robot* owner,
		ProjectileManager* projectileManager);

	// 銃弾の単発発射実行
	void FireGun(
		const DirectX::SimpleMath::Vector3& gunPos,
		const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* owner,
		ProjectileManager* projectileManager);

private:
	// --- 調整用定数パラメータ ---

	// 装弾・リロード系
	static constexpr int SHOTGUN_MAX_AMMO = 8;                    //< 最大装弾数
	static constexpr float SHOTGUN_FIRE_INTERVAL = 0.8f;          //< 連射間隔（秒）
	static constexpr float SHOTGUN_RELOAD_COOL_DOWN_TIME = 2.5f;  //< リロード所要時間（秒）

	// 銃口位置オフセット系 (プレイヤー)
	static constexpr float SHOTGUN_OFFSET_RIGHT = 0.9f;           //< プレイヤー銃口の右オフセット
	static constexpr float SHOTGUN_OFFSET_LEFT = -0.9f;           //< プレイヤー銃口の左オフセット
	static constexpr float SHOTGUN_OFFSET_UP = 1.2f;              //< プレイヤー銃口の上オフセット
	static constexpr float SHOTGUN_OFFSET_FORWARD = 1.5f;         //< プレイヤー銃口の前オフセット

	// 銃口位置オフセット・連射系 (ボス)
	static constexpr float BOSS_FIRE_INTERVAL = 0.6f;             //< ボス専用の連射間隔（鬼連射用）
	static constexpr float BOSS_OFFSET_RIGHT = 4.0f;              //< ボス用銃口の右オフセット
	static constexpr float BOSS_OFFSET_LEFT = -5.0f;              //< ボス用銃口の左オフセット
	static constexpr float BOSS_OFFSET_UP = 5.0f;                 //< ボス用銃口の上オフセット
	static constexpr float BOSS_OFFSET_FORWARD = 8.0f;            //< ボス用銃口の前オフセット

	// 予測射撃・散弾計算系
	static constexpr float SHOTGUN_SPREAD_ANGLE = 0.087f;         //< 拡散角度（ラジアン）約5度
	static constexpr int SPREAD_PELLET_COUNT = 8;                 //< 周囲に展開する散弾の数
	static constexpr float BULLET_SPEED = 100.0f;                 //< 偏差射撃計算用の弾速
	static constexpr float MAX_PREDICTION_TIME = 0.3f;            //< 未来予測の上限時間（秒）
	static constexpr float PREDICTION_BLEND_RATE = 0.8f;          //< 予測座標と現在座標のブレンド率
	static constexpr float EPSILON_SQUARED = 0.001f;              //< ゼロベクトル判定用の微小値二乗
	static constexpr float DEFAULT_AIM_OFFSET_Y = 0.01f;          //< ターゲット不在時の微小仰角

	// --- 状態値・パラメータ ---
	bool m_fireGunFromRight = true;                               //< 左右交互発射のフラグ（trueで右から発射）
};