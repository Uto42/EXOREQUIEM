/*****************************************************************//**
 * @file    MachineGun.cpp
 * @brief   マシンガンの残弾管理、リロード制御、および交互発射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Weapon/MachineGun.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Systems/Sound/SoundManager.h"

 /**
  * @brief コンストラクタ
  */
MachineGun::MachineGun()
	: WeaponBase(GUN_MAX_AMMO, GUN_FIRE_INTERVAL, GUN_RELOAD_COOL_DOWN_TIME)
	, m_fireGunFromRight(true)
{
}

/**
 * @brief 発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimPosOrDir 基本の照準方向または座標
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target 明示的なターゲットロボット（存在する場合）
 * @param[in] projectileManager 弾丸プール管理者
 * @param[in] lockOnSystem ロックオンシステムへの参照
 * @param[in] manualTarget 手動設定されたターゲットポインタ
 */
void MachineGun::TryFire(const DirectX::SimpleMath::Vector3& robotPos, 
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimPosOrDir,
	const Robot* owner, const Robot* target, ProjectileManager* projectileManager,
	LockOnSystem* lockOnSystem, const Robot* manualTarget)
{
	UNREFERENCED_PARAMETER(robotRot);

	if (m_runtime.GetCurrentAmmo() > 0 && m_runtime.GetFireTimer() <= 0.0f && !m_runtime.IsReloading() && projectileManager)
	{
		// ターゲットの決定 (WeaponBaseの共通処理を使用)
		const Robot* targetRobot = DetermineTarget(target, lockOnSystem, manualTarget);

		DirectX::SimpleMath::Vector3 baseDir = aimPosOrDir;
		baseDir.Normalize();

		// 銃口位置の計算
		DirectX::SimpleMath::Vector3 gunPos =
			CalculateGunPosition(robotPos, baseDir, owner);

		// 照準方向（偏差射撃）の計算
		DirectX::SimpleMath::Vector3 finalAimDir =
			CalculateAimDirection(robotPos, gunPos, baseDir, targetRobot, owner);

		// 発射実行
		FireGun(gunPos, finalAimDir, owner, projectileManager);

		// ステータス更新
		m_runtime.DecrementAmmo();
		m_runtime.SetFireTimer(m_runtime.GetFireInterval());
		m_fireGunFromRight = !m_fireGunFromRight;
	}
}

/**
 * @brief 銃弾の発射実行
 * @param[in] gunPos 銃口の座標
 * @param[in] aimDir 発射方向
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] projectileManager 弾丸プール管理者
 */
void MachineGun::FireGun(const DirectX::SimpleMath::Vector3& gunPos,
	const DirectX::SimpleMath::Vector3& aimDir, const Robot* owner, ProjectileManager* projectileManager)
{
	// 銃口の向きに基づいて弾丸の回転を計算
	DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;

	DirectX::SimpleMath::Matrix lookAt =
		DirectX::SimpleMath::Matrix::CreateWorld(DirectX::SimpleMath::Vector3::Zero, aimDir, up);

	DirectX::SimpleMath::Quaternion fireRotation =
		DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookAt);

	// 弾丸の発射
	projectileManager->ShootBullet(gunPos, fireRotation, owner);
	EffectSystem::Instance()->SpawnMuzzleFlash(gunPos, aimDir);
	SoundManager::Instance().PlaySE(L"SE_Gun");
}

/**
 * @brief 銃口位置の計算
 * @param[in] robotPos ロボットの現在座標
 * @param[in] baseDir 基準となる照準方向
 * @param[in] owner 発射元のロボットポインタ
 * @return 計算された銃口のワールド座標
 */
DirectX::SimpleMath::Vector3 MachineGun::CalculateGunPosition(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Vector3& baseDir, const Robot* owner) const
{
	// 基準方向と上方向の外積から右方向ベクトルを計算
	DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(baseDir);
	if (right.LengthSquared() < EPSILON_SQUARED) right = DirectX::SimpleMath::Vector3::Right;
	right.Normalize();

	// 発射主がボスかどうかでオフセットを分岐
	if (dynamic_cast<const BossRobot*>(owner) != nullptr)
	{
		float currentOffsetX = m_fireGunFromRight ? BOSS_OFFSET_RIGHT : BOSS_OFFSET_LEFT;
		return robotPos
			+ (right * currentOffsetX)
			+ (DirectX::SimpleMath::Vector3::Up * BOSS_OFFSET_UP)
			+ (baseDir * BOSS_OFFSET_FORWARD);
	}
	else
	{
		float gunSideOffset = m_fireGunFromRight ? GUN_OFFSET_RIGHT : GUN_OFFSET_LEFT;
		return robotPos
			+ (right * gunSideOffset)
			+ (DirectX::SimpleMath::Vector3::Up * GUN_OFFSET_UP)
			+ (baseDir * GUN_OFFSET_FORWARD);
	}
}

/**
 * @brief 偏差射撃を含む最終的な照準方向の計算
 * @param[in] robotPos ロボットの現在座標
 * @param[in] gunPos 銃口の座標
 * @param[in] baseDir 基準となる照準方向
 * @param[in] targetRobot ターゲットのロボットポインタ
 * @param[in] owner 発射元のロボットポインタ
 * @return 偏差射撃を考慮した最終的な照準方向ベクトル
 */
DirectX::SimpleMath::Vector3 MachineGun::CalculateAimDirection(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Vector3& gunPos, const DirectX::SimpleMath::Vector3& baseDir,
	const Robot* targetRobot, const Robot* owner) const
{
	DirectX::SimpleMath::Vector3 finalAimDir = baseDir;

	if (targetRobot)
	{
		// ターゲットの位置と速度を取得
		DirectX::SimpleMath::Vector3 targetPos = targetRobot->GetAimPosition();

		if (dynamic_cast<const BossRobot*>(owner) != nullptr)
		{
			targetPos.y += BOSS_AIM_HEIGHT_OFFSET;
		}

		DirectX::SimpleMath::Vector3 targetVel = targetRobot->GetVelocity();

		// 算出された銃口位置ではなく、以前と同じ「ロボットの座標」からの距離で計算する
		float timeToHit = DirectX::SimpleMath::Vector3::Distance(robotPos, targetPos) / BULLET_SPEED;

		// 最大予測時間を制限し、予測地点を算出
		float predictionTime = std::min(timeToHit, MAX_PREDICTION_TIME);
		DirectX::SimpleMath::Vector3 futurePos = targetPos + (targetVel * predictionTime);

		// 現在位置と予測位置を補間して最終的な射撃目標を決定
		DirectX::SimpleMath::Vector3 finalPoint =
			DirectX::SimpleMath::Vector3::Lerp(targetPos, futurePos, PREDICTION_BLEND_RATE);

		if (dynamic_cast<const BossRobot*>(owner) != nullptr)
		{
			// ボス：巨体のため、高い銃口からターゲットへ正確に撃ち下ろす
			finalAimDir = finalPoint - gunPos;
		}
		else
		{
			// プレイヤー：TPS視点での下撃ち感を防ぐため、足元基準の角度（水平）を使用
			finalAimDir = finalPoint - robotPos;
		}
	}
	else
	{
		// ロックオンしていないとき、弾の飛ぶ角度を少しだけ上に傾ける
		finalAimDir.y += DEFAULT_AIM_OFFSET_Y;
	}

	finalAimDir.Normalize();
	return finalAimDir;
}