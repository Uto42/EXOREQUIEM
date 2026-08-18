/*****************************************************************//**
 * @file    Player.cpp
 * @brief   プレイヤー機体の演出、武器制御、およびカメラ連携の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Common/DeviceResources.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include "Game/Stage/StageManager.h"
#include "Game/Systems/Input/IManipulator.h"
#include "Game/Systems/Input/AIManipulator.h"
#include "Game/Systems/Input/KeyboardManipulator.h"

/**
 * @brief コンストラクタ
 */
Player::Player()
    : m_yawDegree(0.0f)
    , m_lockOnSystem(nullptr)
    , m_isAutoPilotMode(false)
{
    m_robot = std::make_unique<Robot>();

    // 初期状態では「人間の脳」をセットしておく
    m_manipulator = std::make_unique<KeyboardManipulator>();
}

/**
 * @brief デストラクタ
 */
Player::~Player()
{
}

/**
 * @brief 初期化処理
 * @param[in] device Direct3Dデバイス
 */
void Player::Initialize(ID3D11Device* device)
{
    m_robot->InitializeModel(device, L"Resources/Models/Robot/Robot.sdkmesh", L"Resources/Models/Robot");

	// 初期位置の設定
    m_robot->SetHealth(DEFAULT_MAX_HEALTH);
    m_robot->SetMaxHealth(DEFAULT_MAX_HEALTH);
}

/**
 * @brief ステージ管理クラスの登録
 */
void Player::SetStageManager(StageManager* stageManager)
{
    m_stageManager = stageManager;

    if (m_robot)
    {
        m_robot->SetStageManager(stageManager);
    }
}

/**
 * @brief 更新処理
 * @param[in] dt 経過時間
 */
void Player::Update(float dt)
{
    if (m_manipulator)
    {
        RobotCommand cmd = m_manipulator->GetCommand(dt);

		// プレイヤーが死んでいる場合は、クリア
        if (m_robot->GetHealth() <= 0.0f)
        {
            cmd.Clear();
        }

		// 射撃のコマンドがある場合は、照準方向をカメラの向きに合わせる
        if (cmd.fireGun || cmd.fireMissile)
        {
            cmd.lookDirection = cmd.aimDirection;
        }
        else
        {
            cmd.lookDirection = DirectX::SimpleMath::Vector3::Zero;
        }

        // Robotへの伝達
        m_robot->Update(cmd, dt);
    }

    if (m_stageManager && m_robot)
    {
        m_stageManager->CheckCollision(m_robot.get());
    }
}

/**
 * @brief 描画処理
 * @param[in] context コンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void Player::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states, 
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
    if (!m_isVisible) return;

    // 透明度の計算
    float alpha = CalculateCameraAlpha(view);

    // Robotに描画を依頼（透明度も渡す）
    m_robot->Render(context, states, view, proj, alpha);
}

/**
 * @brief カメラとプレイヤーの距離に基づいた透明度の計算
 * @param[in] view ビュー行列
 * @return float 計算されたアルファ値 
 */
float Player::CalculateCameraAlpha(const DirectX::SimpleMath::Matrix& view)
{
	// カメラの位置をビュー行列から取得
    DirectX::SimpleMath::Vector3 cameraPos = view.Invert().Translation();
    float distance = DirectX::SimpleMath::Vector3::Distance(m_robot->GetPosition(), cameraPos);

	// カメラが近すぎる場合は完全に透明、遠すぎる場合は完全に不透明になるように線形補間
    float alpha = std::clamp((distance - CAMERA_FADE_END_DIST) / 
        (CAMERA_FADE_START_DIST - CAMERA_FADE_END_DIST), 0.0f, 1.0f);

    return alpha;
}

/**
 * @brief 操作モード（人間/AI）の切り替え
 * @param[in] useAI trueでAI操作、falseで人間操作
 * @param[in] sm ステージ管理クラスのポインタ（AI操作時に必要）
 * @param[in] target AIが追尾するターゲットのポインタ（AI操作時に必要）
 */
void Player::SetAutoPilot(bool useAI, StageManager* sm, const Robot* target)
{
    m_isAutoPilotMode = useAI;

    if (m_isAutoPilotMode)
    {
        // AI脳に入れ替え
        m_manipulator = std::make_unique<AIManipulator>(m_robot.get(), target, sm);
    }
    else
    {
        // 人間の脳に戻す
        m_manipulator = std::make_unique<KeyboardManipulator>();

		// カメラの向きと位置をマニピュレーターに同期させる
        m_manipulator->SetCameraOrientation(m_cameraForward, m_cameraRight);

		// カメラ位置も同期
        m_manipulator->SetCameraPosition(m_cameraPosition);
    }
}

/**
 * @brief カメラ正面方向セット
 * @param[in] forward 正面ベクトル
 */
void Player::SetCameraForward(const DirectX::SimpleMath::Vector3& forward)
{
    m_cameraForward = forward;

    if (m_manipulator)
    {
		// マニピュレーターにカメラの向きを同期させる
        m_manipulator->SetCameraOrientation(m_cameraForward, m_cameraRight);
    }
}

/**
 * @brief カメラ右方向セット
 * @param[in] right 右方向ベクトル
 */
void Player::SetCameraRight(const DirectX::SimpleMath::Vector3& right)
{
    m_cameraRight = right;

    if (m_manipulator)
    {
		// マニピュレーターにカメラの向きを同期させる
        m_manipulator->SetCameraOrientation(m_cameraForward, m_cameraRight);
    }
}

/**
 * @brief カメラ正面位置セット
 * @param[in] position カメラの現在座標ベクトル
 */
void Player::SetCameraPosition(const DirectX::SimpleMath::Vector3& position)
{
    m_cameraPosition = position;

    // 脳（マニピュレーター）が存在していれば、位置を同期させる
    if (m_manipulator)
    {
        m_manipulator->SetCameraPosition(m_cameraPosition);
    }
}

/**
 * @brief ガン・ミサイルマネージャーセット
 * @param[in] manager 弾丸管理クラスのポインタ
 */
void Player::SetProjectileManager(ProjectileManager* manager)
{
    if (m_robot && m_robot->GetWeapon()) 
    {
        m_robot->GetWeapon()->SetProjectileManager(manager);
    }
}

/**
 * @brief ロックオンシステムセット
 * @param[in] system ロックオンシステムのポインタ
 */
void Player::SetLockOnSystem(LockOnSystem* system)
{
    m_lockOnSystem = system;
    if (m_robot && m_robot->GetWeapon())
    {
        m_robot->GetWeapon()->SetLockOnSystem(system);
    }
}