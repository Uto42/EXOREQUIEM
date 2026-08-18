/*****************************************************************//**
 * @file    MachineGun.h
 * @brief   マシンガンの残弾管理、リロード制御、および交互発射ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Weapon/WeaponBase.h"

class Robot;
class ProjectileManager;
class LockOnSystem;

class MachineGun : public WeaponBase
{
public:
	// コンストラクタ
	MachineGun();
	// デストラクタ
	virtual ~MachineGun() = default;

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
	// 銃弾の発射実行
	void FireGun(
		const DirectX::SimpleMath::Vector3& gunPos,
		const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* owner,
		ProjectileManager* projectileManager);

	// 銃口位置（オフセット）の計算
	DirectX::SimpleMath::Vector3 CalculateGunPosition(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Vector3& baseDir,
		const Robot* owner) const;

	// 偏差射撃を含む最終的な照準方向の計算
	DirectX::SimpleMath::Vector3 CalculateAimDirection(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Vector3& gunPos,
		const DirectX::SimpleMath::Vector3& baseDir,
		const Robot* targetRobot,
		const Robot* owner) const;

private:
	// --- 調整用定数パラメータ ---

	// 装弾・リロード系
	static constexpr int GUN_MAX_AMMO = 30;                     //< 最大装弾数
	static constexpr float GUN_FIRE_INTERVAL = 0.1f;            //< 連射間隔（秒）
	static constexpr float GUN_RELOAD_COOL_DOWN_TIME = 2.0f;    //< リロード所要時間（秒）

	// 銃口位置オフセット系 (プレイヤー)
	static constexpr float GUN_OFFSET_RIGHT = 0.9f;             //< 銃口の右オフセット
	static constexpr float GUN_OFFSET_LEFT = -0.9f;             //< 銃口の左オフセット
	static constexpr float GUN_OFFSET_UP = 1.2f;                //< 銃口の上オフセット
	static constexpr float GUN_OFFSET_FORWARD = 1.5f;           //< 銃口の前オフセット

	// 銃口位置オフセット系 (ボス)
	static constexpr float BOSS_OFFSET_RIGHT = 4.0f;            //< ボス用銃口の右オフセット
	static constexpr float BOSS_OFFSET_LEFT = -5.0f;            //< ボス用銃口の左オフセット
	static constexpr float BOSS_OFFSET_UP = 5.0f;               //< ボス用銃口の上オフセット
	static constexpr float BOSS_OFFSET_FORWARD = 8.0f;          //< ボス用銃口の前オフセット
	static constexpr float BOSS_AIM_HEIGHT_OFFSET = 1.0f;       //< ボスが狙うターゲットの高さ補正値

	// 予測射撃・計算系
	static constexpr float BULLET_SPEED = 100.0f;               //< 偏差射撃計算用の弾速
	static constexpr float MAX_PREDICTION_TIME = 0.3f;          //< 未来予測の上限時間（秒）
	static constexpr float PREDICTION_BLEND_RATE = 0.8f;        //< 予測座標と現在座標のブレンド率
	static constexpr float EPSILON_SQUARED = 0.001f;            //< ゼロベクトル判定用の微小値二乗
	static constexpr float DEFAULT_AIM_OFFSET_Y = 0.01f;        //< ターゲット不在時の微小仰角

	// --- 状態値・パラメータ ---
	bool m_fireGunFromRight = true;                             //< 左右交互発射のフラグ（trueで右から発射）
};