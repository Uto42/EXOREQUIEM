/*****************************************************************//**
 * @file    DummyAIManipulator.h
 * @brief   チュートリアル用の何もしないダミーAI制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/IManipulator.h"
#include "Game/Systems/Input/RobotCommand.h"

 /**
  * @class DummyAIManipulator
  * @brief チュートリアル等の「動かない・撃たない敵」のための操作クラス
  */
class DummyAIManipulator : public IManipulator
{
public:
    // コンストラクタ
    DummyAIManipulator() = default;
    // デストラクタ
    virtual ~DummyAIManipulator() = default;

    // 最新の入力コマンドを取得（常に空を返す）
    virtual RobotCommand GetCommand(float dt) override;
};