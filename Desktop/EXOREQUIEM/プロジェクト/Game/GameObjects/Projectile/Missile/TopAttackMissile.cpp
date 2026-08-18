/*****************************************************************//**
 * @file    TopAttackMissile.cpp
 * @brief   高高度誘導ミサイル（トップアタック）の誘導アルゴリズム実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Projectile/Missile/TopAttackMissile.h"

/**
 * @brief コンストラクタ
 */
TopAttackMissile::TopAttackMissile()
	: Missile()
	, m_isTerminalPhase(false)
	, m_terminalTargetPos(DirectX::SimpleMath::Vector3::Zero)
{
}

/**
 * @brief デストラクタ
 */
TopAttackMissile::~TopAttackMissile() 
{
}

/**
 * @brief ミサイルの初期化と発射パラメータの設定
 * @param[in] startPos 発射位置のワールド座標
 * @param[in] startDir 発射される向き（正規化ベクトル）
 * @param[in] getPosFunc ターゲットの座標を取得するコールバック関数
 * @param[in] getVelFunc ターゲットの速度を取得するコールバック関数
 * @param[in] isActiveFunc ターゲットが生存しているか確認する関数
 * @param[in] speed ミサイルの最大巡航速度
 * @param[in] turnSpeed ミサイルの旋回性能
 * @param[in] lifeTime 自然消滅するまでの生存時間（秒）
 * @param[in] owner 発射主のロボットへのポインタ
 */
void TopAttackMissile::Initialize(const DirectX::SimpleMath::Vector3& startPos, 
	const DirectX::SimpleMath::Vector3& startDir, std::function<DirectX::SimpleMath::Vector3()> getPosFunc, 
	std::function<DirectX::SimpleMath::Vector3()> getVelFunc, std::function<bool()> isActiveFunc,
	float speed, float turnSpeed, float lifeTime, const Robot* owner)
{
	Missile::Initialize(startPos, startDir, getPosFunc, getVelFunc, isActiveFunc, speed, turnSpeed, lifeTime, owner);

	m_isTerminalPhase = false;
	m_terminalTargetPos = DirectX::SimpleMath::Vector3::Zero;
}

/**
 * @brief 上昇・巡航フェーズと急降下フェーズを判定し、それぞれの目標座標を計算する
 * @param[in] dt 前フレームからの経過時間（秒）
 * @return 補正された最終的なエイム座標
 */
DirectX::SimpleMath::Vector3 TopAttackMissile::CalculateAimPoint(float dt)
{
	UNREFERENCED_PARAMETER(dt);

	// 最終降下フェーズに入っている場合は、確定済みの着弾点へ一直線に向かう
	if (m_isTerminalPhase) return m_terminalTargetPos;

	// ターゲットをロスト、または発射直後の誘導無効時間中は直進させる
	if (!IsTargetActive() || GetNoHomingTimer() > 0.0f)
	{
		return GetPosition() + (GetVelocity() * DUMMY_AIM_DISTANCE);
	}

	DirectX::SimpleMath::Vector3 targetPos = GetTargetPosition();
	DirectX::SimpleMath::Vector3 targetVel = GetTargetVelocity();

	// ターゲットが遠ざかる速度を考慮し、相対速度から着弾までの予測時間を算出する
	DirectX::SimpleMath::Vector3 toTargetDir = targetPos - GetPosition();
	float dist = toTargetDir.Length();
	toTargetDir = (dist > ZERO_DIVISION_EPSILON) ? toTargetDir / dist : DirectX::SimpleMath::Vector3::Forward;

	// ターゲットの速度ベクトルをミサイルの進行方向に射影し、相対速度を計算する
	float escapeSpeed = targetVel.Dot(toTargetDir);
	float relativeSpeed = std::max<float>(GetSpeed() - escapeSpeed, SPEED_EPSILON);

	// ターゲットの頭上に到達するまでの残り上昇距離を計算し、着弾までの時間を算出する
	float remainUpload = std::max<float>(0.0f, (targetPos.y + CRUISE_HEIGHT_OFFSET) - GetPosition().y);
	float baseDist = dist + (remainUpload + CRUISE_HEIGHT_OFFSET);
	float timeToImpact = baseDist / relativeSpeed;

	// ターゲットの未来位置を予測し、現在座標とブレンドして適度な誘導性能に調整する
	DirectX::SimpleMath::Vector3 fullPredictedPos =
		targetPos + (targetVel * std::min<float>(timeToImpact, MAX_PREDICTION_TIME));

	// ターゲットの未来位置と現在位置をブレンドして、急激な旋回を抑制する
	DirectX::SimpleMath::Vector3 blendedAimPoint =
		DirectX::SimpleMath::Vector3::Lerp(targetPos, fullPredictedPos, PREDICTION_BLEND_RATE);

	// 水平距離を監視し、ターゲットの頭上付近に到達したら最終降下（トップアタック）モードへ移行する
	DirectX::SimpleMath::Vector3 flatMissilePos =
		DirectX::SimpleMath::Vector3(GetPosition().x, 0.0f, GetPosition().z);

	// ターゲットの頭上付近の座標を水平面に投影
	DirectX::SimpleMath::Vector3 flatTargetPos =
		DirectX::SimpleMath::Vector3(blendedAimPoint.x, 0.0f, blendedAimPoint.z);

	float horizontalDist = DirectX::SimpleMath::Vector3::Distance(flatMissilePos, flatTargetPos);

	if (horizontalDist < DIVE_TRIGGER_DISTANCE || dist < DIVE_TRIGGER_DISTANCE)
	{
		m_isTerminalPhase = true;

		// 落下にかかる「真の時間」を計算し、そこから最終的な着弾地点を割り出す
		float fallDist = std::max<float>(0.0f, GetPosition().y - targetPos.y);
		float diveSpeed = GetSpeed() * TERMINAL_DIVE_SPEED_MULTIPLIER;

		// 落下距離 ÷ 落下速度 ＝ 落下にかかる時間
		float fallTime = fallDist / std::max<float>(diveSpeed, SPEED_EPSILON);

		// 落下時間ぶんだけの未来位置を計算
		DirectX::SimpleMath::Vector3 finalPredictedPos = targetPos + (targetVel * fallTime);

		// 最終着弾点をロック（Y座標はターゲットの高さに合わせる）
		m_terminalTargetPos = finalPredictedPos;
		m_terminalTargetPos.y = targetPos.y;

		return m_terminalTargetPos;
	}

	// 通常フェーズ中は、ターゲットの頭上（巡航高度）をエイム座標とする
	float targetHeight = targetPos.y + CRUISE_HEIGHT_OFFSET;
	blendedAimPoint.y = std::min<float>(targetHeight, MAX_CRUISE_HEIGHT);

	return blendedAimPoint;
}

