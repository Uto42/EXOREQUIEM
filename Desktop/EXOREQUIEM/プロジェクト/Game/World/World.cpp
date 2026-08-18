/*****************************************************************//**
 * @file    World.cpp
 * @brief   プレイヤー、敵、および共通描画物（影など）のインスタンス保持と更新・描画の統括の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/World/World.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/Stage/StageManager.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"

/**
 * @brief コンストラクタ
 */
World::World()
	: m_stageManager(nullptr)
	, m_device(nullptr)
{
}

/**
 * @brief デストラクタ
 */
World::~World()
{
}

/**
 * @brief 各エンティティおよびマネージャーの初期化
 * @param[in] device ID3D11Device
 * @param[in] context ID3D11DeviceContext
 * @param[in] lockOnSystem ロックオンシステム
 * @param[in] stageManager ステージ管理
 */
void World::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
	LockOnSystem* lockOnSystem, StageManager* stageManager)
{
	m_stageManager = stageManager;
	m_device = device;

	// 発射物（弾丸・ミサイル）を管理するマネージャーを生成し初期化する
	m_projectileManager = std::make_unique<ProjectileManager>();
	m_projectileManager->Initialize(device);
	m_projectileManager->SetStageManager(m_stageManager);

	// プレイヤーを生成して初期位置に配置する
	m_player = std::make_unique<Player>();
	m_player->Initialize(device);
	m_player->SetPosition(INITIAL_PLAYER_POS);

	// プレイヤーが持つ武器に発射物マネージャーを紐付ける
	if (auto* robot = m_player->GetRobot()) {
		if (auto* weapon = robot->GetWeapon()) {
			weapon->SetProjectileManager(m_projectileManager.get());
		}
	}

	// ロックオンシステムとステージマネージャーをプレイヤーに注入する
	m_player->SetLockOnSystem(lockOnSystem);
	m_player->SetStageManager(stageManager);

	// 敵を管理するマネージャーを生成し、必要な依存関係を注入する
	m_enemyManager = std::make_unique<EnemyManager>();
	m_enemyManager->SetProjectileManager(m_projectileManager.get());
	m_enemyManager->Initialize(device);
	m_enemyManager->SetPlayer(m_player.get());
	m_enemyManager->SetStageManager(stageManager);

	// 足元の丸影を描画するシステムを初期化する
	m_blobShadow = std::make_unique<BlobShadow>();
	m_blobShadow->Initialize(device, context);
}

/**
 * @brief チュートリアル用の敵を生成
 */
void World::CreateTrainingEnemies()
{
	if (!m_enemyManager || !m_device) return;

	// 無抵抗なダミー敵を所定の位置に配置する
	m_enemyManager->SpawnEnemy(EnemyType::Dummy, ENEMY_SPAWN_DUMMY1, m_device);
	m_enemyManager->SpawnEnemy(EnemyType::Dummy, ENEMY_SPAWN_DUMMY2, m_device);
	m_enemyManager->SpawnEnemy(EnemyType::Dummy, ENEMY_SPAWN_DUMMY3, m_device);
}

/**
 * @brief ステージ1用の敵を生成
 */
void World::CreateStage1Enemies()
{
	if (!m_enemyManager || !m_device) return;

	// 移動する通常の敵と、固定砲台を配置する
	m_enemyManager->SpawnEnemy(EnemyType::Normal, ENEMY_SPAWN_NORMAL, m_device);
	m_enemyManager->SpawnEnemy(EnemyType::Turret, ENEMY_SPAWN_TURRET1, m_device);
	m_enemyManager->SpawnEnemy(EnemyType::Turret, ENEMY_SPAWN_TURRET2, m_device);
}

/**
 * @brief ステージ2用の敵を生成
 */
void World::CreateStage2Enemies()
{
	if (!m_enemyManager || !m_device) return;

	// 立体的な地形に合わせて敵を配置する
	m_enemyManager->SpawnEnemy(EnemyType::Normal, ENEMY_SPAWN_STAGE2_1, m_device);
	m_enemyManager->SpawnEnemy(EnemyType::Normal, ENEMY_SPAWN_STAGE2_2, m_device);
}

/**
 * @brief ステージ3用のボスを生成
 */
void World::CreateStage3Enemies()
{
	if (!m_enemyManager || !m_device) return;

	// ボスエリアの中央奥にボスを配置する
	m_enemyManager->SpawnEnemy(EnemyType::Boss, ENEMY_SPAWN_STAGE3_BOSS, m_device);
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime 経過時間
 * @param[in] kb キーボード状態
 */
void World::Update(float elapsedTime, const DirectX::Keyboard::State& kb)
{
	UNREFERENCED_PARAMETER(kb);

	// プレイヤーの入力を受け付け、状態を更新する
	if (m_player)
	{
		m_player->Update(elapsedTime);
	}

	// すべての敵のAIや状態を一括で更新する
	if (m_enemyManager)
	{
		m_enemyManager->Update(elapsedTime);
	}

	// 飛んでいる弾丸やミサイルの位置や寿命を更新する
	if (m_projectileManager)
	{
		m_projectileManager->Update(elapsedTime);
	}
}

/**
 * @brief 描画処理
 * @param[in] context ID3D11DeviceContext
 * @param[in] states CommonStates
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 */
void World::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	// モデルを描画する前に、足元の丸影を地形の高さに合わせて描画する
	if (m_blobShadow)
	{
		auto* robot = m_player->GetRobot();

		if (robot && robot->IsVisible())
		{
			DirectX::SimpleMath::Vector3 position = robot->GetPosition();
			float groundHeight = m_stageManager->GetGroundHeight(position);
			m_blobShadow->Render(context, view, proj, position, PLAYER_SHADOW_SIZE, groundHeight);
		}

		// 敵の影を描画
		if (m_enemyManager)
		{
			m_enemyManager->RenderShadows(m_blobShadow.get(), context, view, proj);
		}
	}

	// プレイヤーモデルを描画する
	if (m_player)
	{
		m_player->Render(context, states, view, proj);
	}

	// 敵モデルを一括で描画する
	if (m_enemyManager)
	{
		m_enemyManager->Render(context, states, view, proj);
	}

	// すべての発射物を描画する
	if (m_projectileManager)
	{
		m_projectileManager->Render(context, states, view, proj);
	}
}