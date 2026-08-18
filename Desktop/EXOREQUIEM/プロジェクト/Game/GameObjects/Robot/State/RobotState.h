/*****************************************************************//**
 * @file    RobotState.h
 * @brief   ロボットの各ステートを定義するインターフェースクラス（ステートパターン）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotStateTypes.h"

// 前方宣言
class Robot;
struct RobotCommand;

/**
 * @class RobotState
 * @brief ロボットの各ステートの共通インターフェース
 * @details メンバ変数を持たず、状態遷移と実行ルールのみを規定する純粋仮想クラス
 */
class RobotState
{
public:
	// デストラクタ
    virtual ~RobotState() = default;
    // ステート開始時の処理
    virtual void Enter(Robot* robot) = 0;
    // 毎フレームの更新処理（次のステートを返す）
    virtual RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) = 0;
    // ステート終了時の処理
    virtual void Exit(Robot* robot) = 0;

    // このステートの種類を返す
    virtual RobotStateTypes GetType() const = 0;
};