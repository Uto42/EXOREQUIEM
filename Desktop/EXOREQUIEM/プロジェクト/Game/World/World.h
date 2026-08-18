/*****************************************************************//**
 * @file    World.h
 * @brief   プレイヤー、敵、および共通描画物（影など）のインスタンス保持と更新・描画の統括
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Enemy/EnemyManager.h"
#include "Game/Systems/Effect/Visuals/BlobShadow.h"

class ProjectileManager;
class LockOnSystem;
class StageManager;

// ゲーム世界における全オブジェクトの生成・更新・描画を統括する管理クラス
class World
{
public:
	// コンストラクタ
	World();
	// デストラクタ
	~World();
	// 初期化処理
	void Initialize(
		ID3D11Device* device,
		ID3D11DeviceContext* context,
		LockOnSystem* lockOnSystem, 
		StageManager* stageManager);
	// 更新処理
	void Update(float elapsedTime, const DirectX::Keyboard::State& kb);
	// 描画処理
	void Render(
		ID3D11DeviceContext* context,
		DirectX::CommonStates* states, 
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj);

	// プレイヤーの取得
	Player* GetPlayer() const { return m_player.get(); }
	// エネミーマネージャーを取得
	EnemyManager* GetEnemyManager() const { return m_enemyManager.get(); }
	// ステージマネージャーの取得
	StageManager* GetStageManager() const { return m_stageManager; }

	// チュートリアル用の敵を生成
	void CreateTrainingEnemies();
	// ステージ1用の敵を生成
	void CreateStage1Enemies();
	// ステージ2用の敵を生成
	void CreateStage2Enemies();
	// ステージ3用のボスを生成
	void CreateStage3Enemies();

private:
	// --- 定数パラメータ ---
	static constexpr DirectX::SimpleMath::Vector3 INITIAL_PLAYER_POS = { 0.0f, 0.0f, -30.0f };

	// 敵ユニットの初期配置座標
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_NORMAL = { 0.0f, 0.0f, 40.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_DUMMY1 = { 0.0f, 0.0f, 40.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_DUMMY2 = { -30.0f, 0.0f, 25.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_DUMMY3 = { 40.0f, 0.0f, 30.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_TURRET1 = { -30.0f, 4.0f, 25.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_TURRET2 = { 40.0f, 4.3f, 30.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_STAGE2_1 = { -45.0f, 10.0f, 45.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_STAGE2_2 = { 40.0f, 20.0f, 40.0f };
	static constexpr DirectX::SimpleMath::Vector3 ENEMY_SPAWN_STAGE3_BOSS = { 0.0f, 0.0f, 50.0f };

	static constexpr float PLAYER_SHADOW_SIZE = 4.0f;                //< プレイヤーの足元影の基本サイズ

	// --- メンバ変数 ---
	std::unique_ptr<ProjectileManager> m_projectileManager;          //< 銃弾・ミサイルの一括管理クラス
	std::unique_ptr<Player> m_player;                                //< プレイヤー本体
	std::unique_ptr<EnemyManager> m_enemyManager;                    //< エネミー管理者
	std::unique_ptr<BlobShadow> m_blobShadow;                        //< 足元の丸影描画クラス

	StageManager* m_stageManager;                                    //< ステージ情報への参照
	ID3D11Device* m_device;                                          //< 敵生成時に必要なデバイス
};