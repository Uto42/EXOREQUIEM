/*****************************************************************//**
 * @file    ProjectileManager.cpp
 * @brief   銃弾・各種ミサイルオブジェクトの一括統合プール管理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/16
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Stage/StageManager.h"
#include "Game/Systems/Effect/EffectSystem.h"

 /**
  * @brief コンストラクタ
  */
ProjectileManager::ProjectileManager()
	: m_bulletModel(nullptr)
	, m_missileModel(nullptr)
	, m_stageManager(nullptr)
	, m_gen(std::random_device{}())
{
}

/**
 * @brief デストラクタ
 */
ProjectileManager::~ProjectileManager()
{
	m_projectiles.clear();
}

/**
 * @brief 初期化処理
 * @param[in] device D3D11デバイス
 * @param[in] initialBulletPoolSize オブジェクトプールの初期サイズ
 */
void ProjectileManager::Initialize(ID3D11Device* device, size_t initialBulletPoolSize)
{
	DirectX::EffectFactory fx(device);
	fx.SetDirectory(L"Resources\\Models\\Weapon");

	m_bulletModel = DirectX::Model::CreateFromSDKMESH(device, L"Resources\\Models\\Weapon\\Bullet.sdkmesh", fx);
	m_missileModel = DirectX::Model::CreateFromSDKMESH(device, L"Resources\\Models\\Weapon\\Missile.sdkmesh", fx);

	// 統合プールのメモリをあらかじめ確保
	size_t totalSize = initialBulletPoolSize + INITIAL_MISSILE_POOL_SIZE + INITIAL_TOP_ATTACK_POOL_SIZE;
	m_projectiles.reserve(totalSize);

	// 弾丸オブジェクトの事前生成とプールへの追加
	for (size_t i = 0; i < initialBulletPoolSize; ++i)
	{
		auto bullet = std::make_unique<Bullet>();
		bullet->SetModel(m_bulletModel);
		m_projectiles.push_back(std::move(bullet));
	}
	// ミサイルオブジェクトの事前生成とプールへの追加
	for (size_t i = 0; i < INITIAL_MISSILE_POOL_SIZE; ++i)
	{
		auto cruise = std::make_unique<CruiseMissile>();
		cruise->SetModel(m_missileModel);
		m_projectiles.push_back(std::move(cruise));
	}
	// トップアタックミサイルオブジェクトの事前生成とプールへの追加
	for (size_t i = 0; i < INITIAL_TOP_ATTACK_POOL_SIZE; ++i)
	{
		auto topAttack = std::make_unique<TopAttackMissile>();
		topAttack->SetModel(m_missileModel);
		m_projectiles.push_back(std::move(topAttack));
	}
}

/**
 * @brief 更新処理
 * @param[in] dt 経過時間
 */
void ProjectileManager::Update(float dt)
{
	// 発射物（弾丸・ミサイル）の更新
	for (auto& projectile : m_projectiles)
	{
		if (projectile->IsActive())
		{
			// 弾・ミサイルの共通更新インターフェース
			projectile->Update(dt);
		}
	}
}

/**
 * @brief 描画処理
 * @param[in] context D3D11デバイスコンテキスト
 * @param[in] states 共通ステートオブジェクト
 * @param[in] view ビューマトリクス
 * @param[in] proj プロジェクションマトリクス
 */
void ProjectileManager::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	// 発射物の描画
	for (auto& projectile : m_projectiles)
	{
		if (projectile->IsActive())
		{
			projectile->Render(context, states, view, proj);
		}
	}
}

/**
 * @brief ステージ管理者の設定
 * @param[in] sm ステージ管理者への参照
 */
void ProjectileManager::SetStageManager(StageManager* sm)
{
	m_stageManager = sm;

	// 全ての発射物に対してステージ管理者を設定（型判定が不要に！）
	for (auto& projectile : m_projectiles)
	{
		projectile->SetStageManager(sm);
	}
}

/**
 * @brief 弾丸の発射処理（回転指定）
 * @param[in] startPosition 発射開始座標
 * @param[in] startRotation 発射時の回転クォータニオン
 * @param[in] owner 発射したロボットのポインタ
 */
void ProjectileManager::ShootBullet(const DirectX::SimpleMath::Vector3& startPosition,
	const DirectX::SimpleMath::Quaternion& startRotation, const Robot* owner)
{
	// 非アクティブなオブジェクトを取得（プールが満杯の場合は内部で拡張）
	Bullet* bullet = GetInactiveProjectile<Bullet>();
	bullet->SetModel(m_bulletModel);
	if (m_stageManager) bullet->SetStageManager(m_stageManager);
	bullet->Initialize(startPosition, startRotation, owner);
}