/**
 * @brief 巡航・急降下などのフェーズに応じた旋回処理と速度更新を行う
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] toTarget 目標座標へ向かう正規化された方向ベクトル
 * @param[in] dot 現在の進行方向と目標方向の内積
 */
void TopAttackMissile::CalculateSteering(float dt, const DirectX::SimpleMath::Vector3& toTarget, float dot)
{
	DirectX::SimpleMath::Vector3 currentDir = GetVelocity();
	currentDir.Normalize();

	// 最終降下フェーズでは、旋回性能を無視して強引に真下へ加速させる
	if (m_isTerminalPhase)
	{
		float diveSpeed = GetSpeed() * TERMINAL_DIVE_SPEED_MULTIPLIER;
		SetVelocity(toTarget * diveSpeed);
		return;
	}

	// ターゲットを視界（FOV）に捉える前の初期アプローチ処理
	if (!HasAcquiredTarget())
	{
		float turnRate = GetTurnSpeed() * dt * FREE_HOMING_TURN_BOOST;
		DirectX::SimpleMath::Vector3 newDir = DirectX::SimpleMath::Vector3::Lerp(currentDir, toTarget, turnRate);
		newDir.Normalize();

		float nextSpeed = std::min(GetVelocity().Length() + (MISSILE_ACCELERATION * dt), GetSpeed());
		SetVelocity(newDir * nextSpeed);

		// 視界に入ったらロックオン状態へ移行
		if (dot > ACQUIRE_FOV_DOT) SetHasAcquiredTarget(true);
	}
	else
	{
		float turnMultiplier = (dot < ACQUIRE_FOV_DOT) ? 0.5f : 1.0f;

		// 旋回性能をターゲットの方向との角度に応じて調整し、急激な旋回を抑制する
		float dynamicTurn = GetTurnSpeed() * std::max<float>(dot, MIN_DYNAMIC_TURN_RATE) * turnMultiplier;
		DirectX::SimpleMath::Vector3 newDir = DirectX::SimpleMath::Vector3::Lerp(currentDir, toTarget, dynamicTurn * dt);
		newDir.Normalize();

		float nextSpeed = std::min<float>(GetVelocity().Length() + (MISSILE_ACCELERATION * dt), GetSpeed());
		SetVelocity(newDir * nextSpeed);
	}
}

/**
 * @brief 進行方向にあわせてモデルの姿勢（回転）を更新する
 */
void TopAttackMissile::UpdateRotationFromVelocity()
{
	if (GetVelocity().LengthSquared() > ROTATION_VELOCITY_THRESHOLD)
	{
		DirectX::SimpleMath::Vector3 dir = GetVelocity();
		dir.Normalize();

		DirectX::SimpleMath::Vector3 upVector = DirectX::SimpleMath::Vector3::Up;

		// ミサイルが真下（または真上）を向いた際、外積計算が破綻するジンバルロックを防ぐため
		// 一時的にUpベクトルを前方(Forward)に切り替える
		if (fabsf(dir.Dot(DirectX::SimpleMath::Vector3::Up)) > GIMBAL_LOCK_THRESHOLD)
		{
			upVector = DirectX::SimpleMath::Vector3::Forward;
		}

		// 進行方向とUpベクトルからワールド行列を作成し、そこからクォータニオンを生成する
		DirectX::SimpleMath::Matrix lookAt =
			DirectX::SimpleMath::Matrix::CreateWorld(DirectX::SimpleMath::Vector3::Zero, dir, upVector);

		SetRotation(DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookAt));
	}
}