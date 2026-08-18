/*****************************************************************//**
 * @file    TitleScene.cpp
 * @brief   ゲーム起動後のタイトル画面、ロゴ表示および入力によるシーン遷移制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include <fstream>
#include "Game/Scene/TitleScene/TitleScene.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include "Game/Scene/GameplayScene/GameplayScene.h"
#include "Game/Scene/StandbyScene/StandbyScene.h"
#include <WICTextureLoader.h>
#include "Game/Systems/Sound/SoundManager.h"

/**
 * @brief コンストラクタ
 */
TitleScene::TitleScene()
	: m_time(0.0f)
	, m_blinkTimer(0.0f)
	, m_showText(true)
	, m_titlePosition(DirectX::SimpleMath::Vector2::Zero)
	, m_titleScale(1.0f)
	, m_backgroundRect({ 0, 0, 0, 0 })
	, m_frameSize(DirectX::SimpleMath::Vector2::Zero)
{
}

/**
 * @brief 初期化処理
 */
void TitleScene::Initialize()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

	// タイトルBGMを再生（ループ）
	SoundManager::Instance().PlayBGM(L"BGM_Title");
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime 経過時間
 */
void TitleScene::Update(float elapsedTime)
{
	// 経過時間を蓄積
	m_time += elapsedTime;

	// リサイズ監視
	HandleWindowResize();

	auto kb = DirectX::Keyboard::Get().GetState();
	auto mouse = DirectX::Mouse::Get().GetState();
	m_mouseTracker.Update(mouse);

	// 選択状態の更新をコンポーネントに依頼
	m_menuManager.UpdateSelection(kb, mouse);

	// 点滅タイマーの更新
	m_blinkTimer += elapsedTime;
	if (m_blinkTimer >= BLINK_INTERVAL)
	{
		m_showText = !m_showText;
		m_blinkTimer = 0.0f;
	}

	// 決定・実行処理
	HandleMenuExecution();

	// 定数バッファのデータを更新してGPUへ転送する
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	if (m_timeConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = context->Map(m_timeConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (SUCCEEDED(hr))
		{
			EyeGlowCB* constantBufferData = static_cast<EyeGlowCB*>(mappedResource.pData);

			constantBufferData->eyeUV.x = EYE_PIXEL_X / BG_TEXTURE_WIDTH;
			constantBufferData->eyeUV.y = EYE_PIXEL_Y / BG_TEXTURE_HEIGHT;
			constantBufferData->uvRadius = EYE_GLOW_RADIUS / BG_TEXTURE_WIDTH;

			constantBufferData->time = m_time;

			context->Unmap(m_timeConstantBuffer.Get(), 0);
		}
	}
}

/**
 * @brief ウィンドウサイズの変化を監視し、必要に応じてリソースを再生成する
 */
void TitleScene::HandleWindowResize()
{
	auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
	static long lastWidth = 0;
	static long lastHeight = 0;

	// 横幅または高さが変わった瞬間だけレイアウトを再計算する
	if (size.right != lastWidth || size.bottom != lastHeight) {
		CreateWindowSizeDependentResources();
		lastWidth = size.right;
		lastHeight = size.bottom;
	}
}

/**
 * @brief 決定操作を検出し、シーン遷移を実行する
 */
void TitleScene::HandleMenuExecution()
{
	auto kb = DirectX::Keyboard::Get().GetState();

	// 決定キー・クリック判定
	static bool enterLast = false;
	bool isEnter = (kb.Enter && !enterLast);
	bool isClicked = (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED);
	enterLast = kb.Enter;

	// コンポーネントに決定されたか判定してもらう
	if (m_menuManager.IsExecuted(isEnter, isClicked))
	{
		SoundManager::Instance().PlaySE(L"SE_Decision");

		// 選択されているメニューに応じて処理を分ける
		MenuType currentMenu = static_cast<MenuType>(m_menuManager.GetSelectedIndex());

		switch (currentMenu)
		{
		case MenuType::Start:
			SoundManager::Instance().StopBGM();
			ChangeScene<StandbyScene>();
			break;

		case MenuType::Quit:
			SoundManager::Instance().StopBGM();
			PostQuitMessage(0);
			break;

		default:
			break;
		}
	}
}

/**
 * @brief 描画処理
 */
void TitleScene::Render()
{
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	// カスタムシェーダーによる背景描画
	if (m_eyeGlowVS && m_eyeGlowPS && m_titleTexture)
	{
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->VSSetShader(m_eyeGlowVS.Get(), nullptr, 0);
		context->PSSetShader(m_eyeGlowPS.Get(), nullptr, 0);
		context->PSSetConstantBuffers(0, 1, m_timeConstantBuffer.GetAddressOf());

		ID3D11ShaderResourceView* srvs[] = { m_titleTexture.Get() };
		context->PSSetShaderResources(0, 1, srvs);

		ID3D11SamplerState* samplers[] = { m_samplerState.Get() };
		context->PSSetSamplers(0, 1, samplers);

		// フルスクリーンポリゴンを描画するためのトリック描画（頂点バッファ不要）
		context->Draw(TRICK_VERTEX_COUNT, TRICK_VERTEX_OFFSET);

		ID3D11ShaderResourceView* nullSRV[] = { nullptr };
		context->PSSetShaderResources(0, 1, nullSRV);
	}

	m_spriteBatch->Begin();

	auto viewport = GetUserResources()->GetDeviceResources()->GetScreenViewport();
	float guiScale = viewport.Height / BASE_RESOLUTION_H;

	// 点滅フラグとアルファ値を渡してメニューを描画する
	m_menuManager.Draw(m_spriteBatch.get(), 
		guiScale, DirectX::SimpleMath::Color(DirectX::Colors::White), 
		!m_showText, BLINK_ALPHA_VALUE);

	m_spriteBatch->End();
}

/**
 * @brief 終了処理
 */
void TitleScene::Finalize()
{
}

/**
 * @brief デバイス依存リソースの作成
 */
void TitleScene::CreateDeviceDependentResources()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(
			device,
			L"Resources/Textures/Title/title_background.png",
			nullptr,
			m_titleTexture.ReleaseAndGetAddressOf()
		)
	);

	// --- 頂点シェーダーの読み込み ---
	std::wstring vsPath = L"Resources/Shaders/EyeGlowVS.cso";
	std::ifstream vsFile(vsPath, std::ios::binary | std::ios::ate);
	if (vsFile.is_open()) {
		std::streamsize shaderSize = vsFile.tellg();
		vsFile.seekg(0, std::ios::beg);
		std::vector<char> buffer(shaderSize);
		if (vsFile.read(buffer.data(), shaderSize)) {
			DX::ThrowIfFailed(device->CreateVertexShader(buffer.data(),
				buffer.size(), nullptr, m_eyeGlowVS.ReleaseAndGetAddressOf()));
		}
	}

	// --- ピクセルシェーダーの読み込み ---
	std::wstring psPath = L"Resources/Shaders/EyeGlowPS.cso";
	std::ifstream psFile(psPath, std::ios::binary | std::ios::ate);
	if (psFile.is_open()) {
		std::streamsize shaderSize = psFile.tellg();
		psFile.seekg(0, std::ios::beg);
		std::vector<char> buffer(shaderSize);
		if (psFile.read(buffer.data(), shaderSize)) {
			DX::ThrowIfFailed(device->CreatePixelShader(buffer.data(),
				buffer.size(), nullptr, m_eyeGlowPS.ReleaseAndGetAddressOf()));
		}
	}

	// --- サンプラーステートの作成 ---
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	DX::ThrowIfFailed(device->CreateSamplerState(&sampDesc, m_samplerState.ReleaseAndGetAddressOf()));

	// --- 定数バッファの生成 ---
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(EyeGlowCB);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX::ThrowIfFailed(device->CreateBuffer(&cbDesc, nullptr, m_timeConstantBuffer.ReleaseAndGetAddressOf()));

	// テクスチャアセットのロード
	LoadAssets(device);
}

