/*****************************************************************//**
 * @file    BossRobot.cpp
 * @brief   上下半身が独立して可動するボス用ロボットクラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"

/**
 * @brief コンストラクタ
 */
BossRobot::BossRobot()
	: m_upperRotation(DirectX::SimpleMath::Quaternion::Identity)
	, m_upperHeightOffset(0.0f)
{
}

/**
 * @brief デストラクタ
 */
BossRobot::~BossRobot()
{
}

/**
 * @brief 上半身専用モデルの読み込みとライティング設定
 * @param[in] device D3D11デバイス
 * @param[in] upperModelPath 上半身モデルのファイルパス
 * @param[in] textureDir テクスチャが格納されているディレクトリ
 */
void BossRobot::LoadUpperModel(ID3D11Device* device, const wchar_t* upperModelPath, const wchar_t* textureDir)
{
	DirectX::EffectFactory fx(device);
	fx.SetDirectory(textureDir);
	m_upperModel = DirectX::Model::CreateFromSDKMESH(device, upperModelPath, fx);

	// 読み込んだモデルの全メッシュに対して基本ライティングを有効化する
	if (m_upperModel)
	{
		for (auto& mesh : m_upperModel->meshes)
		{
			for (auto& part : mesh->meshParts)
			{
				auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(part->effect.get());
				if (basicEffect)
				{
					basicEffect->SetLightingEnabled(true);
					basicEffect->SetAmbientLightColor(DirectX::Colors::White);
				}
			}
		}
	}
}

/**
 * @brief 上下半身の向きを同時に初期化・強制設定する
 * @param[in] rot 設定する回転姿勢
 */
void BossRobot::SetRotation(const DirectX::SimpleMath::Quaternion& rot)
{
	// 瞬間移動時などに上下のパーツの向きがズレるのを防ぐため、両方の姿勢を完全に同期させる
	Robot::SetRotation(rot);
	m_upperRotation = rot;
}

/**
 * @brief コマンド入力に基づく上下半身それぞれの独立した旋回処理
 * @param[in] cmd 移動および視線方向の入力コマンド
 * @param[in] dt 経過時間
 */
void BossRobot::UpdateRotation(const RobotCommand& cmd, float dt)
{
	// 下半身の旋回処理
	RobotCommand lowerCmd = cmd;
	lowerCmd.lookDirection = DirectX::SimpleMath::Vector3::Zero;
	Robot::UpdateRotation(lowerCmd, dt);

	if (GetHealth() <= 0.0f)
	{
		return;
	}

	// 上半身の旋回処理
	// ボス特有の重量感を表現するため、下半身とは独立してプレイヤー方向へゆっくりと旋回させる
	if (cmd.lookDirection.LengthSquared() > INPUT_DEADZONE_SQUARED)
	{
		DirectX::SimpleMath::Vector3 lookDir = cmd.lookDirection;
		lookDir.y = 0.0f;
		lookDir.Normalize();

		DirectX::SimpleMath::Quaternion targetRot = 
			DirectX::SimpleMath::Quaternion::LookRotation(-lookDir, DirectX::SimpleMath::Vector3::Up);

		m_upperRotation = DirectX::SimpleMath::Quaternion::Slerp(m_upperRotation, targetRot, UPPER_TURN_SPEED * dt);
	}
	else
	{
		// ターゲットが存在しない（見失った）場合は、上半身を下半身の正面方向へ滑らかに復帰させる
		m_upperRotation = DirectX::SimpleMath::Quaternion::Slerp(m_upperRotation, m_rotation, UPPER_RESET_TURN_SPEED * dt);
	}
}

/**
 * @brief ボスモデルの描画（下半身・上半身の合成）
 * @param[in] context デバイスコンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 * @param[in] alpha 描画の透明度
 */
void BossRobot::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, float alpha)
{
	// 親クラス側の処理で下半身を描画する
	Robot::Render(context, states, view, proj, alpha);

	if (!m_upperModel || !IsVisible() || alpha < ALPHA_VISIBLE_THRESHOLD) return;

	// 下半身の座標を基準に、上半身の取り付け位置をオフセットする
	DirectX::SimpleMath::Vector3 upperPos = m_position;
	upperPos.y += m_upperHeightOffset;

	DirectX::SimpleMath::Matrix upperWorld = 
		DirectX::SimpleMath::Matrix::CreateScale(MODEL_DEFAULT_SCALE) *
		DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_upperRotation) *
		DirectX::SimpleMath::Matrix::CreateRotationY(DirectX::XMConvertToRadians(MODEL_YAW_OFFSET_DEG)) *
		DirectX::SimpleMath::Matrix::CreateTranslation(upperPos);

	// 半透明フェード中かどうかでブレンドステートを切り替えて描画する
	if (alpha < ALPHA_OPAQUE_THRESHOLD)
	{
		m_upperModel->Draw(context, *states, upperWorld, view, proj, false, [=]() {
			context->OMSetBlendState(states->NonPremultiplied(), nullptr, 0xFFFFFFFF);
			});
	}
	else
	{
		m_upperModel->Draw(context, *states, upperWorld, view, proj);
	}
}