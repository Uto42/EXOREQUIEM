/*****************************************************************//**
 * @file    DebugFont.h
 * @brief   画面上にデバッグ情報をテキスト表示するためのクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <string>

class DebugFont
{
private:
	// 2Dテキストの描画情報を保持する構造体
	struct String
	{
		DirectX::SimpleMath::Vector2 pos;			//< 表示位置
		std::wstring                 string;		//< 文字列
		DirectX::SimpleMath::Color   color;			//< 色
		float                        scale = 1.0f;  //< 拡大率
	};

	// 表示文字列の配列
	std::vector<String> m_strings;

protected:
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch; //< スプライトバッチ
	std::unique_ptr<DirectX::SpriteFont>  m_spriteFont;  //< スプライトフォント
	float                                 m_fontHeight;  //< フォントの縦サイズ

public:
	// コンストラクタ
	DebugFont(ID3D11Device* device, ID3D11DeviceContext* context, wchar_t const* fileName);

	// デストラクタ
	virtual ~DebugFont();

	// 書式指定付きの文字列を追加
	template <class... Args>
	void AddString(int x, int y, const DirectX::FXMVECTOR& color, const wchar_t* format, const Args& ... args)
	{
		int textLength = std::swprintf(nullptr, 0, format, args ...);
		if (textLength < 0) return;

		size_t bufferSize = static_cast<size_t>(textLength) + 1;
		std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(bufferSize);
		std::swprintf(buffer.get(), bufferSize, format, args ...);

		AddString(buffer.get(), DirectX::SimpleMath::Vector2{ static_cast<float>(x), static_cast<float>(y) }, color);
	}

	// 文字列を追加
	void AddString(
		const wchar_t* string,
		DirectX::SimpleMath::Vector2 pos,
		DirectX::FXMVECTOR color = DirectX::Colors::White,
		float scale = 1.0f
	);

	// 描画処理
	void Render(DirectX::CommonStates* states);

	// フォントの縦サイズを取得
	float GetFontHeight() const { return m_fontHeight; }
};

class DebugFont3D : protected DebugFont
{
private:
	// 3Dテキストの描画情報を保持する構造体
	struct String
	{
		DirectX::SimpleMath::Vector3 pos;			//< 3D座標
		std::wstring                 string;		//< 文字列
		DirectX::SimpleMath::Color   color;			//< 色
		float                        scale = 1.0f;  //< 拡大率
	};

	std::vector<String>                       m_strings;     //< 表示文字列の配列
	std::unique_ptr<DirectX::BasicEffect>     m_effect;      //< 描画用エフェクト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout; //< 入力レイアウト

public:
	// コンストラクタ
	DebugFont3D(ID3D11Device* device, ID3D11DeviceContext* context, wchar_t const* fileName);

	// デストラクタ
	~DebugFont3D();

	// 書式指定付きの文字列を追加
	template <class... Args>
	void AddString(DirectX::SimpleMath::Vector3 pos, const DirectX::FXMVECTOR& color, const wchar_t* format, const Args& ... args)
	{
		int textLength = std::swprintf(nullptr, 0, format, args ...);
		if (textLength < 0) return;

		size_t bufferSize = static_cast<size_t>(textLength) + 1;
		std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(bufferSize);
		std::swprintf(buffer.get(), bufferSize, format, args ...);

		AddString(buffer.get(), pos, color);
	}

	// 文字列を追加
	void AddString(
		const wchar_t* string,
		DirectX::SimpleMath::Vector3 pos,
		DirectX::FXMVECTOR color = DirectX::Colors::White,
		float scale = 1.0f
	);

	// 描画処理
	void Render(
		ID3D11DeviceContext* context,
		DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj
	);

	// フォントの縦サイズを取得
	float GetFontHeight() const { return m_fontHeight; }
};