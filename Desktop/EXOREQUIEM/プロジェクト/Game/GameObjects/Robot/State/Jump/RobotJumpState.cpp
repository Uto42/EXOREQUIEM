/*****************************************************************//**
 * @file    RobotJumpState.cpp
 * @brief   ロボットのジャンプおよび滞空時の挙動を管理するクラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Jump/RobotJumpState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/RobotCommand.h"

/**
 * @brief ジャンプ状態開始時の初期化
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotJumpState::Enter(Robot* robot)
{
    m_elapsedTime = 0.0f;
    m_jumpHoldTimer = 0.0f;

    // 開始時の接地状況を確認し、空中からの遷移であるかを記録
    m_isAirborne = !robot->IsGrounded();

    // 接地状態からの遷移であれば、垂直方向に初速を適用
    if (robot->IsGrounded())
    {
        DirectX::SimpleMath::Vector3 velocity = robot->GetVelocity();
        velocity.y = JUMP_POWER;
        robot->SetVelocity(velocity);
    }
}

/**
 * @brief ジャンプ状態の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 遷移先の状態
 */
RobotStateTypes RobotJumpState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
    // 死亡している場合は死亡状態へ遷移
    if (robot->GetHealth() <= 0.0f) return RobotStateTypes::Death;

    m_elapsedTime += dt;

    // 空中での移動制御を実行
    HandleAirMovement(robot, cmd);

    // 一定時間経過後に接地を確認した場合は、入力状況に応じた地上状態へ遷移
    if (m_elapsedTime > LANDING_CHECK_DELAY && robot->IsGrounded())
    {
        if (cmd.moveDirection.LengthSquared() > MOVE_INPUT_THRESHOLD)
            return RobotStateTypes::Moving;
        return RobotStateTypes::Idle;
    }

    // 回避コマンドが有効な場合は回避状態へ遷移
    if (cmd.evade && robot->CanEvade() && cmd.moveDirection.LengthSquared() > MOVE_INPUT_THRESHOLD)
    {
        return RobotStateTypes::Evade;
    }

    // ジャンプボタンの入力状況に応じた上昇状態への遷移判定
    if (cmd.jump)
    {
        // 空中での再入力、またはボタンの長押しを検知した場合は上昇状態へ
        if (m_isAirborne)
        {
            return RobotStateTypes::Rise;
        }

        m_jumpHoldTimer += dt;
        if (m_jumpHoldTimer >= JUMP_HOLD_THRESHOLD)
        {
            return RobotStateTypes::Rise;
        }
    }
    else
    {
        // ボタンを離した時点で、以降の入力は空中再入力として扱う
        m_jumpHoldTimer = 0.0f;
        m_isAirborne = true;
    }

    return RobotStateTypes::Jump;
}

/**
 * @brief 空中における移動速度の制御
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] cmd 実行中のコマンド
 */
void RobotJumpState::HandleAirMovement(Robot* robot, const RobotCommand& cmd)
{
    DirectX::SimpleMath::Vector3 moveDir = cmd.moveDirection;

    // 水平方向の速度を更新し、垂直方向の速度は現在の値を維持
    DirectX::SimpleMath::Vector3 currentVelocity = robot->GetVelocity();
    DirectX::SimpleMath::Vector3 newVelocity = moveDir * AIR_MOVE_SPEED;
    newVelocity.y = currentVelocity.y;

    robot->SetVelocity(newVelocity);
}

/**
 * @brief ジャンプ状態終了時の処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotJumpState::Exit(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);
}