/*****************************************************************//**
 * @file    TopAttackMissileLauncher.cpp
 * @brief   高高度ミサイル（トップアタック）の残弾管理、リロード制御、および左右連射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "TopAttackMissileLauncher.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/Systems/Sound/SoundManager.h"

 /**
  * @brief コンストラクタ
  */
TopAttackMissileLauncher::TopAttackMissileLauncher()
	: WeaponBase(MISSILE_MAX_AMMO, MISSILE_FIRE_INTERVAL, MISSILE_RELOAD_COOL_DOWN_TIME)
{
}

/**
 * @brief 発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの回転クォータニオン
 * @param[in] aimPosOrDir 照準の目標座標または方向
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] target 直接指定されたターゲット
 * @param[in] projectileManager 弾丸生成システム
 * @param[in] lockOnSystem ロックオンシステム
 * @param[in] manualTarget 手動で指定されたターゲット
 */
void TopAttackMissileLauncher::TryFire(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimPosOrDir,
	const Robot* owner, const Robot* target, ProjectileManager* projectileManager,
	LockOnSystem* lockOnSystem, const Robot* manualTarget)
{
	if (m_runtime.GetCurrentAmmo() > 0 && m_runtime.GetFireTimer() <= 0.0f && !m_runtime.IsReloading() && projectileManager)
	{
		// ターゲットの決定 (WeaponBaseの共通処理を使用)
		const Robot* finalTarget = DetermineTarget(target, lockOnSystem, manualTarget);

		// トップアタックミサイルのツイン発射を実行
		FireMissilePair(robotPos, robotRot, aimPosOrDir, finalTarget, owner, projectileManager);
		m_runtime.SetFireTimer(m_runtime.GetFireInterval());
	}
}

/**
 * @brief 機体の向き・旋回を考慮した基本ベクトルの算出
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの回転クォータニオン
 * @param[in] target 誘導対象のターゲットポインタ
 * @param[in] owner 発射主のロボットポインタ
 * @param[out] outRight 計算された右方向ベクトル
 * @param[out] outUp 計算された上方向ベクトル
 * @param[out] outForward 計算された前方向ベクトル
 */
void TopAttackMissileLauncher::CalculateBasisVectors(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const Robot* target, const Robot* owner,
	DirectX::SimpleMath::Vector3& outRight, DirectX::SimpleMath::Vector3& outUp,
	DirectX::SimpleMath::Vector3& outForward) const
{
	// 機体の向きに基づいた基本ベクトルの算出
	outUp = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Up, robotRot);
	outRight = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Right, robotRot);
	outForward = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, robotRot);

	// ボスが撃った場合は背中の巨大ミサイルポッドに合わせて座標と向きを補正する
	if (dynamic_cast<const BossRobot*>(owner) != nullptr)
	{
		// ボスにターゲットがいる場合は上半身の旋回を考慮して射出方向を計算する
		if (target != nullptr)
		{
			DirectX::SimpleMath::Vector3 dirToTarget = target->GetPosition() - robotPos;
			dirToTarget.y = 0.0f; // 上半身の旋回は水平方向のみに制限する

			// 距離が近すぎるなど、ゼロベクトルによる計算バグを防ぐための安全対策
			if (dirToTarget.LengthSquared() > EPSILON_SQUARED)
			{
				dirToTarget.Normalize();
				outForward = dirToTarget; // 前方向をターゲットへの向きに置き換える

				// 新しい前方向に合わせて右方向と上方向を正しく作り直す
				outRight = DirectX::SimpleMath::Vector3::Up.Cross(outForward);
				outRight.Normalize();

				// 新しい右方向に合わせて上方向を正しく作り直す
				outUp = outForward.Cross(outRight);
				outUp.Normalize();
			}
		}
	}
}

/**
 * @brief 発射位置オフセットの取得
 * @param[in] owner 発射主のロボットポインタ
 * @param[out] outOffsetXRight 右側のXオフセット
 * @param[out] outOffsetXLeft 左側のXオフセット
 * @param[out] outOffsetY Yオフセット
 * @param[out] outOffsetZ Zオフセット
 */
void TopAttackMissileLauncher::GetLauncherOffsets(const Robot* owner,
	float& outOffsetXRight, float& outOffsetXLeft, float& outOffsetY, float& outOffsetZ) const
{
	// 発射座標オフセットの初期化（プレイヤー用の値を基本とする）
	outOffsetXRight = SHOULDER_OFFSET_X;
	outOffsetXLeft = -SHOULDER_OFFSET_X;

	// Y軸とZ軸のオフセットはプレイヤーとボスで共通の値を使用する
	outOffsetY = SHOULDER_OFFSET_Y;
	outOffsetZ = SHOULDER_OFFSET_Z;

	// ボスが撃った場合は背中の巨大ミサイルポッドに合わせて座標と向きを補正する
	if (dynamic_cast<const BossRobot*>(owner) != nullptr)
	{
		outOffsetXRight = BOSS_OFFSET_RIGHT_X;
		outOffsetXLeft = BOSS_OFFSET_LEFT_X;
		outOffsetY = BOSS_OFFSET_Y;
		outOffsetZ = BOSS_OFFSET_Z;
	}
}

