/*****************************************************************//**
 * @file    CruiseMissileLauncher.cpp
 * @brief   巡航ミサイルの残弾管理、リロード制御、および左右連射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Weapon/CruiseMissileLauncher.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/Systems/Sound/SoundManager.h"

 /**
  * @brief コンストラクタ
  */
CruiseMissileLauncher::CruiseMissileLauncher()
	: WeaponBase(MISSILE_MAX_AMMO, MISSILE_FIRE_INTERVAL, MISSILE_RELOAD_COOL_DOWN_TIME)
{
}

/**
 * @brief 発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの回転クォータニオン
 * @param[in] aimPosOrDir 基本の照準方向または座標 (WeaponControllerから渡される)
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target 優先ターゲット（指定があれば使用）
 * @param[in] projectileManager 弾丸プール管理者
 * @param[in] lockOnSystem ロックオンシステムへの参照
 * @param[in] manualTarget 手動設定されたターゲットポインタ
 */
void CruiseMissileLauncher::TryFire(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimPosOrDir,
	const Robot* owner, const Robot* target, ProjectileManager* projectileManager,
	LockOnSystem* lockOnSystem, const Robot* manualTarget)
{
	if (m_runtime.GetCurrentAmmo() > 0 && m_runtime.GetFireTimer() <= 0.0f && !m_runtime.IsReloading())
	{
		if (projectileManager)
		{
			// ターゲットの決定 (WeaponBaseの共通処理を使用)
			const Robot* finalTarget = DetermineTarget(target, lockOnSystem, manualTarget);

			// ミサイル発射の実行
			FireMissilePair(robotPos, robotRot, aimPosOrDir, finalTarget, owner, projectileManager);
			m_runtime.SetFireTimer(m_runtime.GetFireInterval());
		}
	}
}

/**
 * @brief ミサイルツイン発射の実行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの回転クォータニオン
 * @param[in] aimTargetPos 確定した最終目標座標
 * @param[in] target 誘導対象のターゲットポインタ
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] projectileManager 弾丸プール管理者
 */
void CruiseMissileLauncher::FireMissilePair(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimTargetPos,
	const Robot* target, const Robot* owner, ProjectileManager* projectileManager)
{
	// ロボットの回転から、上方向・右方向・前方方向のベクトルを計算
	DirectX::SimpleMath::Vector3 up =
		DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Up, robotRot);
	DirectX::SimpleMath::Vector3 right =
		DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Right, robotRot);
	DirectX::SimpleMath::Vector3 forward =
		DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, robotRot);

	// 右肩ミサイルの生成と発射音再生
	if (m_runtime.GetCurrentAmmo() > 0)
	{
		DirectX::SimpleMath::Vector3 rightSideDir = CalculateLaunchDirection(right, up, forward, true);

		DirectX::SimpleMath::Vector3 rightShoulderPos =
			robotPos + (right * SHOULDER_OFFSET_X) + (up * SHOULDER_OFFSET_Y);

		FireSingleMissile(rightShoulderPos, rightSideDir, aimTargetPos, target, owner, projectileManager);
	}

	// 左肩ミサイルの生成と発射音再生
	if (m_runtime.GetCurrentAmmo() > 0)
	{
		DirectX::SimpleMath::Vector3 leftSideDir = CalculateLaunchDirection(right, up, forward, false);
		DirectX::SimpleMath::Vector3 leftShoulderPos = robotPos + (right * -SHOULDER_OFFSET_X) + (up * SHOULDER_OFFSET_Y);

		FireSingleMissile(leftShoulderPos, leftSideDir, aimTargetPos, target, owner, projectileManager);
	}
}

/**
 * @brief 初期射出方向の計算
 * @param[in] right ロボットの右方向ベクトル
 * @param[in] up ロボットの上方向ベクトル
 * @param[in] forward ロボットの前方方向ベクトル
 * @param[in] isRightSide 右肩からの発射かどうか（trueで右、falseで左）
 * @return 計算された初期射出方向ベクトル
 */
DirectX::SimpleMath::Vector3 CruiseMissileLauncher::CalculateLaunchDirection(
	const DirectX::SimpleMath::Vector3& right, const DirectX::SimpleMath::Vector3& up,
	const DirectX::SimpleMath::Vector3& forward, bool isRightSide) const
{
	// ミサイルの初期射出方向を計算
	float rightWeight = isRightSide ? LAUNCH_DIR_RIGHT_WEIGHT : -LAUNCH_DIR_RIGHT_WEIGHT;

	// 初期射出方向の計算
	DirectX::SimpleMath::Vector3 launchDir =
		(right * rightWeight) + (up * LAUNCH_DIR_UP_WEIGHT) + (forward * LAUNCH_DIR_BACK_WEIGHT);

	launchDir.Normalize();

	return launchDir;
}

/**
 * @brief 単発ミサイルの発射処理
 * @param[in] spawnPos ミサイルの生成座標
 * @param[in] launchDir ミサイルの射出方向
 * @param[in] aimTargetPos 確定した最終目標座標
 * @param[in] target 誘導対象のターゲットポインタ
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] projectileManager 弾丸プール管理者
 */
void CruiseMissileLauncher::FireSingleMissile(const DirectX::SimpleMath::Vector3& spawnPos,
	const DirectX::SimpleMath::Vector3& launchDir, const DirectX::SimpleMath::Vector3& aimTargetPos,
	const Robot* target, const Robot* owner, ProjectileManager* projectileManager)
{
	// ProjectileManagerへ目標座標（aimTargetPos）を渡す
	projectileManager->ShootCruise(spawnPos, launchDir, target,
		MISSILE_CRUISE_SPEED, MISSILE_CRUISE_TURN, MISSILE_CRUISE_LIFE, owner, aimTargetPos);

	SoundManager::Instance().PlaySE(L"SE_Missile");
	m_runtime.DecrementAmmo();
}