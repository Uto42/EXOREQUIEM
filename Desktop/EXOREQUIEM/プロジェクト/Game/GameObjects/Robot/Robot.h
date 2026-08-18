/*****************************************************************//**
 * @file    Robot.h
 * @brief   プレイヤーおよび敵の共通基底クラス（物理、エネルギー、ダメージ管理）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/Energy/Energy.h"
#include "Game/GameObjects/Robot/State/RobotStateTypes.h"
#include "Game/Systems/Input/RobotCommand.h"
#include <functional>

class IManipulator;
class Thruster;
class StageManager;
class RobotState;
class WeaponController;

class Robot
{
public:
	// 被弾時コールバックの型定義 (発射元座標を受け取る)
	using OnHitCallback = std::function<void(const DirectX::SimpleMath::Vector3&)>;

	// コンストラクタ
	Robot();
	// デストラクタ
	virtual ~Robot();
	// モデルのリソース初期化
	void InitializeModel(ID3D11Device* device, const wchar_t* modelPath, const wchar_t* textureDir);
	// 更新処理
	virtual void Update(const RobotCommand& cmd, float dt);
	// 回転の更新
	virtual void UpdateRotation(const RobotCommand& cmd, float dt);
	// 描画処理
	virtual void Render(
		ID3D11DeviceContext* context, 
		DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, 
		const DirectX::SimpleMath::Matrix& proj,
		float alpha = 1.0f);

	// 物理移動の更新
	void UpdatePhysics(float dt);
	// エネルギー状態の更新
	void UpdateEnergy(float dt);

	// 被ダメージ処理
	void TakeDamage(float damage);
	// エネルギーの消費
	void ConsumeEnergy(float amount);
	// 回避クールダウンの開始
	void StartEvadeCooldown(float duration = 0.5f) { m_evadeCooldownTimer = duration; }

	// ステートの変更
	void ChangeState(RobotStateTypes nextState, const RobotCommand& cmd = {});
	// リセットステート
	void ResetState();

	// コールバックの登録
	void SetOnHitCallback(OnHitCallback callback) { m_onHitCallback = callback; }
	// 衝突時の通知処理
	void OnHit(const DirectX::SimpleMath::Vector3& shooterPos)
	{
		if (m_onHitCallback) {
			m_onHitCallback(shooterPos);
		}
	}

	// 当たり判定サイズの設定
	void SetCollisionSize(float radius, float height)
	{
		m_collisionRadius = radius;
		m_collisionHeight = height;
	}
	// 当たり判定スフィアの取得
	virtual DirectX::BoundingSphere GetBoundingSphere() const;

	// 座標の取得
	const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }
	// 座標の設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_position = pos; }

	// 速度の取得
	const DirectX::SimpleMath::Vector3& GetVelocity() const { return m_velocity; }
	// 速度の設定
	void SetVelocity(const DirectX::SimpleMath::Vector3& vel) { m_velocity = vel; }

	// 回転の取得
	const DirectX::SimpleMath::Quaternion& GetRotation() const { return m_rotation; }
	// 回転の設定
	virtual void SetRotation(const DirectX::SimpleMath::Quaternion& rot) { m_rotation = rot; }

	// 耐久値の取得
	float GetHealth() const { return m_health; }
	// 耐久値の設定
	void SetHealth(float health) { m_health = health; }

	// 最大耐久値の取得
	float GetMaxHealth() const { return m_maxHealth; }
	// 最大耐久値の設定
	void SetMaxHealth(float maxHealth) { m_maxHealth = maxHealth; }

	// アクティブ状態の取得
	bool IsActive() const { return m_isActive; }
	// アクティブ状態の設定
	void SetActive(bool active) { m_isActive = active; }

	// 表示状態の取得
	bool IsVisible() const { return m_isVisible; }
	// 表示状態の設定
	void SetVisible(bool visible) { m_isVisible = visible; }

	// 接地状態の取得
	bool IsGrounded() const { return m_isGrounded; }
	// 接地状態の設定
	void SetGrounded(bool grounded) { m_isGrounded = grounded; }

	// ステージ管理者の取得
	StageManager* GetStageManager() const { return m_stageManager; }
	// ステージ管理者の設定
	void SetStageManager(StageManager* sm);

	// コントローラーの設定
	void SetController(std::unique_ptr<IManipulator> manipulator);
	// 重力適用状態の設定
	void SetGravityEnabled(bool enabled) { m_isGravityEnabled = enabled; }
	// モデル透明度の更新
	void UpdateModelAlpha(float alpha);

	// モデルの取得
	DirectX::Model* GetModel() const { return m_model.get(); }
	// 武器コントローラーの取得
	WeaponController* GetWeapon() { return m_weaponController.get(); }
	// 耐久値割合の取得
	float GetHealthRatio() const { return m_health / m_maxHealth; }
	// 生存状態の取得
	bool IsAlive() const { return m_isActive; }
	// 現在のエネルギー量の取得
	float GetCurrentEnergy() const { return m_energy.GetCurrent(); }
	// 最大エネルギー量の取得
	float GetMaxEnergy() const { return m_energy.GetMax(); }
	// エネルギー割合の取得
	float GetEnergyRatio() const { return m_energy.GetCurrent() / m_energy.GetMax(); }
	// 現在のステートタイプの取得
	RobotStateTypes GetCurrentStateType() const;
	// 当たり判定半径の取得
	float GetRadius() const { return m_collisionRadius * 2.0f; }
	// 回避可能状態の取得
	bool CanEvade() const { return m_evadeCooldownTimer <= 0.0f; }
	// 照準座標の取得
	virtual DirectX::SimpleMath::Vector3 GetAimPosition() const { return m_position; }

protected:
	// --- 調整用定数パラメータ ---
	// 物理・移動系
	static constexpr float GRAVITY_ACCEL = -40.4f;                  //< 重力加速度
	static constexpr float ENERGY_RECOVERY_RATE = 30.0f;            //< エネルギー回復速度
	static constexpr float ENERGY_RECOVERY_DELAY = 1.5f;            //< 回復開始までの時間

	// 旋回・入力系
	static constexpr float ROTATION_SPEED = 8.0f;                   //< 自動旋回時の補間速度
	static constexpr float ROTATION_SPEED_COMBAT = 15.0f;           //< 射撃・ロックオン時の補間速度
	static constexpr float INPUT_DEADZONE_SQUARED = 0.001f;         //< コントローラー入力のデッドゾーン
	static constexpr float ROTATION_INPUT_THRESHOLD = 0.01f;        //< 旋回を開始する移動ベクトルの最小しきい値
	static constexpr float BACKWARD_INPUT_THRESHOLD = -0.1f;        //< 後退中とみなす内積しきい値
	static constexpr float LENGTH_EPSILON_SQUARED = 0.0001f;        //< ゼロベクトル判定を回避するための微小長さ閾値の二乗
	static constexpr float OPPOSITE_DIR_THRESHOLD = -0.99f;         //< Slerpによるバグ（反転）を回避するための180度反転内積閾値
	static constexpr float OPPOSITE_EVASION_OFFSET = 0.1f;          //< 真逆を向いた際に回転経路を確定させる右方向への微小オフセット量

	// 描画・モデル・判定系
	static constexpr float MODEL_DEFAULT_SCALE = 0.15f;             //< 描画時のモデルの基本スケール
	static constexpr float MODEL_YAW_OFFSET_DEG = -90.0f;           //< モデル本来の向きを補正する初期回転角度(度)
	static constexpr float RENDER_MIN_ALPHA = 0.05f;                //< 描画をスキップする最小透明度しきい値
	static constexpr float RENDER_OPAQUE_ALPHA = 0.99f;             //< 不透明オブジェクトとして扱う透明度の境界値
	static constexpr float BASE_RADIUS = 1.0f;                      //< 機体の基本判定半径
	static constexpr float BASE_HEIGHT = 4.0f;                      //< 機体の基本判定高さ
	static constexpr float HALF_RATIO = 0.5f;                       //< 中心や半分の計算用係数
	static constexpr float DEFAULT_HEALTH_VALUE = 1.0f;             //< 耐久値・最大耐久値のデフォルト初期値
	static constexpr unsigned int BLEND_SAMPLE_MASK = 0xFFFFFFFF;   //< 描画時のOMSetBlendStateに指定するデフォルトサンプルマスク

	// --- 状態値・パラメータ ---
	DirectX::SimpleMath::Vector3 m_position;                        //< 座標
	DirectX::SimpleMath::Vector3 m_velocity;                        //< 速度
	DirectX::SimpleMath::Quaternion m_rotation;                     //< 回転
	DirectX::SimpleMath::Matrix m_worldMatrix;                      //< ワールド行列
	float m_health = 0.0f;                                          //< 現在の耐久値
	float m_maxHealth = 0.0f;                                       //< 最大耐久値
	float m_energyRecoveryTimer = 0.0f;                             //< エネルギー回復ディレイ用タイマー
	float m_evadeCooldownTimer = 0.0f;                              //< 回避クールダウン
	float m_collisionRadius = 0.0f;                                 //< 当たり判定の半径
	float m_collisionHeight = 0.0f;                                 //< 当たり判定の高さ
	bool m_isActive = false;                                        //< アクティブフラグ
	bool m_isGrounded = false;                                      //< 接地フラグ
	bool m_isGravityEnabled = false;                                //< 重力適用フラグ
	bool m_isVisible = false;                                       //< 表示フラグ
	Energy m_energy;                                                //< エネルギー管理
	OnHitCallback m_onHitCallback;                                  //< ヒットコールバック

	// --- リソース ---
	std::unique_ptr<DirectX::Model> m_model;                        //< モデル
	std::unique_ptr<Thruster> m_thruster;                           //< スラスター
	std::unique_ptr<IManipulator> m_manipulator;                    //< 入力コントローラー
	std::unique_ptr<RobotState> m_currentState;                     //< 現在のステート
	std::unique_ptr<WeaponController> m_weaponController;           //< 武器コントローラー

	// --- 外部システム参照 ---
	StageManager* m_stageManager = nullptr;                         //< ステージ管理者への参照
};