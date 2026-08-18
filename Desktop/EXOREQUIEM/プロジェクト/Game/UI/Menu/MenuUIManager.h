/*****************************************************************//**
 * @file    MenuUIManager.h
 * @brief   各シーン共通のメニューUI（レイアウト、入力、描画）を管理するコンポーネント
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <vector>

class MenuUIManager
{
public:
	struct MenuItem
	{
		DirectX::SimpleMath::Vector2 position;
		RECT hitRect = { 0, 0, 0, 0 };
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
		DirectX::SimpleMath::Vector2 size;
	};

	// コンストラクタ
	MenuUIManager();
	// デストラクタ
	~MenuUIManager() = default;
	// 初期化処理
	void Initialize(ID3D11ShaderResourceView* frameTexture, const DirectX::SimpleMath::Vector2& frameSize);
	// 画面サイズ変更時などに、各項目の座標と当たり判定を計算する
	void UpdateLayout(float guiScale, float startX, float startY, float stepY,
		float frameScaleX = 0.5f, float frameScaleY = 0.3f);
	// キーボードとマウスの入力から、現在選択されている項目を更新する
	void UpdateSelection(const DirectX::Keyboard::State& kb, const DirectX::Mouse::State& mouse);
	// 描画処理
	void Draw(DirectX::SpriteBatch* spriteBatch, float guiScale,
		const DirectX::SimpleMath::Color& defaultColor = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f),
		bool enableBlink = false, float blinkAlpha = 0.5f);

	// メニュー項目を追加する
	void AddItem(ID3D11ShaderResourceView* itemTexture, const DirectX::SimpleMath::Vector2& itemSize);

	// 全項目を削除する
	void ClearItems();

	// 決定操作が行われたか判定する
	bool IsExecuted(bool isEnterPressed, bool isMouseClicked) const;

	// 現在選択されているインデックスを取得
	int GetSelectedIndex() const { return m_selectedIndex; }
	// 現在選択されているインデックスを設定
	void SetSelectedIndex(int index) { m_selectedIndex = index; }

	// マウスが何らかのメニュー上にあるか判定する
	bool IsMouseOverAny(int mouseX, int mouseY) const;

private:
	//--- 定数 ---
	static constexpr float HALF_RATIO = 0.5f;                    //< 中心や半分を計算するための割合
	static constexpr float SELECTED_EFFECT_SCALE = 1.1f;         //< 選択中の拡大倍率
	static constexpr float TEXT_SCALE_X_MULTIPLIER = 0.8f;       //< 枠に対する文字のXスケール調整係数
	static constexpr float TEXT_SCALE_Y_MULTIPLIER = 0.7f;       //< 枠に対する文字のYスケール調整係数

	//--- 内部変数 ---
	std::vector<MenuItem> m_items;                               //< メニュー項目のリスト
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frameTexture; //< 選択枠のテクスチャ
	DirectX::SimpleMath::Vector2 m_frameSize;                    //< 選択枠のサイズ

	//--- 選択状態の管理 ---
	int m_selectedIndex;                                         //< 現在選択中のインデックス
	float m_frameScaleX;                                         //< 枠の横スケール
	float m_frameScaleY;                                         //< 枠の縦スケール

	//--- 入力状態の管理 ---
	bool m_upLast;                                               //< 1フレーム前のUp入力状態
	bool m_downLast;                                             //< 1フレーム前のDown入力状態
	int m_lastMouseX;                                            //< 1フレーム前のマウスX座標
	int m_lastMouseY;                                            //< 1フレーム前のマウスY座標
};