/*****************************************************************//**
 * @file    WeaponController.h
 * @brief   ロボット共通武装システム：残弾管理、リロード制御、および発射ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <memory>
#include <array>
#include "Game/GameObjects/Weapon/WeaponBase.h"

 // 前方宣言
class ProjectileManager;
class LockOnSystem;
class Robot;
class StageManager;

// 武器の装備セット（ロードアウト）
enum class WeaponSet
{
	Standard = 0,   // Aセット：マシンガン ＋ 巡航ミサイル
	Heavy = 1,      // Bセット：ショットガン ＋ 高高度ミサイル
	Count           // 武器セットの総数
};

/**
 * @class WeaponController
 * @brief ロボット（プレイヤー・敵共通）の武装制御クラス
 */
class WeaponController
{
public:
	// コンストラクタ
	WeaponController();
	// デストラクタ
	~WeaponController();
	// 更新処理
	void Update(float dt);

	// 弾丸・ミサイルマネージャーの取得 / 設定
	ProjectileManager* GetProjectileManager() const { return m_projectileManager; }
	void SetProjectileManager(ProjectileManager* manager) { m_projectileManager = manager; }

	// ロックオンシステムの取得 / 設定
	LockOnSystem* GetLockOnSystem() const { return m_lockOnSystem; }
	void SetLockOnSystem(LockOnSystem* system) { m_lockOnSystem = system; }

	// 外部からターゲットの直接設定
	void SetTarget(const Robot* target, bool isPlayer)
	{
		m_manualTarget = target;
		m_isManualTargetPlayer = isPlayer;
	}

	// ステージマネージャーの登録
	void SetStageManager(StageManager* sm)
	{
		m_stageManager = sm;
	}

	// 現在の武器セットの取得
	WeaponSet GetCurrentWeaponSet() const { return m_currentSet; }

	// 武器セットの切り替え
	void ToggleWeaponSet();

	// 重武装セットの解放
	void UnlockHeavySet() { m_isHeavySetUnlocked = true; }

	// --- 発射・リロード操作 ---

	// ガン（メイン武器）の発射試行
	void TryFireGun(
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot, 
		const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* owner, 
		const Robot* target = nullptr);

	// ミサイル（サブ武器）の発射試行
	void TryFireMissile(
		const DirectX::SimpleMath::Vector3& cameraPos,
		const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot,
		const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* owner, 
		const Robot* target = nullptr);

	// ショットガン専用の発射試行（ボスの独立操作用）
	void TryFireShotgun(const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimDir,
		const Robot* owner, const Robot* target = nullptr);

	// 高高度ミサイル専用の発射試行（ボスの独立操作用）
	void TryFireHighAltitudeMissile(const DirectX::SimpleMath::Vector3& robotPos,
		const DirectX::SimpleMath::Quaternion& robotRot, const DirectX::SimpleMath::Vector3& aimPos,
		const Robot* owner, const Robot* target = nullptr);

	// リロードの開始
	void StartGunReload();
	void StartMissileReload();

	// --- 状態取得 ---

	// 残弾数の取得
	int GetGunAmmo() const;
	int GetMissileAmmo() const;

	// リロード進捗率の取得
	float GetGunReloadTimeRate() const;
	float GetMissileReloadTimeRate() const;

	// 指定した武器セットのメイン武器（腕）の残弾数を取得
	int GetPrimaryAmmo(WeaponSet set) const;
	// 指定した武器セットのメイン武器（腕）の最大装弾数を取得
	int GetPrimaryMaxAmmo(WeaponSet set) const;
	// 指定した武器セットのメイン武器（腕）がリロード中か取得
	bool IsPrimaryReloading(WeaponSet set) const;
	// 指定した武器セットのメイン武器（腕）のリロード進捗率を取得
	float GetPrimaryReloadTimeRate(WeaponSet set) const;

	// 指定した武器セットのサブ武器（肩）の残弾数を取得
	int GetSecondaryAmmo(WeaponSet set) const;
	// 指定した武器セットのサブ武器（肩）の最大装弾数を取得
	int GetSecondaryMaxAmmo(WeaponSet set) const;
	// 指定した武器セットのサブ武器（肩）がリロード中か取得
	bool IsSecondaryReloading(WeaponSet set) const;
	// 指定した武器セットのサブ武器（肩）のリロード進捗率を取得
	float GetSecondaryReloadTimeRate(WeaponSet set) const;

private:
	// 現在アクティブな武器を取得するヘルパー関数
	WeaponBase* GetCurrentPrimaryWeapon() const { return m_primaryWeapons[static_cast<size_t>(m_currentSet)].get(); }
	WeaponBase* GetCurrentSecondaryWeapon() const { return m_secondaryWeapons[static_cast<size_t>(m_currentSet)].get(); }

private:
	// --- 調整用定数パラメータ ---
	static constexpr float TARGET_LOCK_OFFSET_Y = 1.0f;       //< ロックオン対象を狙う際の上方向オフセット
	static constexpr float RAYCAST_START_OFFSET_Y = 1.5f;     //< 非ロックオン時のレイキャスト開始高度
	static constexpr float AIM_RAYCAST_DISTANCE = 500.0f;     //< 照準レイキャストの最大射程距離
	static constexpr float SAFE_AM_DISTANCE = 200.0f;         //< レイが不発だった場合の安全用前方目標距離

	// --- 状態値・パラメータ ---
	WeaponSet m_currentSet = WeaponSet::Standard;             //< 現在選択中の武器セット
	bool m_isHeavySetUnlocked = false;                        //< Heavyセット（ショットガン等）が解放されているか
	bool m_isManualTargetPlayer = false;                      //< マニュアルターゲットがプレイヤーかどうかのフラグ

	// --- 武器リソース ---
	// インデックスは WeaponSet の値(0: Standard, 1: Heavy)に対応
	std::array<std::unique_ptr<WeaponBase>, static_cast<size_t>(WeaponSet::Count)> m_primaryWeapons;
	std::array<std::unique_ptr<WeaponBase>, static_cast<size_t>(WeaponSet::Count)> m_secondaryWeapons;

	// --- 外部システム参照 ---
	ProjectileManager* m_projectileManager = nullptr;         //< 武器の発射処理で使用する弾・ミサイル管理への参照
	LockOnSystem* m_lockOnSystem = nullptr;                   //< ロックオンシステムへの参照
	StageManager* m_stageManager = nullptr;                   //< ステージ管理者への参照
	const Robot* m_manualTarget = nullptr;                    //< 外部から直接指定されたターゲット
};