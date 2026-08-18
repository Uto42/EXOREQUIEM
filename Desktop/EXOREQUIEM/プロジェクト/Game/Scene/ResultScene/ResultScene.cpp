/*****************************************************************//**
 * @file    ResultScene.cpp
 * @brief   戦闘結果（リザルト）画面の表示、クリアランクの計算とスコア集計
 *
 * @author  名前
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Scene/ResultScene/ResultScene.h"
#include "Game/Scene/StandbyScene/StandbyScene.h"
#include "Game/Scene/GameplayScene/GameplayScene.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include <WICTextureLoader.h>
#include "Game/Systems/Sound/SoundManager.h"

// 静的メンバ変数の実体定義
bool  ResultScene::s_isGameClear = false;
float ResultScene::s_clearTime = 0.0f;
float ResultScene::s_remainingHP = 0.0f;
float ResultScene::s_maxHP = 100.0f;

// ランク表示位置
const DirectX::SimpleMath::Vector2 ResultScene::POS_RANK(960.0f, 500.0f);

/**
 * @brief コンストラクタ
 */
ResultScene::ResultScene()
	: m_backgroundRect({ 0, 0, 0, 0 })
	, m_blinkTimer(0.0f)
	, m_showText(true)
	, m_finalRank(0)
	, m_rankColor(1.0f, 1.0f, 1.0f, 1.0f)
	, m_sizeGuide(0.0f, 0.0f)
{
}

/**
 * @brief 初期化処理
 */
void ResultScene::Initialize()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

	// クリアランクの計算
	CalculateRank();
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime フレーム間経過時間
 */
void ResultScene::Update(float elapsedTime)
{
	// ユーザーが自由にUIを選択できるよう、マウスを絶対座標モードに戻す
	UpdateSystemState();

	auto kb = DirectX::Keyboard::Get().GetState();
	auto mouse = DirectX::Mouse::Get().GetState();

	// マウスやキーボードの入力をメニューコンポーネントに渡して選択状態を更新する
	m_menuManager.UpdateSelection(kb, mouse);

	// 決定・実行処理
	HandleMenuExecution();

	// アニメーション更新
	UpdateAnimation(elapsedTime);
}

/**
 * @brief マウスカーソルやシステム状態の更新
 */
void ResultScene::UpdateSystemState()
{
	// ゲーム中のカメラ操作用モードから、UI操作用の絶対座標モードにリセットする
	DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	// ウィンドウ外へのクリップを解除
	ClipCursor(nullptr);

	// カーソルを表示状態にする
	while (ShowCursor(TRUE) < 0) {}

	auto mouse = DirectX::Mouse::Get().GetState();
	m_mouseTracker.Update(mouse);
}

/**
 * @brief 決定操作の検出とメニュー項目の実行
 */
void ResultScene::HandleMenuExecution()
{
	auto kb = DirectX::Keyboard::Get().GetState();

	// マウスの左クリックが押された瞬間を検出する
	bool isClicked = (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED);
	static bool enterLast = false;
	bool isEnter = (kb.Enter && !enterLast);
	enterLast = kb.Enter;

	// メニューコンポーネント側にクリックやエンターキーが押されたかの判定を委譲する
	if (m_menuManager.IsExecuted(isEnter, isClicked))
	{
		SoundManager::Instance().PlaySE(L"SE_Decision");

		MenuType currentMenu = static_cast<MenuType>(m_menuManager.GetSelectedIndex());

		if (currentMenu == MenuType::Retry) {
			ChangeScene<GameplayScene, GameLoadingScreen>();
		}
		else {
			ChangeScene<StandbyScene, GameLoadingScreen>();
		}
	}
}

/**
 * @brief 点滅などの演出用タイマー更新
 * @param[in] elapsedTime フレーム間経過時間
 */
void ResultScene::UpdateAnimation(float elapsedTime)
{
	m_blinkTimer += elapsedTime;
	if (m_blinkTimer >= BLINK_INTERVAL)
	{
		m_showText = !m_showText;
		m_blinkTimer = 0.0f;
	}
}

/**
 * @brief 描画処理
 */
void ResultScene::Render()
{
	m_spriteBatch->Begin();

	// 背景とヘッダーの描画
	DrawBackground();
	DrawResultHeader();

	// メニュー（リトライ・戻る）の描画をコンポーネントに依頼する
	auto viewport = GetUserResources()->GetDeviceResources()->GetScreenViewport();
	float guiScale = viewport.Height / BASE_RESOLUTION_H;
	m_menuManager.Draw(m_spriteBatch.get(), guiScale);

	// ランクの描画
	DrawRank();
	DrawGuideUI();

	m_spriteBatch->End();
}

/**
 * @brief 背景の描画
 */
void ResultScene::DrawBackground()
{
	if (m_backgroundTexture)
	{
		m_spriteBatch->Draw(m_backgroundTexture.Get(), m_backgroundRect, DirectX::Colors::White);
	}
}

/**
 * @brief リザルトヘッダー（MISSION COMPLETE / FAILED）の描画
 */
