/*****************************************************************//**
 * @file    AIManipulator.cpp
 * @brief   ロボットの汎用AI制御（間合い管理、アクション判断、視線チェック）の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Input/AIManipulator.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Stage/StageManager.h"

/**
 * @brief コンストラクタ
 * @param[in] pawn 制御対象となるロボット
 * @param[in] target 追跡・攻撃対象となるロボット
 * @param[in] stageManager ステージの情報を管理するマネージャー
 */
AIManipulator::AIManipulator(Robot* pawn, const Robot* target, StageManager* stageManager)
	: m_pawn(pawn)
	, m_target(target)
	, m_stageManager(stageManager)
	, m_brainState(AIBrainState::Combat)
	, m_decisionTimer_Jump(INITIAL_JUMP_TIMER)
	, m_decisionTimer_Evade(INITIAL_EVADE_TIMER)
	, m_decisionTimer_Rise(INITIAL_RISE_TIMER)
	, m_strafeTimer(0.0f)
	, m_randGen(std::random_device{}())
{
}

/**
 * @brief AIによる行動コマンドの生成
 * @param[in] dt 前フレームからの経過時間
 * @return RobotCommand 各種入力フラグおよびベクトルを含むコマンド
 */
RobotCommand AIManipulator::GetCommand(float dt)
{
	RobotCommand cmd;
	cmd.moveDirection = DirectX::SimpleMath::Vector3::Zero;
	cmd.jump = false;
	cmd.evade = false;
	cmd.fireGun = false;
	cmd.fireMissile = false;

	// 自身またはターゲットが無効、あるいは死亡している場合は空のコマンドを返す
	if (!m_pawn || !m_target || m_pawn->GetHealth() <= 0.0f || m_target->GetHealth() <= 0.0f)
	{
		return cmd;
	}

	DirectX::SimpleMath::Vector3 dirToTarget, aimDir;
	float dist, heightDiff;

	// ターゲットに関する各種メトリクス（方向・距離・照準など）の計算
	CalculateTargetMetrics(dirToTarget, dist, heightDiff, aimDir);

	cmd.lookDirection = dirToTarget;
	cmd.aimDirection = aimDir;

	// 戦闘ロジックの更新
	UpdateCombatLogic(cmd, dirToTarget, dist, heightDiff, dt);

	return cmd;
}

/**
 * @brief 戦闘時における移動・射撃・アクション判断の更新
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] dirToTarget ターゲットへの水平方向
 * @param[in] dist ターゲットとの距離
 * @param[in] heightDiff ターゲットとの高度差
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::UpdateCombatLogic(RobotCommand& cmd, const DirectX::SimpleMath::Vector3& dirToTarget,
	float dist, float heightDiff, float dt)
{
	// 視線チェックに基づいた射撃および移動制御
	bool canSeePlayer = CheckLineOfSight(dirToTarget, dist);

	// 戦闘時の基本的な移動方針の決定と射撃制御
	DetermineCombatMovement(cmd, canSeePlayer, dirToTarget, dist, dt);

	// スタック回避（障害物センサー）ロジック
	PerformObstacleAvoidance(cmd.moveDirection);

	// アクション（回避・ジャンプ・上昇）の総合判断
	UpdateActionDecisions(cmd, heightDiff, dt);
}

/**
 * @brief ターゲットへの視線が遮られていないかの確認
 * @param[in] direction 確認する方向
 * @param[in] distance 確認する距離
 * @return 視線が通っている場合はtrue
 */
bool AIManipulator::CheckLineOfSight(const DirectX::SimpleMath::Vector3& direction, float distance) const
{
	if (!m_stageManager) return true;

	DirectX::SimpleMath::Vector3 eyePos = 
		m_pawn->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, EYE_HEIGHT, 0.0f);

	return !m_stageManager->RayCast(eyePos, direction, distance);
}

/**
 * @brief ターゲットに関する各種メトリクス（方向・距離・照準など）の計算
 * @param[out] outDirToTarget ターゲットへの水平方向ベクトル（正規化済み）
 * @param[out] outDist ターゲットとの水平距離
 * @param[out] outHeightDiff ターゲットとの高度差
 * @param[out] outAimDir 正規化された最終的な照準方向ベクトル
 */
