/*****************************************************************//**
 * @file    RobotIdleState.h
 * @brief   ロボット共通の待機状態時の入力監視およびステート遷移
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"

class Robot;
struct RobotCommand;

/**
 * @class RobotIdleState
 * @brief 待機（Idle）ステート
 */
class RobotIdleState : public RobotState
{
public:
    // コンストラクタ
    RobotIdleState() = default;
    // デストラクタ
    ~RobotIdleState()override = default;
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) override;
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Idle; }

private:
    // --- 静的定数パラメータ ---
    static constexpr float MOVE_INPUT_THRESHOLD = 0.001f; //< 移動入力を検知するベクトルの長さのしきい値

};