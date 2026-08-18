/*****************************************************************//**
 * @file    CruiseMissileLauncher.h
 * @brief   巡航ミサイルの残弾管理、リロード制御、および左右連射ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Weapon/WeaponBase.h"

class Robot;
class ProjectileManager;
class LockOnSystem;

class CruiseMissileLauncher : public WeaponBase
{
public:
	// コンストラクタ
	CruiseMissileLauncher();
	// デストラクタ
	virtual ~CruiseMissileLauncher() = default;

	// 発射試行 (IWeaponインターフェースの実装)
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
	// ミサイルツイン発射の実行
	void FireMissilePair(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot,
		const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* target,
		const Robot* owner,
		ProjectileManager* projectileManager);

	// 初期射出方向の計算
	DirectX::SimpleMath::Vector3 CalculateLaunchDirection(
		const DirectX::SimpleMath::Vector3& right,
		const DirectX::SimpleMath::Vector3& up,
		const DirectX::SimpleMath::Vector3& forward,
		bool isRightSide) const;

	// 単発ミサイルの発射処理
	void FireSingleMissile(
		const DirectX::SimpleMath::Vector3& spawnPos,
		const DirectX::SimpleMath::Vector3& launchDir,
		const DirectX::SimpleMath::Vector3& aimTargetPos,
		const Robot* target,
		const Robot* owner,
		ProjectileManager* projectileManager);

private:
	// --- 調整用定数パラメータ ---

	// 装弾・リロード系
	static constexpr int MISSILE_MAX_AMMO = 20;                     //< 最大装弾数
	static constexpr float MISSILE_FIRE_INTERVAL = 0.1f;            //< 連射間隔（秒）
	static constexpr float MISSILE_RELOAD_COOL_DOWN_TIME = 3.0f;    //< リロード所要時間（秒）

	// ミサイル性能系
	static constexpr float MISSILE_CRUISE_SPEED = 65.0f;            //< 巡航速度
	static constexpr float MISSILE_CRUISE_TURN = 6.5f;              //< 旋回性能
	static constexpr float MISSILE_CRUISE_LIFE = 4.0f;              //< 寿命

	// 射出方向・軌道系
	static constexpr float LAUNCH_DIR_RIGHT_WEIGHT = 1.0f;          //< 射出方向の横への広がり重み
	static constexpr float LAUNCH_DIR_UP_WEIGHT = 0.8f;             //< 射出方向の上へのポップアップ重み
	static constexpr float LAUNCH_DIR_BACK_WEIGHT = -1.2f;          //< 射出方向の後方へのバックブラスト重み

	// 発射位置オフセット系
	static constexpr float SHOULDER_OFFSET_X = 1.0f;                //< 肩のミサイル発射位置（左右の幅オフセット）
	static constexpr float SHOULDER_OFFSET_Y = 2.5f;                //< 肩のミサイル発射位置（高さオフセット）
};