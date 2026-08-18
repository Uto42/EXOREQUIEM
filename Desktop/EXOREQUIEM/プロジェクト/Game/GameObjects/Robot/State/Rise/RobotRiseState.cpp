/*****************************************************************//**
 * @file    RobotRiseState.cpp
 * @brief   ロボットの上昇（飛行）状態における移動制御とエネルギー管理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Rise/RobotRiseState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/RobotCommand.h"

/**
 * @brief 上昇状態開始時の初期化
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotRiseState::Enter(Robot* robot)
{
    // スラスターによる自力飛行を行うため、一時的に重力処理を無効化
    robot->SetGravityEnabled(false);
}

/**
 * @brief 上昇状態の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 遷移先の状態
 */
RobotStateTypes RobotRiseState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
    // 死亡している場合は死亡状態へ遷移
    if (robot->GetHealth() <= 0.0f) return RobotStateTypes::Death;

    // エネルギー切れ、またはボタンが離された場合は落下（ジャンプ）状態へ遷移
    if (robot->GetCurrentEnergy() <= 0.0f || !cmd.jump)
    {
        return RobotStateTypes::Jump;
    }

    // 接地を検知した場合は、入力状況に応じた地上状態へ遷移
    if (robot->IsGrounded())
    {
        if (cmd.moveDirection.LengthSquared() > MOVE_INPUT_THRESHOLD)
            return RobotStateTypes::Moving;
        return RobotStateTypes::Idle;
    }

    // 回避入力がある場合は回避状態へ遷移
    if (cmd.evade && robot->CanEvade() && cmd.moveDirection.LengthSquared() > MOVE_INPUT_THRESHOLD)
    {
        return RobotStateTypes::Evade;
    }

    // 上昇移動の実行とエネルギーの消費更新
    UpdateMovement(robot, cmd, dt);

    return RobotStateTypes::Rise;
}

/**
 * @brief 上昇および空中移動の物理計算
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] cmd 実行中のコマンド
 * @param[in] dt 前フレームからの経過時間
 */
void RobotRiseState::UpdateMovement(Robot* robot, const RobotCommand& cmd, float dt)
{
    // 経過時間に応じたエネルギーの消費
    robot->ConsumeEnergy(RISE_ENERGY_COST * dt);

    DirectX::SimpleMath::Vector3 moveDir = cmd.moveDirection;
    DirectX::SimpleMath::Vector3 newVelocity = DirectX::SimpleMath::Vector3::Zero;

    // 水平方向の移動制御
    if (moveDir.LengthSquared() > MOVE_INPUT_THRESHOLD)
    {
        moveDir.Normalize();

        newVelocity.x = moveDir.x * AIR_MOVE_SPEED;
        newVelocity.z = moveDir.z * AIR_MOVE_SPEED;
    }

    // 垂直方向の制御：高度制限に達していない場合のみ上昇速度を適用
    DirectX::SimpleMath::Vector3 currentPos = robot->GetPosition();

    if (currentPos.y < MAX_RISE_HEIGHT)
    {
        newVelocity.y = RISE_SPEED;
    }
    else
    {
        // 高度制限到達時は垂直速度をゼロにしてホバリング状態を維持
        newVelocity.y = HOVERING_VERTICAL_SPEED;
    }

    robot->SetVelocity(newVelocity);
}

/**
 * @brief 上昇状態終了時の処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotRiseState::Exit(Robot* robot)
{
    // 飛行状態の終了に伴い、重力処理を再有効化
    robot->SetGravityEnabled(true);
}