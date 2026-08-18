/*****************************************************************//**
 * @file    LockOnSystem.cpp
 * @brief   ロックオン制御システム：複数ターゲットからの最適対象選出、遮蔽物判定、および注視モードの管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Stage/StageManager.h"

/**
 * @brief コンストラクタ
 */
LockOnSystem::LockOnSystem()
	: m_playerRobot(nullptr)
	, m_currentTarget(nullptr)
	, m_stageManager(nullptr)
	, m_isLockOnActive(false)
	, m_isHardLockMode(false)
{
}

/**
 * @brief デストラクタ
 */
LockOnSystem::~LockOnSystem()
{
}

/**
 * @brief 初期化処理
 * @param[in] playerRobot プレイヤーへのポインタ
 */
void LockOnSystem::Initialize(Robot* playerRobot)
{
	m_playerRobot = playerRobot;
}

/**
 * @brief ハードロック切り替え
 */
void LockOnSystem::ToggleHardLock()
{
	m_isHardLockMode = !m_isHardLockMode;
}

/**
 * @brief ロックオン対象の選定と更新
 * @param[in] targets 全ターゲットのリスト
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @param[in] mouseDeltaX マウスのX軸移動量
 * @param[in] mouseDeltaY マウスのY軸移動量
 * @param[in] dt 経過時間
 */
void LockOnSystem::Update(const std::vector<Robot*>& targets, 
	const DirectX::SimpleMath::Vector3& cameraForward, float mouseDeltaX, float mouseDeltaY, float dt)
{
	UNREFERENCED_PARAMETER(dt);

	if (!m_playerRobot) return;

	bool currentValid = IsCurrentTargetValid(cameraForward);
	
	// 手ブレによる誤作動を防ぐため、閾値を超えた明確な操作のみをマウス移動として検知する
	bool isMouseMoving = (fabs(mouseDeltaX) > MOUSE_INPUT_THRESHOLD || fabs(mouseDeltaY) > MOUSE_INPUT_THRESHOLD);

	if (m_isHardLockMode)
	{
		if (!currentValid)
		{
			// 現在のターゲットを撃破または見失った場合、即座に次の候補を検索してロック状態を維持する
			m_currentTarget = FindBestTarget(targets, cameraForward);
			if (!m_currentTarget)
			{
				m_isLockOnActive = false;
				m_isHardLockMode = false;
			}
		}
		else if (isMouseMoving)
		{
			// ハードロック中であっても、プレイヤーが意図的にマウスを振った場合はターゲットの乗り換えを許容する
			Robot* bestTarget = FindBestTarget(targets, cameraForward);
			if (bestTarget)
			{
				m_currentTarget = bestTarget;
			}
		}
	}
	else
	{
		// ソフトロック時は常に画面内の一番最適な敵を自動で捕捉し続ける
		m_currentTarget = FindBestTarget(targets, cameraForward);
		m_isLockOnActive = (m_currentTarget != nullptr);
	}
}

/**
 * @brief 現在のターゲットがロック維持条件を満たしているか検証
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @return ロック維持可能であればtrue
 */
