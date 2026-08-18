/*****************************************************************//**
 * @file    BossAIManipulator.cpp
 * @brief   ボスのAI制御（移動、攻撃パターンのフェーズ管理、壁避け）の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Input/BossAIManipulator.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Stage/StageManager.h"
#include <algorithm>

/**
 * @brief コンストラクタ
 * @param[in] owner 制御対象となるボス
 * @param[in] target 追跡対象となるプレイヤー
 */
BossAIManipulator::BossAIManipulator(Robot* owner, Robot* target)
	: m_owner(owner)
	, m_target(target)
	, m_attackTimer(0.0f)
	, m_circleDirection(DIRECTION_RIGHT)
{
}

/**
 * @brief デストラクタ
 */
BossAIManipulator::~BossAIManipulator()
{
}

/**
 * @brief 毎フレーム呼ばれ、ボスの行動入力を決定する
 * @param[in] dt 前フレームからの経過時間
 * @return RobotCommand 生成された入力コマンド
 */
RobotCommand BossAIManipulator::GetCommand(float dt)
{
	RobotCommand cmd;
	cmd.moveDirection = DirectX::SimpleMath::Vector3::Zero;
	cmd.lookDirection = DirectX::SimpleMath::Vector3::Zero;

	// ジャンプや回避は行わない
	cmd.jump = false;
	cmd.evade = false;
	cmd.fireGun = false;
	cmd.fireShotgun = false;
	cmd.fireHighAltitudeMissile = false;

	if (!m_owner || !m_owner->IsActive() || m_owner->GetHealth() <= 0.0f ||
		!m_target || !m_target->IsActive() || m_target->GetHealth() <= 0.0f)
	{
		return cmd;
	}

	DirectX::SimpleMath::Vector3 dirToTarget, trueDirToTarget;
	float currentDistance;

	// ターゲットへの方向や距離、視線・照準の更新
	UpdateTargetMetrics(cmd, dirToTarget, trueDirToTarget, currentDistance);

	bool isLowHP;
	// HPによるフェーズ移行と行動サイクルの更新
	UpdatePhaseAndCycle(cmd, dt, isLowHP);

	float idealDistance = 0.0f;
	float moveSpeedMultiplier = 1.0f;
	bool isTooClose = false;

	// 各フェーズに応じたアクションと目標距離の設定
	DetermineAttackActions(cmd, currentDistance, isLowHP, idealDistance, moveSpeedMultiplier, isTooClose);

	DirectX::SimpleMath::Vector3 finalDir, tangent;
	// 目標距離に応じた接近・離反力と周回ベクトルの算出
	DetermineMovement(finalDir, tangent, dirToTarget, currentDistance, idealDistance, isTooClose);

	// 障害物回避（壁避け）ロジックを適用して最終的な移動コマンドを決定
	AvoidObstacles(cmd, finalDir, tangent, dirToTarget, moveSpeedMultiplier, isTooClose);

	return cmd;
}

/**
 * @brief ターゲットへの各種メトリクス（方向・距離・照準）を算出し、コマンドに設定する
 * @param[in,out] cmd コマンド構造体への参照
 * @param[out] outDirToTarget ターゲットへの水平方向ベクトル
 * @param[out] outTrueDirToTarget ターゲットへの実際の方向ベクトル（高低差含む）
 * @param[out] outCurrentDistance ターゲットとの水平距離
 */
void BossAIManipulator::UpdateTargetMetrics(RobotCommand& cmd, DirectX::SimpleMath::Vector3& outDirToTarget,
	DirectX::SimpleMath::Vector3& outTrueDirToTarget, float& outCurrentDistance) const
{
	// ターゲットと自身の位置を取得
	DirectX::SimpleMath::Vector3 ownerPos = m_owner->GetPosition();
	DirectX::SimpleMath::Vector3 targetPos = m_target->GetPosition();

	// ターゲットへの方向ベクトルを算出（高低差を含む）
	outTrueDirToTarget = targetPos - ownerPos;

	// 視線方向の算出（水平方向のみ）
	outDirToTarget = outTrueDirToTarget;
	outDirToTarget.y = 0.0f;
	if (outDirToTarget.LengthSquared() > EPSILON_SQ)
	{
		cmd.lookDirection = outDirToTarget;
		cmd.lookDirection.Normalize();
	}

	outCurrentDistance = outDirToTarget.Length();

	// 射撃方向の算出（上下の高低差を反映）
	if (outTrueDirToTarget.LengthSquared() > EPSILON_SQ)
	{
		DirectX::SimpleMath::Vector3 aimDir = outTrueDirToTarget;
		aimDir.Normalize();

		cmd.aimDirection = aimDir;
		cmd.aimOriginPosition = ownerPos;
	}
}

