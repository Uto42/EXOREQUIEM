/*****************************************************************//**
 * @file    Enemy.cpp
 * @brief   敵ユニットの基底クラス：演出、武器制御、AIロジックの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/Systems/Input/RobotCommand.h"
#include "Game/Systems/Input/IManipulator.h"
#include "Game/GameObjects/Projectile/ProjectileManager.h"
#include "Game/Stage/StageManager.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"

/**
 * @brief コンストラクタ
 * @details 敵の実体となるRobotオブジェクトを生成する
 */
Enemy::Enemy()
    : m_playerRobot(nullptr)
    , m_stageManager(nullptr)
{
	// Robotオブジェクトを共有ポインタで生成し、m_robotに格納
    m_robot = std::make_shared<Robot>();
}

/**
 * @brief デストラクタ
 */
Enemy::~Enemy()
{
}

/**
 * @brief 初期化処理
 * @param[in] device DirectX11デバイス
 * @param[in] initialPos 初期配置座標
 * @param[in] isActive 初期アクティブフラグ
 */
void Enemy::Initialize(ID3D11Device* device, const DirectX::SimpleMath::Vector3& initialPos,
    bool isActive, const wchar_t* modelPath, const wchar_t* textureDir)
{
	// ロボットの初期位置とアクティブ状態を設定
    m_robot->SetPosition(initialPos);
    m_robot->SetActive(isActive);

	// ロボットのモデルとテクスチャを初期化
    m_robot->InitializeModel(device, modelPath, textureDir);

    // 武器システムのターゲットとしてプレイヤーを登録
    if (m_robot->GetWeapon() && m_playerRobot) 
    {
        m_robot->GetWeapon()->SetTarget(m_playerRobot, true);
    }
}

/**
 * @brief 更新処理
 * @param[in] dt 前フレームからの経過時間
 */
void Enemy::Update(float dt)
{
    if (!m_robot || !m_robot->IsActive()) return;

    // AIから次のアクション命令を取得し、ロボットに伝達
    if (m_manipulator)
    {
        RobotCommand command = m_manipulator->GetCommand(dt);

        m_robot->Update(command, dt);
    }

    // ステージ（地面・壁）との衝突判定
    if (m_stageManager)
    {
        m_stageManager->CheckCollision(m_robot.get());
    }
}

/**
 * @brief 旋回更新処理
 * @details 射撃時にプレイヤーを正面に捉え続けるための水平旋回
 * @param[in] dt 前フレームからの経過時間
 */
void Enemy::UpdateRotation(float dt)
{
	// ロボットの現在の回転を取得し、正面方向ベクトルを算出
    DirectX::SimpleMath::Quaternion currentRot = m_robot->GetRotation();
    DirectX::SimpleMath::Vector3 currentForward = 
        DirectX::SimpleMath::Vector3::TransformNormal(
            DirectX::SimpleMath::Vector3::Forward, 
            DirectX::SimpleMath::Matrix::CreateFromQuaternion(currentRot));

    currentForward.y = 0.0f;

	// Yを0にした結果、ゼロベクトルになっていないか確認
    if (currentForward.LengthSquared() > FORWARD_EPSILON_SQ)
    {
        currentForward.Normalize();
        currentRot = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(
            DirectX::SimpleMath::Matrix::CreateWorld(
                DirectX::SimpleMath::Vector3::Zero, 
                currentForward, 
                DirectX::SimpleMath::Vector3::Up));

        m_robot->SetRotation(currentRot);
    }

    // ターゲットへの方向を算出（高度差は無視）
    DirectX::SimpleMath::Vector3 dirToPlayer = m_playerRobot->GetPosition() - m_robot->GetPosition();
    dirToPlayer.y = 0.0f;

    // Yを0にした結果、ゼロベクトルになっていないか確認
    if (dirToPlayer.LengthSquared() > ROTATION_EPSILON_SQ)
    {
        dirToPlayer.Normalize();
        DirectX::SimpleMath::Vector3 targetDir = -dirToPlayer;

        // 現在の正面方向を再取得（直立化後）
        currentForward = DirectX::SimpleMath::Vector3::TransformNormal(
            DirectX::SimpleMath::Vector3::Forward, 
            DirectX::SimpleMath::Matrix::CreateFromQuaternion(currentRot));

        currentForward.y = 0.0f;
        currentForward.Normalize();

        // プレイヤーへの向きが現在の向きと完全に真逆の場合、計算破綻を防ぐためベクトルを横にずらす
        if (currentForward.Dot(targetDir) < REVERSE_DIRECTION_DOT_THRESHOLD)
        {
            DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(currentForward);
            targetDir += right * AVOID_REVERSE_OFFSET;
            targetDir.Normalize();
        }

        // CreateWorld を用いて、ロールが混入しない安全な回転行列を生成
        DirectX::SimpleMath::Quaternion targetRot = 
            DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(
                DirectX::SimpleMath::Matrix::CreateWorld(
                    DirectX::SimpleMath::Vector3::Zero, 
                    targetDir,
                    DirectX::SimpleMath::Vector3::Up));

		// 現在の回転と目標の回転を球面線形補間（Slerp）で補間し、滑らかに旋回させる
        DirectX::SimpleMath::Quaternion nextRot = 
            DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRot, SHOOTING_TURN_SPEED * dt);

		// robotの回転を更新
        m_robot->SetRotation(nextRot);
    }
}

/**
 * @brief ロボットオブジェクトのセット
 * @param[in] robot 共有ポインタで渡されるロボットオブジェクト
 */
void Enemy::SetRobot(std::shared_ptr<Robot> robot)
{
    m_robot = robot;
}

/**
 * @brief ターゲットとして選択可能か（生存しているか）
 * @return bool trueなら生存
 */
bool Enemy::IsTargetable() const
{
    return (m_robot->GetHealth() > 0.0f);
}

/**
 * @brief ステージ管理オブジェクトの登録
 * @param[in] stageManager
 */
void Enemy::SetStageManager(StageManager* stageManager)
{
    m_stageManager = stageManager;

    if (m_robot)
    {
        m_robot->SetStageManager(stageManager);
    }
}

/**
 * @brief 描画処理
 * @param[in] context D3D11デバイスコンテキスト
 * @param[in] states 共通ステートオブジェクト
 * @param[in] view ビューマトリクス
 * @param[in] proj プロジェクションマトリクス
 */
void Enemy::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states, 
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
    // ロボット本体の描画
    m_robot->Render(context, states, view, proj);
}

/**
 * @brief 武装管理システムのセットアップ
 * @param[in] gm ガンマネージャー
 * @param[in] mm ミサイルマネージャー
 */
void Enemy::SetProjectileManager(ProjectileManager* pm)
{
    if (m_robot && m_robot->GetWeapon())
    {
        m_robot->GetWeapon()->SetProjectileManager(pm);
    }
}