/*****************************************************************//**
 * @file    Missile.h
 * @brief   各種ミサイルの共通基底クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <functional>
#include <memory>
#include "Game/GameObjects/Projectile/ProjectileBase.h"

class StageManager;

// 各種ミサイル（弾道・誘導など）の共通基底となるクラス
class Missile : public ProjectileBase
{
public:
	// コンストラクタ
	Missile();
	// デストラクタ
	virtual ~Missile() override;
	// 初期化処理（ミサイルの初期化と発射パラメータの設定）
	virtual void Initialize(
		const DirectX::SimpleMath::Vector3& startPos,
		const DirectX::SimpleMath::Vector3& startDir,
		std::function<DirectX::SimpleMath::Vector3()> getPosFunc,
		std::function<DirectX::SimpleMath::Vector3()> getVelFunc,
		std::function<bool()> isActiveFunc,
		float speed, float turnSpeed, float lifeTime, const Robot* owner);
	// 更新処理
	void Update(float dt) override;
	// 描画処理
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj) override;

	// 当たり判定（球）の取得
	DirectX::BoundingSphere GetCollider() const override;

protected:
	// ターゲット捕捉フラグ
	bool HasAcquiredTarget() const { return m_hasAcquiredTarget; }
	// ターゲット補足セット
	void SetHasAcquiredTarget(bool hasAcquired) { m_hasAcquiredTarget = hasAcquired; }

	// 最大巡航速度
	float GetSpeed() const { return m_speed; }

	// 旋回性能
	float GetTurnSpeed() const { return m_turnSpeed; }

	// 発射直後の誘導無効化タイマー
	float GetNoHomingTimer() const { return m_noHomingTimer; }

	// 目標座標の計算（派生クラスで実装）
	virtual DirectX::SimpleMath::Vector3 CalculateAimPoint(float dt) = 0;
	// 旋回計算と速度更新（派生クラスで実装）
	virtual void CalculateSteering(float dt, const DirectX::SimpleMath::Vector3& toTarget, float dot) = 0;
	// 進行方向にあわせた姿勢更新
	virtual void UpdateRotationFromVelocity();

	// 移動・旋回・衝突判定の統合処理
	void UpdateMovement(float dt, const DirectX::SimpleMath::Vector3& aimPoint);
	// 誘導計算のトリガー
	void UpdateHoming(float dt, const DirectX::SimpleMath::Vector3& aimPoint);
	// 目標への旋回処理
	void ProcessHomingSteer(float dt, const DirectX::SimpleMath::Vector3& aimPoint);
	// ステージとの衝突判定
	void CheckWorldCollision(float dt);

	// ターゲット座標の取得
	DirectX::SimpleMath::Vector3 GetTargetPosition() const;
	// ターゲット速度の取得
	DirectX::SimpleMath::Vector3 GetTargetVelocity() const;
	// ターゲットの生存確認
	bool IsTargetActive() const;

protected:
	// --- 調整用定数パラメータ ---
	static constexpr float MISSILE_DAMAGE = 10.0f;              //< 命中時のダメージ量
	static constexpr float INITIAL_NO_HOMING_TIME = 0.5f;       //< 発射直後の誘導無効時間
	static constexpr float LAUNCH_SPEED_RATE = 0.5f;            //< 発射直後の初速レート
	static constexpr float COLLISION_RADIUS = 0.5f;             //< 当たり判定の半径
	static constexpr float SMOKE_OFFSET_DISTANCE = 1.0f;        //< スモーク発生位置のオフセット
	static constexpr float MODEL_DEFAULT_SCALE = 0.3f;          //< モデルの描画スケール
	static constexpr float DUMMY_AIM_DISTANCE = 1000.0f;        //< ターゲット不在時の仮目標距離
	static constexpr float SPEED_EPSILON = 0.001f;              //< ゼロ除算防止用の速度閾値
	static constexpr float MAX_PREDICTION_TIME = 3.0f;          //< 偏差射撃の最大予測時間
	static constexpr float PREDICTION_BLEND_RATE = 0.5f;        //< 予測座標とのブレンド率
	static constexpr float MIN_HOMING_DISTANCE = 5.0f;          //< 誘導を打ち切る最小距離
	static constexpr float ZERO_DIVISION_EPSILON = 0.0001f;     //< ゼロ除算防止用の距離閾値
	static constexpr float FREE_HOMING_TURN_BOOST = 2.0f;       //< 非ロックオン時の旋回ブースト倍率
	static constexpr float MISSILE_ACCELERATION = 50.0f;        //< ミサイルの加速度
	static constexpr float ACQUIRE_FOV_DOT = 0.707f;            //< ターゲットを視界に捉える内積閾値
	static constexpr float MIN_DYNAMIC_TURN_RATE = 0.1f;        //< ロックオン時の最低旋回レート
	static constexpr float ROTATION_VELOCITY_THRESHOLD = 0.01f; //< 姿勢更新を行う最低速度の閾値
	static constexpr float GROUND_HEIGHT_THRESHOLD = 0.0f;      //< 地面と判定するY座標の閾値

private:
	// --- 状態値・トランスフォーム ---
	bool m_hasAcquiredTarget = false;                           //< ターゲットを視界に捉えたかのフラグ

	// --- 性能・ステータスパラメータ ---
	float m_speed = 0.0f;                                       //< 最大巡航速度
	float m_turnSpeed = 0.0f;                                   //< 旋回性能
	float m_noHomingTimer = 0.0f;                               //< 発射直後の誘導無効化タイマー

	// --- コールバック関数 ---
	std::function<DirectX::SimpleMath::Vector3()> m_getTargetPos; //< ターゲット座標取得関数
	std::function<DirectX::SimpleMath::Vector3()> m_getTargetVel; //< ターゲット速度取得関数
	std::function<bool()> m_isTargetActive;                       //< ターゲット生存確認関数
};