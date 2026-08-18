/*****************************************************************//**
 * @file    Missile.cpp
 * @brief   各種ミサイルの共通基底クラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Projectile/Missile/Missile.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Stage/StageManager.h"

 /**
  * @brief コンストラクタ
  */
Missile::Missile()
	: m_speed(0.0f)
	, m_turnSpeed(0.0f)
	, m_noHomingTimer(0.0f)
	, m_hasAcquiredTarget(false)
{
	// ダメージ量をセット
	SetDamage(MISSILE_DAMAGE);
}

/**
 * @brief デストラクタ
 */
Missile::~Missile()
{
}

/**
 * @brief ミサイルの初期化と発射パラメータの設定
 * @param[in] startPos 発射位置のワールド座標
 * @param[in] startDir 発射される向き（正規化ベクトル）
 * @param[in] getPosFunc ターゲットの座標を取得するコールバック関数
 * @param[in] getVelFunc ターゲットの速度を取得するコールバック関数
 * @param[in] isActiveFunc ターゲットが生存しているか確認する関数
 * @param[in] speed ミサイルの最大巡航速度
 * @param[in] turnSpeed ミサイルの旋回性能
 * @param[in] lifeTime 自然消滅するまでの生存時間（秒）
 * @param[in] owner 発射主のロボットへのポインタ
 */
void Missile::Initialize(const DirectX::SimpleMath::Vector3& startPos,
	const DirectX::SimpleMath::Vector3& startDir, std::function<DirectX::SimpleMath::Vector3()> getPosFunc,
	std::function<DirectX::SimpleMath::Vector3()> getVelFunc, std::function<bool()> isActiveFunc,
	float speed, float turnSpeed, float lifeTime, const Robot* owner)
{
	SetPosition(startPos);
	SetShooterPosition(startPos);

	DirectX::SimpleMath::Matrix lookMat =
		DirectX::SimpleMath::Matrix::CreateWorld(
			DirectX::SimpleMath::Vector3::Zero,
			startDir, DirectX::SimpleMath::Vector3::Up);

	// ミサイルの初期姿勢を発射方向に合わせる
	SetRotation(DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookMat));

	m_speed = speed;
	m_turnSpeed = turnSpeed;
	SetLifeTime(lifeTime);
	SetCurrentLifeTime(lifeTime);

	m_getTargetPos = getPosFunc;
	m_getTargetVel = getVelFunc;
	m_isTargetActive = isActiveFunc;
	SetOwnerRobot(owner);

	// 発射直後はアクティブ状態にする
	SetActive(true);
	m_hasAcquiredTarget = false;
	m_noHomingTimer = INITIAL_NO_HOMING_TIME;

	// 発射直後は最大速度ではなく、一定の初速（射出速度）を与える
	SetVelocity(startDir * (m_speed * LAUNCH_SPEED_RATE));
}

/**
 * @brief 毎フレームの更新処理
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void Missile::Update(float dt)
{
	if (!IsActive()) return;

	// 寿命チェック（自然消滅）
	float currentLife = GetCurrentLifeTime() - dt;
	SetCurrentLifeTime(currentLife);
	if (currentLife <= 0.0f)
	{
		SetActive(false);
		return;
	}

	// 発射直後の不自然な急旋回を防ぐため、誘導開始までのディレイを設ける
	if (m_noHomingTimer > 0.0f)
	{
		m_noHomingTimer -= dt;
	}

	// 派生クラスで実装されたアルゴリズムを呼び出し、弾道と姿勢を更新
	DirectX::SimpleMath::Vector3 aimPoint = CalculateAimPoint(dt);
	UpdateMovement(dt, aimPoint);

	SetPosition(GetPosition() + (GetVelocity() * dt));

	// 弾体の後方（ローカルZ軸の逆方向）からスモークエフェクトを発生させる
	DirectX::SimpleMath::Vector3 dir =
		DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, GetRotation());

	// スモークの発生位置をミサイルの後方にオフセットして計算
	DirectX::SimpleMath::Vector3 smokePos = GetPosition() - (dir * SMOKE_OFFSET_DISTANCE);
	EffectSystem::Instance()->SpawnMissileSmoke(smokePos);
}

/**
 * @brief ミサイルモデルの描画
 * @param[in] context デバイスコンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void Missile::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (!IsActive() || !GetModel()) return;

	DirectX::SimpleMath::Matrix world =
		DirectX::SimpleMath::Matrix::CreateScale(MODEL_DEFAULT_SCALE) *
		DirectX::SimpleMath::Matrix::CreateFromQuaternion(GetRotation()) *
		DirectX::SimpleMath::Matrix::CreateTranslation(GetPosition());

	GetModel()->Draw(context, *states, world, view, proj);
}

/**
 * @brief 移動・旋回・衝突判定の統合処理
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] aimPoint 目標座標
 */
