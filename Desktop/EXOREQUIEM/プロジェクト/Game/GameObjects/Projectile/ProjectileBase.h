/*****************************************************************//**
 * @file    ProjectileBase.h
 * @brief   すべての弾・ミサイルオブジェクトの共通基底クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Robot;
class StageManager;

class ProjectileBase
{
public:
	// コンストラクタ
	ProjectileBase() = default;
	// デストラクタ
	virtual ~ProjectileBase() = default;
	// 更新処理
	virtual void Update(float dt) = 0;
	// 描画処理
	virtual void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj) = 0;
	
	// 判定の取得
	virtual DirectX::BoundingSphere GetCollider() const = 0;

	// アクティブ状態の取得
	bool IsActive() const { return m_isActive; }
	// アクティブ状態の設定
	void SetActive(bool active) { m_isActive = active; }

	// 座標の取得
	const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }
	// 座標の設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_position = pos; }

	// 回転の取得
	const DirectX::SimpleMath::Quaternion& GetRotation() const { return m_rotation; }
	// 回転の設定
	void SetRotation(const DirectX::SimpleMath::Quaternion& rot) { m_rotation = rot; }

	// 速度の取得
	const DirectX::SimpleMath::Vector3& GetVelocity() const { return m_velocity; }
	// 速度の設定
	void SetVelocity(const DirectX::SimpleMath::Vector3& vel) { m_velocity = vel; }

	// 発射元座標の取得
	const DirectX::SimpleMath::Vector3& GetShooterPosition() const { return m_shooterPosition; }
	// 発射元座標の設定
	void SetShooterPosition(const DirectX::SimpleMath::Vector3& pos) { m_shooterPosition = pos; }

	// 最大寿命の取得
	float GetLifeTime() const { return m_lifeTime; }
	// 最大寿命の設定
	void SetLifeTime(float lifeTime) { m_lifeTime = lifeTime; }

	// 現在の残り寿命の取得
	float GetCurrentLifeTime() const { return m_currentLifeTime; }
	// 現在の残り寿命の設定
	void SetCurrentLifeTime(float current) { m_currentLifeTime = current; }

	// ダメージ量の取得
	float GetDamage() const { return m_damage; }
	// ダメージ量の設定
	void SetDamage(float damage) { m_damage = damage; }

	// 所有者ロボットの取得
	const Robot* GetOwnerRobot() const { return m_ownerRobot; }
	// 所有者ロボットの設定
	void SetOwnerRobot(const Robot* owner) { m_ownerRobot = owner; }

	// モデルの取得
	std::shared_ptr<DirectX::Model> GetModel() const { return m_model; }
	// モデルの設定
	void SetModel(std::shared_ptr<DirectX::Model> model) { m_model = model; }

	// ステージ管理者の取得
	StageManager* GetStageManager() const { return m_stageManager; }
	// ステージ管理者の設定
	void SetStageManager(StageManager* sm) { m_stageManager = sm; }

private:
	// --- 状態値・トランスフォーム ---
	DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3::Zero;            //< 位置
	DirectX::SimpleMath::Quaternion m_rotation = DirectX::SimpleMath::Quaternion::Identity;  //< 回転
	DirectX::SimpleMath::Vector3 m_velocity = DirectX::SimpleMath::Vector3::Zero;            //< 速度
	DirectX::SimpleMath::Vector3 m_shooterPosition = DirectX::SimpleMath::Vector3::Zero;     //< 発射元の位置

	bool m_isActive = false;                                 //< アクティブ状態

	// --- 性能・ステータスパラメータ ---
	float m_lifeTime = 0.0f;                                 //< 最大寿命
	float m_currentLifeTime = 0.0f;                          //< 現在の残り寿命
	float m_damage = 0.0f;                                   //< ダメージ量

	// --- リソース・外部システム参照 ---
	std::shared_ptr<DirectX::Model> m_model = nullptr;       //< モデルリソース
	const Robot* m_ownerRobot = nullptr;                     //< 撃ったロボットへのポインタ
	StageManager* m_stageManager = nullptr;					 //< ステージ管理者へのポインタ
};