/**
 * @brief 初期射出方向の計算
 * @param[in] right 計算された右方向ベクトル
 * @param[in] up 計算された上方向ベクトル
 * @param[in] forward 計算された前方向ベクトル
 * @param[in] isRightSide 右側からの発射かどうか
 * @return 計算された初期射出方向ベクトル
 */
DirectX::SimpleMath::Vector3 TopAttackMissileLauncher::CalculateLaunchDirection(
	const DirectX::SimpleMath::Vector3& right, const DirectX::SimpleMath::Vector3& up,
	const DirectX::SimpleMath::Vector3& forward, bool isRightSide) const
{
	// ミサイルの初期射出方向を計算（真上に向けて勢いよくポップアップさせるための重み付け）
	float rightWeight = isRightSide ? LAUNCH_DIR_RIGHT_WEIGHT : -LAUNCH_DIR_RIGHT_WEIGHT;

	// 重み付けされたベクトルを合成して射出方向を決定
	DirectX::SimpleMath::Vector3 launchDir = 
		(right * rightWeight) + (up * LAUNCH_DIR_UP_WEIGHT) + (forward * LAUNCH_DIR_BACK_WEIGHT);
	launchDir.Normalize();

	return launchDir;
}

/**
 * @brief 単発ミサイルの発射処理
 * @param[in] spawnPos ミサイルの生成座標
 * @param[in] launchDir ミサイルの射出方向
 * @param[in] aimTargetPos 照準の目標座標
 * @param[in] target 誘導対象のターゲットポインタ
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] projectileManager 弾丸生成システム
 */
void TopAttackMissileLauncher::FireSingleMissile(const DirectX::SimpleMath::Vector3& spawnPos,
	const DirectX::SimpleMath::Vector3& launchDir, const DirectX::SimpleMath::Vector3& aimTargetPos,
	const Robot* target, const Robot* owner, ProjectileManager* projectileManager)
{
	// ミサイルの生成と発射
	projectileManager->ShootTopAttack(spawnPos, launchDir, target,
		MISSILE_TOP_SPEED, MISSILE_TOP_TURN, MISSILE_TOP_LIFE, owner, aimTargetPos);

	SoundManager::Instance().PlaySE(L"SE_Missile");
	m_runtime.DecrementAmmo();
}

/**
 * @brief ミサイルツイン発射の実行（トップアタック弾道用）
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの回転クォータニオン
 * @param[in] aimTargetPos 照準の目標座標
 * @param[in] target 誘導対象のターゲットポインタ
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] projectileManager 弾丸生成システム
 */
void TopAttackMissileLauncher::FireMissilePair(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimTargetPos,
	const Robot* target, const Robot* owner, ProjectileManager* projectileManager)
{
	// 機体の向き・旋回を考慮した基本ベクトルの算出
	DirectX::SimpleMath::Vector3 right, up, forward;
	CalculateBasisVectors(robotPos, robotRot, target, owner, right, up, forward);

	// 発射座標オフセットの取得（プレイヤーとボスで異なる）
	float offsetRightX, offsetLeftX, offsetY, offsetZ;
	GetLauncherOffsets(owner, offsetRightX, offsetLeftX, offsetY, offsetZ);

	// 右側（プレイヤーの右肩、またはボスの右ポッド）からミサイルを発射
	if (m_runtime.GetCurrentAmmo() > 0)
	{
		DirectX::SimpleMath::Vector3 rightSideDir = CalculateLaunchDirection(right, up, forward, true);

		// 確定した機体基準のベクトルを使って発射座標を計算する（位置ズレ防止）
		DirectX::SimpleMath::Vector3 rightShoulderPos =
			robotPos + (right * offsetRightX) + (up * offsetY) + (forward * offsetZ);

		// 右側のミサイル発射
		FireSingleMissile(rightShoulderPos, rightSideDir, aimTargetPos, target, owner, projectileManager);
	}

	// 左側（プレイヤーの左肩、またはボスの左ポッド）からミサイルを発射
	if (m_runtime.GetCurrentAmmo() > 0)
	{
		DirectX::SimpleMath::Vector3 leftSideDir = CalculateLaunchDirection(right, up, forward, false);

		// 確定した機体基準のベクトルを使って発射座標を計算する（位置ズレ防止）
		DirectX::SimpleMath::Vector3 leftShoulderPos =
			robotPos + (right * offsetLeftX) + (up * offsetY) + (forward * offsetZ);

		// 左側のミサイル発射
		FireSingleMissile(leftShoulderPos, leftSideDir, aimTargetPos, target, owner, projectileManager);
	}
}