void AIManipulator::CalculateTargetMetrics(DirectX::SimpleMath::Vector3& outDirToTarget, float& outDist,
	float& outHeightDiff, DirectX::SimpleMath::Vector3& outAimDir) const
{
	// ターゲットとの距離および高度差の算出
	DirectX::SimpleMath::Vector3 toTarget = m_target->GetPosition() - m_pawn->GetPosition();
	outHeightDiff = toTarget.y;

	toTarget.y = 0.0f;
	float distSq = toTarget.LengthSquared();
	outDirToTarget = toTarget;
	if (distSq > NORMALIZE_EPSILON) outDirToTarget.Normalize();
	outDist = sqrt(distSq);

	// 射撃方向の算出（機体中心付近を狙う）
	DirectX::SimpleMath::Vector3 myMuzzlePos = 
		m_pawn->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, PAWN_MUZZLE_Y, 0.0f);

	DirectX::SimpleMath::Vector3 targetBodyPos =
		m_target->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, TARGET_BODY_Y, 0.0f);

	DirectX::SimpleMath::Vector3 aimDir = targetBodyPos - myMuzzlePos;

	aimDir.Normalize();
	outAimDir = aimDir;
}

/**
 * @brief 戦闘時における基本的な移動方針の決定と射撃制御
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] canSeePlayer ターゲットが見えているかどうかのフラグ
 * @param[in] dirToTarget ターゲットへの水平方向
 * @param[in] dist ターゲットとの距離
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::DetermineCombatMovement(RobotCommand& cmd, bool canSeePlayer,
	const DirectX::SimpleMath::Vector3& dirToTarget, float dist, float dt)
{
	// 基本となる移動方向の決定
	if (canSeePlayer)
	{
		m_fireIntervalTimer -= dt;

		if (m_fireIntervalTimer <= 0.0f)
		{
			m_fireIntervalTimer = FIRE_CYCLE_TOTAL;
		}

		if (m_fireIntervalTimer > FIRE_REST_TIME)
		{
			cmd.fireGun = true;
			cmd.fireMissile = true;
		}
		else
		{
			cmd.fireGun = false;
			cmd.fireMissile = false;
		}

		if (dist < OPTIMAL_DISTANCE - DISTANCE_DEAD_ZONE)
		{
			cmd.moveDirection = -dirToTarget; // 近すぎるので後退
		}
		else if (dist > OPTIMAL_DISTANCE + DISTANCE_DEAD_ZONE)
		{
			cmd.moveDirection = dirToTarget; // 遠いので接近
		}
		else
		{
			// 適正距離なのでカニ歩き
			m_strafeTimer += dt;
			float strafeDir = (fmod(m_strafeTimer, STRAFE_CYCLE) > STRAFE_SWITCH_TIME) ? 1.0f : -1.0f;
			DirectX::SimpleMath::Vector3 right = 
				DirectX::SimpleMath::Vector3(-dirToTarget.z, 0.0f, dirToTarget.x) * strafeDir;

			cmd.moveDirection = right;
		}
	}
	else
	{
		cmd.fireGun = false;
		cmd.fireMissile = false;

		// 見えない時は単なる直進ではなく、斜めに進んで回り込おうとする
		m_strafeTimer += dt;
		float strafeDir = (fmod(m_strafeTimer, STRAFE_CYCLE) > STRAFE_SWITCH_TIME) ? 1.0f : -1.0f;
		DirectX::SimpleMath::Vector3 right = 
			DirectX::SimpleMath::Vector3(-dirToTarget.z, 0.0f, dirToTarget.x) * strafeDir;

		cmd.moveDirection = dirToTarget + (right * STRAFE_FORWARD_BLEND_RATIO); // 直進＋横移動をブレンド
		cmd.moveDirection.Normalize();
	}
}

/**
 * @brief 進行方向の障害物を検知して迂回ルートを探す
 * @param[in,out] ioMoveDirection 入出力となる移動方向ベクトル
 */
