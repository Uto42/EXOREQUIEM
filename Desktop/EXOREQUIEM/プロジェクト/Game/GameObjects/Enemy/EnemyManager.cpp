/*****************************************************************//**
 * @file    EnemyManager.cpp
 * @brief   複数エネミーの動的生成、一括更新・描画、および生存管理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Enemy/EnemyManager.h"
#include "Game/Systems/Effect/Visuals/BlobShadow.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/GameObjects/Enemy/Boss/BossEnemy.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"
#include "Game/Stage/StageManager.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include "Game/Systems/Input/AIManipulator.h"
#include "Game/Systems/Input/TurretAIManipulator.h"
#include "Game/Systems/Input/DummyAIManipulator.h"
#include "Game/Systems/Input/BossAIManipulator.h"

/**
 * @brief コンストラクタ
 * @details 敵専用のGunManagerを生成し、管理用リストを初期化する
 */
EnemyManager::EnemyManager()
	: m_player(nullptr)
	, m_bulletModel(nullptr)
	, m_projectileManager(nullptr)
	, m_stageManager(nullptr)
{
}

/**
 * @brief デストラクタ
 * @details 管理下にある全ての敵インスタンスを明示的に破棄する
 */
EnemyManager::~EnemyManager()
{
	m_enemies.clear();
}

/**
 * @brief システムの初期化
 * @param[in] device DirectX11デバイス
 */
void EnemyManager::Initialize(ID3D11Device* device)
{
	UNREFERENCED_PARAMETER(device);
}

/**
 * @brief 敵を指定の座標に出現させる
 * @param[in] type 出現させる敵のバリエーション
 * @param[in] position 配置するワールド座標
 * @param[in] device DirectX11デバイス
 */
void EnemyManager::SpawnEnemy(EnemyType type, const DirectX::SimpleMath::Vector3& position, ID3D11Device* device)
{
	// 敵のタイプに応じたインスタンスの生成
	std::unique_ptr<Enemy> newEnemy = nullptr;
	if (type == EnemyType::Boss)
	{
		newEnemy = std::make_unique<BossEnemy>();
	}
	else
	{
		newEnemy = std::make_unique<Enemy>();
	}

	if (newEnemy)
	{
		// 外部依存関係の注入
		newEnemy->SetPlayer(m_player->GetRobot());
		newEnemy->SetProjectileManager(m_projectileManager);
		newEnemy->SetStageManager(m_stageManager);

		Robot* myRobot = newEnemy->GetRobot();
		Robot* targetRobot = m_player ? m_player->GetRobot() : nullptr;

		// 敵のバリエーションごとの固有セットアップ（モデル・ステータス・AI）
		if (type == EnemyType::Normal)
		{
			newEnemy->Initialize(device, position, true, PATH_NORMAL_MDL, PATH_NORMAL_TEX);
			myRobot->SetHealth(HP_NORMAL);
			myRobot->SetMaxHealth(HP_NORMAL);

			auto ai = std::make_unique<AIManipulator>(myRobot, targetRobot, m_stageManager);
			newEnemy->SetController(std::move(ai));
		}
		else if (type == EnemyType::Turret)
		{
			newEnemy->Initialize(device, position, true, PATH_TURRET_MDL, PATH_TURRET_TEX);
			myRobot->SetHealth(HP_TURRET);
			myRobot->SetMaxHealth(HP_TURRET);

			// 砲台のため物理挙動をロックする
			myRobot->SetGravityEnabled(false);
			myRobot->SetVelocity(DirectX::SimpleMath::Vector3::Zero);

			auto turretAI = std::make_unique<TurretAIManipulator>(myRobot, targetRobot);
			newEnemy->SetController(std::move(turretAI));
		}
		else if (type == EnemyType::Dummy)
		{
			newEnemy->Initialize(device, position, true, PATH_NORMAL_MDL, PATH_NORMAL_TEX);
			myRobot->SetHealth(HP_DUMMY);
			myRobot->SetMaxHealth(HP_DUMMY);

			// チュートリアル等のためにアクションを行わないダミーAIを乗せる
			auto dummyAI = std::make_unique<DummyAIManipulator>();
			newEnemy->SetController(std::move(dummyAI));
		}
		else if (type == EnemyType::Boss)
		{
			BossEnemy* bossEnemy = dynamic_cast<BossEnemy*>(newEnemy.get());
			if (bossEnemy)
			{
				bossEnemy->InitializeBoss(device, position, PATH_BOSS_LOWER, PATH_BOSS_UPPER, PATH_BOSS_TEX);
			}

			myRobot->SetHealth(HP_BOSS);
			myRobot->SetMaxHealth(HP_BOSS);
			myRobot->SetCollisionSize(BOSS_COLLISION_R, BOSS_COLLISION_H);

			if (auto weapon = myRobot->GetWeapon())
			{
				weapon->ToggleWeaponSet();
			}

			auto bossAI = std::make_unique<BossAIManipulator>(myRobot, targetRobot);
			newEnemy->SetController(std::move(bossAI));
		}

		// 出現直後に最初からプレイヤーの方向を向かせる
		if (targetRobot)
		{
			DirectX::SimpleMath::Vector3 dirToPlayer = targetRobot->GetPosition() - position;
			dirToPlayer.y = 0.0f;

			// 完全に重なっている場合のゼロ除算を防ぐための距離チェック
			if (dirToPlayer.LengthSquared() > EPSILON_SQUARED)
			{
				// X軸・Z軸方向の傾きを含ませず、純粋なY軸旋回のみを適用して姿勢の破綻を防ぐ
				float yaw = std::atan2(dirToPlayer.x, dirToPlayer.z);
				DirectX::SimpleMath::Quaternion initRot = 
					DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::Up, yaw);

				myRobot->SetRotation(initRot);
			}
			else
			{
				// 安全策として固定の方向（手前）を向かせる
				DirectX::SimpleMath::Quaternion initRot = 
					DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::Up, DirectX::XM_PI);

				myRobot->SetRotation(initRot);
			}
		}

		m_enemies.push_back(std::move(newEnemy));
	}
}

