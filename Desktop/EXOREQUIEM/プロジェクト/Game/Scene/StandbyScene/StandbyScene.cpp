/*****************************************************************//**
 * @file    StandbyScene.cpp
 * @brief   出撃待機画面（機体展示・メニュー選択）の挙動と描画
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Scene/StandbyScene/StandbyScene.h"
#include "Game/Scene/GameplayScene/GameplayScene.h"
#include "Game/Scene/TitleScene/TitleScene.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include <WICTextureLoader.h>
#include "Game/Systems/Sound/SoundManager.h"

/**
 * @brief コンストラクタ
 */
StandbyScene::StandbyScene()
{
}

/**
 * @brief 初期化処理
 */
void StandbyScene::Initialize()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

	// BGMを再生（ループ）
	SoundManager::Instance().PlayBGM(L"BGM_Standby");
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime 経過時間
 */
void StandbyScene::Update(float elapsedTime)
{
	// 絶対座標モードに戻す
	DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	// カーソルの位置制限を解除
	ClipCursor(nullptr);

	// カーソルを表示
	while (ShowCursor(TRUE) < CURSOR_VISIBLE_THRESHOLD) {}

	// 入力の基本更新
	auto mouse = DirectX::Mouse::Get().GetState();
	m_mouseTracker.Update(mouse);

	auto kb = DirectX::Keyboard::Get().GetState();
	m_keyboardTracker.Update(kb);

	// チュートリアル表示中の処理（表示中ならここで終了）
	if (HandleTutorialInput()) return;

	// 状態による分岐
	if (m_currentState == SceneState::MainMenu)
	{
		// コンポーネントに入力を投げて更新
		m_mainMenuManager.UpdateSelection(kb, mouse);
		HandleMenuExecution();
	}
	else if (m_currentState == SceneState::StageSelect)
	{
		// キャンセル（バックスペースやエスケープ、右クリックでメインメニューに戻る）
		if (m_keyboardTracker.pressed.Escape || m_keyboardTracker.pressed.Back ||
			m_mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED)
		{
			m_currentState = SceneState::MainMenu;
		}
		else
		{
			// コンポーネントに入力を投げて更新
			m_stageMenuManager.UpdateSelection(kb, mouse);
			HandleStageExecution();
		}
	}

	// カメラとタイマーの更新
	UpdateCamera(elapsedTime);
}

/**
 * @brief チュートリアル表示中の入力待ち処理
 * @return true: チュートリアル表示中のため、以降の更新をスキップする
 */
bool StandbyScene::HandleTutorialInput()
{
	if (!m_isShowingHowToPlay) return false;

	// チュートリアル表示中は、Enterキーまたはマウスクリックで閉じる
	if (m_keyboardTracker.pressed.Enter ||
		m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED ||
		m_mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED)
	{
		m_isShowingHowToPlay = false; // 閉じる
	}

	return true;
}

/**
 * @brief 決定操作の検出とメニュー項目の実行
 */
void StandbyScene::HandleMenuExecution()
{
	bool isKeyAction = m_keyboardTracker.pressed.Enter;
	bool isClickAction = (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED);

	// コンポーネントに「決定されたか？」を聞く
	if (m_mainMenuManager.IsExecuted(isKeyAction, isClickAction))
	{
		SoundManager::Instance().PlaySE(L"SE_Decision");

		MenuType currentMenu = static_cast<MenuType>(m_mainMenuManager.GetSelectedIndex());

		switch (currentMenu)
		{
		case MenuType::Select:
			m_currentState = SceneState::StageSelect;
			break;

		case MenuType::HowToPlay:
			m_isShowingHowToPlay = true;
			break;

		case MenuType::Title:
			ChangeScene<TitleScene>();
			break;
		}
	}
}

/**
 * @brief ステージ選択の決定処理
 */
void StandbyScene::HandleStageExecution()
{
	bool isKeyAction = m_keyboardTracker.pressed.Enter;
	bool isClickAction = (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED);

	// コンポーネントに「決定されたか？」を聞く
	if (m_stageMenuManager.IsExecuted(isKeyAction, isClickAction))
	{
		SoundManager::Instance().PlaySE(L"SE_Decision");

		StageMenuType currentMenu = static_cast<StageMenuType>(m_stageMenuManager.GetSelectedIndex());

		switch (currentMenu)
		{
		case StageMenuType::Training:
			GameplayScene::s_nextStageType = StageType::Training;
			ChangeScene<GameplayScene, GameLoadingScreen>();
			break;

		case StageMenuType::Stage1:
			GameplayScene::s_nextStageType = StageType::Stage1;
			ChangeScene<GameplayScene, GameLoadingScreen>();
			break;

		case StageMenuType::Stage2:
			GameplayScene::s_nextStageType = StageType::Stage2;
			ChangeScene<GameplayScene, GameLoadingScreen>();
			break;

		case StageMenuType::Stage3:
			GameplayScene::s_nextStageType = StageType::Stage3;
			ChangeScene<GameplayScene, GameLoadingScreen>();
			break;
		}
	}
}

