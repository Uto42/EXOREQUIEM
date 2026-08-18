/*****************************************************************//**
 * @file    KeyboardManipulator.cpp
 * @brief   キーボード・マウス入力をRobotCommandに変換するコントローラー
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Input/KeyboardManipulator.h"

/**
 * @brief コンストラクタ
 */
KeyboardManipulator::KeyboardManipulator()
    : m_cameraForward(DirectX::SimpleMath::Vector3::Forward)
    , m_cameraRight(DirectX::SimpleMath::Vector3::Right)
{
}

/**
 * @brief 毎フレーム入力を読み取りRobotCommandを返す
 * @param dt 経過時間（未使用）
 * @return RobotCommand構造体に変換されたプレイヤーの入力状態
 */
RobotCommand KeyboardManipulator::GetCommand(float dt)
{
    UNREFERENCED_PARAMETER(dt);

    RobotCommand cmd;

    auto kb = DirectX::Keyboard::Get().GetState();
    auto mouse = DirectX::Mouse::Get().GetState();
    m_tracker.Update(mouse);
    m_kbTracker.Update(kb);

    // --- 移動方向（カメラ基準） ---
    DirectX::SimpleMath::Vector3 moveDir = DirectX::SimpleMath::Vector3::Zero;
    if (kb.W) moveDir += m_cameraForward;
    if (kb.S) moveDir -= m_cameraForward;
    if (kb.A) moveDir -= m_cameraRight;
    if (kb.D) moveDir += m_cameraRight;

    moveDir.y = 0.0f;

    if (moveDir.LengthSquared() > NORMALIZE_EPSILON) moveDir.Normalize();
    cmd.moveDirection = moveDir;

    // --- アクション ---
    cmd.jump = kb.Space;
    cmd.evade = m_kbTracker.pressed.LeftShift;

    // --- 射撃 ---
    cmd.fireGun = mouse.leftButton;
    cmd.fireMissile = mouse.rightButton;
    cmd.reload = m_kbTracker.pressed.R;

    // --- クールタイムタイマーの更新 ---
    if (m_weaponSwitchTimer > 0.0f)
    {
        m_weaponSwitchTimer -= dt;
    }

    // --- 武器セットの切り替え ---
    cmd.switchWeapon = false;

    int currentWheelValue = mouse.scrollWheelValue;
    int wheelDelta = currentWheelValue - m_lastScrollWheelValue;
    m_lastScrollWheelValue = currentWheelValue; // 常に最新の値を保存しておく

    // ホイールが動いていて、かつクールタイムが明けている場合のみ切り替える
    if (wheelDelta != 0 && m_weaponSwitchTimer <= 0.0f)
    {
        cmd.switchWeapon = true;

        // クールタイム中はホイール入力を無視する
        m_weaponSwitchTimer = WEAPON_SWITCH_COOLDOWN;
    }

    // --- 照準方向 ---
    cmd.aimDirection = m_cameraForward;
    cmd.aimOriginPosition = m_cameraPosition;

    return cmd;
}

/**
 * @brief カメラの前方・右方向をセット
 * @param forward カメラの前方ベクトル
 * @param right カメラの右方向ベクトル
 */
void KeyboardManipulator::SetCameraOrientation(const DirectX::SimpleMath::Vector3& forward, 
    const DirectX::SimpleMath::Vector3& right)
{
	m_cameraForward = forward;
	m_cameraRight = right;
}