bool LockOnSystem::IsCurrentTargetValid(const DirectX::SimpleMath::Vector3& cameraForward) const
{
	if (!m_currentTarget || !m_currentTarget->IsActive() || m_currentTarget->GetHealth() <= ZERO_HEALTH)
	{
		return false;
	}

	if (m_isHardLockMode)
	{
		const DirectX::SimpleMath::Vector3 myPos = m_playerRobot->GetPosition();
		const DirectX::SimpleMath::Vector3 targetPos = m_currentTarget->GetPosition();
		float distSq = (targetPos - myPos).LengthSquared();

		// ハードロック中は激しい回り込みに対応するため、維持限界距離を通常より広く設定する
		float maxDist = MAX_LOCK_ON_DISTANCE * HARD_LOCK_RANGE_SCALE;
		bool isInRange = distSq <= (maxDist * maxDist);

		// 壁などの完全な遮蔽物に隠れた場合は、視線が切れたと判断してロック解除対象とする
		bool isVisible = true;
		if (m_stageManager)
		{
			// ターゲットの中心を狙うため、Y軸方向にオフセットを加えた位置でレイキャストを行う
			DirectX::SimpleMath::Vector3 start = myPos + DirectX::SimpleMath::Vector3(0, RAYCAST_OFFSET_Y, 0);
			DirectX::SimpleMath::Vector3 end = targetPos + DirectX::SimpleMath::Vector3(0, RAYCAST_OFFSET_Y, 0);
			DirectX::SimpleMath::Vector3 rayDir = end - start;
			float rayDist = rayDir.Length();

			if (rayDist > MIN_RAYCAST_DISTANCE && m_stageManager->RayCast(start, rayDir / rayDist, rayDist))
			{
				isVisible = false;
			}
		}

		return isInRange && isVisible;
	}

	// ソフトロック時は、敵が画面外に出ていないかどうかの視野角チェックを含めた通常判定を行う
	return CanLockOnToTarget(m_currentTarget, cameraForward);
}

/**
 * @brief マウスの入力方向（左右）に基づき、現在のターゲットの隣にいる敵を探す
 * @param[in] targets 全ターゲットのリスト
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @param[in] mouseDeltaX マウスのX軸移動量
 * @return 条件を満たす次のターゲット（いなければnullptr）
 */
Robot* LockOnSystem::FindNextTargetInDirection(const std::vector<Robot*>& targets,
	const DirectX::SimpleMath::Vector3& cameraForward, float mouseDeltaX) const
{
	if (!m_currentTarget || !m_playerRobot) return nullptr;

	// 現在のターゲットと自機の位置を取得
	DirectX::SimpleMath::Vector3 currentTargetPos = m_currentTarget->GetPosition();
	DirectX::SimpleMath::Vector3 myPos = m_playerRobot->GetPosition();

	// カメラ正面と世界の「上（Y軸）」を外積することで、画面上の真右を指すベクトルを算出する
	DirectX::SimpleMath::Vector3 cameraRight = DirectX::SimpleMath::Vector3::UnitY.Cross(cameraForward);
	cameraRight.Normalize();

	// マウスが右（正）か左（負）かによって、隣の敵を探すための基準方向を決定
	float sign = (mouseDeltaX > 0.0f) ? 1.0f : -1.0f;
	DirectX::SimpleMath::Vector3 searchDir = cameraRight * sign;

	Robot* bestNextTarget = nullptr;
	float minAngleDiff = INITIAL_MIN_ANGLE;

	for (auto* robot : targets)
	{
		if (!robot || robot == m_playerRobot || robot == m_currentTarget) continue;
		if (!robot->IsActive() || robot->GetHealth() <= ZERO_HEALTH) continue;
		if (!CanLockOnToTarget(robot, cameraForward)) continue;

		// 現在ロックしている敵の位置を原点とした、調査対象への方向ベクトル
		DirectX::SimpleMath::Vector3 toCandidate = robot->GetPosition() - currentTargetPos;
		toCandidate.Normalize();

		// 内積がプラスであれば、入力された方向（右または左）の半空間に敵が存在していると判定できる
		float dotDir = searchDir.Dot(toCandidate);
		if (dotDir > 0.0f)
		{
			DirectX::SimpleMath::Vector3 toCurrent = currentTargetPos - myPos;
			toCurrent.Normalize();

			DirectX::SimpleMath::Vector3 toNext = robot->GetPosition() - myPos;
			toNext.Normalize();

			// 現在の敵の方向と、候補の敵の方向の「成す角」をラジアンで算出し、すぐ隣にいる敵を特定する
			float angle = acosf(std::clamp(toCurrent.Dot(toNext), -1.0f, 1.0f));

			if (angle < minAngleDiff)
			{
				minAngleDiff = angle;
				bestNextTarget = robot;
			}
		}
	}

	return bestNextTarget;
}

/**
 * @brief ロックオン可能判定（視界・遮蔽物チェック）
 * @param[in] target 判定対象のターゲット
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @return ロックオン可能であればtrue
 */
