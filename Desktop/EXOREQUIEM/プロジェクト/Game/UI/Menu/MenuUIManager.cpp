/*****************************************************************//**
 * @file    MenuUIManager.cpp
 * @brief   各シーン共通のメニューUI（レイアウト、入力、描画）を管理するコンポーネントの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/Menu/MenuUIManager.h"

/**
 * @brief コンストラクタ
 */
MenuUIManager::MenuUIManager()
	: m_selectedIndex(0)
	, m_frameScaleX(0.5f)
	, m_frameScaleY(0.3f)
	, m_upLast(false)
	, m_downLast(false)
	, m_lastMouseX(0)
	, m_lastMouseY(0)
{
}

/**
 * @brief 枠の画像とサイズを設定する
 * @param[in] frameTexture 選択枠のテクスチャ
 * @param[in] frameSize 選択枠のサイズ
 */
void MenuUIManager::Initialize(ID3D11ShaderResourceView* frameTexture, 
	const DirectX::SimpleMath::Vector2& frameSize)
{
	m_frameTexture = frameTexture;
	m_frameSize = frameSize;
}

/**
 * @brief メニュー項目を追加する
 * @param[in] itemTexture メニュー項目のテクスチャ
 * @param[in] itemSize メニュー項目のサイズ
 */
void MenuUIManager::AddItem(ID3D11ShaderResourceView* itemTexture, const DirectX::SimpleMath::Vector2& itemSize)
{
	// メニュー項目の初期状態を設定してリストに登録する
	MenuItem item;
	item.position = DirectX::SimpleMath::Vector2::Zero;
	item.hitRect = { 0, 0, 0, 0 };
	item.texture = itemTexture;
	item.size = itemSize;
	m_items.push_back(item);
}

/**
 * @brief 全項目を削除する
 */
void MenuUIManager::ClearItems()
{
	// 登録されたメニュー項目を全て破棄し、選択状態も先頭にリセットする
	m_items.clear();
	m_selectedIndex = 0;
}

/**
 * @brief 画面サイズ変更時などに、各項目の座標と当たり判定を計算する
 * @param[in] guiScale GUIのスケール値
 * @param[in] startX メニューのX座標開始位置
 * @param[in] startY メニューのY座標開始位置
 * @param[in] stepY メニュー項目間の縦間隔
 * @param[in] frameScaleX 選択枠の横スケール
 * @param[in] frameScaleY 選択枠の縦スケール
 */
void MenuUIManager::UpdateLayout(float guiScale, float startX, float startY, float stepY,
	float frameScaleX, float frameScaleY)
{
	m_frameScaleX = frameScaleX;
	m_frameScaleY = frameScaleY;

	// マウス判定用矩形の半分の幅と高さを算出する
	float hitWidth = m_frameSize.x * frameScaleX * guiScale * HALF_RATIO;
	float hitHeight = m_frameSize.y * frameScaleY * guiScale * HALF_RATIO;

	// 各項目の中心座標を決定し、そこから当たり判定用の矩形領域を設定する
	for (size_t i = 0; i < m_items.size(); ++i)
	{
		m_items[i].position = DirectX::SimpleMath::Vector2(startX, startY + (i * stepY));

		m_items[i].hitRect.left = static_cast<long>(m_items[i].position.x - hitWidth);
		m_items[i].hitRect.right = static_cast<long>(m_items[i].position.x + hitWidth);
		m_items[i].hitRect.top = static_cast<long>(m_items[i].position.y - hitHeight);
		m_items[i].hitRect.bottom = static_cast<long>(m_items[i].position.y + hitHeight);
	}
}

/**
 * @brief キーボードとマウスの入力から、現在選択されている項目を更新する
 * @param[in] kb キーボードの入力状態
 * @param[in] mouse マウスの入力状態
 */
void MenuUIManager::UpdateSelection(const DirectX::Keyboard::State& kb, const DirectX::Mouse::State& mouse)
{
	if (m_items.empty()) return;

	int maxIndex = static_cast<int>(m_items.size());

	// キーボードの上下入力（W/Sキー含む）を検知する
	bool isUpNow = (kb.Up || kb.W);
	bool isDownNow = (kb.Down || kb.S);

	// 新たに上キーが押された場合は一つ上の項目へ移動（先頭なら末尾へループ）
	if (isUpNow && !m_upLast)
	{
		m_selectedIndex = (m_selectedIndex - 1 + maxIndex) % maxIndex;
	}
	// 新たに下キーが押された場合は一つ下の項目へ移動（末尾なら先頭へループ）
	if (isDownNow && !m_downLast)
	{
		m_selectedIndex = (m_selectedIndex + 1) % maxIndex;
	}

	// 押しっぱなしによる連続移動を防ぐため、今回の入力状態を保存する
	m_upLast = isUpNow;
	m_downLast = isDownNow;

	// マウスが実際に移動したかどうかを判定する
	bool hasMouseMoved = (mouse.x != m_lastMouseX || mouse.y != m_lastMouseY);
	m_lastMouseX = mouse.x;
	m_lastMouseY = mouse.y;

	// マウスが動いた場合のみ、マウス座標による項目のホバー判定を行う
	if (hasMouseMoved)
	{
		for (int i = 0; i < maxIndex; ++i)
		{
			const RECT& hitRectReference = m_items[i].hitRect;
			if (mouse.x >= hitRectReference.left && mouse.x <= hitRectReference.right &&
				mouse.y >= hitRectReference.top && mouse.y <= hitRectReference.bottom)
			{
				// マウスが重なっている項目を選択状態とする
				m_selectedIndex = i;
			}
		}
	}
}