/**
 * @brief HP割合によるフェーズ判定と行動サイクルのタイマー更新を行う
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] dt 前フレームからの経過時間
 * @param[out] outIsLowHP HPが閾値を下回っているかどうかのフラグ
 */
void BossAIManipulator::UpdatePhaseAndCycle(RobotCommand& cmd, float dt, bool& outIsLowHP)
{
	// HP割合によるフェーズ判定
	outIsLowHP = (m_owner->GetHealth() < m_owner->GetMaxHealth() * HP_THRESHOLD_RATIO);

	if (outIsLowHP && !m_isPhase2)
	{
		cmd.switchWeapon = true; // ボスの武器をマシンガンセットに持ち替える
		m_isPhase2 = true;
	}

	// 行動サイクルの更新
	m_attackTimer += dt;
	float totalCycleTime = PHASE_DURATION_MAIN_WEAPON + PHASE_DURATION_MISSILE + PHASE_DURATION_COOLDOWN;

	if (m_attackTimer >= totalCycleTime)
	{
		m_attackTimer = 0.0f;
		m_circleDirection *= REVERSE_DIRECTION; // 1サイクルごとに周回方向を反転
	}
}

/**
 * @brief 現在のフェーズに応じた攻撃アクションの決定と目標距離の設定を行う
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] currentDistance ターゲットとの現在の水平距離
 * @param[in] isLowHP HPが閾値を下回っているかどうかのフラグ
 * @param[out] outIdealDistance 現在のフェーズにおける理想的な距離
 * @param[out] outMoveSpeedMultiplier 移動速度の倍率
 * @param[out] outIsTooClose ターゲットに近すぎるかどうかの判定フラグ
 */
void BossAIManipulator::DetermineAttackActions(RobotCommand& cmd, float currentDistance,
	bool isLowHP, float& outIdealDistance, float& outMoveSpeedMultiplier, bool& outIsTooClose) const
{
	outIdealDistance = 0.0f;
	outMoveSpeedMultiplier = 1.0f;
	outIsTooClose = false;

	// 各フェーズに応じたアクションと目標距離の設定
	if (m_attackTimer < PHASE_DURATION_MAIN_WEAPON)
	{
		if (!isLowHP)
		{
			// 前半戦：ショットガン
			outIdealDistance = DISTANCE_IDEAL_SHOTGUN;
			cmd.fireShotgun = true;

			if (currentDistance < DISTANCE_TOO_CLOSE_SHOTGUN)
			{
				outIsTooClose = true;
			}
		}
		else
		{
			// 後半戦：マシンガン
			outIdealDistance = DISTANCE_IDEAL_MACHINEGUN;
			cmd.fireGun = true;

			if (currentDistance < DISTANCE_TOO_CLOSE_MACHINEGUN)
			{
				outIsTooClose = true;
			}
		}
	}
	else if (m_attackTimer < PHASE_DURATION_MAIN_WEAPON + PHASE_DURATION_MISSILE)
	{
		// ミサイルフェーズ：足を止めて射撃
		cmd.fireHighAltitudeMissile = true;
		outMoveSpeedMultiplier = 0.0f;
	}
	else
	{
		// クールダウンフェーズ
		if (!isLowHP)
		{
			outIdealDistance = DISTANCE_IDEAL_COOLDOWN;
		}
		else
		{
			// 後半戦はクールダウン中もマシンガンを連射
			outIdealDistance = DISTANCE_IDEAL_MACHINEGUN;
			cmd.fireGun = true;

			if (currentDistance < DISTANCE_TOO_CLOSE_MACHINEGUN)
			{
				outIsTooClose = true;
			}
		}
	}
}