/**
 * @brief カメラ行列とタイマーの更新
 */
void StandbyScene::UpdateCamera(float elapsedTime)
{
	m_timer += elapsedTime;

	// カメラ設定
	m_view = DirectX::SimpleMath::Matrix::CreateLookAt(CAMERA_POS, CAMERA_TARGET, DirectX::SimpleMath::Vector3::Up);
}

/**
 * @brief 描画処理
 */
void StandbyScene::Render()
{
	// 背景描画 (2D)
	DrawBackground();

	// プレイヤーモデル描画 (3D)
	DrawPlayerModel();

	// UIおよび操作説明画像の描画 (2D)
	m_spriteBatch->Begin();

	// 状態によって描画するUIをコンポーネントに任せる
	if (m_currentState == SceneState::MainMenu)
	{
		m_mainMenuManager.Draw(m_spriteBatch.get(), m_guiScale);
	}
	else if (m_currentState == SceneState::StageSelect)
	{
		m_stageMenuManager.Draw(m_spriteBatch.get(), m_guiScale);
	}

	// 操作説明画像の描画
	DrawHowToPlay();
	// ガイドUIの描画（操作説明の補助）
	DrawGuideUI();

	m_spriteBatch->End();
}

/**
 * @brief 背景の描画
 */
void StandbyScene::DrawBackground()
{
	m_spriteBatch->Begin();
	if (m_backgroundTexture)
	{
		m_spriteBatch->Draw(m_backgroundTexture.Get(), m_backgroundRect, DirectX::Colors::White);
	}
	m_spriteBatch->End();
}

/**
 * @brief 展示用プレイヤーモデル（3D）の描画
 */
void StandbyScene::DrawPlayerModel()
{
	auto deviceResources = GetUserResources()->GetDeviceResources();
	auto context = deviceResources->GetD3DDeviceContext();
	auto states = GetUserResources()->GetCommonStates();

	// 奥行きバッファのクリア（背景の上に描画するため）
	context->ClearDepthStencilView(
		deviceResources->GetDepthStencilView(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
		CLEAR_DEPTH_VALUE, CLEAR_STENCIL_VALUE);

	// 配置行列の計算
	DirectX::SimpleMath::Matrix scale = DirectX::SimpleMath::Matrix::CreateScale(MODEL_RENDER_SCALE);

	DirectX::SimpleMath::Matrix rotation = 
		DirectX::SimpleMath::Matrix::CreateRotationY(m_timer * MODEL_ROTATION_SPEED);

	DirectX::SimpleMath::Matrix translation = 
		DirectX::SimpleMath::Matrix::CreateTranslation(MODEL_POSITION_X, MODEL_POSITION_Y, MODEL_POSITION_Z);

	DirectX::SimpleMath::Matrix world = scale * rotation * translation;

	// ライト・エフェクト設定
	m_playerModel->UpdateEffects([&](DirectX::IEffect* effect) {
		auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(effect);
		if (basicEffect) {
			basicEffect->SetWorld(world);
			basicEffect->SetView(m_view);
			basicEffect->SetProjection(m_proj);
			basicEffect->SetLightingEnabled(true);
			basicEffect->SetAmbientLightColor(DirectX::Colors::Gray);
		}
		});

	m_playerModel->Draw(context, *states, world, m_view, m_proj);
}

/**
 * @brief 操作説明画像の描画
 */
void StandbyScene::DrawHowToPlay()
{
	if (!m_isShowingHowToPlay) return;

	// 画面中央に表示するための座標計算
	auto outputSize = GetUserResources()->GetDeviceResources()->GetOutputSize();
	float screenW = static_cast<float>(outputSize.right);
	float screenH = static_cast<float>(outputSize.bottom);
	DirectX::SimpleMath::Vector2 screenCenter(screenW * HALF_RATIO, screenH * HALF_RATIO);

	// 画面の高さに比例してリサイズ表示
	float tutorialScale = (screenH * TUTORIAL_IMAGE_SCALE_RATIO) / m_sizeHowToPlayImage.y;

	// 描画
	m_spriteBatch->Draw(
		m_howToPlayTexture.Get(),
		screenCenter,
		nullptr,
		DirectX::Colors::White,
		SPRITE_ROTATION_NONE,
		m_sizeHowToPlayImage * HALF_RATIO,
		tutorialScale
	);
}

/**
 * @brief 終了処理
 */
void StandbyScene::Finalize()
{
}

/**
 * @brief デバイス依存リソースの作成
 */
void StandbyScene::CreateDeviceDependentResources()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

	// モデルの読み込み
	LoadModel(device);

	// テクスチャアセットの読み込み
	LoadAssets(device);
}

