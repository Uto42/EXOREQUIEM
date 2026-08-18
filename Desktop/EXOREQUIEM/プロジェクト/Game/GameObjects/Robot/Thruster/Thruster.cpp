/*****************************************************************//**
 * @file    Thruster.cpp
 * @brief   機体スラスターの視覚効果：移動入力に応じた噴射エフェクトの動的生成および描画制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 * *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/Thruster/Thruster.h"
#include <WICTextureLoader.h>

// 定数の実体定義
const DirectX::SimpleMath::Vector3 Thruster::THRUSTER_COLOR(3.0f, 0.5f, 0.1f);
const DirectX::SimpleMath::Vector3 Thruster::MAIN_THRUSTER_BASE_DIR(0.0f, -0.5f, -1.0f);
const DirectX::SimpleMath::Vector3 Thruster::MODEL_BASE_DIR(0.0f, 1.0f, 0.0f);

/**
 * @brief コンストラクタ
 */
Thruster::Thruster()
	: m_uvOffset(0.0f)
{
}

/**
 * @brief デストラクタ
 */
Thruster::~Thruster()
{
	m_thrusters.clear();
}

/**
 * @brief 初期化処理
 * @param[in] device Direct3Dデバイス
 */
void Thruster::Initialize(ID3D11Device* device)
{
	DirectX::EffectFactory fx(device);
	fx.SetDirectory(L"Resources/Models");

	// モデル読み込み
	m_model = DirectX::Model::CreateFromSDKMESH(device, L"Resources/Models/Robot/ThrusterModel.sdkmesh", fx);

	// テクスチャ読み込み
	DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/Effects/effect_thruster.png", 
		nullptr, m_texture.ReleaseAndGetAddressOf());

	// エフェクト作成 (DGSLEffect)
	m_effect = std::make_unique<DirectX::DGSLEffect>(device, nullptr);
	m_effect->SetTextureEnabled(true);
	m_effect->SetVertexColorEnabled(false);
	m_effect->SetLightingEnabled(false); // 自己発光

	// 入力レイアウト作成
	if (m_model && !m_model->meshes.empty())
	{
		const auto& part = m_model->meshes[0]->meshParts[0];
		void const* shaderByteCode;
		size_t      byteCodeLength;
		m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		device->CreateInputLayout(
			part->vbDecl->data(),
			static_cast<UINT>(part->vbDecl->size()),
			shaderByteCode,
			byteCodeLength,
			m_inputLayout.ReleaseAndGetAddressOf()
		);
	}

	// --- スラスター配置設定 ---
	m_thrusters.clear();

	// メインスラスター (背中) の向き計算
	DirectX::SimpleMath::Vector3 mainDir = MAIN_THRUSTER_BASE_DIR;
	mainDir.Normalize();

	// 左右に一対配置
	m_thrusters.push_back({ DirectX::SimpleMath::Vector3( MAIN_THRUSTER_OFFSET_X, 
			MAIN_THRUSTER_OFFSET_Y, MAIN_THRUSTER_OFFSET_Z), mainDir, true, INTENSITY_MIN });

	m_thrusters.push_back({ DirectX::SimpleMath::Vector3(-MAIN_THRUSTER_OFFSET_X, 
		MAIN_THRUSTER_OFFSET_Y, MAIN_THRUSTER_OFFSET_Z), mainDir, true, INTENSITY_MIN });
}

/**
 * @brief 更新処理
 * @param[in] dt 経過時間
 * @param[in] isMoving 移動中フラグ
 * @param[in] isBack 後退中フラグ
 */
void Thruster::Update(float dt, bool isMoving, bool isBack)
{
	// UVスクロール更新
	m_uvOffset -= UV_SCROLL_SPEED * dt;
	if (m_uvOffset < -UV_WRAP_BOUND)
	{
		m_uvOffset += UV_WRAP_BOUND;
	}

	// 各スラスターの出力更新
	for (auto& thruster : m_thrusters)
	{
		float targetIntensity = INTENSITY_MIN;

		if (thruster.isMain)
		{
			if (isMoving) targetIntensity = INTENSITY_MAX;
		}
		else
		{
			if (isBack) targetIntensity = INTENSITY_MAX;
		}

		// 現在の出力から目標値へ徐々に変化させる (Lerp)
		float diff = targetIntensity - thruster.currentIntensity;
		thruster.currentIntensity += diff * INTENSITY_LERP_SPEED * dt;

		// クランプ処理
		if (thruster.currentIntensity < INTENSITY_MIN) thruster.currentIntensity = INTENSITY_MIN;
		if (thruster.currentIntensity > INTENSITY_MAX) thruster.currentIntensity = INTENSITY_MAX;
	}
}