/**
 * @brief マウスが何らかのメニュー上にあるか判定する
 * @param[in] mouseX マウスのX座標
 * @param[in] mouseY マウスのY座標
 * @return 判定結果
 */
bool MenuUIManager::IsMouseOverAny(int mouseX, int mouseY) const
{
	// 登録されている全項目の当たり判定とマウス座標を比較する
	for (const auto& item : m_items)
	{
		if (mouseX >= item.hitRect.left && mouseX <= item.hitRect.right &&
			mouseY >= item.hitRect.top && mouseY <= item.hitRect.bottom)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief 決定操作が行われたか判定する
 * @param[in] isEnterPressed エンターキーが押されたか
 * @param[in] isMouseClicked マウスがクリックされたか
 * @return 決定操作が実行された場合はtrue
 */
bool MenuUIManager::IsExecuted(bool isEnterPressed, bool isMouseClicked) const
{
	if (m_items.empty()) return false;

	bool isMouseHit = false;

	// マウスクリック時は、現在選択中の項目の上にマウスポインタが存在するか確認する
	if (isMouseClicked)
	{
		const RECT& hitRectReference = m_items[m_selectedIndex].hitRect;
		if (m_lastMouseX >= hitRectReference.left && m_lastMouseX <= hitRectReference.right &&
			m_lastMouseY >= hitRectReference.top && m_lastMouseY <= hitRectReference.bottom)
		{
			isMouseHit = true;
		}
	}

	// エンターキーが押されたか、有効な項目上でクリックされた場合に決定とみなす
	return isEnterPressed || isMouseHit;
}

/**
 * @brief メニュー項目を描画する
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] guiScale GUIのスケール値
 * @param[in] defaultColor デフォルトの文字色
 * @param[in] enableBlink 選択中の項目を点滅させるか
 * @param[in] blinkAlpha 点滅時のアルファ値
 */
void MenuUIManager::Draw(DirectX::SpriteBatch* spriteBatch, float guiScale,
	const DirectX::SimpleMath::Color& defaultColor, bool enableBlink, float blinkAlpha)
{
	if (!m_frameTexture || m_items.empty()) return;

	// 枠画像の中心を原点として設定する
	DirectX::SimpleMath::Vector2 frameOrigin = m_frameSize * HALF_RATIO;

	for (size_t i = 0; i < m_items.size(); ++i)
	{
		auto& item = m_items[i];
		if (!item.texture || item.size.y <= 0) continue;

		DirectX::SimpleMath::Vector2 position = item.position;
		bool isSelected = (static_cast<int>(i) == m_selectedIndex);
		
		// 選択中の項目はスケールを少し拡大して強調する
		float effectScale = isSelected ? SELECTED_EFFECT_SCALE : 1.0f;
		DirectX::SimpleMath::Vector2 scaleVector(
			m_frameScaleX * effectScale * guiScale, m_frameScaleY * effectScale * guiScale);

		// 土台となる選択枠を描画する（選択中は黄色、非選択時は白）
		spriteBatch->Draw(
			m_frameTexture.Get(), position, nullptr,
			isSelected ? DirectX::Colors::Yellow : DirectX::Colors::White,
			0.0f, frameOrigin, scaleVector
		);

		// 指定色をベースにしつつ、点滅フラグが有効なら選択中のみアルファ値を変更する
		DirectX::SimpleMath::Color textColor = defaultColor;
		if (isSelected && enableBlink)
		{
			textColor.A(blinkAlpha);
		}

		// テキスト画像が枠内に収まるよう、縦横それぞれの縮小率を計算して小さい方を採用する
		float textScaleY = (m_frameSize.y * m_frameScaleY * TEXT_SCALE_Y_MULTIPLIER * effectScale * guiScale) / item.size.y;
		float textScaleX = (m_frameSize.x * m_frameScaleX * TEXT_SCALE_X_MULTIPLIER * effectScale * guiScale) / item.size.x;
		float textScale = std::min(textScaleY, textScaleX);

		// 文字画像を枠の中心に重ねて描画する
		spriteBatch->Draw(
			item.texture.Get(), position, nullptr,
			textColor, 0.0f, item.size * HALF_RATIO, textScale
		);
	}
}