/**
 * @brief モデルリソースのロード
 * @param[in] device  Direct3Dデバイス
 */
void StandbyScene::LoadModel(ID3D11Device* device)
{
	DirectX::EffectFactory fx(device);
	fx.SetDirectory(L"Resources/Models/Robot");
	m_playerModel = DirectX::Model::CreateFromSDKMESH(device, L"Resources/Models/Robot/Robot.sdkmesh", fx);
}

/**
 * @brief テクスチャリソースのロード
 * @param[in] device  Direct3Dデバイス
 */
void StandbyScene::LoadAssets(ID3D11Device* device)
{
	// 背景ロード
	DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/Standby/standby_background.jpg",
		nullptr, m_backgroundTexture.ReleaseAndGetAddressOf()));

	// 選択枠（フレーム）のロードとサイズ取得
	DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/Common/common_select.png",
		nullptr, m_frameTexture.ReleaseAndGetAddressOf()));

	Microsoft::WRL::ComPtr<ID3D11Resource> resFrame;
	m_frameTexture->GetResource(resFrame.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texFrame;
	if (SUCCEEDED(resFrame.As(&texFrame)))
	{
		D3D11_TEXTURE2D_DESC desc;
		texFrame->GetDesc(&desc);
		m_frameSize = DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
	}

	// コンポーネントにフレーム画像を設定
	m_mainMenuManager.Initialize(m_frameTexture.Get(), m_frameSize);
	m_stageMenuManager.Initialize(m_frameTexture.Get(), m_frameSize);

	// 単体画像ロード用ヘルパー
	auto LoadTexture = [&](const wchar_t* path, ID3D11ShaderResourceView** srv, DirectX::SimpleMath::Vector2* outSize)
		{
			DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, path, nullptr, srv));
			Microsoft::WRL::ComPtr<ID3D11Resource> res;
			(*srv)->GetResource(res.GetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
			if (SUCCEEDED(res.As(&tex)))
			{
				D3D11_TEXTURE2D_DESC desc;
				tex->GetDesc(&desc);
				*outSize = DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
			}
		};

	// チュートリアル画像ロード
	LoadTexture(L"Resources/Textures/Standby/standby_HowToPlay_guide.png",
		m_howToPlayTexture.ReleaseAndGetAddressOf(), &m_sizeHowToPlayImage);

	LoadTexture(L"Resources/Textures/Standby/standby_guide_menu.png",
		m_guideMenuTex.ReleaseAndGetAddressOf(), &m_sizeGuideMenu);
	LoadTexture(L"Resources/Textures/Standby/standby_guide_stage.png",
		m_guideStageTex.ReleaseAndGetAddressOf(), &m_sizeGuideStage);
	LoadTexture(L"Resources/Textures/Standby/standby_guide_howtoplay.png",
		m_guideHowtoplayTex.ReleaseAndGetAddressOf(), &m_sizeGuideHowtoplay);

	// --- メインメニューのロード ---
	auto LoadMainMenuTex = [&](const wchar_t* path)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex;
			DirectX::SimpleMath::Vector2 size;
			LoadTexture(path, tex.ReleaseAndGetAddressOf(), &size);
			m_mainMenuManager.AddItem(tex.Get(), size);
		};

	LoadMainMenuTex(L"Resources/Textures/Standby/standby_font_select.png");
	LoadMainMenuTex(L"Resources/Textures/Standby/standby_font_HowToPlay.png");
	LoadMainMenuTex(L"Resources/Textures/Standby/standby_font_returnTitle.png");

	// --- ステージ選択メニューのロード ---
	auto LoadStageMenuTex = [&](const wchar_t* path)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex;
			DirectX::SimpleMath::Vector2 size;
			LoadTexture(path, tex.ReleaseAndGetAddressOf(), &size);
			m_stageMenuManager.AddItem(tex.Get(), size);
		};

	LoadStageMenuTex(L"Resources/Textures/Standby/standby_font_training.png");
	LoadStageMenuTex(L"Resources/Textures/Standby/standby_font_stage1.png");
	LoadStageMenuTex(L"Resources/Textures/Standby/standby_font_stage2.png");
	LoadStageMenuTex(L"Resources/Textures/Standby/standby_font_boss.png");
}