void Missile::UpdateMovement(float dt, const DirectX::SimpleMath::Vector3& aimPoint)
{
	// 目標への誘導計算を行い、速度ベクトルを更新する
	UpdateHoming(dt, aimPoint);

	// 進行方向にあわせてモデルの姿勢（回転）を更新する
	UpdateRotationFromVelocity();

	// ステージ（壁や床）との衝突を判定し、爆発させる
	CheckWorldCollision(dt);
}

/**
 * @brief 目標への誘導計算をトリガーする
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] aimPoint 目標座標
 */
void Missile::UpdateHoming(float dt, const DirectX::SimpleMath::Vector3& aimPoint)
{
	// 誘導無効時間が終了し、かつターゲットが生きている場合のみ追尾を行う
	if (m_noHomingTimer <= 0.0f && IsTargetActive())
	{
		ProcessHomingSteer(dt, aimPoint);
	}
}

/**
 * @brief ターゲットとの距離や角度を算出し、具体的な旋回処理を呼び出す
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] aimPoint 目標座標
 */
void Missile::ProcessHomingSteer(float dt, const DirectX::SimpleMath::Vector3& aimPoint)
{
	DirectX::SimpleMath::Vector3 diff = aimPoint - GetPosition();
	float dist = diff.Length();

	// ターゲットに極端に近づいた場合は、異常な急旋回（荒ぶり）を防ぐため誘導を打ち切る
	if (dist < MIN_HOMING_DISTANCE && m_getTargetPos) return;

	DirectX::SimpleMath::Vector3 toTarget = diff;
	if (dist > ZERO_DIVISION_EPSILON)
	{
		toTarget.Normalize();
	}
	else
	{
		// 完全に重なっている場合のゼロ除算を防ぐため、現在の進行方向を維持する
		toTarget = GetVelocity();
		toTarget.Normalize();
	}

	DirectX::SimpleMath::Vector3 currentDir = GetVelocity();
	currentDir.Normalize();

	float dot = currentDir.Dot(toTarget);

	// 派生クラスで実装された旋回アルゴリズムを呼び出す
	CalculateSteering(dt, toTarget, dot);
}

/**
 * @brief 進行方向にあわせてモデルの姿勢（回転）を更新する
 */
void Missile::UpdateRotationFromVelocity()
{
	// 停止状態（速度ゼロ）で向きを計算しようとして真上を向くバグを防ぐ
	if (GetVelocity().LengthSquared() > ROTATION_VELOCITY_THRESHOLD)
	{
		DirectX::SimpleMath::Vector3 dir = GetVelocity();
		dir.Normalize();
		DirectX::SimpleMath::Matrix lookAt =
			DirectX::SimpleMath::Matrix::CreateWorld(
				DirectX::SimpleMath::Vector3::Zero,
				dir,
				DirectX::SimpleMath::Vector3::Up);

		SetRotation(DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(lookAt));
	}
}

/**
 * @brief ステージ（壁や床）との衝突を判定し、爆発させる
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void Missile::CheckWorldCollision(float dt)
{
	// 高速移動による壁のすり抜け（トンネル効果）を防ぐため、レイキャストで衝突を判定する
	if (GetStageManager())
	{
		float moveDist = GetVelocity().Length() * dt;
		DirectX::SimpleMath::Vector3 dir = GetVelocity();
		dir.Normalize();

		if (GetStageManager()->RayCast(GetPosition(), dir, moveDist))
		{
			SetActive(false);
			EffectSystem::Instance()->SpawnExplosion(GetPosition());
			return;
		}
	}

	// ステージ外や床面への衝突に対するフェールセーフ
	if (GetPosition().y <= GROUND_HEIGHT_THRESHOLD)
	{
		SetActive(false);
		EffectSystem::Instance()->SpawnExplosion(GetPosition());
	}
}

/**
 * @brief 当たり判定の取得
 * @return 球状の当たり判定
 */
DirectX::BoundingSphere Missile::GetCollider() const
{
	return DirectX::BoundingSphere(GetPosition(), COLLISION_RADIUS);
}

/**
 * @brief ターゲット座標の取得ヘルパー
 * @return ターゲットのワールド座標
 */
DirectX::SimpleMath::Vector3 Missile::GetTargetPosition() const
{
	return (m_getTargetPos && IsTargetActive()) ? m_getTargetPos() : DirectX::SimpleMath::Vector3::Zero;
}

/**
 * @brief ターゲットの生存確認ヘルパー
 * @return 生存していればtrue
 */
bool Missile::IsTargetActive() const
{
	return m_isTargetActive ? m_isTargetActive() : false;
}

/**
 * @brief ターゲット速度の取得ヘルパー
 * @return ターゲットの速度ベクトル
 */
DirectX::SimpleMath::Vector3 Missile::GetTargetVelocity() const
{
	return (m_getTargetVel && IsTargetActive()) ? m_getTargetVel() : DirectX::SimpleMath::Vector3::Zero;
}