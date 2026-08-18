/*****************************************************************//**
 * @file    RobotMovingState.h
 * @brief   ロボットの移動状態における入力処理およびエフェクト生成
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"
#include <random>

class Robot;
struct RobotCommand;

/**
 * @class RobotMovingState
 * @brief ロボット共通の移動状態・入力された方向に移動と回転を行う
 */
class RobotMovingState : public RobotState
{
public:
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) override;
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Moving; }

private:
    // 物理的な移動・回転の計算
    void HandleLocomotion(Robot* robot, const struct RobotCommand& cmd);

private:
    // --- 静的定数パラメータ ---
    static constexpr float MOVE_SPEED = 20.0f;                //< 基本移動速度
    static constexpr float MOVE_INPUT_THRESHOLD = 0.001f;     //< 移動入力を検知するベクトルの長さのしきい値
    static constexpr float DUST_SPAWN_CHANCE = 0.5f;          //< 接地移動時の土煙エフェクト発生確率 (50%)
    static constexpr float DUST_HEIGHT_OFFSET = 0.1f;         //< 土煙を発生させる足元のY座標オフセット
    static constexpr float DUST_BACKWARD_OFFSET = -0.5f;      //< 機体後方へのエフェクト発生オフセット距離
    static constexpr float DUST_SIDE_RANDOM_RANGE = 0.5f;     //< エフェクト発生位置の左右ランダム幅(±)

};