/**
 * @brief 弾丸の発射処理（方向指定）
 * @param[in] startPosition 発射開始座標
 * @param[in] direction 発射方向ベクトル
 * @param[in] owner 発射したロボットのポインタ
 */
void ProjectileManager::ShootBulletVector(const DirectX::SimpleMath::Vector3& startPosition,
	const DirectX::SimpleMath::Vector3& direction, const Robot* owner)
{
	if (direction.LengthSquared() <= MIN_DIRECTION_LENGTH_SQUARED) return;

	DirectX::SimpleMath::Vector3 dir = direction;
	dir.Normalize();

	// 発射方向ベクトルから回転クォータニオンを生成するためのワールド行列を作成
	DirectX::SimpleMath::Matrix lookAt =
		DirectX::SimpleMath::Matrix::CreateWorld(DirectX::SimpleMath::Vector3::Zero, dir, DirectX::SimpleMath::Vector3::Up);

	// ワールド行列から回転クォータニオンを生成
	DirectX::SimpleMath::Quaternion rot = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookAt);

	// 弾を発射する
	ShootBullet(startPosition, rot, owner);
}

/**
 * @brief ミサイル誘導用のコールバック関数を生成する
 * @param[in] target ロックオン対象のロボットポインタ（非ロックオン時はnullptr）
 * @param[in] aimTargetPos 非ロックオン時の目標座標
 * @param[out] outGetPos ターゲット座標を取得するコールバック関数
 * @param[out] outGetVel ターゲット速度を取得するコールバック関数
 * @param[out] outIsActive ターゲットが有効かどうかを取得するコールバック関数
 */
void ProjectileManager::BindTargetFunctions(const Robot* target,
	const DirectX::SimpleMath::Vector3& aimTargetPos, std::function<DirectX::SimpleMath::Vector3()>& outGetPos,
	std::function<DirectX::SimpleMath::Vector3()>& outGetVel, std::function<bool()>& outIsActive)
{
	if (target)
	{
		// ターゲット追従
		outGetPos = [target]() {
			return target ? (target->GetPosition() +
				(DirectX::SimpleMath::Vector3::Up * TARGET_CENTER_OFFSET_Y)) : DirectX::SimpleMath::Vector3::Zero;
			};
		outGetVel = [target]() {
			return target ? target->GetVelocity() : DirectX::SimpleMath::Vector3::Zero;
			};
		outIsActive = [target]() -> bool {
			return target && target->IsActive() && target->GetHealth() > 0.0f;
			};
	}
	else
	{
		// 非ロックオン時：目標座標への固定誘導
		outGetPos = [aimTargetPos]() { return aimTargetPos; };
		outGetVel = []() { return DirectX::SimpleMath::Vector3::Zero; };
		outIsActive = []() -> bool { return true; };
	}
}

/**
 * @brief 巡航ミサイル発射処理（方向指定）
 * @param[in] startPosition 発射開始座標
 * @param[in] startDirection 発射方向ベクトル
 * @param[in] target ロックオン対象のロボットポインタ
 * @param[in] speed ミサイルの初速
 * @param[in] turnSpeed ミサイルの旋回速度
 * @param[in] lifeTime ミサイルの生存時間（秒）
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] aimTargetPos 非ロックオン時の目標座標
 */
void ProjectileManager::ShootCruise(const DirectX::SimpleMath::Vector3& startPosition,
	const DirectX::SimpleMath::Vector3& startDirection, const Robot* target, float speed,
	float turnSpeed, float lifeTime, const Robot* owner, const DirectX::SimpleMath::Vector3& aimTargetPos)
{
	std::function<DirectX::SimpleMath::Vector3()> getPos, getVel;
	std::function<bool()> isActive;

	// ターゲット情報のバインド
	BindTargetFunctions(target, aimTargetPos, getPos, getVel, isActive);

	// 非アクティブなオブジェクトを取得（プールが満杯の場合は内部で拡張）
	CruiseMissile* missile = GetInactiveProjectile<CruiseMissile>();

	// ミサイルを初期化して設定
	missile->SetModel(m_missileModel);
	// 拡張時に生成された弾にも確実にステージ管理者を渡す
	if (m_stageManager) missile->SetStageManager(m_stageManager);
	missile->Initialize(startPosition, startDirection, getPos, getVel, isActive,
		speed, turnSpeed, lifeTime, owner);
}

