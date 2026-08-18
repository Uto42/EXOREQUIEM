/*****************************************************************//**
 * @file    Robot.cpp
 * @brief   プレイヤーおよび敵の共通基底クラス（物理、エネルギー、ダメージ管理）の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Robot.h"
#include "Game/Systems/Input/IManipulator.h"
#include "Game/GameObjects/Robot/Thruster/Thruster.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Systems/Input/RobotCommand.h"
#include "Game/Stage/StageManager.h"
#include "Game/GameObjects/Robot/State/RobotState.h"

#include "Game/GameObjects/Robot/State/Death/RobotDeathState.h"
#include "Game/GameObjects/Robot/State/Evade/RobotEvadeState.h"
#include "Game/GameObjects/Robot/State/Idle/RobotIdleState.h"
#include "Game/GameObjects/Robot/State/Jump/RobotJumpState.h"
#include "Game/GameObjects/Robot/State/Moving/RobotMovingState.h"
#include "Game/GameObjects/Robot/State/Rise/RobotRiseState.h"

#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include "Game/Systems/LockOn/LockOnSystem.h"

/**
 * @brief コンストラクタ
 */
Robot::Robot()
	: m_position(DirectX::SimpleMath::Vector3::Zero)
	, m_velocity(DirectX::SimpleMath::Vector3::Zero)
	, m_rotation(DirectX::SimpleMath::Quaternion::Identity)
	, m_isActive(true)
	, m_health(DEFAULT_HEALTH_VALUE)
	, m_maxHealth(DEFAULT_HEALTH_VALUE)
	, m_isGrounded(true)
	, m_isGravityEnabled(true)
	, m_isVisible(true)
	, m_stageManager(nullptr)
	, m_energyRecoveryTimer(0.0f)
	, m_evadeCooldownTimer(0.0f)
	, m_collisionRadius(BASE_RADIUS)
	, m_collisionHeight(BASE_HEIGHT)
{
	// 武器とスラスターの制御クラスを生成
	m_weaponController = std::make_unique<WeaponController>();
	m_thruster = std::make_unique<Thruster>();

	// 初期状態のステートを生成して実行
	m_currentState = std::make_unique<RobotIdleState>();
	m_currentState->Enter(this);
}

/**
 * @brief デストラクタ
 */
Robot::~Robot()
{
}

/**
 * @brief 3Dモデルの初期化
 * @param[in] device DirectX11デバイス
 * @param[in] modelPath モデルファイルのパス
 * @param[in] textureDir テクスチャディレクトリのパス
 */
void Robot::InitializeModel(ID3D11Device* device, const wchar_t* modelPath, const wchar_t* textureDir)
{
	DirectX::EffectFactory fx(device);
	fx.SetDirectory(textureDir);
	m_model = DirectX::Model::CreateFromSDKMESH(device, modelPath, fx);

	// モデルのマテリアルおよびライティングの基本設定
	if (m_model)
	{
		for (auto& mesh : m_model->meshes)
		{
			for (auto& part : mesh->meshParts)
			{
				auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(part->effect.get());
				if (basicEffect)
				{
					basicEffect->SetLightingEnabled(true);
					basicEffect->SetAmbientLightColor(DirectX::Colors::Gray);
				}
			}
		}
	}

	if (m_thruster)
	{
		m_thruster->Initialize(device);
	}
}

/**
 * @brief 更新処理
 * @param[in] cmd 司令コマンド
 * @param[in] dt 前フレームからの経過時間
 */
