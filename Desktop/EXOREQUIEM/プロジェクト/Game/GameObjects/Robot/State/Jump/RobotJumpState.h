/*****************************************************************//**
 * @file    RobotJumpState.h
 * @brief   ロボットのジャンプおよび滞空時の挙動を管理するクラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"

class Robot;
struct RobotCommand;

/**
 * @class RobotJumpState
 * @brief ロボット共通のジャンプ状態・空中移動と、長押しによる上昇(Rise)への遷移を管理する
 */
class RobotJumpState : public RobotState
{
public:
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) override;
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Jump; }

private:
    // 空中移動の処理
    void HandleAirMovement(Robot* robot, const struct RobotCommand& cmd);

private:
    // --- 静的定数パラメータ ---
    static constexpr float JUMP_POWER = 20.0f;             //< ジャンプの初速(上方向)
    static constexpr float AIR_MOVE_SPEED = 20.0f;         //< 空中での移動速度
    static constexpr float JUMP_HOLD_THRESHOLD = 0.2f;     //< 上昇(Rise)へ移行するまでの長押し時間
    static constexpr float LANDING_CHECK_DELAY = 0.1f;     //< ジャンプ開始直後の着地判定を無視する猶予時間
    static constexpr float MOVE_INPUT_THRESHOLD = 0.001f;  //< 移動入力を検知するベクトルの長さのしきい値

    // --- 状態値・タイマー ---
    float m_elapsedTime = 0.0f;                      //< ジャンプ開始からの経過時間
    float m_jumpHoldTimer = 0.0f;                    //< ジャンプ入力の長押し時間
    bool m_isAirborne = false;                       //< 空中（落下中）かどうかのフラグ

};