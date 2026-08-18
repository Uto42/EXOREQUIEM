/*****************************************************************//**
 * @file    GameLoadingScreen.cpp
 * @brief   ロード画面の描画および読み込み状況の進行表示
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include "Game/Scene/TitleScene/TitleScene.h"
#include "Game/Scene/GameplayScene/GameplayScene.h"
#include <WICTextureLoader.h>

bool GameLoadingScreen::s_isBooting = true;

// 定数の実体定義
const DirectX::SimpleMath::Vector2 GameLoadingScreen::BG_POSITION(-50.0f, 0.0f);

/**
 * @brief コンストラクタ
 */
GameLoadingScreen::GameLoadingScreen()
    : m_rotationAngle(0.0f)
    , m_iconOrigin(DirectX::SimpleMath::Vector2::Zero)
	, m_bgScale(1.0f)
	, m_iconScale(1.0f)
{
}

/**
 * @brief デストラクタ
 */
GameLoadingScreen::~GameLoadingScreen()
{
}

/**
 * @brief 初期化処理
 */
void GameLoadingScreen::Initialize()
{
	// デバイス依存リソースの作成
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime デルタタイム
 */
void GameLoadingScreen::Update(float elapsedTime)
{
    // サイズ変更検知
    auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
    static long lastWidth = 0;
    if (size.right != lastWidth) {
        CreateWindowSizeDependentResources();
        lastWidth = size.right;
    }

    // アイコンを回転させる
    m_rotationAngle += elapsedTime * ROTATION_SPEED; // 1秒で180度回転

	// 回転角度が2πを超えたら0に戻す
    if (m_rotationAngle >= DirectX::XM_2PI)
    {
        m_rotationAngle -= DirectX::XM_2PI;
    }


   if (s_isBooting)
   {
       // 起動時は、ロードが明けたらタイトル画面へ行く
       s_isBooting = false;
       ChangeScene<TitleScene>();
   }
   else
   {
       // 待機画面から来た通常時は、ゲーム本編へ行く
       ChangeScene<GameplayScene>();
   }

}

/**
 * @brief 描画処理
 */
void GameLoadingScreen::Render()
{
    m_spriteBatch->Begin();

    // 背景（ローディング画像）描画
    // 左上基準 (Origin = 0,0)
    m_spriteBatch->Draw(
        m_loadingTexture.Get(),
        BG_POSITION,
        nullptr,
        DirectX::Colors::White,
        0.0f,
        DirectX::SimpleMath::Vector2::Zero,
        m_bgScale
    );

    // ロード中アイコン描画
    // 中心基準 (Originを使用)
    // アイコン描画
    m_spriteBatch->Draw(
        m_loadingIconTexture.Get(),
        m_iconPosition,
        nullptr,
        DirectX::Colors::White,
        m_rotationAngle,
        m_iconOrigin,
        m_iconScale
    );

    m_spriteBatch->End();

}

/**
 * @brief 終了処理
 */
void GameLoadingScreen::Finalize()
{
}

/**
 * @brief デバイス依存リソースの作成
 */
void GameLoadingScreen::CreateDeviceDependentResources()
{
    auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
    auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

    // SpriteBatch 作成
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

    // 画像を読み込み
    DX::ThrowIfFailed(
        DirectX::CreateWICTextureFromFile(
            device,
            L"Resources/Textures/Loading/loading_Icon.png",
            nullptr,
            m_loadingIconTexture.ReleaseAndGetAddressOf()
        )
    );

	// アイコンの中心座標を計算
    if (m_loadingIconTexture)
    {
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        m_loadingIconTexture->GetResource(resource.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
        if (SUCCEEDED(resource.As(&texture2D)))
        {
            D3D11_TEXTURE2D_DESC desc;
            texture2D->GetDesc(&desc);
            m_iconOrigin = DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width) *
                HALF_RATIO, static_cast<float>(desc.Height) * HALF_RATIO);
        }
    }

    // 画像を読み込み
    DX::ThrowIfFailed(
        DirectX::CreateWICTextureFromFile(
            device,
            L"Resources/Textures/Loading/loading_background.png",
            nullptr,
            m_loadingTexture.ReleaseAndGetAddressOf()
        )
    );
}

/**
 * @brief ウィンドウサイズ依存リソースの作成
 */
void GameLoadingScreen::CreateWindowSizeDependentResources()
{
    auto size = GetUserResources()->GetDeviceResources()->GetOutputSize();
    float W = static_cast<float>(size.right);
    float H = static_cast<float>(size.bottom);

    // スケールの計算 (画面の高さに基づいたスケーリング例)
    float scale = H / BASE_HEIGHT;
    m_bgScale = scale;                              // 背景も画面に合わせる
    m_iconScale = scale * ICON_BASE_SCALE_RATIO;    // アイコンは背景に対して30%の大きさ

    // 位置の計算 (画面サイズに対する割合で配置)
    // 背景を中央に
    m_bgPosition = DirectX::SimpleMath::Vector2::Zero;

    // アイコンを右下に配置
    m_iconPosition = DirectX::SimpleMath::Vector2(W * ICON_POSITION_X_RATIO, H * ICON_POSITION_Y_RATIO);
}

/**
 * @brief デバイスロスト時の処理
 */
void GameLoadingScreen::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_loadingTexture.Reset();
    m_loadingIconTexture.Reset();

    Finalize();
}