void Robot::Update(const RobotCommand& cmd, float dt)
{
	// 旋回制御
	UpdateRotation(cmd, dt);

	// 武器の切り替え・リロード・各種射撃コマンドの処理
	if (m_weaponController)
	{
		m_weaponController->Update(dt);

		if (cmd.switchWeapon)
		{
			m_weaponController->ToggleWeaponSet();
		}

		if (cmd.reload)
		{
			m_weaponController->StartGunReload();
			m_weaponController->StartMissileReload();
		}

		if (cmd.fireGun)
		{
			m_weaponController->TryFireGun(m_position, m_rotation, cmd.aimDirection, this);
		}

		if (cmd.fireMissile)
		{
			m_weaponController->TryFireMissile(cmd.aimOriginPosition, m_position, m_rotation, cmd.aimDirection, this);
		}

		if (cmd.fireShotgun)
		{
			// TryFireShotgunを直接呼び出し
			m_weaponController->TryFireShotgun(m_position, m_rotation, cmd.aimDirection, this);
		}

		if (cmd.fireHighAltitudeMissile)
		{
			// TryFireHighAltitudeMissileを直接呼び出し
			m_weaponController->TryFireHighAltitudeMissile(m_position, m_rotation, cmd.aimOriginPosition, this, nullptr);
		}
	}

	if (m_evadeCooldownTimer > 0.0f)
	{
		m_evadeCooldownTimer -= dt;
	}

	// スラスターエフェクトの更新（移動や後退を検知して制御）
	if (m_thruster)
	{
		bool isAnyMove = cmd.moveDirection.LengthSquared() > 0.0f || cmd.jump || cmd.rise;

		DirectX::SimpleMath::Vector3 forward =
			DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, m_rotation);

		bool isBack = cmd.moveDirection.Dot(forward) < -BACKWARD_INPUT_THRESHOLD;

		m_thruster->Update(dt, isAnyMove, isBack);
	}

	// FSM(ステートマシン)の更新と状態遷移判定
	if (m_currentState)
	{
		RobotStateTypes nextStateType = m_currentState->Update(this, cmd, dt);
		if (nextStateType != m_currentState->GetType())
		{
			ChangeState(nextStateType, cmd);
		}
	}

	// 物理シミュレーション（速度・座標の更新）
	UpdatePhysics(dt);

	// エネルギーの自動回復処理
	UpdateEnergy(dt);
}

/**
 * @brief 旋回（方向転換）の更新
 * @param[in] cmd 司令コマンド
 * @param[in] dt 前フレームからの経過時間
 */
void Robot::UpdateRotation(const RobotCommand& cmd, float dt)
{
	// 現在のロボットの前方ベクトルをワールド空間で表す
	DirectX::SimpleMath::Vector3 currentForward = 
		DirectX::SimpleMath::Vector3::TransformNormal(
			DirectX::SimpleMath::Vector3::Forward, 
			DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_rotation));

	currentForward.y = 0.0f;
	if (currentForward.LengthSquared() > LENGTH_EPSILON_SQUARED)
	{
		currentForward.Normalize();
		// 自作のLookRotationの特異点を避け、DirectX標準の堅牢な行列生成を使用
		m_rotation = 
			DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(
				DirectX::SimpleMath::Matrix::CreateWorld(
					DirectX::SimpleMath::Vector3::Zero,
					currentForward,
					DirectX::SimpleMath::Vector3::Up));
	}

	// 向きたい方向（視線）が明示的に指定されている場合の旋回処理
	if (cmd.lookDirection.LengthSquared() > INPUT_DEADZONE_SQUARED)
	{
		DirectX::SimpleMath::Vector3 lookDir = cmd.lookDirection;
		lookDir.y = 0.0f;

		if (lookDir.LengthSquared() > LENGTH_EPSILON_SQUARED)
		{
			lookDir.Normalize();
			DirectX::SimpleMath::Vector3 targetDir = -lookDir;

			// Slerpがバク宙やバレルロールの経路を選ばないよう、真逆(180度)の場合は確実に右へずらす
			if (currentForward.Dot(targetDir) < OPPOSITE_DIR_THRESHOLD)
			{
				DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(currentForward);
				targetDir += right * OPPOSITE_EVASION_OFFSET;
				targetDir.Normalize();
			}

			// DirectXの行列生成を使用して、回転行列からクォータニオンを生成
			DirectX::SimpleMath::Quaternion targetRot = 
				DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(
					DirectX::SimpleMath::Matrix::CreateWorld(
						DirectX::SimpleMath::Vector3::Zero, 
						targetDir, 
						DirectX::SimpleMath::Vector3::Up));

			// 射撃・ロックオン中は素早く、それ以外は標準速度で旋回させる
			float turnSpeed = (cmd.fireGun || cmd.fireMissile) ? ROTATION_SPEED_COMBAT : ROTATION_SPEED;
			m_rotation = DirectX::SimpleMath::Quaternion::Slerp(m_rotation, targetRot, turnSpeed * dt);
		}
	}
	// 視線指定がない場合は、移動入力による自動旋回を行う
	else if (cmd.moveDirection.LengthSquared() > ROTATION_INPUT_THRESHOLD)
	{
		DirectX::SimpleMath::Vector3 moveDir = cmd.moveDirection;
		moveDir.y = 0.0f;

		if (moveDir.LengthSquared() > LENGTH_EPSILON_SQUARED)
		{
			moveDir.Normalize();
			DirectX::SimpleMath::Vector3 targetDir = -moveDir;

			// Slerpがバク宙やバレルロールの経路を選ばないよう、真逆(180度)の場合は確実に右へずらす
			if (currentForward.Dot(targetDir) < OPPOSITE_DIR_THRESHOLD)
			{
				DirectX::SimpleMath::Vector3 right = DirectX::SimpleMath::Vector3::Up.Cross(currentForward);
				targetDir += right * OPPOSITE_EVASION_OFFSET;
				targetDir.Normalize();
			}

			// DirectXの行列生成を使用して、回転行列からクォータニオンを生成
			DirectX::SimpleMath::Quaternion targetRot =
				DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(
					DirectX::SimpleMath::Matrix::CreateWorld(
						DirectX::SimpleMath::Vector3::Zero, 
						targetDir, 
						DirectX::SimpleMath::Vector3::Up));

			m_rotation = DirectX::SimpleMath::Quaternion::Slerp(m_rotation, targetRot, ROTATION_SPEED * dt);
		}
	}
}

