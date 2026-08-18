/*****************************************************************//**
 * @file    RobotRiseState.h
 * @brief   ロボットの上昇（飛行）状態における移動制御とエネルギー管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"

class Robot;
struct RobotCommand;

/**
 * @class RobotRiseState
 * @brief ロボット共通の上昇（ホバリング・飛行）状態
 * ジャンプボタン長押しによる推力上昇とエネルギー消費を管理する
 */
class RobotRiseState : public RobotState
{
public:
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) override;
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Rise; }

private:
    // 上昇・空中移動の更新
    void UpdateMovement(Robot* robot, const struct RobotCommand& cmd, float dt);

private:
    // --- 静的定数パラメータ ---
    static constexpr float HOVERING_VERTICAL_SPEED = 0.0f;  //< 高度制限到達時の垂直速度（ホバリング維持）
    static constexpr float RISE_SPEED = 15.0f;              //< 上昇速度
    static constexpr float AIR_MOVE_SPEED = 20.0f;          //< 空中での水平移動速度
    static constexpr float RISE_ENERGY_COST = 15.0f;        //< 1秒あたりの消費エネルギー
    static constexpr float MAX_RISE_HEIGHT = 48.0f;         //< 最大上昇高度
    static constexpr float MOVE_INPUT_THRESHOLD = 0.001f;   //< 移動入力を検知するベクトルの長さのしきい値

};