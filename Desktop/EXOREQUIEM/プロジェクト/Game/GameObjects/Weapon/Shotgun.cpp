/*****************************************************************//**
 * @file    Shotgun.cpp
 * @brief   ショットガンの残弾管理、リロード制御、および3方向発射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Weapon/Shotgun.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Systems/Sound/SoundManager.h"

 /**
  * @brief コンストラクタ
  */
Shotgun::Shotgun()
	: WeaponBase(SHOTGUN_MAX_AMMO, SHOTGUN_FIRE_INTERVAL, SHOTGUN_RELOAD_COOL_DOWN_TIME)
	, m_fireGunFromRight(true)
{
}

/**
 * @brief 発射試行と散弾の展開処理
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimPosOrDir 基本の照準方向または座標
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] target 明示的なターゲットロボット（存在する場合）
 * @param[in] projectileManager 弾丸生成システム
 * @param[in] lockOnSystem ロックオンシステム
 * @param[in] manualTarget 手動で指定されたターゲット
 */
void Shotgun::TryFire(const DirectX::SimpleMath::Vector3& robotPos, 
	const DirectX::SimpleMath::Quaternion& robotRot,
	const DirectX::SimpleMath::Vector3& aimPosOrDir, const Robot* owner,
	const Robot* target, ProjectileManager* projectileManager,
	LockOnSystem* lockOnSystem, const Robot* manualTarget)
{
	UNREFERENCED_PARAMETER(robotRot);

	if (m_runtime.GetCurrentAmmo() > 0 && m_runtime.GetFireTimer() <= 0.0f && !m_runtime.IsReloading() && projectileManager)
	{
		// ターゲットの決定 (WeaponBaseの共通処理)
		const Robot* targetRobot = DetermineTarget(target, lockOnSystem, manualTarget);

		// 機体座標を基準とした発射方向の算出
		DirectX::SimpleMath::Vector3 finalAimDir =
			CalculateAimDirection(robotPos, aimPosOrDir, targetRobot);

		// 左右交互の発射口座標の算出
		DirectX::SimpleMath::Vector3 gunPos = CalculateGunPosition(robotPos, finalAimDir, owner);

		// 偏差射撃の射線再計算
		DirectX::SimpleMath::Vector3 forward =
			CalculateFinalForward(robotPos, gunPos, finalAimDir, targetRobot, owner);

		// 散弾の発射実行
		ExecuteSpreadFire(gunPos, forward, owner, projectileManager);

		// パラメータと発射口の更新
		m_runtime.DecrementAmmo();

		// ボスは固有の鬼連射設定を適用し、プレイヤーは標準間隔を適用する
		if (dynamic_cast<const BossRobot*>(owner) != nullptr)
		{
			m_runtime.SetFireTimer(BOSS_FIRE_INTERVAL);
		}
		else
		{
			m_runtime.SetFireTimer(m_runtime.GetFireInterval());
		}

		// 次回発射のために左右の銃口を切り替える
		m_fireGunFromRight = !m_fireGunFromRight;
	}
}

/**
 * @brief 最終的な発射方向の計算（偏差射撃）
 * @param[in] robotPos ロボットの現在座標
 * @param[in] baseAimDir 基本の射撃方向
 * @param[in] targetRobot 確定済みのターゲットポインタ
 * @return 補正された最終的な発射方向ベクトル
 */
