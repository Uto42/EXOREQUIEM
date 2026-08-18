/*****************************************************************//**
 * @file    ProjectileManager.h
 * @brief   銃弾・各種ミサイルオブジェクトの一括統合プール管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/16
 *********************************************************************/

#pragma once

#include <functional>
#include <random>
#include <vector>
#include <memory>
#include <typeinfo>
#include "Game/GameObjects/Projectile/ProjectileBase.h"
#include "Game/GameObjects/Projectile/Bullet/Bullet.h"
#include "Game/GameObjects/Projectile/Missile/CruiseMissile.h"
#include "Game/GameObjects/Projectile/Missile/TopAttackMissile.h"
#include "Game/GameObjects/Robot/Robot.h"

class StageManager;

class ProjectileManager
{
public:
	// コンストラクタ
	ProjectileManager();
	// デストラクタ
	~ProjectileManager();
	// 初期化処理
	void Initialize(ID3D11Device* device, size_t initialGunPoolSize = DEFAULT_GUN_POOL_SIZE);
	// 更新処理
	void Update(float dt);
	// 描画処理
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);

	// ステージ管理者の設定
	void SetStageManager(StageManager* sm);

	// 統合された発射物プールの取得
	const std::vector<std::unique_ptr<ProjectileBase>>& GetProjectiles() const { return m_projectiles; }

	// 発射処理（回転指定）
	void ShootBullet(const DirectX::SimpleMath::Vector3& startPosition,
		const DirectX::SimpleMath::Quaternion& startRotation, const Robot* owner);
	// 発射処理（方向指定）
	void ShootBulletVector(const DirectX::SimpleMath::Vector3& startPosition,
		const DirectX::SimpleMath::Vector3& direction, const Robot* owner);

	// ヘルパー関数：ミサイルのターゲット情報をバインドする
	void BindTargetFunctions(
		const Robot* target,
		const DirectX::SimpleMath::Vector3& aimTargetPos,
		std::function<DirectX::SimpleMath::Vector3()>& outGetPos,
		std::function<DirectX::SimpleMath::Vector3()>& outGetVel,
		std::function<bool()>& outIsActive
	);

	// 巡航ミサイル発射処理（方向指定）
	void ShootCruise(const DirectX::SimpleMath::Vector3& startPosition,
		const DirectX::SimpleMath::Vector3& startDirection, const Robot* target,
		float speed, float turnSpeed, float lifeTime, const Robot* owner,
		const DirectX::SimpleMath::Vector3& aimTargetPos = DirectX::SimpleMath::Vector3::Zero);
	// 巡航ミサイル発射処理
	void ShootCruise(const DirectX::SimpleMath::Vector3& startPosition, const Robot* target,
		float speed, float turnSpeed, float lifeTime, const Robot* owner,
		const DirectX::SimpleMath::Vector3& aimDir = DirectX::SimpleMath::Vector3::Zero);
	// トップアタックミサイル発射処理（方向指定）
	void ShootTopAttack(const DirectX::SimpleMath::Vector3& startPosition,
		const DirectX::SimpleMath::Vector3& startDirection, const Robot* target,
		float speed, float turnSpeed, float lifeTime, const Robot* owner,
		const DirectX::SimpleMath::Vector3& aimTargetPos = DirectX::SimpleMath::Vector3::Zero);

private:
	// プールから非アクティブな指定型のオブジェクトを探すか、新規生成するテンプレート関数
	template<typename T>
	T* GetInactiveProjectile()
	{
		for (auto& p : m_projectiles)
		{
			if (!p->IsActive())
			{
				if (T* typedProj = dynamic_cast<T*>(p.get()))
				{
					return typedProj;
				}
			}
		}

		// プールに空きがない場合は新規作成して拡張
		auto newProj = std::make_unique<T>();
		T* ptr = newProj.get();
		m_projectiles.push_back(std::move(newProj));
		return ptr;
	}

	// --- 調整用定数パラメータ ---
	static constexpr size_t DEFAULT_GUN_POOL_SIZE = 30;					//< 銃弾の初期プールサイズ
	static constexpr size_t INITIAL_MISSILE_POOL_SIZE = 10;				//< 巡航ミサイルの初期プールサイズ
	static constexpr size_t INITIAL_TOP_ATTACK_POOL_SIZE = 10;			//< トップアタックミサイルの初期プールサイズ

	// --- 衝突判定用のしきい値 ---
	static constexpr float GROUND_HEIGHT_THRESHOLD = 0.5f;				//< 地面との衝突判定の高さしきい値

	// --- 軌道用のしきい値 ---
	static constexpr float ZERO_DIVISION_EPSILON = 0.001f;				//< ゼロ除算を防止するための極小距離閾値
	static constexpr float MIN_DIRECTION_LENGTH_SQUARED = 0.0001f;		//< 発射方向ベクトルの最小長さの二乗
	static constexpr float TARGET_CENTER_OFFSET_Y = 1.0f;				//< ターゲットの中心位置を上方向に補正するオフセット値

	// --- 軌道用のパラメータ ---
	static constexpr double RANDOM_SIDE_PROBABILITY = 0.5;				//< ランダムに左右どちらの方向に回避するかの確率
	static constexpr float SIGN_POSITIVE = 1.0f;						//< ランダム回避方向の正の符号
	static constexpr float SIGN_NEGATIVE = -1.0f;						//< ランダム回避方向の負の符号

	static constexpr float CIRCUS_STANDARD_RIGHT_WEIGHT = 2.0f;			//< 軌道の右方向の重み
	static constexpr float CIRCUS_STANDARD_UP_WEIGHT = 1.5f;			//< 軌道の上方向の重み

	static constexpr float CIRCUS_CLOSE_RANGE_LIMIT = 15.0f;			//< 軌道を適用する距離の閾値
	static constexpr float CIRCUS_CLOSE_RIGHT_WEIGHT = 1.0f;			//< 軌道の右方向の重み
	static constexpr float CIRCUS_CLOSE_UP_WEIGHT = 2.0f;				//< 軌道の上方向の重み
	static constexpr float CIRCUS_CLOSE_FORWARD_WEIGHT = 1.0f;			//< 軌道の前方向の重み

	// --- 状態値・パラメータ ---
	std::vector<std::unique_ptr<ProjectileBase>> m_projectiles;			//< 統合された発射物プール
	std::mt19937 m_gen;

	// --- リソース ---
	std::shared_ptr<DirectX::Model> m_bulletModel = nullptr;			//< 銃弾モデル
	std::shared_ptr<DirectX::Model> m_missileModel = nullptr;			//< ミサイルモデル

	// --- 外部システム参照 ---
	StageManager* m_stageManager = nullptr;								//< ステージ管理者
};