void ResultScene::DrawResultHeader()
{
	auto viewport = GetUserResources()->GetDeviceResources()->GetScreenViewport();
	float guiScale = viewport.Height / BASE_RESOLUTION_H;
	float centerX = viewport.Width * HALF_RATIO;
	float centerY = viewport.Height * HEADER_Y_RATIO;

	// クリア状態に応じてテクスチャとサイズを選択
	ID3D11ShaderResourceView* tex = s_isGameClear ? m_texClear.Get() : m_texGameOver.Get();
	DirectX::SimpleMath::Vector2 size = s_isGameClear ? m_sizeClear : m_sizeGameOver;

	if (tex && size.y > 0)
	{
		// ヘッダーロゴは他のメニュー文字より目立たせるため、大きめに描画する
		float scale = (HEADER_TARGET_HEIGHT * guiScale) / size.y;

		m_spriteBatch->Draw(
			tex,
			DirectX::SimpleMath::Vector2(centerX, centerY),
			nullptr,
			DirectX::Colors::White,
			0.0f,
			size * HALF_RATIO,
			scale
		);
	}
}

/**
 * @brief ランク計算ロジック
 */
void ResultScene::CalculateRank()
{
	// ゲームオーバー時は無条件でCランクにする
	if (!s_isGameClear)
	{
		m_finalRank = 'C';
		return;
	}

	float hpRatio = (s_maxHP > 0) ? (s_remainingHP / s_maxHP) : 0.0f;

	// クリアタイムと残りHPの割合から総合ランクを決定する
	if (s_clearTime <= RANK_S_TIME_LIMIT && hpRatio >= RANK_S_HP_RATIO)
	{
		m_finalRank = 'S';
	}
	else if (s_clearTime <= RANK_A_TIME_LIMIT && hpRatio >= RANK_A_HP_RATIO)
	{
		m_finalRank = 'A';
	}
	else if (s_clearTime <= RANK_B_TIME_LIMIT && hpRatio >= RANK_B_HP_RATIO)
	{
		m_finalRank = 'B';
	}
	else
	{
		m_finalRank = 'C';
	}
}

/**
 * @brief ランク結果の描画
 */
void ResultScene::DrawRank()
{
	auto viewport = GetUserResources()->GetDeviceResources()->GetScreenViewport();
	float guiScale = viewport.Height / BASE_RESOLUTION_H;
	float W = static_cast<float>(viewport.Width);
	float H = static_cast<float>(viewport.Height);

	float rankY = H * RANK_Y_RATIO;
	float labelX = W * RANK_LABEL_X_RATIO;
	float valueX = W * RANK_VALUE_X_RATIO;

	// "RANK" ラベルの描画
	if (m_texRankLabel && m_sizeRankLabel.y > 0)
	{
		float scale = (RANK_LABEL_TARGET_HEIGHT * guiScale) / m_sizeRankLabel.y;
		m_spriteBatch->Draw(m_texRankLabel.Get(), DirectX::SimpleMath::Vector2(labelX, rankY), nullptr,
			DirectX::Colors::White, 0.0f, m_sizeRankLabel * HALF_RATIO, scale);
	}

	// 決定されたアルファベット (S, A, B, C) に応じたテクスチャを選択して描画する
	ID3D11ShaderResourceView* rankTex = nullptr;
	DirectX::SimpleMath::Vector2 rankSize = DirectX::SimpleMath::Vector2::Zero;

	switch (m_finalRank)
	{
	case 'S': rankTex = m_texRankS.Get(); rankSize = m_sizeRankS; break;
	case 'A': rankTex = m_texRankA.Get(); rankSize = m_sizeRankA; break;
	case 'B': rankTex = m_texRankB.Get(); rankSize = m_sizeRankB; break;
	default:  rankTex = m_texRankC.Get(); rankSize = m_sizeRankC; break;
	}

	if (rankTex && rankSize.y > 0)
	{
		float scale = (RANK_VALUE_TARGET_HEIGHT * guiScale) / rankSize.y;
		m_spriteBatch->Draw(rankTex, DirectX::SimpleMath::Vector2(valueX, rankY), nullptr,
			DirectX::Colors::White, 0.0f, rankSize * HALF_RATIO, scale);
	}
}

/**
 * @brief 終了処理
 */
void ResultScene::Finalize()
{
}

/**
 * @brief デバイス依存リソースの作成
 */
void ResultScene::CreateDeviceDependentResources()
{
	auto userRes = GetUserResources();
	auto device = userRes->GetDeviceResources()->GetD3DDevice();
	auto context = userRes->GetDeviceResources()->GetD3DDeviceContext();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

	LoadAssets(device);
}

/**
 * @brief テクスチャリソースのロード
 * @param[in] device Direct3Dデバイス
 */