/**
 * @brief 描画処理
 * @param[in] context コンテキスト
 * @param[in] states コモンステート
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 * @param[in] playerPos プレイヤー位置
 * @param[in] playerRot プレイヤー回転
 */
void Thruster::Render(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, 
	const DirectX::SimpleMath::Vector3& playerPos, const DirectX::SimpleMath::Quaternion& playerRot)
{
	if (!m_model || !m_effect || !m_inputLayout || !m_texture)
	{
		return;
	}

	// 描画設定
	context->OMSetBlendState(states->NonPremultiplied(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(states->DepthRead(), 0);

	// エフェクト設定
	m_effect->SetView(view);
	m_effect->SetProjection(proj);
	m_effect->SetTexture(m_texture.Get());

	// UV変換行列 (縦に縮めてスクロール)
	DirectX::SimpleMath::Matrix uvScale = DirectX::SimpleMath::Matrix::CreateScale(1.0f, UV_SCALE_Y, 1.0f);
	DirectX::SimpleMath::Matrix uvTrans = DirectX::SimpleMath::Matrix::CreateTranslation(0.0f, m_uvOffset, 0.0f);
	m_effect->SetUVTransform(uvScale * uvTrans);

	// 色設定
	m_effect->SetDiffuseColor(THRUSTER_COLOR);
	m_effect->SetEmissiveColor(THRUSTER_COLOR);
	m_effect->SetAlpha(EFFECT_DEFAULT_ALPHA);

	context->IASetInputLayout(m_inputLayout.Get());

	// 各スラスターを描画
	for (const auto& thruster : m_thrusters)
	{
		// 出力がほぼゼロなら描画しない
		if (thruster.currentIntensity < RENDER_MIN_INTENSITY) continue;

		// ワールド行列の計算と設定
		DirectX::SimpleMath::Matrix worldMat = CalculateThrusterWorldMatrix(thruster, playerPos, playerRot);
		m_effect->SetWorld(worldMat);
		m_effect->Apply(context);

		for (const auto& mesh : m_model->meshes)
		{
			for (const auto& part : mesh->meshParts)
			{
				part->Draw(context, m_effect.get(), m_inputLayout.Get());
			}
		}
	}

	// 設定を元に戻す
	context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(states->DepthDefault(), 0);
}

/**
 * @brief 単体スラスターのワールド行列計算
 * @param[in] config スラスター個体設定
 * @param[in] playerPos プレイヤー座標
 * @param[in] playerRot プレイヤー回転
 * @return ワールド変換行列
 */
DirectX::SimpleMath::Matrix Thruster::CalculateThrusterWorldMatrix(const ThrusterConfig& config,
	const DirectX::SimpleMath::Vector3& playerPos, const DirectX::SimpleMath::Quaternion& playerRot) const
{
	// 形状パラメータ
	float length = config.currentIntensity * MAX_EFFECT_LENGTH;
	float width = EFFECT_WIDTH;

	// スケール行列
	DirectX::SimpleMath::Matrix scaleMat = DirectX::SimpleMath::Matrix::CreateScale(width, length, width);

	// 回転行列 (モデル自体の基準方向から、指定の噴射方向へ向ける)
	DirectX::SimpleMath::Quaternion rotQ = 
		DirectX::SimpleMath::Quaternion::FromToRotation(MODEL_BASE_DIR, config.direction);

	// ワールド行列合成
	DirectX::SimpleMath::Matrix worldMat = 
		scaleMat
		* DirectX::SimpleMath::Matrix::CreateFromQuaternion(rotQ)
		* DirectX::SimpleMath::Matrix::CreateTranslation(config.offset)
		* DirectX::SimpleMath::Matrix::CreateFromQuaternion(playerRot)
		* DirectX::SimpleMath::Matrix::CreateTranslation(playerPos);

	return worldMat;
}