DirectX::SimpleMath::Vector3 Shotgun::CalculateAimDirection(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Vector3& baseAimDir, const Robot* targetRobot) const
{
	DirectX::SimpleMath::Vector3 finalAimDir = baseAimDir;

	if (targetRobot)
	{
		// ターゲットの位置と速度を取得し、予測射撃のための未来位置を計算する
		DirectX::SimpleMath::Vector3 targetPos = targetRobot->GetAimPosition();
		DirectX::SimpleMath::Vector3 targetVel = targetRobot->GetVelocity();
		DirectX::SimpleMath::Vector3 myPos = robotPos;

		// ターゲットまでの距離と弾速から、命中までの時間を計算する
		float dist = DirectX::SimpleMath::Vector3::Distance(myPos, targetPos);
		float timeToHit = dist / BULLET_SPEED;

		// 予測時間を最大値で制限し、ターゲットの未来位置を算出する
		float predictionTime = std::min(timeToHit, MAX_PREDICTION_TIME);
		DirectX::SimpleMath::Vector3 futurePos = targetPos + (targetVel * predictionTime);

		// 予測位置と現在位置の間を補間して、滑らかな射撃方向を算出する
		DirectX::SimpleMath::Vector3 finalPoint =
			DirectX::SimpleMath::Vector3::Lerp(targetPos, futurePos, PREDICTION_BLEND_RATE);

		finalAimDir = finalPoint - myPos;
	}
	else
	{
		// ターゲット不在時は地面撃ちを防ぐために微小な仰角を付ける
		finalAimDir.y += DEFAULT_AIM_OFFSET_Y;
	}

	finalAimDir.Normalize();
	return finalAimDir;
}

/**
 * @brief 銃口位置の計算
 * @param[in] robotPos ロボットの現在座標
 * @param[in] finalAimDir 確定した発射方向ベクトル
 * @param[in] owner 発射主のロボットポインタ
 * @return 実際の銃口のワールド座標
 */
DirectX::SimpleMath::Vector3 Shotgun::CalculateGunPosition(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Vector3& finalAimDir, const Robot* owner) const
{
	DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(finalAimDir);
	if (right.LengthSquared() < EPSILON_SQUARED) right = DirectX::SimpleMath::Vector3::Right;
	right.Normalize();

	if (dynamic_cast<const BossRobot*>(owner) != nullptr)
	{
		// 左右交互のフラグに応じて、ボスの右腕・左腕のオフセットを使い分ける
		float currentOffsetX = m_fireGunFromRight ? BOSS_OFFSET_RIGHT : BOSS_OFFSET_LEFT;

		return robotPos
			+ (right * currentOffsetX)
			+ (DirectX::SimpleMath::Vector3::Up * BOSS_OFFSET_UP)
			+ (finalAimDir * BOSS_OFFSET_FORWARD);
	}
	else
	{
		// プレイヤーの場合は標準の武器サイズに合わせたオフセットを使用する
		float gunSideOffset = m_fireGunFromRight ? SHOTGUN_OFFSET_RIGHT : SHOTGUN_OFFSET_LEFT;

		return robotPos
			+ (right * gunSideOffset)
			+ (DirectX::SimpleMath::Vector3::Up * SHOTGUN_OFFSET_UP)
			+ (finalAimDir * SHOTGUN_OFFSET_FORWARD);
	}
}

/**
 * @brief 銃口位置を基準とした最終的な偏差射撃方向の再計算
 * @param[in] robotPos ロボットの現在座標
 * @param[in] gunPos 実際の銃口位置
 * @param[in] baseAimDir 基本の射撃方向
 * @param[in] targetRobot ターゲットのロボットポインタ
 * @param[in] owner 発射主のロボットポインタ
 * @return 再計算された射撃方向ベクトル
 */