void AIManipulator::PerformObstacleAvoidance(DirectX::SimpleMath::Vector3& ioMoveDirection) const
{
	// スタック回避（障害物センサー）ロジック
	if (ioMoveDirection.LengthSquared() > 0.0f && m_stageManager)
	{
		ioMoveDirection.Normalize();

		// 機体の腰の高さから、進みたい方向へ短いレイ（触覚）を飛ばす
		DirectX::SimpleMath::Vector3 sensorPos = 
			m_pawn->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, SENSOR_HEIGHT_OFFSET, 0.0f);

		// 進みたい方向に壁があるか？
		if (m_stageManager->RayCast(sensorPos, ioMoveDirection, SENSOR_RAY_LENGTH))
		{
			// 壁があった場合は進む方向を左右に90度ずらして空いている道を探す
			DirectX::SimpleMath::Vector3 rightDir =
				DirectX::SimpleMath::Vector3(-ioMoveDirection.z, 0.0f, ioMoveDirection.x);
			DirectX::SimpleMath::Vector3 leftDir = -rightDir;

			bool isRightBlocked = m_stageManager->RayCast(sensorPos, rightDir, SENSOR_RAY_LENGTH);
			bool isLeftBlocked = m_stageManager->RayCast(sensorPos, leftDir, SENSOR_RAY_LENGTH);

			if (!isRightBlocked)
			{
				ioMoveDirection = rightDir; // 右は空いているので右に逃げる
			}
			else if (!isLeftBlocked)
			{
				ioMoveDirection = leftDir;  // 左が空いているので左に逃げる
			}
			else
			{
				ioMoveDirection = DirectX::SimpleMath::Vector3::Zero; // 両方ダメなら一旦止まる（壁にめり込まないため）
			}
		}
	}
}

/**
 * @brief アクション（回避・ジャンプ・上昇）の総合更新
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] heightDiff ターゲットとの高度差
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::UpdateActionDecisions(RobotCommand& cmd, float heightDiff, float dt)
{
	// 回避の判断
	DecideEvade(cmd, dt);
	// ジャンプの判断
	DecideJump(cmd, heightDiff, dt);
	// 上昇の判断
	DecideRise(cmd, heightDiff, dt);
}

/**
 * @brief 回避アクションのタイマー管理と実行判定
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::DecideEvade(RobotCommand& cmd, float dt)
{
	m_decisionTimer_Evade -= dt;
	if (m_decisionTimer_Evade <= 0.0f)
	{
		m_decisionTimer_Evade = EVADE_TIMER_BASE +
			std::uniform_real_distribution<float>(0.0f, EVADE_TIMER_RAND)(m_randGen);
		if (m_pawn->CanEvade()) cmd.evade = true;
	}
}

/**
 * @brief ジャンプアクションのタイマー管理と実行判定
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] heightDiff ターゲットとの高度差
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::DecideJump(RobotCommand& cmd, float heightDiff, float dt)
{
	m_decisionTimer_Jump -= dt;
	if (m_decisionTimer_Jump <= 0.0f)
	{
		// ジャンプのタイマーをリセット（ランダム要素を加える）
		m_decisionTimer_Jump = JUMP_TIMER_BASE +
			std::uniform_real_distribution<float>(0.0f, JUMP_TIMER_RAND)(m_randGen);
		if (m_pawn->IsGrounded())
		{
			// 高度差に応じてジャンプ確率を決定
			float jumpProb = (heightDiff > JUMP_HEIGHT_THRESH) ? JUMP_PROB_HIGH : JUMP_PROB_NORMAL;
			if (std::uniform_real_distribution<float>(0.0f, 1.0f)(m_randGen) < jumpProb)
			{
				cmd.jump = true;
			}
		}
	}
}

/**
 * @brief 上昇アクションのタイマー管理と実行判定
 * @param[in,out] cmd コマンド構造体への参照
 * @param[in] heightDiff ターゲットとの高度差
 * @param[in] dt 前フレームからの経過時間
 */
void AIManipulator::DecideRise(RobotCommand& cmd, float heightDiff, float dt)
{
	m_decisionTimer_Rise -= dt;
	if (m_decisionTimer_Rise <= 0.0f)
	{
		// 上昇のタイマーをリセット（ランダム要素を加える）
		m_decisionTimer_Rise = RISE_TIMER_BASE +
			std::uniform_real_distribution<float>(0.0f, RISE_TIMER_RAND)(m_randGen);
		if (m_pawn->IsGrounded() && heightDiff > RISE_HEIGHT_THRESH)
		{
			cmd.rise = true;
		}
	}
}