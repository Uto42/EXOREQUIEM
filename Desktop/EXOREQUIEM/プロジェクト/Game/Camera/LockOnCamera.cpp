/*****************************************************************//**
 * @file    LockOnCamera.cpp
 * @brief   ロックオン時のエイムアシストおよびプレイヤーへの向き反映を行うカメラコンポーネントの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Camera/LockOnCamera.h"
#include "Game/World/World.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/GameObjects/Robot/Robot.h"

/**
 * @brief コンストラクタ
 * @param[in] world ワールドコンテキストへのポインタ
 * @param[in] lockOnSystem ロックオンシステムへのポインタ
 */
LockOnCamera::LockOnCamera(World* world, LockOnSystem* lockOnSystem)
	: m_world(world)
	, m_lockOnSystem(lockOnSystem)
	, m_assistRate(0.0f)
{
}

/**
 * @brief 毎フレームのロックオンアシスト計算とプレイヤーへのデータ反映
 * @param[in,out] camera 本体への参照
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void LockOnCamera::Update(Camera& camera, float dt)
{
	if (!m_world || !m_lockOnSystem) return;

	// ロックオンアシストの適用
	ApplyLockOnAssist(camera, dt);

	// 移動制御のために、カメラの向き情報をプレイヤーに反映する
	ReflectToPlayer(camera);
}

/**
 * @brief ロックオン対象の敵との位置関係からアシスト適用を統括する
 * @param[in,out] camera 本体への参照
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void LockOnCamera::ApplyLockOnAssist(Camera& camera, float dt)
{
	if (!m_lockOnSystem->IsHardLockMode()) return;

	// ロックオン対象が存在しない場合はアシストを行わない
	Robot* currentTarget = m_lockOnSystem->GetCurrentTarget();
	if (!currentTarget) return;

	// マウス入力の強さに応じてアシスト率を更新する
	UpdateAssistRate(camera.GetMouseDeltaX(), camera.GetMouseDeltaY(), dt);
	if (m_assistRate <= ASSIST_MIN_THRESHOLD) return;

	// ロックオン対象との位置関係から理想のYaw/Pitch角を計算する
	float idealYaw = 0.0f, idealPitch = 0.0f;
	CalculateIdealAngles(camera, currentTarget, idealYaw, idealPitch);

	// その理想角度に向かって、デッドゾーンを考慮した滑らかな追従処理を行う
	ApplyTracking(camera, idealYaw, idealPitch, dt);
}

/**
 * @brief マウス入力の強さに応じてカメラ追従のアシスト率を更新する
 * @param[in] mouseDeltaX マウスのX移動量
 * @param[in] mouseDeltaY マウスのY移動量
 * @param[in] dt 経過時間（秒）
 */
void LockOnCamera::UpdateAssistRate(float mouseDeltaX, float mouseDeltaY, float dt)
{
	float mouseSpeed = sqrtf(mouseDeltaX * mouseDeltaX + mouseDeltaY * mouseDeltaY);

	// 大きく速くマウスを振った瞬間は「切り替えたい意思表示」とみなし、遅延ゼロでアシストを切る
	if (mouseSpeed > SWING_RELEASE_SPEED)
	{
		m_assistRate = 0.0f;
		return;
	}

	float reduction = std::clamp((mouseSpeed - MOUSE_INPUT_MIN) / (MOUSE_INPUT_MAX - MOUSE_INPUT_MIN), 0.0f, 1.0f);
	float targetAssistRate = 1.0f - reduction;

	// 戻る(アシストが強まる)方向はゆっくり、弱まる方向は速く追従させる非対称補間
	float smoothRate = (targetAssistRate > m_assistRate) ? ASSIST_SMOOTH_RETURN : ASSIST_SMOOTH_RELEASE;

	// フレームレート非依存の滑らかな補間（FPSが変動しても同じ速度で変化させる計算）
	float alpha = 1.0f - expf(-smoothRate * dt);
	m_assistRate += (targetAssistRate - m_assistRate) * alpha;
}

/**
 * @brief ターゲットとの位置関係から、カメラが向くべき理想の角度（Yaw, Pitch）を計算する
 * @param[in] camera 本体への参照
 * @param[in] target ロックオン対象
 * @param[out] outYaw 算出された理想のYaw角
 * @param[out] outPitch 算出された理想のPitch角
 */