/**
 * @brief すべての敵のステータスを初期状態にリセットして復活させる
 */
void EnemyManager::ResetAllEnemies()
{
	for (const auto& enemy : m_enemies)
	{
		if (auto robot = enemy->GetRobot())
		{
			robot->SetHealth(robot->GetMaxHealth());
			robot->SetActive(true);
			robot->SetVisible(true);
			robot->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
			robot->ResetState();
		}
	}
}

/**
 * @brief 全エネミーおよび敵弾の更新処理
 * @param[in] dt 前フレームからの経過時間
 */
void EnemyManager::Update(float dt)
{
	for (auto& enemy : m_enemies)
	{
		enemy->Update(dt);
	}
}

/**
 * @brief 全エネミーおよび敵弾の描画処理
 * @param[in] context デバイスコンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void EnemyManager::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states, 
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	for (const auto& enemy : m_enemies)
	{
		enemy->Render(context, states, view, proj);
	}
}

/**
 * @brief 生存しているエネミーの足元に影を投影する
 * @param[in] shadowRenderer 影描画システム
 * @param[in] context デバイスコンテキスト
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void EnemyManager::RenderShadows(BlobShadow* shadowRenderer, ID3D11DeviceContext* context, 
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (!shadowRenderer) return;

	for (const auto& enemy : m_enemies)
	{
		// 敵が生存しているか、アクティブか、可視状態かをチェック
		auto* robot = enemy->GetRobot();
		if (!robot || !robot->IsActive() || !robot->IsVisible()) continue;

		DirectX::SimpleMath::Vector3 pos = robot->GetPosition();

		// レイキャスト等を用いて足元の地面の高さを正確に取得し、影のY座標を補正する
		float groundY = 0.0f;
		if (m_stageManager)
		{
			groundY = m_stageManager->GetGroundHeight(pos);
		}

		// ボスかどうかで影のサイズを切り替える
		float shadowScale = ENEMY_SHADOW_SCALE;
		if (dynamic_cast<BossEnemy*>(enemy.get()) != nullptr)
		{
			shadowScale = BOSS_SHADOW_SCALE;
		}

		shadowRenderer->Render(context, view, proj, pos, shadowScale, groundY);
	}
}

/**
 * @brief ステージ情報の登録
 * @details 敵本体だけでなく、敵の弾丸がステージ（壁や床）に当たるように設定する
 * @param[in] stageManager ステージ管理オブジェクト
 */
void EnemyManager::SetStageManager(StageManager* stageManager)
{
	m_stageManager = stageManager;
}