/**
 * @brief ウィンドウサイズ依存リソースの作成
 */
void TitleScene::CreateWindowSizeDependentResources()
{
	auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();

	float W = static_cast<float>(size.right);
	float H = static_cast<float>(size.bottom);
	float guiScale = H / BASE_RESOLUTION_H;

	// 背景矩形の設定
	m_backgroundRect = { 0, 0, size.right, size.bottom };

	// メニューレイアウトの再計算を依頼する
	float centerX = W * MENU_BASE_X_RATIO + (MENU_X_OFFSET * guiScale);
	float startY = H * MENU_START_Y_RATIO + (MENU_Y_OFFSET * guiScale);
	float frameRealHeight = m_frameSize.y * MENU_FRAME_SCALE_Y * guiScale;
	float stepY = frameRealHeight * MENU_SPACING_MULTIPLIER;

	m_menuManager.UpdateLayout(guiScale, centerX, startY, stepY, MENU_FRAME_SCALE_X, MENU_FRAME_SCALE_Y);
}

/**
 * @brief 必要なテクスチャリソースをロードする
 * @param[in] device Direct3Dデバイス
 */
void TitleScene::LoadAssets(ID3D11Device* device)
{
	// 選択枠画像のロード
	DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device,
		L"Resources/Textures/Common/common_select.png", nullptr, m_frameTexture.ReleaseAndGetAddressOf()));

	// 選択枠画像のサイズを取得する
	Microsoft::WRL::ComPtr<ID3D11Resource> textureResource;
	m_frameTexture->GetResource(textureResource.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
	if (SUCCEEDED(textureResource.As(&texture2D))) {
		D3D11_TEXTURE2D_DESC textureDesc;
		texture2D->GetDesc(&textureDesc);
		m_frameSize = DirectX::SimpleMath::Vector2(static_cast<float>(textureDesc.Width), static_cast<float>(textureDesc.Height));
	}

	// 初期設定を渡す
	m_menuManager.Initialize(m_frameTexture.Get(), m_frameSize);

	// メニュー項目画像ロード用ヘルパー関数
	auto LoadMenuTex = [&](const wchar_t* path)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> itemTex;
			DirectX::SimpleMath::Vector2 itemSize;

			DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, path, nullptr, itemTex.ReleaseAndGetAddressOf()));

			Microsoft::WRL::ComPtr<ID3D11Resource> itemResource;
			itemTex->GetResource(itemResource.GetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11Texture2D> itemTexture2D;
			if (SUCCEEDED(itemResource.As(&itemTexture2D)))
			{
				D3D11_TEXTURE2D_DESC itemDesc;
				itemTexture2D->GetDesc(&itemDesc);
				itemSize = DirectX::SimpleMath::Vector2(static_cast<float>(itemDesc.Width), static_cast<float>(itemDesc.Height));
			}

			// コンポーネントに項目を追加
			m_menuManager.AddItem(itemTex.Get(), itemSize);
		};

	// 0番目: Start, 1番目: Quit として追加
	LoadMenuTex(L"Resources/Textures/Title/title_font_start.png");
	LoadMenuTex(L"Resources/Textures/Title/title_font_quit.png");
}

/**
 * @brief デバイスロスト時の処理
 */
void TitleScene::OnDeviceLost()
{
	m_spriteBatch.reset();
	m_titleTexture.Reset();
	m_frameTexture.Reset();

	m_eyeGlowPS.Reset();
	m_timeConstantBuffer.Reset();

	m_menuManager.ClearItems();
}