/**
 * @brief ターゲットとの距離に基づき、基本となる移動方向（接近・離反・周回）を算出する
 * @param[out] outFinalDir 算出された最終的な基本移動方向
 * @param[out] outTangent 周回用の接線ベクトル
 * @param[in] dirToTarget ターゲットへの水平方向ベクトル
 * @param[in] currentDistance ターゲットとの現在の距離
 * @param[in] idealDistance 現在のフェーズにおける理想的な距離
 * @param[in] isTooClose ターゲットに近すぎるかどうかのフラグ
 */
void BossAIManipulator::DetermineMovement(DirectX::SimpleMath::Vector3& outFinalDir, 
	DirectX::SimpleMath::Vector3& outTangent, const DirectX::SimpleMath::Vector3& dirToTarget,
	float currentDistance, float idealDistance, bool isTooClose) const
{
	DirectX::SimpleMath::Vector3 toTarget = dirToTarget;
	if (toTarget.LengthSquared() > EPSILON_SQ)
	{
		toTarget.Normalize();
	}

	// 周回用の接線ベクトルを算出
	outTangent = DirectX::SimpleMath::Vector3(-toTarget.z, 0.0f, toTarget.x) * m_circleDirection;

	// 目標距離に応じた接近・離反力の算出
	float distanceError = currentDistance - idealDistance;
	float approachForce = distanceError * APPROACH_FORCE_MULTIPLIER;

	if (isTooClose)
	{
		// 接近しすぎている場合は緊急後退
		approachForce = ESCAPE_FORCE_VALUE;
		outFinalDir = toTarget * approachForce;
	}
	else
	{
		approachForce = std::max(MIN_APPROACH_FORCE, std::min(MAX_APPROACH_FORCE, approachForce));
		outFinalDir = (outTangent * TANGENT_BLEND_NORMAL) + (toTarget * approachForce);
	}
}

/**
 * @brief 進行方向の障害物を検知し、必要に応じて壁を避ける方向に移動ベクトルを補正してコマンドに適用する
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] finalDir 基本となる移動方向ベクトル
 * @param[in] tangent 現在の周回用接線ベクトル
 * @param[in] dirToTarget ターゲットへの水平方向ベクトル
 * @param[in] moveSpeedMultiplier 現在の移動速度倍率
 * @param[in] isTooClose ターゲットに近すぎるかどうかのフラグ
 */
void BossAIManipulator::AvoidObstacles(RobotCommand& cmd, DirectX::SimpleMath::Vector3 finalDir, 
	DirectX::SimpleMath::Vector3 tangent, const DirectX::SimpleMath::Vector3& dirToTarget,
	float moveSpeedMultiplier, bool isTooClose)
{
	// 障害物回避（壁避け）ロジック
	if (finalDir.LengthSquared() > EPSILON_SQ && moveSpeedMultiplier > 0.0f)
	{
		finalDir.Normalize();
		finalDir *= moveSpeedMultiplier;

		StageManager* stageManager = m_owner->GetStageManager();
		if (stageManager)
		{
			// 腰の高さから、進みたい方向へ短いレイ（触覚）を飛ばす
			DirectX::SimpleMath::Vector3 ownerPos = m_owner->GetPosition();
			DirectX::SimpleMath::Vector3 sensorPos = 
				ownerPos + DirectX::SimpleMath::Vector3(0.0f, SENSOR_HEIGHT_OFFSET, 0.0f);

			DirectX::SimpleMath::Vector3 toTarget = dirToTarget;
			if (toTarget.LengthSquared() > EPSILON_SQ)
			{
				toTarget.Normalize();
			}

			if (stageManager->RayCast(sensorPos, finalDir, SENSOR_RAY_LENGTH))
			{
				// 壁に衝突する場合は周回方向を反転
				m_circleDirection *= REVERSE_DIRECTION;
				tangent = DirectX::SimpleMath::Vector3(-toTarget.z, 0.0f, toTarget.x) * m_circleDirection;

				if (!isTooClose)
				{
					finalDir = (tangent * TANGENT_BLEND_EVADE);
					finalDir.Normalize();
				}

				if (stageManager->RayCast(sensorPos, finalDir, SENSOR_RAY_LENGTH))
				{
					finalDir = toTarget;
				}
			}
		}

		cmd.moveDirection = finalDir;
	}
}