void ResultScene::LoadAssets(ID3D11Device* device)
{
	// 画像ロード・サイズ取得ヘルパー
	auto LoadTexture = 
		[&](const wchar_t* path, ID3D11ShaderResourceView** srv, DirectX::SimpleMath::Vector2* outSize)
		{
			DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, path, nullptr, srv));
			Microsoft::WRL::ComPtr<ID3D11Resource> res;
			(*srv)->GetResource(res.GetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
			if (SUCCEEDED(res.As(&tex)))
			{
				D3D11_TEXTURE2D_DESC desc;
				tex->GetDesc(&desc);
				*outSize = 
					DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
			}
		};

	// 背景
	DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/Result/result_background.png",
		nullptr, m_backgroundTexture.ReleaseAndGetAddressOf()));

	// フォント・ラベル
	LoadTexture(L"Resources/Textures/Result/result_font_complete.png", m_texClear.ReleaseAndGetAddressOf(), &m_sizeClear);
	LoadTexture(L"Resources/Textures/Result/result_font_failed.png", m_texGameOver.ReleaseAndGetAddressOf(), &m_sizeGameOver);

	LoadTexture(L"Resources/Textures/Common/common_select.png", m_frameTexture.ReleaseAndGetAddressOf(), &m_frameSize);
	LoadTexture(L"Resources/Textures/Result/result_font_rank.png", m_texRankLabel.ReleaseAndGetAddressOf(), &m_sizeRankLabel);

	LoadTexture(L"Resources/Textures/Result/result_guide.png", m_guideTex.ReleaseAndGetAddressOf(), &m_sizeGuide);

	// ランク画像
	LoadTexture(L"Resources/Textures/Result/result_rank_s.png", m_texRankS.ReleaseAndGetAddressOf(), &m_sizeRankS);
	LoadTexture(L"Resources/Textures/Result/result_rank_a.png", m_texRankA.ReleaseAndGetAddressOf(), &m_sizeRankA);
	LoadTexture(L"Resources/Textures/Result/result_rank_b.png", m_texRankB.ReleaseAndGetAddressOf(), &m_sizeRankB);
	LoadTexture(L"Resources/Textures/Result/result_rank_c.png", m_texRankC.ReleaseAndGetAddressOf(), &m_sizeRankC);

	// コンポーネントに初期設定を渡す
	m_menuManager.Initialize(m_frameTexture.Get(), m_frameSize);

	// メニュー項目のロード
	auto LoadMenuTex = [&](const wchar_t* path)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex;
			DirectX::SimpleMath::Vector2 size;
			LoadTexture(path, tex.ReleaseAndGetAddressOf(), &size);
			m_menuManager.AddItem(tex.Get(), size);
		};

	LoadMenuTex(L"Resources/Textures/Result/result_font_retry.png");
	LoadMenuTex(L"Resources/Textures/Result/result_font_backStandby.png");
}

/**
 * @brief ウィンドウサイズ依存リソースの作成
 */
void ResultScene::CreateWindowSizeDependentResources()
{
	auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
	float W = static_cast<float>(size.right);
	float H = static_cast<float>(size.bottom);
	float guiScale = H / BASE_RESOLUTION_H;

	m_backgroundRect = { 0, 0, size.right, size.bottom };

	// メニューコンポーネントにレイアウトの再計算を依頼する
	float centerX = W * HALF_RATIO;
	float startY = H * MENU_START_Y_RATIO;
	float stepY = m_frameSize.y * MENU_FRAME_SCALE_Y * guiScale * MENU_SPACING_MULTIPLIER;

	m_menuManager.UpdateLayout(guiScale, centerX, startY, stepY, MENU_FRAME_SCALE_X, MENU_FRAME_SCALE_Y);
}

/**
 * @brief 操作ガイドUIを画面左下に描画する
 */
void ResultScene::DrawGuideUI()
{
	if (!m_guideTex || m_sizeGuide.y <= 0) return;

	auto viewport = GetUserResources()->GetDeviceResources()->GetScreenViewport();
	float guiScale = viewport.Height / BASE_RESOLUTION_H;
	float screenH = static_cast<float>(viewport.Height);

	// 解像度に依存せず、左下に固定するための座標計算
	float margin = GUIDE_MARGIN * guiScale;
	DirectX::SimpleMath::Vector2 pos(margin, screenH - margin);

	// 画像の左下を原点に設定する（配置座標から右上に向かって描画されるように）
	DirectX::SimpleMath::Vector2 origin(0.0f, m_sizeGuide.y);

	// 画面の高さに合わせてスケールを調整する
	float scale = (GUIDE_TARGET_HEIGHT * guiScale) / m_sizeGuide.y;

	m_spriteBatch->Draw(
		m_guideTex.Get(),
		pos,
		nullptr,
		DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, GUIDE_ALPHA), // 少し透明にして主張を抑える
		0.0f,
		origin,
		scale
	);
}

/**
 * @brief デバイスロスト時の処理
 */
void ResultScene::OnDeviceLost()
{
	m_spriteBatch.reset();
	m_backgroundTexture.Reset();
	m_texClear.Reset();
	m_texGameOver.Reset();
	m_frameTexture.Reset();
	m_texRankLabel.Reset();
	m_texRankS.Reset();
	m_texRankA.Reset();
	m_texRankB.Reset();
	m_texRankC.Reset();

	m_menuManager.ClearItems();
}