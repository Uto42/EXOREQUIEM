/*****************************************************************//**
 * @file    TurretAIManipulator.cpp
 * @brief   固定砲台（タレット）型ロボットのAI制御ロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Input/TurretAIManipulator.h"
#include "Game/GameObjects/Robot/Robot.h"

/**
 * @brief コンストラクタ
 * @param[in] pawn 制御対象となるロボット
 * @param[in] target 攻撃対象となるロボット
 */
TurretAIManipulator::TurretAIManipulator(Robot* pawn, Robot* target)
    : m_pawn(pawn)
    , m_target(target)
    , m_fireTimer(INITIAL_FIRE_TIMER)
{
}

/**
 * @brief AIによる次の行動コマンドの生成
 * @param[in] dt 前フレームからの経過時間
 * @return RobotCommand 生成された制御コマンド
 */
RobotCommand TurretAIManipulator::GetCommand(float dt)
{
    RobotCommand cmd;
    cmd.moveDirection = DirectX::SimpleMath::Vector3::Zero; // 固定砲台のため移動は行わない
    cmd.lookDirection = DirectX::SimpleMath::Vector3::Zero;
    cmd.jump = false;
    cmd.evade = false;
    cmd.fireGun = false;
    cmd.fireMissile = false;

    // 自身またはターゲットが無効、あるいは死亡している場合は何もしない
    if (!m_pawn || !m_target || m_pawn->GetHealth() <= 0.0f || m_target->GetHealth() <= 0.0f)
    {
        return cmd;
    }

	// ターゲットへの方向ベクトルを算出し、視線方向として設定
    DirectX::SimpleMath::Vector3 dirToPlayer = m_target->GetPosition() - m_pawn->GetPosition();
    cmd.lookDirection = dirToPlayer;

    // ターゲットの中心（胸付近）を狙うためのエイム方向を算出
    DirectX::SimpleMath::Vector3 myMuzzlePos = 
        m_pawn->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
    DirectX::SimpleMath::Vector3 targetBodyPos = 
        m_target->GetPosition() + DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);

	// エイム方向の正規化と距離計算
    DirectX::SimpleMath::Vector3 aimDir = targetBodyPos - myMuzzlePos;
    float dist = aimDir.Length();

    if (dist > NORMALIZE_EPSILON) aimDir.Normalize();
    cmd.aimDirection = aimDir;

    // 射撃実行判定：射程距離内にターゲットが存在する場合、一定間隔で射撃
    if (dist < ATTACK_RANGE)
    {
        m_fireTimer -= dt;
        if (m_fireTimer <= 0.0f)
        {
            cmd.fireGun = true;                 // 射撃コマンドを発行
            m_fireTimer = FIRE_INTERVAL;        // 次の射撃までの間隔（秒）を再設定
        }
    }

    return cmd;
}