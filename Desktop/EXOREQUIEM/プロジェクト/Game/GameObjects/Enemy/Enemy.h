/*****************************************************************//**
 * @file    Enemy.h
 * @brief   敵ユニットの基底クラス：ステート制御、および偏差射撃ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/IManipulator.h"

class ProjectileManager;
class StageManager;
class ThrusterController;

class Enemy
{
public:
	// コンストラクタ
	Enemy();
	// デストラクタ
	virtual ~Enemy();
	// 初期化処理
	virtual void Initialize(
		ID3D11Device* device,
		const DirectX::SimpleMath::Vector3& initialPos,
		bool isActive,
		const wchar_t* modelPath = L"Resources/Models/Robot/Enemy_Normal/Enemy_Normal.sdkmesh",
		const wchar_t* textureDir = L"Resources/Models/Robot/Enemy_Normal");
	// 更新処理
	virtual void Update(float dt);
	// 描画処理
	virtual void Render(
		ID3D11DeviceContext* context,
		DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj);

	// 回転の更新
	virtual void UpdateRotation(float dt);

	// ロボット実体の取得
	Robot* GetRobot() const { return m_robot.get(); }
	// ロボット共有ポインタの取得
	std::shared_ptr<Robot> GetRobotShared() const { return m_robot; }
	// ロボットのセット
	void SetRobot(std::shared_ptr<Robot> robot);

	// ターゲット取得
	const Robot* GetPlayer() const { return m_playerRobot; }
	// ターゲット設定
	void SetPlayer(const Robot* playerRobot) { m_playerRobot = playerRobot; }

	// 座標の取得
	const DirectX::SimpleMath::Vector3& GetPosition() const { return m_robot->GetPosition(); }
	// 座標の設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { if (m_robot) m_robot->SetPosition(pos); }
	// 速度の取得
	const DirectX::SimpleMath::Vector3& GetVelocity() const { return m_robot->GetVelocity(); }
	// 速度の設定
	void SetVelocity(const DirectX::SimpleMath::Vector3& velocity) { if (m_robot) m_robot->SetVelocity(velocity); }
	// 接地状態取得
	bool IsGrounded() const { return m_robot ? m_robot->IsGrounded() : true; }
	// 接地状態の設定
	void SetGrounded(bool isGrounded) { if (m_robot) m_robot->SetGrounded(isGrounded); }

	// 耐久値取得
	float GetHealth() const { return m_robot ? m_robot->GetHealth() : 0.0f; }
	// 最大耐久値取得
	float GetMaxHealth() const { return m_robot ? m_robot->GetMaxHealth() : 0.0f; }
	// ダメージ処理
	void TakeDamage(float damage) { if (m_robot) m_robot->TakeDamage(damage); }
	// ロックオン可能か
	bool IsTargetable() const;
	// アクティブ状態か
	bool IsActive() const { return m_robot && m_robot->IsActive(); }

	// ステージ管理の設定
	void SetStageManager(StageManager* stageManager);
	// コントローラーの設定
	void SetController(std::unique_ptr<IManipulator> manipulator) { m_manipulator = std::move(manipulator); }
	// 武器マネージャーのセット
	void SetProjectileManager(ProjectileManager* pm);

protected:
	// --- 派生クラス用アクセサ ---
	IManipulator* GetManipulator() const { return m_manipulator.get(); }

	// --- 制御用コンポーネント ---
	std::shared_ptr<Robot> m_robot;										//< ロボット本体

private:
	// --- 定数パラメータ ---
	static constexpr float SHOOTING_TURN_SPEED = 15.0f;					//< 射撃時のプレイヤー追従旋回速度
	static constexpr float ROTATION_EPSILON_SQ = 0.001f;				//< 旋回計算をスキップするしきい値
	static constexpr float FORWARD_EPSILON_SQ = 0.0001f;				//< 正面ベクトルのゼロ判定しきい値
	static constexpr float REVERSE_DIRECTION_DOT_THRESHOLD = -0.99f;	//< 向きが真逆と判定する内積のしきい値
	static constexpr float AVOID_REVERSE_OFFSET = 0.1f;					//< 真逆時の計算破綻を防ぐためのオフセット値

	// --- 内部コンポーネント ---
	std::unique_ptr<IManipulator> m_manipulator;						//< AIコントローラー

	// --- 外部システム参照 ---
	StageManager* m_stageManager = nullptr;								//< ステージ管理者
	const Robot* m_playerRobot = nullptr;								//< ターゲットとなるプレイヤー
};