/*****************************************************************//**
 * @file    PauseManager.cpp
 * @brief   ゲームの一時停止（ポーズ）状態の監視およびポーズ画面のオーバーレイ描画の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Pause/PauseManager.h"
#include <WICTextureLoader.h>

/**
 * @brief コンストラクタ
 */
PauseManager::PauseManager()
	: m_isPaused(false)
	, m_executedMenu(MenuType::None)
	, m_timer(0.0f)
	, m_guiScale(1.0f)
	, m_sizePauseLogo(DirectX::SimpleMath::Vector2::Zero)
	, m_frameSize(DirectX::SimpleMath::Vector2::Zero)
	, m_sizeHowToPlayImage(DirectX::SimpleMath::Vector2::Zero)
{
}

/**
 * @brief 初期化処理
 * @param[in] device ID3D11Device
 */
void PauseManager::Initialize(ID3D11Device* device)
{
	uint32_t whitePixelData = WHITE_PIXEL_VALUE;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initialData = {};
	initialData.pSysMem = &whitePixelData;
	initialData.SysMemPitch = sizeof(uint32_t);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> baseTexture;
	device->CreateTexture2D(&textureDesc, &initialData, baseTexture.GetAddressOf());
	device->CreateShaderResourceView(baseTexture.Get(), nullptr, m_blackTexture.GetAddressOf());

	// テクスチャロード用のヘルパー関数
	auto LoadTexture = 
		[&](const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
			DirectX::SimpleMath::Vector2& outSize)
		{
		if (SUCCEEDED(DirectX::CreateWICTextureFromFile(device, path, 
			nullptr, shaderResourceView.ReleaseAndGetAddressOf())))
		{
			Microsoft::WRL::ComPtr<ID3D11Resource> resource;
			shaderResourceView->GetResource(resource.GetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
			if (SUCCEEDED(resource.As(&texture2D))) 
			{
				D3D11_TEXTURE2D_DESC desc2D;
				texture2D->GetDesc(&desc2D);
				outSize = DirectX::SimpleMath::Vector2(static_cast<float>(desc2D.Width), static_cast<float>(desc2D.Height));
			}
		}
		};

	LoadTexture(L"Resources/Textures/Pause/pause_logo.png", m_pauseLogoTexture, m_sizePauseLogo);
	LoadTexture(L"Resources/Textures/Common/common_select.png", m_frameTexture, m_frameSize);
	LoadTexture(L"Resources/Textures/Pause/pause_HowToPlay_guide.png", m_howToPlayTexture, m_sizeHowToPlayImage);

	// コンポーネントの初期化
	m_menuUI.Initialize(m_frameTexture.Get(), m_frameSize);

	// メニュー用テクスチャのロード用ヘルパー関数
	auto LoadMenuTexture = [&](const wchar_t* path)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> itemTex;
			DirectX::SimpleMath::Vector2 itemSize = DirectX::SimpleMath::Vector2::Zero;

			DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, path, nullptr, itemTex.ReleaseAndGetAddressOf()));
			Microsoft::WRL::ComPtr<ID3D11Resource> resource;
			itemTex->GetResource(resource.GetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
			if (SUCCEEDED(resource.As(&texture2D)))
			{
				D3D11_TEXTURE2D_DESC desc2D;
				texture2D->GetDesc(&desc2D);
				itemSize = DirectX::SimpleMath::Vector2(static_cast<float>(desc2D.Width), static_cast<float>(desc2D.Height));
			}

			m_menuUI.AddItem(itemTex.Get(), itemSize);
		};

	// 登録順序をMenuTypeの定義（Resume, Retry, Title, Quit）と一致させる
	LoadMenuTexture(L"Resources/Textures/Pause/pause_font_resume.png");
	LoadMenuTexture(L"Resources/Textures/Pause/pause_font_retry.png");
	LoadMenuTexture(L"Resources/Textures/Pause/pause_font_return.png");
	LoadMenuTexture(L"Resources/Textures/Pause/pause_font_quit.png");
}

/**
 * @brief 更新処理
 * @param[in] kb キーボードの状態
 * @param[in] dt 経過時間
 * @return ポーズ中かどうか
 */
