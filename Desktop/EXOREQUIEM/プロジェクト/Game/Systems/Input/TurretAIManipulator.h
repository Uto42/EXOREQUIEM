/*****************************************************************//**
 * @file    TurretAIManipulator.h
 * @brief   固定砲台（タレット）型ロボットのAI制御ロジック
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/IManipulator.h"

class Robot;

class TurretAIManipulator : public IManipulator
{
public:
    // コンストラクタ
    TurretAIManipulator(Robot* pawn, Robot* target);
    // デストラクタ
    ~TurretAIManipulator() override = default;

    // 最新の入力コマンドを取得
    RobotCommand GetCommand(float dt) override;

private:
	/// --- 調整用定数パラメータ ---
    // コンストラクタ初期化用
    static constexpr float INITIAL_FIRE_TIMER = 1.5f;       //< 開幕から最初の射撃までの猶予時間

    // 安全・バグ防止用の閾値
    static constexpr float NORMALIZE_EPSILON = 0.001f;      //< ゼロ除算防止用の微小値

    // タレット性能・難易度調整用定数群
    static constexpr float ATTACK_RANGE = 150.0f;           //< 射撃を開始する射程距離
    static constexpr float FIRE_INTERVAL = 0.5f;            //< 射撃間隔（秒）

    Robot* m_pawn;                                          //< 操作対象の機体
    Robot* m_target;                                        //< 攻撃対象の機体
    float m_fireTimer;                                      //< 射撃間隔のタイマー
};