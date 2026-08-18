/*****************************************************************//**
 * @file    RobotEvadeState.h
 * @brief   ロボットの回避状態（ブースト移動）の挙動を管理するクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"

class Robot;
struct RobotCommand;

/**
 * @class RobotEvadeState
 * @brief ロボット共通の回避（ブーストダッシュ）ステート
 */
class RobotEvadeState : public RobotState
{
public:
	// コンストラクタ
    RobotEvadeState(const DirectX::SimpleMath::Vector3& evadeDir);
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt);
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Evade; }

private:
    // --- 静的定数パラメータ ---
    static constexpr float VERTICAL_VELOCITY_DAMPING = 0.5f;       //< 回避開始時の垂直方向速度の減衰係数
    static constexpr float INPUT_DEADZONE_SQUARED = 0.001f;        //< 回避終了時の移動入力継続判定のしきい値
    static constexpr float EVADE_DURATION = 0.3f;                  //< 回避時間
    static constexpr float EVADE_SPEED = 55.0f;                    //< 回避速度
    static constexpr float EVADE_ENERGY_COST = 12.5f;              //< 消費エネルギー
    static constexpr float EVADE_COOLDOWN_ADD_TIME = 0.3f;         //< クールタイムの追加猶予時間

    // --- 状態値・タイマー ---
    float m_evadeTimer{ 0.0f };                                        //< 回避持続タイマー
    DirectX::SimpleMath::Vector3 m_evadeDirection{ 0.0f, 0.0f, 0.0f }; //< 回避進行方向
};