/*****************************************************************//**
 * @file    Player.h
 * @brief   プレイヤー機体の演出、武器制御、およびカメラ連携の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/IManipulator.h"

class LockOnSystem;
class ProjectileManager;
class ParticleManager;
struct RobotCommand;

class Player
{
public:
	// コンストラクタ
	Player();
	// デストラクタ
	~Player();
	// 初期化処理
	void Initialize(ID3D11Device* device);
	// 更新処理
	void Update(float dt);
	// 描画処理
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);

	// ロボット実体の取得
	Robot* GetRobot() const { return m_robot.get(); }
	// ロボット共有ポインタの取得
	std::shared_ptr<Robot> GetRobotShared() const { return m_robot; }

	// 操作モード（人間 or AI）を切り替える関数
	void SetAutoPilot(bool useAI, StageManager* sm = nullptr, const Robot* target = nullptr);
	// 現在オートパイロット中かどうかを返す
	bool IsAutoPilot() const { return m_isAutoPilotMode; }

	// 座標の取得
	const DirectX::SimpleMath::Vector3& GetPosition() const { return m_robot->GetPosition(); }
	// 座標の設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { if (m_robot) m_robot->SetPosition(pos); }

	// 速度の取得
	const DirectX::SimpleMath::Vector3& GetVelocity() const { return m_robot->GetVelocity(); }
	// 速度の設定
	void SetVelocity(const DirectX::SimpleMath::Vector3& velocity) { if (m_robot) m_robot->SetVelocity(velocity); }

	// 回転情報の取得
	DirectX::SimpleMath::Quaternion GetRotation() const { return m_robot ? m_robot->GetRotation() : DirectX::SimpleMath::Quaternion::Identity; }
	// 回転の設定
	void SetPlayerRotation(const DirectX::SimpleMath::Quaternion& rotation) { if (m_robot) m_robot->SetRotation(rotation); }

	// 接地状態の設定
	void SetGrounded(bool isGrounded) { if (m_robot) m_robot->SetGrounded(isGrounded); }
	// 判定用ボックスの取得
	DirectX::BoundingSphere GetBoundingSphere() const { return m_robot->GetBoundingSphere(); }

	// カメラ位置の設定
	void SetCameraPosition(const DirectX::SimpleMath::Vector3& position);
	// カメラ正面方向の取得
	const DirectX::SimpleMath::Vector3& GetCameraForward() const { return m_cameraForward; }
	// カメラ正面方向の設定
	void SetCameraForward(const DirectX::SimpleMath::Vector3& forward);
	// カメラ右方向の取得
	const DirectX::SimpleMath::Vector3& GetCameraRight() const { return m_cameraRight; }
	// カメラ右方向の設定
	void SetCameraRight(const DirectX::SimpleMath::Vector3& right);

	// ロックオン用Yaw角度の取得
	float GetYaw() const { return m_yawDegree; }
	// ロックオン用Yaw角度の設定
	void SetYaw(float degree) { m_yawDegree = degree; }
	// 正面ベクトルの取得
	DirectX::SimpleMath::Vector3 GetForward() const
	{
		float rad = DirectX::XMConvertToRadians(m_yawDegree);
		return DirectX::SimpleMath::Vector3(sinf(rad), 0.0f, cosf(rad));
	}

	// 耐久値の取得
	float GetHealth() const { return m_robot ? m_robot->GetHealth() : 0.0f; }
	// 最大耐久値の取得
	float GetMaxHealth() const { return m_robot ? m_robot->GetMaxHealth() : 0.0f; }
	// 耐久値割合の取得
	float GetHealthRatio() const
	{
		if (!m_robot || m_robot->GetMaxHealth() <= 0.0f) return 0.0f;
		return m_robot->GetHealth() / m_robot->GetMaxHealth();
	}
	// ダメージ処理
	void TakeDamage(float damage) { if (m_robot) m_robot->TakeDamage(damage); }
	// 生存確認
	bool IsAlive() const { return m_robot && m_robot->GetHealth() > 0.0f; }

	// 最大エネルギーの取得
	float GetMaxEnergy() const { return m_robot ? m_robot->GetMaxEnergy() : 0.0f; }
	// エネルギー割合の取得
	float GetEnergyRatio() const
	{
		if (!m_robot || m_robot->GetMaxEnergy() <= 0.0f) return 0.0f;
		return m_robot->GetCurrentEnergy() / m_robot->GetMaxEnergy();
	}

	// 現在のステートタイプ取得
	RobotStateTypes GetCurrentStateType() const { return m_robot ? m_robot->GetCurrentStateType() : RobotStateTypes::Idle; }

	// ターゲット可能かの取得
	bool IsTargetable() const { return m_isTargetable; }
	// ターゲット可能フラグの設定
	void SetTargetable(bool targetable) { m_isTargetable = targetable; }
	// 表示フラグの設定
	void SetVisible(bool isVisible) { m_isVisible = isVisible; }
	// フリーカメラ時などに強制的に不透明（アルファ値1.0）にするかどうかの設定
	void SetForceOpaque(bool force) { m_forceOpaque = force; }

	// ステージ管理者の取得
	StageManager* GetStageManager() const { return m_robot ? m_robot->GetStageManager() : nullptr; }
	// ステージ管理者の設定
	void SetStageManager(StageManager* stageManager);
	// ロックオンシステムの設定
	void SetLockOnSystem(LockOnSystem* system);
	// ガン・ミサイルマネージャーの設定
	void SetProjectileManager(ProjectileManager* manager);

private:
	// カメラ距離に応じた透明度の計算
	float CalculateCameraAlpha(const DirectX::SimpleMath::Matrix& view);

	// --- 調整用定数パラメータ ---
	static constexpr float DEFAULT_MAX_HEALTH = 1000.0f;        //< プレイヤーの初期最大体力
	static constexpr float CAMERA_FADE_START_DIST = 3.0f;       //< プレイヤーが半透明になり始めるカメラ距離
	static constexpr float CAMERA_FADE_END_DIST = 1.0f;         //< プレイヤーが完全に透明になるカメラ距離
	static constexpr float EVADE_COOLDOWN_DURATION = 0.2f;      //< クールダウン時間

	// --- 実体・コンポーネント ---
	std::shared_ptr<Robot> m_robot;                             //< ロボット本体
	std::unique_ptr<IManipulator> m_manipulator;                //< 操作コントローラー（脳）

	// --- 外部システム参照 ---
	LockOnSystem* m_lockOnSystem = nullptr;                     //< ロックオンシステム
	StageManager* m_stageManager = nullptr;                     //< ステージ管理者への参照

	// --- 状態値・パラメータ ---
	DirectX::SimpleMath::Vector3 m_cameraForward;               //< カメラ正面方向
	DirectX::SimpleMath::Vector3 m_cameraRight;                 //< カメラ右方向
	DirectX::SimpleMath::Vector3 m_cameraPosition;              //< カメラ位置
	float m_yawDegree = 0.0f;                                   //< ロックオン用Yaw角度

	bool m_isVisible = true;                                    //< 表示フラグ
	bool m_isTargetable = true;                                 //< ターゲット可能フラグ
	bool m_isAutoPilotMode = false;                             //< オートパイロットフラグ
	bool m_forceOpaque = false;                                 //< フリーカメラ時などに強制的に不透明にするフラグ
};