/**
 * @brief 描画処理
 * @param[in] context コンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 * @param[in] alpha 透明度
 */
void Robot::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, float alpha)
{
	if (!m_model || !m_isVisible || alpha < RENDER_MIN_ALPHA) return;

	// ワールド行列の算出（スケール -> 回転 -> 座標）
	m_worldMatrix = DirectX::SimpleMath::Matrix::CreateScale(MODEL_DEFAULT_SCALE)
		* DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_rotation)
		* DirectX::SimpleMath::Matrix::CreateRotationY(DirectX::XMConvertToRadians(MODEL_YAW_OFFSET_DEG))
		* DirectX::SimpleMath::Matrix::CreateTranslation(m_position);

	UpdateModelAlpha(alpha);

	// フェード（半透明）中はブレンドステートを切り替えて描画する
	if (alpha < RENDER_OPAQUE_ALPHA)
	{
		m_model->Draw(context, *states, m_worldMatrix, view, proj, false, [=]() {
			context->OMSetBlendState(states->NonPremultiplied(), nullptr, BLEND_SAMPLE_MASK);
			});
	}
	else
	{
		m_model->Draw(context, *states, m_worldMatrix, view, proj);
	}

	if (m_thruster)
	{
		m_thruster->Render(context, states, view, proj, m_position, m_rotation);
	}
}

/**
 * @brief 物理挙動の更新（重力および座標計算）
 * @param[in] dt 前フレームからの経過時間
 */
void Robot::UpdatePhysics(float dt)
{
	// 死亡時は水平方向の移動を完全に停止させ、空中の場合のみ落下を継続する
	if (GetCurrentStateType() == RobotStateTypes::Death)
	{
		m_velocity.x = 0.0f;
		m_velocity.z = 0.0f;

		if (!m_isGrounded && m_isGravityEnabled)
		{
			m_velocity.y += GRAVITY_ACCEL * dt;
		}
	}
	else
	{
		// 通常の重力加速
		if (!m_isGrounded && m_isGravityEnabled)
		{
			m_velocity.y += GRAVITY_ACCEL * dt;
		}
	}

	m_position += m_velocity * dt;
}

/**
 * @brief エネルギー管理の更新
 * @param[in] dt 前フレームからの経過時間
 */
void Robot::UpdateEnergy(float dt)
{
	m_energyRecoveryTimer += dt;

	// 消費から一定時間経過している場合のみ自動回復を実行する
	if (m_energyRecoveryTimer >= ENERGY_RECOVERY_DELAY)
	{
		m_energy.Recover(ENERGY_RECOVERY_RATE * dt);
	}
}

/**
 * @brief 現在の状態タイプを取得
 * @return RobotStateTypes 現在のステート
 */
