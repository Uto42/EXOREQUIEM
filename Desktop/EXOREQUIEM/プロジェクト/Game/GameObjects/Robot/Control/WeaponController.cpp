/*****************************************************************//**
 * @file    WeaponController.cpp
 * @brief   ロボット共通武装システム：残弾管理、リロード制御、および発射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include "Game/GameObjects/Weapon/MachineGun.h"
#include "Game/GameObjects/Weapon/CruiseMissileLauncher.h"
#include "Game/GameObjects/Weapon/Shotgun.h"
#include "Game/GameObjects/Weapon/TopAttackMissileLauncher.h"
#include "Game/Stage/StageManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Robot/Robot.h"

 /**
  * @brief コンストラクタ
  * @details ポリモーフィズムを利用して各武器セットを初期化・保持する
  */
WeaponController::WeaponController()
	: m_projectileManager(nullptr)
	, m_lockOnSystem(nullptr)
	, m_manualTarget(nullptr)
	, m_isManualTargetPlayer(false)
	, m_stageManager(nullptr)
{
	// プライマリ（メイン）武器のセットアップ
	m_primaryWeapons[static_cast<size_t>(WeaponSet::Standard)] = std::make_unique<MachineGun>();
	m_primaryWeapons[static_cast<size_t>(WeaponSet::Heavy)] = std::make_unique<Shotgun>();

	// セカンダリ（サブ）武器のセットアップ
	m_secondaryWeapons[static_cast<size_t>(WeaponSet::Standard)] = std::make_unique<CruiseMissileLauncher>();
	m_secondaryWeapons[static_cast<size_t>(WeaponSet::Heavy)] = std::make_unique<TopAttackMissileLauncher>();
}

/**
 * @brief デストラクタ
 */
WeaponController::~WeaponController()
{
}

/**
 * @brief 更新処理
 * @param[in] dt デルタタイム
 * @details 全ての武器インスタンスの更新処理を呼び出す
 */
void WeaponController::Update(float dt)
{
	for (auto& weapon : m_primaryWeapons)
	{
		if (weapon) weapon->Update(dt);
	}

	for (auto& weapon : m_secondaryWeapons)
	{
		if (weapon) weapon->Update(dt);
	}
}

/**
 * @brief 武器セットの切り替え処理
 * @details 現在のセットから別のセットへ切り替える
 */
void WeaponController::ToggleWeaponSet()
{
	if (m_currentSet == WeaponSet::Standard)
	{
		m_currentSet = WeaponSet::Heavy;
	}
	else
	{
		m_currentSet = WeaponSet::Standard;
	}
}

/**
 * @brief ガン（メイン武器）の手動リロード開始
 * @details 現在選択中のメイン武器に対してリロードを要求する
 */
void WeaponController::StartGunReload()
{
	if (auto* weapon = GetCurrentPrimaryWeapon())
	{
		weapon->StartReload();
	}
}

/**
 * @brief ミサイル（サブ武器）の手動リロード開始
 * @details 現在選択中のサブ武器に対してリロードを要求する
 */
void WeaponController::StartMissileReload()
{
	if (auto* weapon = GetCurrentSecondaryWeapon())
	{
		weapon->StartReload();
	}
}

/**
 * @brief ガン（メイン武器）の発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimDir 照準方向
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target ターゲットのロボットポインタ
 * @details IWeaponのシグネチャに準拠し、ポリモーフィックに現在のメイン武器を発射する
 */
void WeaponController::TryFireGun(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimDir,
	const Robot* owner, const Robot* target)
{
	if (auto* weapon = GetCurrentPrimaryWeapon())
	{
		weapon->TryFire(robotPos, robotRot, aimDir, owner, target,
			m_projectileManager, m_lockOnSystem, m_manualTarget);
	}
}