bool PauseManager::Update(const DirectX::Keyboard::State& kb, float dt)
{
	m_keyboardTracker.Update(kb);

	// ESCキーでの切り替え
	if (m_keyboardTracker.pressed.Escape)
	{
		m_isPaused = !m_isPaused;
		if (m_isPaused)
		{
			m_timer = 0.0f;
		}
		else
		{
			m_executedMenu = MenuType::Resume; // ESCで閉じた場合も再開扱い
		}
	}

	if (!m_isPaused)
	{
		// ポーズが解除されているときはカーソルを隠す
		while (ShowCursor(FALSE) >= CURSOR_VISIBLE_THRESHOLD) {}
		return false;
	}

	// ポーズ中のみマウスカーソルやシステム状態を強制更新
	DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
	ClipCursor(nullptr);
	while (ShowCursor(TRUE) < CURSOR_VISIBLE_THRESHOLD) {}

	// カーソル状態を最新にしてからマウス情報を取得
	auto mouse = DirectX::Mouse::Get().GetState();
	m_mouseTracker.Update(mouse);

	m_timer += dt;

	// 選択状態の更新をコンポーネントに委譲
	m_menuUI.UpdateSelection(kb, mouse);

	// 決定操作の検出と実行
	bool isEnter = m_keyboardTracker.pressed.Enter;
	bool isClicked = (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED);

	if (m_menuUI.IsExecuted(isEnter, isClicked))
	{
		m_executedMenu = static_cast<MenuType>(m_menuUI.GetSelectedIndex());

		if (m_executedMenu == MenuType::Resume)
		{
			m_isPaused = false;
			// 決定して閉じた瞬間もカーソルを隠す
			while (ShowCursor(FALSE) >= CURSOR_VISIBLE_THRESHOLD) {}
		}
	}

	return m_isPaused;
}

/**
 * @brief メニューのレイアウト再計算
 * @param[in] W 画面の横幅
 * @param[in] H 画面の縦幅
 */
void PauseManager::UpdateMenuLayout(float W, float H)
{
	m_guiScale = H / BASE_RESOLUTION_H;

	float baseX = W * MENU_BASE_X_RATIO;
	float baseY = H * MENU_BASE_Y_RATIO;
	float offsetV = MENU_LINE_SPACING * m_guiScale;

	// 配置と当たり判定の計算をコンポーネントに委譲
	m_menuUI.UpdateLayout(m_guiScale, baseX, baseY, offsetV, MENU_FRAME_SCALE_X, MENU_FRAME_SCALE_Y);
}

/**
 * @brief 描画処理
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] outputSize 描画領域のサイズ
 */
void PauseManager::Render(DirectX::SpriteBatch* spriteBatch, const RECT& outputSize)
{
	if (!m_isPaused) return;

	float screenW = static_cast<float>(outputSize.right);
	float screenH = static_cast<float>(outputSize.bottom);

	// レイアウト（座標・当たり判定）の計算
	UpdateMenuLayout(screenW, screenH);

	// 全体暗転
	spriteBatch->Draw(
		m_blackTexture.Get(), 
		outputSize, 
		nullptr, 
		DirectX::SimpleMath::Color(DirectX::Colors::Black) 
		* BACKGROUND_ALPHA);

	// 操作説明画像の描画
	if (m_howToPlayTexture && m_sizeHowToPlayImage.y > 0)
	{
		DirectX::SimpleMath::Vector2 rightCenter(screenW * GUIDE_POS_X_RATIO, screenH * GUIDE_POS_Y_RATIO);

		float tutorialScale = (screenH * GUIDE_TARGET_HEIGHT_RATIO) / m_sizeHowToPlayImage.y;

		spriteBatch->Draw(
			m_howToPlayTexture.Get(),
			rightCenter, 
			nullptr,
			DirectX::SimpleMath::Color(DirectX::Colors::White),
			0.0f,
			m_sizeHowToPlayImage 
			* HALF_RATIO, 
			tutorialScale
		);
	}

	// ポーズロゴの描画
	if (m_pauseLogoTexture)
	{
		float logoBlinkAlpha = (sinf(m_timer * BLINK_SPEED) * HALF_RATIO) + HALF_RATIO;

		DirectX::SimpleMath::Vector2 logoPos(screenW * MENU_BASE_X_RATIO, screenH * LOGO_POS_Y_RATIO);
		spriteBatch->Draw(
			m_pauseLogoTexture.Get(),
			logoPos, 
			nullptr, 
			DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, logoBlinkAlpha),
			0.0f, 
			m_sizePauseLogo 
			* HALF_RATIO, 
			m_guiScale);
	}

	// メニュー項目の描画をコンポーネントに委譲
	m_menuUI.Draw(spriteBatch, m_guiScale, DirectX::SimpleMath::Color(DirectX::Colors::White), false, 1.0f);
}