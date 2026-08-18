/*****************************************************************//**
 * @file    RobotIdleState.cpp
 * @brief   ロボット共通の待機状態時の入力監視およびステート遷移
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Idle/RobotIdleState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/RobotCommand.h"

/**
 * @brief 待機状態開始時の初期化
 * @details 水平方向の速度をリセットし、垂直方向の速度のみを維持した停止状態へ移行する
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotIdleState::Enter(Robot* robot)
{
    DirectX::SimpleMath::Vector3 currentVelocity = robot->GetVelocity();
    currentVelocity.x = 0.0f;
    currentVelocity.z = 0.0f;
    robot->SetVelocity(currentVelocity);
}

/**
 * @brief 待機状態の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 遷移先の状態
 */
RobotStateTypes RobotIdleState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
    UNREFERENCED_PARAMETER(dt);

    // 移動入力の有無を確認し、入力があれば移動状態へ遷移
    if (cmd.moveDirection.LengthSquared() > MOVE_INPUT_THRESHOLD)
    {
        return RobotStateTypes::Moving;
    }

    // 接地中にジャンプ入力があった場合はジャンプ状態へ遷移
    if (cmd.jump && robot->IsGrounded())
    {
        return RobotStateTypes::Jump;
    }

    return RobotStateTypes::Idle;
}

/**
 * @brief 待機状態終了時の処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotIdleState::Exit(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);
}