void LockOnCamera::CalculateIdealAngles(Camera& camera, Robot* target, float& outYaw, float& outPitch) const
{
	// ターゲットの中心座標を取得し、Y軸方向にオフセットを加えて狙う位置を調整
	DirectX::SimpleMath::Vector3 enemyPos = target->GetBoundingSphere().Center;
	enemyPos.y += TARGET_OFFSET_Y;

	DirectX::SimpleMath::Vector3 eyePos = camera.GetEye();
	DirectX::SimpleMath::Vector3 toEnemy = enemyPos - eyePos;

	// 距離と高さからPitch角を計算する
	outYaw = atan2f(toEnemy.x, toEnemy.z);
	float dist = toEnemy.Length();
	float basePitch = (dist > EPSILON_DIST) ? asinf(toEnemy.y / dist) : 0.0f;

	// 接近時の上下ブレを封じ、一定距離以内でカメラを強制的に水平に保つ処理
	float horizontalDist = sqrtf(toEnemy.x * toEnemy.x + toEnemy.z * toEnemy.z);
	
	// 距離を 0.0 ～ 1.0 の割合に変換（遠い=1.0、近い=0.0）
	float pitchFactor = std::clamp((horizontalDist - PITCH_FLAT_MIN_DIST) 
		/ (PITCH_FLAT_START_DIST - PITCH_FLAT_MIN_DIST), 0.0f, 1.0f);
	
	// Smoothstep公式：t^2 * (3 - 2t)
	// 直線的な変化(Linear)ではなく、S字カーブを描いて滑らかに水平に向かせるための計算式
	pitchFactor = pitchFactor * pitchFactor * (3.0f - 2.0f * pitchFactor);

	outPitch = basePitch * pitchFactor;
}

/**
 * @brief 理想の角度へ向かって、デッドゾーンを考慮した滑らかな追従処理を行う
 * @param[in,out] camera 本体への参照
 * @param[in] idealYaw 理想のYaw角
 * @param[in] idealPitch 理想のPitch角
 * @param[in] dt 経過時間（秒）
 */
void LockOnCamera::ApplyTracking(Camera& camera, float idealYaw, float idealPitch, float dt) const
{
	float currentYaw = camera.GetYTmp();
	float currentPitch = camera.GetXTmp();

	// 角度の差分を -PI ～ PI の範囲に丸める（左右どちらに回るのが近いか判定）
	float diffYaw = DirectX::XMScalarModAngle(idealYaw - currentYaw);
	float diffPitch = idealPitch - currentPitch;

	// 画面中央（innerRad）では追尾せず、離れる（outerRad）ほど強く追尾させるための重み付け
	auto SmoothWeight = [](float diff, float innerRad, float outerRad) -> float
		{
			float t = std::clamp((fabsf(diff) - innerRad) / (outerRad - innerRad), 0.0f, 1.0f);
			// Smoothstep公式を適用して、追尾の掛かり始めと終わりを滑らかにする
			return t * t * (3.0f - 2.0f * t);
		};

	float wYaw = SmoothWeight(diffYaw, INNER_YAW, OUTER_YAW);
	float wPitch = SmoothWeight(diffPitch, INNER_PITCH, OUTER_PITCH);

	// FPS非依存の追従計算。wYawやwPitchが0に近い（画面中央）ならalphaも0になり追従しない
	float alphaYaw = 1.0f - expf(-TRACK_RATE * wYaw * dt);
	float alphaPitch = 1.0f - expf(-TRACK_RATE * wPitch * dt);

	// 算出した割合(alpha)とマウス入力によるアシスト率(m_assistRate)を掛けて最終的な角度を決定
	float newYaw = currentYaw + diffYaw * alphaYaw * m_assistRate;
	float newPitch = currentPitch + diffPitch * alphaPitch * m_assistRate;

	// Pitch角度を制限範囲内にクランプして、極端な上下を防ぐ
	camera.SetCameraYawPitch(newYaw, newPitch);
}

/**
 * @brief 最終的なカメラの向き（Forward/Rightベクトル）をプレイヤーの移動制御用に通知・反映
 * @param[in] camera 本体への参照
 */
void LockOnCamera::ReflectToPlayer(Camera& camera)
{
	// プレイヤーの移動制御に必要なカメラの向きベクトルを計算して反映
	Player* player = m_world->GetPlayer();
	if (!player) return;

	DirectX::SimpleMath::Matrix viewMatrix = camera.GetViewMatrix();

	// ビュー行列から右方向ベクトルと正面方向ベクトルを抽出
	DirectX::SimpleMath::Vector3 realRight(viewMatrix._11, viewMatrix._21, viewMatrix._31);
	DirectX::SimpleMath::Vector3 realForward(-viewMatrix._13, -viewMatrix._23, -viewMatrix._33);

	// 正規化して単位ベクトルにする
	realRight.Normalize();
	realForward.Normalize();

	// プレイヤーに反映
	player->SetCameraForward(realForward);
	player->SetCameraRight(realRight);
	player->SetCameraPosition(camera.GetEye());
}