RobotStateTypes Robot::GetCurrentStateType() const
{
	if (m_currentState)
	{
		return m_currentState->GetType();
	}
	return RobotStateTypes::Idle;
}

/**
 * @brief ステージマネージャーの設定
 * @param[in] sm ステージマネージャーへのポインタ
 */
void Robot::SetStageManager(StageManager* sm)
{
	m_stageManager = sm;

	if (m_weaponController)
	{
		m_weaponController->SetStageManager(sm);
	}
}

/**
 * @brief モデルパーツ全体の透明度を一括設定
 * @param[in] alpha 透明度
 */
void Robot::UpdateModelAlpha(float alpha)
{
	for (const auto& mesh : m_model->meshes)
	{
		for (const auto& part : mesh->meshParts)
		{
			auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(part->effect.get());
			if (basicEffect) basicEffect->SetAlpha(alpha);
		}
	}
}

/**
 * @brief ステート（状態）の切り替え実行
 * @param[in] nextState 次のステート
 * @param[in] cmd 司令コマンド
 */
void Robot::ChangeState(RobotStateTypes nextState, const RobotCommand& cmd)
{
	if (m_currentState)
	{
		m_currentState->Exit(this);
	}

	switch (nextState)
	{
	case RobotStateTypes::Idle:
		m_currentState = std::make_unique<RobotIdleState>();
		break;
	case RobotStateTypes::Moving:
		m_currentState = std::make_unique<RobotMovingState>();
		break;
	case RobotStateTypes::Evade:
	{
		// 状態を作る前に、機体側で入力方向から「どっちに回避するか」を確定させる
		DirectX::SimpleMath::Vector3 evadeDir = DirectX::SimpleMath::Vector3::Zero;

		if (cmd.moveDirection.LengthSquared() > INPUT_DEADZONE_SQUARED)
		{
			for (size_t i = 0; i < 1; i++) {} // 意図的なスコープ維持用
			evadeDir = cmd.moveDirection;
			evadeDir.Normalize();
		}
		else
		{
			// 入力がない場合は、機体の正面方向へ回避させる
			evadeDir = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, m_rotation);
		}

		m_currentState = std::make_unique<RobotEvadeState>(evadeDir);
	}
	break;
	case RobotStateTypes::Rise:
		m_currentState = std::make_unique<RobotRiseState>();
		break;
	case RobotStateTypes::Jump:
		m_currentState = std::make_unique<RobotJumpState>();
		break;
	case RobotStateTypes::Death:
		m_currentState = std::make_unique<RobotDeathState>();
		break;
	}

	if (m_currentState)
	{
		m_currentState->Enter(this);
	}
}

/*
 * @brief リセットステート
 */
void Robot::ResetState()
{
	RobotCommand emptyCmd;
	emptyCmd.Clear();
	ChangeState(RobotStateTypes::Idle, emptyCmd);
}

/**
 * @brief エネルギーの消費処理
 * @param[in] amount 消費量
 */
void Robot::ConsumeEnergy(float amount)
{
	m_energy.Consume(amount);
	m_energyRecoveryTimer = 0.0f;
}

/**
 * @brief AIまたはプレイヤーコントローラーの設定
 * @param[in] manipulator コントローラーオブジェクトの所有権
 */
void Robot::SetController(std::unique_ptr<IManipulator> manipulator)
{
	m_manipulator = std::move(manipulator);
}

/**
 * @brief ダメージ算出および死亡判定処理
 * @param[in] damage 被ダメージ量
 */
void Robot::TakeDamage(float damage)
{
	if (!m_isActive) return;

	m_health -= damage;
	if (m_health <= 0.0f)
	{
		m_health = 0.0f;
		ChangeState(RobotStateTypes::Death);
	}
}

/**
 * @brief 当たり判定範囲の取得 (球形式)
 * @return BoundingSphere 球形式の当たり判定範囲
 */
DirectX::BoundingSphere Robot::GetBoundingSphere() const
{
	DirectX::BoundingSphere sphere;

	// 球の中心は、機体の足元から高さの半分だけ上にオフセットした位置とする
	sphere.Center = m_position + DirectX::SimpleMath::Vector3(0.0f, m_collisionHeight * HALF_RATIO, 0.0f);
	sphere.Radius = m_collisionRadius;

	return sphere;
}