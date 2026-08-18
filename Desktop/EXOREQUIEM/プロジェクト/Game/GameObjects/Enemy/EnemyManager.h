/*****************************************************************//**
 * @file    EnemyManager.h
 * @brief   複数エネミーの動的生成、一括更新・描画、および生存管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

// 前方宣言
class Enemy;
class Player;
class ProjectileManager;
class StageManager;
class BlobShadow;

// 敵の種類
enum class EnemyType
{
	Normal,
	Turret,
	Dummy,
	Boss
};

// 敵の生成とライフサイクルを管理するシステム
class EnemyManager
{
public:
	// コンストラクタ
	EnemyManager();
	// デストラクタ
	~EnemyManager();
	// 初期化処理
	void Initialize(ID3D11Device* device);
	// 更新処理
	void Update(float dt);
	// 描画処理
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);
	// 生存している敵の影を描画
	void RenderShadows(BlobShadow* shadowRenderer, ID3D11DeviceContext* context,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);

	// 敵を指定の座標に出現させる
	void SpawnEnemy(EnemyType type, const DirectX::SimpleMath::Vector3& position, ID3D11Device* device);

	// すべての敵のステータスを初期状態にリセットして復活させる
	void ResetAllEnemies();

	// 管理中の敵リストを取得
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return m_enemies; }

	// プレイヤーへの参照を設定
	void SetPlayer(Player* player) { m_player = player; }
	// 弾・ミサイル管理への参照を設定
	void SetProjectileManager(ProjectileManager* manager) { m_projectileManager = manager; }
	// ステージ管理への参照を設定
	void SetStageManager(StageManager* stageMgr);
	// 共有弾モデルを設定
	void SetSharedBulletModel(std::shared_ptr<DirectX::Model> model) { m_bulletModel = model; }

private:
	// --- 調整用定数パラメータ ---
	static constexpr float ENEMY_SHADOW_SCALE = 4.0f;       //< 敵の丸影の初期サイズ倍率
	static constexpr float BOSS_SHADOW_SCALE = 20.0f;       //< ボスの丸影のサイズ倍率
	static constexpr float EPSILON_SQUARED = 0.0001f;       //< ゼロ除算判定用の極小値（二乗値）

	// ステータス設定
	static constexpr float HP_NORMAL = 200.0f;              //< ノーマル敵の初期HP
	static constexpr float HP_TURRET = 50.0f;               //< タレットの初期HP
	static constexpr float HP_DUMMY = 100.0f;               //< ダミーの初期HP
	static constexpr float HP_BOSS = 3500.0f;               //< ボスの初期HP
	static constexpr float BOSS_COLLISION_R = 7.0f;         //< ボスの当たり判定半径
	static constexpr float BOSS_COLLISION_H = 15.0f;        //< ボスの当たり判定高さ

	// リソースパス
	static constexpr const wchar_t* PATH_NORMAL_MDL  = L"Resources/Models/Robot/Enemy_Normal/Enemy_Normal.sdkmesh";
	static constexpr const wchar_t* PATH_NORMAL_TEX  = L"Resources/Models/Robot/Enemy_Normal";
	static constexpr const wchar_t* PATH_TURRET_MDL  = L"Resources/Models/Robot/Enemy_Turret/Enemy_Turret.sdkmesh";
	static constexpr const wchar_t* PATH_TURRET_TEX  = L"Resources/Models/Robot/Enemy_Turret";
	static constexpr const wchar_t* PATH_BOSS_LOWER  = L"Resources/Models/Robot/Enemy_Boss/Enemy_Boss_Robot_Lower.sdkmesh";
	static constexpr const wchar_t* PATH_BOSS_UPPER  = L"Resources/Models/Robot/Enemy_Boss/Enemy_Boss_Robot_Upper.sdkmesh";
	static constexpr const wchar_t* PATH_BOSS_TEX    = L"Resources/Models/Robot/Enemy_Boss";

	// --- 管理データ・コンテナ ---
	std::vector<std::unique_ptr<Enemy>> m_enemies;          //< 管理下の敵リスト

	// --- リソース ---
	std::shared_ptr<DirectX::Model> m_bulletModel;          //< 敵が使用する共有弾モデル

	// --- 外部システム参照 ---
	Player* m_player;                                       //< プレイヤーへの参照
	ProjectileManager* m_projectileManager;                 //< 武器管理への参照
	StageManager* m_stageManager;                           //< ステージ管理への参照
};