bool LockOnSystem::CanLockOnToTarget(Robot* target, const DirectX::SimpleMath::Vector3& cameraForward) const
{
	if (!m_playerRobot || !target) return false;

	// 自機とターゲットの距離がロックオン可能範囲内かどうかを判定
	const DirectX::SimpleMath::Vector3 myPos = m_playerRobot->GetPosition();
	const DirectX::SimpleMath::Vector3 targetPos = target->GetPosition();
	const DirectX::SimpleMath::Vector3 toTarget = targetPos - myPos;

	if (toTarget.LengthSquared() > MAX_LOCK_ON_DISTANCE * MAX_LOCK_ON_DISTANCE) return false;

	DirectX::SimpleMath::Vector3 normalizedToTarget = toTarget;
	normalizedToTarget.Normalize();

	// カメラの視線ベクトルと敵へのベクトルの内積をとり、指定の視野角コサイン値に収まっているか判定
	if (cameraForward.Dot(normalizedToTarget) < LIMIT_ANGLE) return false;

	if (m_stageManager)
	{
		// 敵と自機の間に遮蔽物がある場合はロックオン不可とする
		DirectX::SimpleMath::Vector3 start = myPos + DirectX::SimpleMath::Vector3(0, RAYCAST_OFFSET_Y, 0);
		DirectX::SimpleMath::Vector3 end = targetPos + DirectX::SimpleMath::Vector3(0, RAYCAST_OFFSET_Y, 0);
		DirectX::SimpleMath::Vector3 rayDir = end - start;
		float rayDist = rayDir.Length();

		if (rayDist > MIN_RAYCAST_DISTANCE && m_stageManager->RayCast(start, rayDir / rayDist, rayDist))
		{
			return false;
		}
	}

	return true;
}

/**
 * @brief ロックオン可能な敵の中から最も優先度の高いターゲットを検索
 * @param[in] targets 全ターゲットのリスト
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @return 最も優先度の高い敵へのポインタ（いなければnullptr）
 */
Robot* LockOnSystem::FindBestTarget(const std::vector<Robot*>& targets, 
	const DirectX::SimpleMath::Vector3& cameraForward) const
{
	Robot* bestTarget = nullptr;
	float  maxScore = INITIAL_MAX_SCORE;

	for (auto* robot : targets)
	{
		if (!robot || robot == m_playerRobot) continue;
		if (!robot->IsActive() || robot->GetHealth() <= ZERO_HEALTH) continue;

		if (CanLockOnToTarget(robot, cameraForward))
		{
			// 距離と画面中央への近さの合計スコアが一番高いものをロック対象として選出する
			float score = CalcScore(robot, cameraForward);
			if (score > maxScore)
			{
				maxScore = score;
				bestTarget = robot;
			}
		}
	}

	return bestTarget;
}

/**
 * @brief ターゲットの優先度を決定する評価スコアを計算する
 * @param[in] target 評価対象のターゲット
 * @param[in] cameraForward カメラの正面方向ベクトル
 * @return 算出された評価スコア（高いほど優先）
 */
float LockOnSystem::CalcScore(Robot* target, const DirectX::SimpleMath::Vector3& cameraForward) const
{
	DirectX::SimpleMath::Vector3 toTarget = target->GetPosition() - m_playerRobot->GetPosition();
	float dist = toTarget.Length();

	toTarget.Normalize();
	
	// カメラの正面ベクトルに近ければ近いほど1.0に近づく（画面中央にいるほど高評価）
	float dot = cameraForward.Dot(toTarget);

	// 距離が近いほど1.0に近づき、ロックの限界距離で0.0になるように割合を正規化する
	float distanceScore = MAX_RATIO - (dist / MAX_LOCK_ON_DISTANCE);
	
	// あらかじめ定めたパラメータの比率（画面正面度70%、近さ30%）を乗算して最終評価とする
	return (dot * WEIGHT_DOT) + (distanceScore * WEIGHT_DISTANCE);
}