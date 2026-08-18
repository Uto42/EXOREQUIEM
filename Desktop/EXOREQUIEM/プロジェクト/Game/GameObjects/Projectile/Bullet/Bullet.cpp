/*****************************************************************//**
 * @file    Bullet.cpp
 * @brief   マシンガンの弾丸発射とリロード制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Projectile/Bullet/Bullet.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Stage/StageManager.h"

 /**
  * @brief コンストラクタ
  */
Bullet::Bullet()
	: m_isHit(false)
{
	// 最大寿命のセット
	SetLifeTime(MAX_LIFE_TIME);
	// 現在の残り寿命のセット
	SetCurrentLifeTime(MAX_LIFE_TIME);
	// ダメージ量のセット
	SetDamage(BULLET_DAMAGE);
}

/**
 * @brief 初期化処理
 * @param startPosition 発射開始座標
 * @param startRotation 発射時の回転クォータニオン
 * @param owner 発射元のロボットのポインタ
 */
void Bullet::Initialize(const DirectX::SimpleMath::Vector3& startPosition,
	const DirectX::SimpleMath::Quaternion& startRotation, const Robot* owner)
{
	SetPosition(startPosition);
	SetRotation(startRotation);
	SetOwnerRobot(owner);
	SetActive(true);
	m_isHit = false;
	SetCurrentLifeTime(GetLifeTime());
	SetShooterPosition(startPosition);

	// 初期速度ベクトルを設定 (発射時の向きから前方ベクトルを作成)
	DirectX::SimpleMath::Vector3 forward =
		DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Forward, GetRotation());

	// 正規化して速度を設定
	forward.Normalize();
	SetVelocity(forward * BULLET_SPEED);
}

/**
 * @brief 更新処理
 * @param dt 経過時間
 */
void Bullet::Update(float dt)
{
	if (!IsActive())
	{
		return;
	}

	// 寿命タイマーを更新
	float currentLife = GetCurrentLifeTime() - dt;
	SetCurrentLifeTime(currentLife);
	if (currentLife <= 0.0f)
	{
		SetActive(false);
		return;
	}

	// 移動前に地形との当たり判定を行う
	CheckWorldCollision(dt);

	// 衝突して非アクティブになっていなければ移動処理を行う
	if (IsActive())
	{
		SetPosition(GetPosition() + (GetVelocity() * dt));
	}
}

/**
 * @brief ステージ（壁・地面）との衝突判定
 * @param dt 経過時間
 */
void Bullet::CheckWorldCollision(float dt)
{
	// 地面との判定
	if (GetPosition().y <= GROUND_HEIGHT_THRESHOLD)
	{
		SetActive(false);
		EffectSystem::Instance()->SpawnExplosion(GetPosition());
		return;
	}

	// ステージ（壁や障害物）との判定
	if (GetStageManager())
	{
		float moveDistance = GetVelocity().Length() * dt;

		// レイキャストの距離がゼロ除算の閾値を超える場合のみ判定を行う
		if (moveDistance > ZERO_DIVISION_EPSILON)
		{
			DirectX::SimpleMath::Vector3 direction = GetVelocity();
			direction.Normalize();

			if (GetStageManager()->RayCast(GetPosition(), direction, moveDistance))
			{
				SetActive(false);
				EffectSystem::Instance()->SpawnExplosion(GetPosition());
			}
		}
	}
}

/**
 * @brief 描画処理
 * @param context Direct3Dデバイスコンテキスト
 * @param states 共通ステート
 * @param view ビュー行列
 * @param proj プロジェクション行列
 */
void Bullet::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (!IsActive() || !GetModel())
	{
		return;
	}

	// ワールド行列の計算
	// スケール -> 回転 -> 90度補正 -> 平行移動
	DirectX::SimpleMath::Matrix world =
		DirectX::SimpleMath::Matrix::CreateScale(MODEL_DEFAULT_SCALE)
		* DirectX::SimpleMath::Matrix::CreateFromQuaternion(GetRotation())
		* DirectX::SimpleMath::Matrix::CreateRotationY(DirectX::XMConvertToRadians(MODEL_YAW_OFFSET_DEG))
		* DirectX::SimpleMath::Matrix::CreateTranslation(GetPosition());

	GetModel()->Draw(context, *states, world, view, proj);
}

/**
 * @brief 当たり判定（球）を取得
 * @return BoundingSphere 判定用ボリューム
 */
DirectX::BoundingSphere Bullet::GetCollider() const
{
	// 現在位置を中心に、定義された半径の球を返す
	return DirectX::BoundingSphere(GetPosition(), COLLISION_RADIUS);
}