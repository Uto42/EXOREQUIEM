/*****************************************************************//**
 * @file    TopAttackMissileLauncher.h
 * @brief   高高度ミサイル（トップアタック）の残弾管理、リロード制御、および左右連射ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Weapon/WeaponBase.h"

class Robot;
class ProjectileManager;
class LockOnSystem;

class TopAttackMissileLauncher : public WeaponBase
{
public:
	// コンストラクタ
	TopAttackMissileLauncher();
	// デストラクタ
	virtual ~TopAttackMissileLauncher() = default;

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
		const DirectX::SimpleMath::Vector3& aimTargetPos,
		const Robot* target,
		const Robot* owner,
		ProjectileManager* projectileManager);

	// 機体の向き・旋回を考慮した基本ベクトルの算出
	void CalculateBasisVectors(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot,
		const Robot* target,
		const Robot* owner,
		DirectX::SimpleMath::Vector3& outRight,
		DirectX::SimpleMath::Vector3& outUp,
		DirectX::SimpleMath::Vector3& outForward) const;

	// 発射位置オフセットの取得
	void GetLauncherOffsets(
		const Robot* owner,
		float& outOffsetXRight,
		float& outOffsetXLeft,
		float& outOffsetY, 
		float& outOffsetZ) const;

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
	static constexpr int MISSILE_MAX_AMMO = 12;                     //< 最大装弾数
	static constexpr float MISSILE_FIRE_INTERVAL = 0.2f;            //< 連射間隔（秒）
	static constexpr float MISSILE_RELOAD_COOL_DOWN_TIME = 4.5f;    //< リロード所要時間（秒）

	// ミサイル性能系
	static constexpr float MISSILE_TOP_SPEED = 75.0f;               //< ミサイルの最高速度
	static constexpr float MISSILE_TOP_TURN = 5.5f;                 //< ミサイルの旋回性能
	static constexpr float MISSILE_TOP_LIFE = 6.0f;                 //< ミサイルの寿命（秒）

	// 射出方向・軌道系
	static constexpr float LAUNCH_DIR_RIGHT_WEIGHT = 0.4f;          //< 射出時の右方向への広がり具合
	static constexpr float LAUNCH_DIR_UP_WEIGHT = 1.8f;             //< 射出時に真上へ打ち上げる強さ
	static constexpr float LAUNCH_DIR_BACK_WEIGHT = -0.2f;          //< 射出時に後ろへ下がる強さ

	// 発射位置オフセット系 (プレイヤー)
	static constexpr float SHOULDER_OFFSET_X = 1.2f;                //< プレイヤー肩の発射位置（左右の幅）
	static constexpr float SHOULDER_OFFSET_Y = 2.7f;                //< プレイヤー肩の発射位置（高さ）
	static constexpr float SHOULDER_OFFSET_Z = 0.0f;                //< プレイヤー肩の発射位置（前後のズレ）

	// 発射位置オフセット系 (ボス)
	static constexpr float BOSS_OFFSET_RIGHT_X = 3.5f;              //< ボス用ポッドの発射位置（右の幅）
	static constexpr float BOSS_OFFSET_LEFT_X = -3.5f;              //< ボス用ポッドの発射位置（左の幅）
	static constexpr float BOSS_OFFSET_Y = 12.0f;                   //< ボス用ポッドの発射位置（高さ）
	static constexpr float BOSS_OFFSET_Z = -2.0f;                   //< ボス用ポッドの発射位置（少し後ろ）

	// 計算系
	static constexpr float EPSILON_SQUARED = 0.001f;                //< ゼロベクトル判定用の微小値二乗
};