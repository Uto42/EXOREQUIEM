/*****************************************************************//**
 * @file    DebugFont.cpp
 * @brief   画面上にデバッグ情報をテキスト表示するためのクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 ********************************************************************/

#include "pch.h"
#include "DebugFont.h"
#include "DirectXHelpers.h"
#include "VertexTypes.h"

/**
 * @brief コンストラクタ
 * @param[in] device   DirectX11デバイス
 * @param[in] context  デバイスコンテキスト
 * @param[in] fileName フォントファイルのパス
 */
DebugFont::DebugFont(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	wchar_t const* fileName
)
	: m_fontHeight(0.0f)
{
	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
	m_spriteFont = std::make_unique<DirectX::SpriteFont>(device, fileName);

	// フォントの縦サイズを取得する
	m_fontHeight = m_spriteFont->GetLineSpacing();
}

/**
 * @brief デストラクタ
 */
DebugFont::~DebugFont()
{
	m_spriteFont.reset();
	m_spriteBatch.reset();
}

/**
 * @brief 描画する文字列を登録する関数
 * @param[in] string 表示文字列
 * @param[in] pos 表示座標
 * @param[in] color 描画色
 * @param[in] scale 拡大率
 */
void DebugFont::AddString(
	const wchar_t* string,
	DirectX::SimpleMath::Vector2 pos,
	DirectX::FXMVECTOR color,
	float scale
)
{
	String str;

	str.string = std::wstring(string);
	str.pos = pos;
	str.color = color;
	str.scale = scale;

	m_strings.push_back(str);
}

/**
 * @brief 描画処理
 * @param[in] states DirectXのステート
 */
void DebugFont::Render(
	DirectX::CommonStates* states
)
{
	m_spriteBatch->Begin(
		DirectX::SpriteSortMode_Deferred,
		nullptr,
		nullptr,
		states->DepthNone(),
		states->CullCounterClockwise()
	);

	for (size_t i = 0; i < m_strings.size(); i++)
	{
		m_spriteFont->DrawString(
			m_spriteBatch.get(),
			m_strings[i].string.c_str(),
			m_strings[i].pos,
			m_strings[i].color,
			0.0f,
			DirectX::SimpleMath::Vector2(0.0f, 0.0f),
			m_strings[i].scale
		);
	}

	m_spriteBatch->End();

	// 次フレームのために登録内容を破棄
	m_strings.clear();
}

// --- DebugFont3D ---

/**
 * @brief コンストラクタ（3D版）
 * @param[in] device DirectX11デバイス
 * @param[in] context デバイスコンテキスト
 * @param[in] fileName フォントファイルのパス
 */
DebugFont3D::DebugFont3D(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	wchar_t const* fileName
)
	: DebugFont(device, context, fileName)
{
	// エフェクト作成
	m_effect = std::make_unique<DirectX::BasicEffect>(device);
	m_effect->SetTextureEnabled(true);
	m_effect->SetVertexColorEnabled(true);
	m_effect->SetLightingEnabled(false);

	// 入力レイアウト作成
	DX::ThrowIfFailed(
		CreateInputLayoutFromEffect(
			device,
			m_effect.get(),
			DirectX::VertexPositionColorTexture::InputElements,
			DirectX::VertexPositionColorTexture::InputElementCount,
			m_inputLayout.ReleaseAndGetAddressOf()
		)
	);
}

/**
 * @brief デストラクタ（3D版）
 */
DebugFont3D::~DebugFont3D()
{
	m_inputLayout.Reset();
	m_effect.reset();
}

/**
 * @brief 描画する文字列を登録する関数（3D版）
 * @param[in] string 表示文字列
 * @param[in] pos ワールド座標
 * @param[in] color 描画色
 * @param[in] scale 表示スケール
 */
void DebugFont3D::AddString(
	const wchar_t* string,
	DirectX::SimpleMath::Vector3 pos,
	DirectX::FXMVECTOR color,
	float scale
)
{
	String str;

	str.string = std::wstring(string);
	str.pos = pos;
	str.color = color;

	// スケール設定 1.0でおおよそ1mになるように調整している
	str.scale = scale / m_fontHeight;

	m_strings.push_back(str);
}

/**
 * @brief 描画処理（3D版）
 * @param[in] context デバイスコンテキスト
 * @param[in] states DirectXのステート
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 */
void DebugFont3D::Render(
	ID3D11DeviceContext* context,
	DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view,
	const DirectX::SimpleMath::Matrix& proj
)
{
	// スクリーン座標はY軸が＋－逆なので
	DirectX::SimpleMath::Matrix invertY = DirectX::SimpleMath::Matrix::CreateScale(1.0f, -1.0f, 1.0f);

	// ビュー行列の回転を打ち消す行列を作成する
	DirectX::SimpleMath::Matrix invView = view.Invert();
	invView._41 = 0.0f;
	invView._42 = 0.0f;
	invView._43 = 0.0f;

	// エフェクトにビュー行列と射影行列を設定する
	m_effect->SetView(view);
	m_effect->SetProjection(proj);

	for (size_t i = 0; i < m_strings.size(); i++)
	{
		m_spriteBatch->Begin(
			DirectX::SpriteSortMode_Deferred,
			nullptr,
			nullptr,
			states->DepthNone(),
			states->CullCounterClockwise(),
			[=]
			{
				// ワールド行列作成
				DirectX::SimpleMath::Matrix world = 
					invertY * invView * DirectX::SimpleMath::Matrix::CreateTranslation(m_strings[i].pos);

				m_effect->SetWorld(world);
				m_effect->Apply(context);
				context->IASetInputLayout(m_inputLayout.Get());
			}
		);

		// 中心座標を求める
		DirectX::SimpleMath::Vector2 textOrigin = 
			DirectX::SimpleMath::Vector2(m_spriteFont->MeasureString(m_strings[i].string.c_str())) / 2.0f;

		m_spriteFont->DrawString(
			m_spriteBatch.get(),
			m_strings[i].string.c_str(),
			DirectX::SimpleMath::Vector2::Zero,
			m_strings[i].color,
			0.0f,
			textOrigin,
			m_strings[i].scale
		);

		m_spriteBatch->End();
	}

	m_strings.clear();
}