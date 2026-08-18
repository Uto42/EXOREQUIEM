/*****************************************************************//**
 * @file    RobotEvadeState.cpp
 * @brief   ロボットの回避状態（ブースト移動）の挙動を管理するクラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Evade/RobotEvadeState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/RobotCommand.h"

/**
 * @brief コンストラクタ
 * @param[in] evadeDir 回避の進行方向
 */
RobotEvadeState::RobotEvadeState(const DirectX::SimpleMath::Vector3& evadeDir)
    : m_evadeDirection(evadeDir)
    , m_evadeTimer(0.0f)
{
}

/**
 * @brief 回避状態開始時の初期化
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotEvadeState::Enter(Robot* robot)
{
    // エネルギーが不足している場合は回避を行わずに終了
    if (robot->GetCurrentEnergy() < EVADE_ENERGY_COST)
    {
        m_evadeTimer = 0.0f;
        return;
    }

    // 必要エネルギーの消費とタイマーの設定
    robot->ConsumeEnergy(EVADE_ENERGY_COST);
    m_evadeTimer = EVADE_DURATION;

    // 次の回避が可能になるまでのクールタイムを設定
    robot->StartEvadeCooldown(EVADE_DURATION + EVADE_COOLDOWN_ADD_TIME);

    // 挙動制御：重力を一時的に無効化し、垂直方向の速度を減衰させる
    robot->SetGravityEnabled(false);
    DirectX::SimpleMath::Vector3 currentVel = robot->GetVelocity();

    currentVel.y *= VERTICAL_VELOCITY_DAMPING;
    robot->SetVelocity(currentVel);

}

/**
 * @brief 回避状態の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 遷移先の状態
 */
RobotStateTypes RobotEvadeState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
    // 死亡している場合は即座に死亡状態へ
    if (robot->GetHealth() <= 0.0f) return RobotStateTypes::Death;

    // 開始時にエネルギー不足だった場合は待機状態へ
    if (m_evadeTimer <= 0.0f) return RobotStateTypes::Idle;

    m_evadeTimer -= dt;

    // 回避速度を適用（垂直方向の速度は現在の値を維持）
    DirectX::SimpleMath::Vector3 currentVel = robot->GetVelocity();
    DirectX::SimpleMath::Vector3 evadeVel = m_evadeDirection * EVADE_SPEED;

    evadeVel.y = currentVel.y;
    robot->SetVelocity(evadeVel);

    // 回避時間の終了判定と遷移先決定
    if (m_evadeTimer <= 0.0f)
    {

        // 空中にいるかジャンプ入力がある場合はジャンプ状態へ遷移
        if (!robot->IsGrounded() || cmd.jump)
        {
            return RobotStateTypes::Jump;
        }

        // 地上かつ移動入力が継続しているなら移動状態へ、それ以外は待機状態へ
        if (cmd.moveDirection.LengthSquared() > INPUT_DEADZONE_SQUARED)
        {
            return RobotStateTypes::Moving;
        }
        return RobotStateTypes::Idle;
    }

    return RobotStateTypes::Evade;
}

/**
 * @brief 回避状態終了時の処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotEvadeState::Exit(Robot* robot)
{
    // 一時的に停止していた重力処理を再開
    robot->SetGravityEnabled(true);
}