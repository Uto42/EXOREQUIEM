/*****************************************************************//**
 * @file    RobotDeathState.h
 * @brief   ロボットの死亡状態（爆発演出から消滅まで）の挙動を管理するクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/State/RobotState.h"

class Robot;
struct RobotCommand;

/**
 * @class RobotDeathState
 * @brief ロボット共通の死亡ステート
 * 連続爆発の演出を行い、最終的に大爆発して機能を停止する
 */
class RobotDeathState : public RobotState
{
public:
    // ステート開始処理
    void Enter(Robot* robot) override;
    // ステート更新処理
    RobotStateTypes Update(Robot* robot, const RobotCommand& cmd, float dt) override;
    // ステート終了処理
    void Exit(Robot* robot) override;

    // ステートタイプの取得
    RobotStateTypes GetType() const override { return RobotStateTypes::Death; }

private:
    // 連続する小爆発の更新
    void UpdateSmallExplosion(Robot* robot, float dt);
    // 最終的な大爆発と消滅処理
    void ExecuteFinalDeath(Robot* robot);

    // 爆発を発生させる中心座標の取得
    DirectX::SimpleMath::Vector3 GetExplosionCenter(Robot* robot) const;

private:
    // --- 静的定数パラメータ ---
    static constexpr float EXPLOSION_CENTER_OFFSET_Y = 1.0f;       //< 爆発の中心位置を機体の足元から引き上げる高さオフセット
    static constexpr float SMALL_EXPLOSION_END_TIME = 2.3f;        //< 小爆発演出を終了する時間
    static constexpr float FINAL_EXPLOSION_START_TIME = 2.5f;      //< 最終大爆発を実行する時間
    static constexpr float SMALL_EXPLOSION_INTERVAL = 0.6f;        //< 小爆発の間隔タイマーしきい値
    static constexpr float SMALL_EXPLOSION_OFFSET_RANGE = 0.7f;    //< 小爆発の発生オフセット範囲
    static constexpr float FINAL_EXPLOSION_OFFSET_RANGE = 1.2f;    //< 大爆発の拡散オフセット範囲
    static constexpr int MAX_SMALL_EXPLOSION_COUNT = 3;            //< 小爆発の最大回数
    static constexpr int FINAL_EXPLOSION_EFFECT_COUNT = 15;        //< 最終大爆発のエフェクト生成数

    // --- 状態値・タイマー ---
    float m_timer = 0.0f;                                          //< 死亡からの経過時間
    float m_explosionTimer = 0.0f;                                 //< 小爆発の間隔タイマー
    int m_explosionCount = 0;                                      //< 実行された小爆発の回数
    bool m_finalExplosionTriggered = false;                        //< 大爆発が済んだかどうかのフラグ
};