/**
 * @brief ミサイル（サブ武器）の発射試行
 * @param[in] cameraPos カメラの現在座標
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimDir 照準方向
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target ロックオン対象のロボットポインタ（非ロックオン時はnullptr）
 * @details 照準地点の計算後、現在のサブ武器へ発射指示を出す
 */
void WeaponController::TryFireMissile(const DirectX::SimpleMath::Vector3& cameraPos,
	const DirectX::SimpleMath::Vector3& robotPos, const DirectX::SimpleMath::Quaternion& robotRot,
	const DirectX::SimpleMath::Vector3& aimDir, const Robot* owner, const Robot* target)
{
	UNREFERENCED_PARAMETER(cameraPos);

	if (auto* weapon = GetCurrentSecondaryWeapon())
	{
		DirectX::SimpleMath::Vector3 finalAimPos;

		if (target)
		{
			// 敵の当たり判定の中心を目標に設定
			finalAimPos = target->GetAimPosition();
		}
		else
		{
			// 胸や頭の高さに合わせる
			DirectX::SimpleMath::Vector3 rayStartPos =
				robotPos + DirectX::SimpleMath::Vector3(0.0f, TARGET_LOCK_OFFSET_Y, 0.0f);

			// 非ロックオン中：カメラレティクルの中心を捉える
			finalAimPos = m_stageManager ?
				m_stageManager->GetCameraAimPosition(rayStartPos, aimDir, AIM_RAYCAST_DISTANCE)
				: robotPos + (aimDir * SAFE_AM_DISTANCE); // 安全装置
		}

		weapon->TryFire(robotPos, robotRot, finalAimPos, owner, target,
			m_projectileManager, m_lockOnSystem, m_manualTarget);
	}
}

/**
 * @brief ショットガンの発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimDir 照準方向
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target ターゲットのロボットポインタ
 */
void WeaponController::TryFireShotgun(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimDir,
	const Robot* owner, const Robot* target)
{
	// Heavyセットのメイン武器（ショットガン）を強制的に呼び出す
	if (auto* weapon = m_primaryWeapons[static_cast<size_t>(WeaponSet::Heavy)].get())
	{
		weapon->TryFire(robotPos, robotRot, aimDir, owner, target,
			m_projectileManager, m_lockOnSystem, m_manualTarget);
	}
}

/**
 * @brief 高高度ミサイルの発射試行
 * @param[in] robotPos ロボットの現在座標
 * @param[in] robotRot ロボットの現在回転
 * @param[in] aimPos 照準座標
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] target ターゲットのロボットポインタ
 */
void WeaponController::TryFireHighAltitudeMissile(const DirectX::SimpleMath::Vector3& robotPos,
	const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimPos,
	const Robot* owner, const Robot* target)
{
	// Heavyセットのサブ武器（高高度ミサイル）を強制的に呼び出す
	if (auto* weapon = m_secondaryWeapons[static_cast<size_t>(WeaponSet::Heavy)].get())
	{
		weapon->TryFire(robotPos, robotRot, aimPos, owner, target,
			m_projectileManager, m_lockOnSystem, m_manualTarget);
	}
}

/**
 * @brief ガン残弾数取得
 * @return 現在選択中のメイン武器の残弾数
 */
int WeaponController::GetGunAmmo() const
{
	if (auto* weapon = GetCurrentPrimaryWeapon())
	{
		return weapon->GetCurrentAmmo();
	}
	return 0;
}

/**
 * @brief ミサイル残弾数取得
 * @return 現在選択中のサブ武器の残弾数
 */
int WeaponController::GetMissileAmmo() const
{
	if (auto* weapon = GetCurrentSecondaryWeapon())
	{
		return weapon->GetCurrentAmmo();
	}
	return 0;
}

/**
 * @brief ガンのリロード進捗率の取得
 * @return 現在選択中のメイン武器のリロード進捗率（0.0f ～ 1.0f）
 */