DirectX::SimpleMath::Vector3 Shotgun::CalculateFinalForward(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Vector3& gunPos, const DirectX::SimpleMath::Vector3& baseAimDir,
	const Robot* targetRobot, const Robot* owner) const
{
	DirectX::SimpleMath::Vector3 forward = baseAimDir;

	if (targetRobot)
	{
		DirectX::SimpleMath::Vector3 targetPos = targetRobot->GetAimPosition();
		DirectX::SimpleMath::Vector3 targetVel = targetRobot->GetVelocity();
		float dist = DirectX::SimpleMath::Vector3::Distance(robotPos, targetPos);

		float timeToHit = dist / BULLET_SPEED;

		// 時間の上限を設けて、ターゲットの未来位置を算出する
		float predictionTime = std::min(timeToHit, MAX_PREDICTION_TIME);

		DirectX::SimpleMath::Vector3 futurePos = targetPos + (targetVel * predictionTime);

		// 最終的な射撃位置を補間して算出する
		DirectX::SimpleMath::Vector3 finalPoint =
			DirectX::SimpleMath::Vector3::Lerp(targetPos, futurePos, PREDICTION_BLEND_RATE);

		if (dynamic_cast<const BossRobot*>(owner) != nullptr)
		{
			// ボス：巨体のため、高い銃口からターゲットへ正確に撃ち下ろす
			forward = finalPoint - gunPos;
		}
		else
		{
			// プレイヤー：TPS視点での下撃ち感を防ぐため、足元基準の角度（水平）を使用
			forward = finalPoint - robotPos;
		}
		forward.Normalize();
	}

	return forward;
}

/**
 * @brief 散弾の展開と発射実行
 * @param[in] gunPos 銃口のワールド座標
 * @param[in] forward 基準となる発射方向ベクトル
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] projectileManager 弾丸生成システム
 */
void Shotgun::ExecuteSpreadFire(const DirectX::SimpleMath::Vector3& gunPos,
	const DirectX::SimpleMath::Vector3& forward, const Robot* owner, ProjectileManager* projectileManager)
{
	// 基準となる右方向と上方向のベクトルを算出（真上・真下時のゼロベクトル対策を含む）
	DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(forward);
	if (right.LengthSquared() < EPSILON_SQUARED) right = DirectX::SimpleMath::Vector3::Right;
	right.Normalize();

	DirectX::SimpleMath::Vector3 up = forward.Cross(right);
	up.Normalize();

	// 中央の1発を発射
	FireGun(gunPos, forward, owner, projectileManager);

	// 周囲の散弾を発射（360度を等分して円形に配置）
	for (int i = 0; i < SPREAD_PELLET_COUNT; ++i)
	{
		float angle = DirectX::XM_2PI * (float)i / (float)SPREAD_PELLET_COUNT;

		// 傾けるための「回転軸」を計算
		DirectX::SimpleMath::Vector3 axis = std::cos(angle) * up + std::sin(angle) * right;
		axis.Normalize();

		// 回転軸を基準に、拡散角度分だけ外側へ傾ける
		DirectX::SimpleMath::Quaternion spreadRot =
			DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, SHOTGUN_SPREAD_ANGLE);

		DirectX::SimpleMath::Vector3 spreadDir = DirectX::SimpleMath::Vector3::Transform(forward, spreadRot);

		FireGun(gunPos, spreadDir, owner, projectileManager);
	}

	EffectSystem::Instance()->SpawnMuzzleFlash(gunPos, forward);
	SoundManager::Instance().PlaySE(L"SE_Gun");
}

/**
 * @brief 銃弾の単発発射実行
 * @param[in] gunPos 銃口のワールド座標
 * @param[in] aimDir 弾丸の進行方向ベクトル
 * @param[in] owner 発射主のロボットポインタ
 * @param[in] projectileManager 弾丸生成システム
 */
void Shotgun::FireGun(const DirectX::SimpleMath::Vector3& gunPos, const DirectX::SimpleMath::Vector3& aimDir,
	const Robot* owner, ProjectileManager* projectileManager)
{
	// 弾丸の進行方向を基準に、弾丸の回転を計算する
	DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;
	DirectX::SimpleMath::Matrix lookAt =
		DirectX::SimpleMath::Matrix::CreateWorld(DirectX::SimpleMath::Vector3::Zero, aimDir, up);
	DirectX::SimpleMath::Quaternion fireRotation = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookAt);

	// 弾丸の生成と発射
	projectileManager->ShootBullet(gunPos, fireRotation, owner);
}