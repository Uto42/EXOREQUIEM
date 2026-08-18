/*****************************************************************//**
 * @file    KeyboardManipulator.h
 * @brief   キーボード・マウス入力をRobotCommandに変換するコントローラー
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/IManipulator.h"

 /**
  * @class KeyboardManipulator
  * @brief プレイヤーのキー入力・マウス操作を読み取り、ロボットへの命令セットを生成するクラス
  */
class KeyboardManipulator : public IManipulator
{
public:
	// コンストラクタs
    KeyboardManipulator();

    // 毎フレーム入力を読み取りRobotCommandを返す
    RobotCommand GetCommand(float dt) override;

    void SetCameraOrientation(const DirectX::SimpleMath::Vector3& forward, 
        const DirectX::SimpleMath::Vector3& right) override;

	// カメラ位置のセット
    void SetCameraPosition(const DirectX::SimpleMath::Vector3& position) { m_cameraPosition = position; }

    // カメラ方向のセット
    void SetCameraForward(const DirectX::SimpleMath::Vector3& forward) { m_cameraForward = forward; }

    // カメラ右方向のセット
    void SetCameraRight(const DirectX::SimpleMath::Vector3& right) { m_cameraRight = right; }

    // Trackerの取得（武器コントローラーが使う）
    const DirectX::Mouse::ButtonStateTracker& GetTracker() const { return m_tracker; }

private:
	/// --- 調整用定数パラメータ ---
	static constexpr float NORMALIZE_EPSILON = 0.001f;      //< ベクトルの正規化でゼロ除算を防止するための値
    static constexpr float WEAPON_SWITCH_COOLDOWN = 0.3f;   //< 武器切り替えのクールタイム（秒）

    DirectX::Mouse::ButtonStateTracker m_tracker;           //< マウスボタンの状態トラッカー
    DirectX::Keyboard::KeyboardStateTracker m_kbTracker;    //< マウスの入力トラッカー
    DirectX::SimpleMath::Vector3 m_cameraForward;           //< カメラの前方ベクトル
    DirectX::SimpleMath::Vector3 m_cameraRight;             //< カメラの右方向ベクトル
	DirectX::SimpleMath::Vector3 m_cameraPosition;          //< カメラの位置ベクトル

	int m_lastScrollWheelValue = 0;				            //< 前フレームのスクロールホイールの値
    float m_weaponSwitchTimer = 0.0f;                       //< 切り替えを防ぐためのタイマー
};