float WeaponController::GetGunReloadTimeRate() const
{
	if (auto* weapon = GetCurrentPrimaryWeapon())
	{
		return weapon->GetReloadTimeRate();
	}
	return 1.0f;
}

/**
 * @brief ミサイルのリロード進捗率の取得
 * @return 現在選択中のサブ武器のリロード進捗率（0.0f ～ 1.0f）
 */
float WeaponController::GetMissileReloadTimeRate() const
{
	if (auto* weapon = GetCurrentSecondaryWeapon())
	{
		return weapon->GetReloadTimeRate();
	}
	return 1.0f;
}

/**
 * @brief 指定した武器セットのメイン武器（腕）の残弾数を取得
 * @param[in] set 取得対象の武器セット
 * @return 残弾数
 */
int WeaponController::GetPrimaryAmmo(WeaponSet set) const
{
	if (auto* w = m_primaryWeapons[static_cast<size_t>(set)].get()) return w->GetCurrentAmmo();
	return 0;
}

/**
 * @brief 指定した武器セットのメイン武器（腕）の最大装弾数を取得
 * @param[in] set 取得対象の武器セット
 * @return 最大装弾数
 */
int WeaponController::GetPrimaryMaxAmmo(WeaponSet set) const
{
	if (auto* w = m_primaryWeapons[static_cast<size_t>(set)].get()) return w->GetMaxAmmo();
	return 1; // ゼロ割り防止
}

/**
 * @brief 指定した武器セットのメイン武器（腕）がリロード中か取得
 * @param[in] set 取得対象の武器セット
 * @return リロード中ならtrue
 */
bool WeaponController::IsPrimaryReloading(WeaponSet set) const
{
	if (auto* w = m_primaryWeapons[static_cast<size_t>(set)].get()) return w->IsReloading();
	return false;
}

/**
 * @brief 指定した武器セットのメイン武器（腕）のリロード進捗率を取得
 * @param[in] set 取得対象の武器セット
 * @return リロード進捗率（0.0f ～ 1.0f）
 */
float WeaponController::GetPrimaryReloadTimeRate(WeaponSet set) const
{
	if (auto* w = m_primaryWeapons[static_cast<size_t>(set)].get()) return w->GetReloadTimeRate();
	return 1.0f;
}

/**
 * @brief 指定した武器セットのサブ武器（肩）の残弾数を取得
 * @param[in] set 取得対象の武器セット
 * @return 残弾数
 */
int WeaponController::GetSecondaryAmmo(WeaponSet set) const
{
	if (auto* w = m_secondaryWeapons[static_cast<size_t>(set)].get()) return w->GetCurrentAmmo();
	return 0;
}

/**
 * @brief 指定した武器セットのサブ武器（肩）の最大装弾数を取得
 * @param[in] set 取得対象の武器セット
 * @return 最大装弾数
 */
int WeaponController::GetSecondaryMaxAmmo(WeaponSet set) const
{
	if (auto* w = m_secondaryWeapons[static_cast<size_t>(set)].get()) return w->GetMaxAmmo();
	return 1; // ゼロ割り防止
}

/**
 * @brief 指定した武器セットのサブ武器（肩）がリロード中か取得
 * @param[in] set 取得対象の武器セット
 * @return リロード中ならtrue
 */
bool WeaponController::IsSecondaryReloading(WeaponSet set) const
{
	if (auto* w = m_secondaryWeapons[static_cast<size_t>(set)].get()) return w->IsReloading();
	return false;
}

/**
 * @brief 指定した武器セットのサブ武器（肩）のリロード進捗率を取得
 * @param[in] set 取得対象の武器セット
 * @return リロード進捗率（0.0f ～ 1.0f）
 */
float WeaponController::GetSecondaryReloadTimeRate(WeaponSet set) const
{
	if (auto* w = m_secondaryWeapons[static_cast<size_t>(set)].get()) return w->GetReloadTimeRate();
	return 1.0f;
}