/**
 * @brief 巡航ミサイル発射処理
 * @param[in] startPosition 発射開始座標
 * @param[in] target ロックオン対象のロボットポインタ
 * @param[in] speed ミサイルの速度
 * @param[in] turnSpeed ミサイルの旋回速度
 * @param[in] lifeTime ミサイルの生存時間（秒）
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] aimDir 基本の照準方向
 */
void ProjectileManager::ShootCruise(const DirectX::SimpleMath::Vector3& startPosition, const Robot* target,
	float speed, float turnSpeed, float lifeTime, const Robot* owner, const DirectX::SimpleMath::Vector3& aimDir)
{
	UNREFERENCED_PARAMETER(aimDir);

	if (!target) return;

	// ターゲットの中心座標を取得（Y軸方向にオフセットを加える）
	DirectX::SimpleMath::Vector3 targetPos =
		target->GetPosition() + (DirectX::SimpleMath::Vector3::Up * TARGET_CENTER_OFFSET_Y);

	DirectX::SimpleMath::Vector3 toTarget = targetPos - startPosition;
	toTarget.Normalize();

	// ターゲットへの方向ベクトルと上方向ベクトルの外積を計算し、右方向ベクトルを求める
	DirectX::SimpleMath::Vector3 right = toTarget.Cross(DirectX::SimpleMath::Vector3::Up);
	std::bernoulli_distribution dist(RANDOM_SIDE_PROBABILITY);
	float sideFactor = dist(m_gen) ? SIGN_POSITIVE : -SIGN_NEGATIVE;

	// 発射方向ベクトルを計算
	DirectX::SimpleMath::Vector3 launchDir =
		(right * sideFactor * CIRCUS_STANDARD_RIGHT_WEIGHT) +
		(DirectX::SimpleMath::Vector3::Up * CIRCUS_STANDARD_UP_WEIGHT);

	launchDir.Normalize();

	// 近距離補正：ターゲットとの距離が一定以下の場合、上方向への飛び出しを強める
	float distance = DirectX::SimpleMath::Vector3::Distance(startPosition, targetPos);
	if (distance < CIRCUS_CLOSE_RANGE_LIMIT)
	{
		launchDir = (right * sideFactor * CIRCUS_CLOSE_RIGHT_WEIGHT) +
			(DirectX::SimpleMath::Vector3::Up * CIRCUS_CLOSE_UP_WEIGHT) + (toTarget * CIRCUS_CLOSE_FORWARD_WEIGHT);
		launchDir.Normalize();
	}

	// 旋回ミサイルを発射する
	ShootCruise(startPosition, launchDir, target, speed, turnSpeed, lifeTime, owner);
}

/**
 * @brief 頭上攻撃ミサイル発射処理（方向指定）
 * @param[in] startPosition 発射開始座標
 * @param[in] startDirection 発射方向ベクトル
 * @param[in] target ロックオン対象のロボットポインタ
 * @param[in] speed ミサイルの初速
 * @param[in] turnSpeed ミサイルの旋回速度
 * @param[in] lifeTime ミサイルの生存時間（秒）
 * @param[in] owner 発射元のロボットポインタ
 * @param[in] aimTargetPos 非ロックオン時の目標座標
 */
void ProjectileManager::ShootTopAttack(const DirectX::SimpleMath::Vector3& startPosition,
	const DirectX::SimpleMath::Vector3& startDirection, const Robot* target, float speed,
	float turnSpeed, float lifeTime, const Robot* owner, const DirectX::SimpleMath::Vector3& aimTargetPos)
{
	std::function<DirectX::SimpleMath::Vector3()> getPos, getVel;
	std::function<bool()> isActive;

	// ターゲット情報のバインド
	BindTargetFunctions(target, aimTargetPos, getPos, getVel, isActive);

	// 非アクティブなオブジェクトを取得（プールが満杯の場合は内部で拡張）
	TopAttackMissile* missile = GetInactiveProjectile<TopAttackMissile>();

	// ミサイルを初期化して設定
	missile->SetModel(m_missileModel);
	// 拡張時に生成された弾にも確実にステージ管理者を渡す
	if (m_stageManager) missile->SetStageManager(m_stageManager);
	missile->Initialize(startPosition, startDirection, getPos, getVel, isActive,
		speed, turnSpeed, lifeTime, owner);
}