/**
 * @brief ウィンドウサイズ依存リソースの作成
 */
void StandbyScene::CreateWindowSizeDependentResources()
{
	auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
	float W = static_cast<float>(size.right);
	float H = static_cast<float>(size.bottom);
	m_guiScale = H / BASE_RESOLUTION_H;

	// カメラ設定 (初期投影行列の生成)
	m_view = DirectX::SimpleMath::Matrix::CreateLookAt(
		CAMERA_POS, CAMERA_TARGET, DirectX::SimpleMath::Vector3::Up);

	m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		DirectX::XM_PIDIV4, W / H, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

	// レイアウト（メニュー配置・当たり判定）の計算
	UpdateMenuLayout(W, H);

	m_backgroundRect = { RECT_ORIGIN_COORD, RECT_ORIGIN_COORD, size.right, size.bottom };
}

/**
 * @brief メニューのレイアウト（座標・当たり判定）を再計算する
 * @param[in] W  画面の横幅
 * @param[in] H  画面の縦幅
 */
void StandbyScene::UpdateMenuLayout(float W, float H)
{
	float frameScaleX = MENU_FRAME_SCALE_X;
	float frameScaleY = MENU_FRAME_SCALE_Y;

	// コンポーネントにメインメニューのレイアウト計算を依頼
	float mainBaseX = W * MENU_BASE_X_RATIO;
	float mainBaseY = H * MENU_BASE_Y_RATIO;
	float mainStepY = MENU_LINE_SPACING * m_guiScale;

	m_mainMenuManager.UpdateLayout(m_guiScale, mainBaseX, mainBaseY, mainStepY, frameScaleX, frameScaleY);

	// コンポーネントにステージ選択メニューのレイアウト計算を依頼
	float stageBaseX = W * MENU_BASE_X_RATIO;
	float stageBaseY = H * MENU_BASE_Y_RATIO;
	float stageStepY = (STAGE_MENU_LINE_SPACING + STAGE_MENU_Y_OFFSET) * m_guiScale;

	m_stageMenuManager.UpdateLayout(m_guiScale, stageBaseX, stageBaseY, stageStepY, frameScaleX, frameScaleY);
}

/**
 * @brief 状態に応じた操作ガイドUIを画面左下に描画する
 */
void StandbyScene::DrawGuideUI()
{
	ID3D11ShaderResourceView* texToDraw = nullptr;
	DirectX::SimpleMath::Vector2 sizeToDraw = DirectX::SimpleMath::Vector2::Zero;

	// 現在の状態に応じて描画する画像を決定
	if (m_isShowingHowToPlay)
	{
		texToDraw = m_guideHowtoplayTex.Get();
		sizeToDraw = m_sizeGuideHowtoplay;
	}
	else if (m_currentState == SceneState::MainMenu)
	{
		texToDraw = m_guideMenuTex.Get();
		sizeToDraw = m_sizeGuideMenu;
	}
	else if (m_currentState == SceneState::StageSelect)
	{
		texToDraw = m_guideStageTex.Get();
		sizeToDraw = m_sizeGuideStage;
	}

	if (!texToDraw || sizeToDraw.y <= 0) return;

	auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
	float screenH = static_cast<float>(size.bottom);

	// 解像度に依存せず、左下に固定するための座標計算
	float margin = GUIDE_MARGIN * m_guiScale;
	DirectX::SimpleMath::Vector2 pos(margin, screenH - margin);

	// 画像の左下を原点に設定する（配置座標から右上に向かって描画されるように）
	DirectX::SimpleMath::Vector2 origin(SPRITE_ORIGIN_X_LEFT, sizeToDraw.y);

	// ガイドのスケール（画面の高さに合わせてリサイズ）
	float scale = (GUIDE_TARGET_HEIGHT * m_guiScale) / sizeToDraw.y;

	m_spriteBatch->Draw(
		texToDraw,
		pos,
		nullptr,
		GUIDE_COLOR,
		SPRITE_ROTATION_NONE,
		origin,
		scale
	);
}

/**
 * @brief デバイスロスト時の処理
 */
void StandbyScene::OnDeviceLost()
{
	m_spriteBatch.reset();
	m_playerModel.reset();
	m_backgroundTexture.Reset();
	m_frameTexture.Reset();

	m_mainMenuManager.ClearItems();
	m_stageMenuManager.ClearItems();
}