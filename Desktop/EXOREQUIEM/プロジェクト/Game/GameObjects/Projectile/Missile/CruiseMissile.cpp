/*****************************************************************//**
 * @file    CruiseMissile.cpp
 * @brief   巡航ミサイルの誘導アルゴリズム実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Projectile/Missile/CruiseMissile.h"

 /**
  * @brief コンストラクタ
  */
CruiseMissile::CruiseMissile() : Missile()
{
}

/**
 * @brief デストラクタ
 */
CruiseMissile::~CruiseMissile()
{
}

/**
 * @brief ターゲットの速度と距離から未来の着弾予測座標を計算する
 * @param[in] dt 前フレームからの経過時間（秒）
 * @return 補正された最終的なエイム座標
 */
DirectX::SimpleMath::Vector3 CruiseMissile::CalculateAimPoint(float dt)
{
	UNREFERENCED_PARAMETER(dt);

	// ターゲットを見失っている、または発射直後の誘導無効時間中（ディレイ）はそのまま直進させる
	if (!IsTargetActive() || GetNoHomingTimer() > 0.0f)
	{
		return GetPosition() + (GetVelocity() * DUMMY_AIM_DISTANCE);
	}

	// ターゲットの現在座標と速度を取得し、距離を算出する
	DirectX::SimpleMath::Vector3 targetPos = GetTargetPosition();
	DirectX::SimpleMath::Vector3 targetVel = GetTargetVelocity();
	float dist = DirectX::SimpleMath::Vector3::Distance(GetPosition(), targetPos);

	// 現在の弾速と距離から着弾までの予測時間（偏差）を算出し、異常な遠未来を予測しないよう制限する
	float timeToImpact = (GetSpeed() > SPEED_EPSILON) ? (dist / GetSpeed()) : 0.0f;
	timeToImpact = std::min<float>(timeToImpact, MAX_PREDICTION_TIME);

	// 現在座標と未来座標をブレンドして適度な「誘導の甘さ」を持たせる
	DirectX::SimpleMath::Vector3 fullPredictedPos = targetPos + (targetVel * timeToImpact);
	return DirectX::SimpleMath::Vector3::Lerp(targetPos, fullPredictedPos, PREDICTION_BLEND_RATE);
}

/**
 * @brief 目標へ向かうための旋回処理と加速度の更新
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] toTarget 目標座標へ向かう正規化された方向ベクトル
 * @param[in] dot 現在の進行方向と目標方向の内積（1.0で完全正面）
 */
void CruiseMissile::CalculateSteering(float dt, const DirectX::SimpleMath::Vector3& toTarget, float dot)
{
	DirectX::SimpleMath::Vector3 currentDir = GetVelocity();
	currentDir.Normalize();

	// ターゲットを視界（FOV）に捉える前の初期アプローチ処理
	if (!HasAcquiredTarget())
	{
		// ターゲットを視界に収めるため、一時的に旋回速度をブーストして強引に振り向かせる
		float turnRate = GetTurnSpeed() * dt * FREE_HOMING_TURN_BOOST;
		DirectX::SimpleMath::Vector3 newDir = DirectX::SimpleMath::Vector3::Lerp(currentDir, toTarget, turnRate);
		newDir.Normalize();

		float nextSpeed = std::min<float>(GetVelocity().Length() + (MISSILE_ACCELERATION * dt), GetSpeed());
		SetVelocity(newDir * nextSpeed);

		// ターゲットが設定された視野角（内積閾値）に入ったら、本格的なロックオン追尾モードへ移行する
		if (dot > ACQUIRE_FOV_DOT) SetHasAcquiredTarget(true);
	}
	// ターゲットを視界に捉えた後（ロックオン状態）の誘導処理
	else
	{
		// プレイヤーの急激な回避行動などでターゲットが視界から外れた場合は、旋回を打ち切って直進状態にする
		if (dot < ACQUIRE_FOV_DOT)
		{
			SetVelocity(currentDir * GetVelocity().Length());
		}
		// 視界に捉え続けている場合は、角度のズレ（内積）に応じて滑らかに追尾旋回を行う
		else
		{
			float dynamicTurn = GetTurnSpeed() * std::max<float>(dot, MIN_DYNAMIC_TURN_RATE);
			DirectX::SimpleMath::Vector3 newDir =
				DirectX::SimpleMath::Vector3::Lerp(currentDir, toTarget, dynamicTurn * dt);

			newDir.Normalize();

			// 速度を加速させつつ、最大速度を超えないように制限する
			float nextSpeed = std::min(GetVelocity().Length() + (MISSILE_ACCELERATION * dt), GetSpeed());
			SetVelocity(newDir * nextSpeed);
		}
	}
}