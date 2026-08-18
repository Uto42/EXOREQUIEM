/*****************************************************************//**
 * @file    RobotMovingState.cpp
 * @brief   ロボットの移動状態における入力処理およびエフェクト生成の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Moving/RobotMovingState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Input/RobotCommand.h"
#include "Game/Systems/Effect/EffectSystem.h"

/**
 * @brief 移動状態開始時の初期化
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotMovingState::Enter(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);
}

/**
 * @brief 移動状態の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 遷移先の状態
 */
RobotStateTypes RobotMovingState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
    UNREFERENCED_PARAMETER(dt);

    // 死亡している場合は死亡状態へ遷移
    if (robot->GetHealth() <= 0.0f) return RobotStateTypes::Death;

    // 入力が途絶えた場合は待機状態へ遷移
    if (cmd.moveDirection.LengthSquared() < MOVE_INPUT_THRESHOLD)
    {
        return RobotStateTypes::Idle;
    }

    // 回避入力がある場合は回避状態へ遷移
    if (cmd.evade && robot->CanEvade())
    {
        return RobotStateTypes::Evade;
    }

    // 接地中にジャンプ入力がある場合はジャンプ状態へ遷移
    if (cmd.jump && robot->IsGrounded())
    {
        return RobotStateTypes::Jump;
    }

    // 物理的な移動処理および演出の実行
    HandleLocomotion(robot, cmd);

    return RobotStateTypes::Moving;
}

/**
 * @brief 移動速度の計算および土煙エフェクトの生成
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] cmd 実行中のコマンド
 */
void RobotMovingState::HandleLocomotion(Robot* robot, const RobotCommand& cmd)
{
    // 移動速度の適用（垂直方向の速度成分は維持）
    DirectX::SimpleMath::Vector3 moveDir = cmd.moveDirection;
    DirectX::SimpleMath::Vector3 currentVelocity = robot->GetVelocity();
    DirectX::SimpleMath::Vector3 newVelocity = moveDir * MOVE_SPEED;

    newVelocity.y = currentVelocity.y;

    robot->SetVelocity(newVelocity);

    // 接地状態での移動時に土煙エフェクトを低確率で生成
    if (robot->IsGrounded() && moveDir.LengthSquared() > 0.0f)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // 生成頻度の調整（約50%の確率）
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(gen) < DUST_SPAWN_CHANCE)
        {
            DirectX::SimpleMath::Vector3 footPos = robot->GetPosition();
            footPos.y = DUST_HEIGHT_OFFSET;

            // 機体の向きに合わせてエフェクトの発生位置を後方にオフセット
            DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateFromQuaternion(robot->GetRotation());

			// ランダムな横方向オフセットを生成して土煙の位置を微調整
            std::uniform_real_distribution<float> dist(-DUST_SIDE_RANDOM_RANGE, DUST_SIDE_RANDOM_RANGE);
            DirectX::SimpleMath::Vector3 offset = DirectX::SimpleMath::Vector3(dist(gen), 0.0f, DUST_BACKWARD_OFFSET);

			// 機体の回転を考慮してオフセットを変換し、土煙の発生位置を決定
            footPos += DirectX::SimpleMath::Vector3::Transform(offset, rot);

			// エフェクトシステムに土煙の生成を依頼
            EffectSystem::Instance()->SpawnGroundDust(footPos);
        }
    }
}

/**
 * @brief 移動状態終了時の処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotMovingState::Exit(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);
}