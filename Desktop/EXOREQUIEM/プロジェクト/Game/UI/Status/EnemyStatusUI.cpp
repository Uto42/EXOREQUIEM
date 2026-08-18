/*****************************************************************//**
 * @file    EnemyStatusUI.cpp
 * @brief   敵ユニットのHP情報を3D座標から2Dスクリーンへ変換して表示するHUD制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/Status/EnemyStatusUI.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/GameObjects/Enemy/Boss/BossEnemy.h"
#include <WICTextureLoader.h>

/**
 * @brief コンストラクタ
 */
EnemyStatusUI::EnemyStatusUI()
{
}

/**
 * @brief デストラクタ
 */
EnemyStatusUI::~EnemyStatusUI()
{
}

/**
 * @brief 初期化処理
 * 各種テクスチャの読み込みと単色テクスチャの生成
 * @param[in] device Direct3Dデバイス
 */
void EnemyStatusUI::Initialize(ID3D11Device* device)
{
    // 白い1x1テクスチャを作る
    uint32_t               pixel = WHITE_PIXEL;
    D3D11_SUBRESOURCE_DATA initData = { &pixel, sizeof(uint32_t), 0 };
    D3D11_TEXTURE2D_DESC   desc = {};

    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // テクスチャ読み込み
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_frame.png",
        nullptr, m_frame.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_enemy_hp.png",
        nullptr, m_hpBar.GetAddressOf());
    DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_gauge_background.png",
        nullptr, m_back.GetAddressOf());

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    device->CreateTexture2D(&desc, &initData, tex.GetAddressOf());
    device->CreateShaderResourceView(tex.Get(), nullptr, m_textureWhite.GetAddressOf());
}

/**
 * @brief 描画処理
 * 敵の3D座標をスクリーン座標へ投影し、HPバーを頭上に描画する
 * @param[in,out] spriteBatch スプライトバッチ
 * @param[in] enemies エネミーリスト
 * @param[in] view ビュー行列
 * @param[in] proj 投影行列
 * @param[in] viewport ビューポート
 */
void EnemyStatusUI::Render(DirectX::SpriteBatch* spriteBatch, 
    const std::vector<std::unique_ptr<Enemy>>& enemies, const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& proj, const D3D11_VIEWPORT& viewport)
{
    if (!m_hpBar) return;

    // スクリーン解像度に基づいた動的UIスケール計算
    float guiScale = viewport.Height / BASE_SCREEN_HEIGHT;

    for (const auto& enemyPtr : enemies)
    {
        Enemy* enemy = enemyPtr.get();

        // 非アクティブまたは死亡時は描画をスキップ
        if (!enemy || !enemy->IsActive() || enemy->GetHealth() <= 0.0f)
        {
            continue;
        }

        // ボス判定を行い、専用の描画関数に振り分ける
        if (dynamic_cast<BossEnemy*>(enemy))
        {
            RenderBossHealthBar(spriteBatch, enemy, guiScale, viewport);
        }
        else
        {
            RenderNormalEnemyHealthBar(spriteBatch, enemy, guiScale, view, proj, viewport);
        }
    }
}

/**
 * @brief ボス専用のHPゲージを描画位置を計算して描画
 * @param[in,out] spriteBatch スプライトバッチ
 * @param[in] boss ボスエネミーのポインタ
 * @param[in] guiScale 画面解像度に基づいたUIスケール値
 * @param[in] viewport 現在のビューポート情報
 */
void EnemyStatusUI::RenderBossHealthBar(DirectX::SpriteBatch* spriteBatch, Enemy* boss,
    float guiScale, const D3D11_VIEWPORT& viewport)
{
    float currentHealth = boss->GetHealth();
    float maxHealth = boss->GetMaxHealth();

    // ボスの場合：画面上部の中央に、大きく固定表示する
    float bossBarWidth = viewport.Width * BOSS_BAR_WIDTH_RATIO;
    float bossBarHeight = BASE_BAR_HEIGHT * guiScale * BOSS_BAR_HEIGHT_RATIO;

    DirectX::SimpleMath::Vector2 bossHpSize(bossBarWidth, bossBarHeight);
    DirectX::SimpleMath::Vector2 bossHpDrawScale(bossHpSize.x / HP_TEX_WIDTH, bossHpSize.y / HP_TEX_HEIGHT);

    // 画面の中央上部になるように座標を計算
    float posX = (viewport.Width - bossBarWidth) * HALF_RATIO;
    float posY = viewport.Height * BOSS_BAR_POS_Y_RATIO;
    DirectX::SimpleMath::Vector2 bossHpPos(posX, posY);

    DrawHealthBar(spriteBatch, bossHpPos, bossHpDrawScale, bossHpSize, currentHealth, maxHealth);
}

/**
 * @brief 通常の敵のHPゲージをワールド座標から変換して描画
 * @param[in,out] spriteBatch スプライトバッチ
 * @param[in] enemy 通常エネミーのポインタ
 * @param[in] guiScale 画面解像度に基づいたUIスケール値
 * @param[in] view 現在のビュー行列
 * @param[in] proj 現在の投影行列
 * @param[in] viewport 現在のビューポート情報
 */
void EnemyStatusUI::RenderNormalEnemyHealthBar(DirectX::SpriteBatch* spriteBatch, Enemy* enemy,
    float guiScale, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, const D3D11_VIEWPORT& viewport)
{
    float currentHealth = enemy->GetHealth();
    float maxHealth = enemy->GetMaxHealth();

    DirectX::SimpleMath::Vector2 hpSize 
        = DirectX::SimpleMath::Vector2(BASE_BAR_WIDTH * guiScale, BASE_BAR_HEIGHT * guiScale);
    DirectX::SimpleMath::Vector2 hpDrawScale 
        = DirectX::SimpleMath::Vector2(hpSize.x / HP_TEX_WIDTH, hpSize.y / HP_TEX_HEIGHT);

    // 敵の3Dワールド座標を取得し、頭上位置へオフセットを加算
    DirectX::SimpleMath::Vector3 offsetWorldPos = enemy->GetPosition();
    offsetWorldPos.y += Y_OFFSET;

    // 3D座標からスクリーン座標へ変換
    DirectX::SimpleMath::Vector3 screenPos = XMVector3Project(
        offsetWorldPos, viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height,
        viewport.MinDepth, viewport.MaxDepth, proj, view, DirectX::SimpleMath::Matrix::Identity
    );

    // 画面外、およびカメラの背面ならスキップ
    if (screenPos.z < MIN_DEPTH || screenPos.z > MAX_DEPTH)
    {
        return;
    }

    // ゲージの描画開始基点を計算 (X座標を中心合わせにするために横幅の半分を引く)
    DirectX::SimpleMath::Vector2 hpPos(screenPos.x - (hpSize.x * HALF_RATIO), screenPos.y);

    // 個別のHPバー描画を実行
    DrawHealthBar(spriteBatch, hpPos, hpDrawScale, hpSize, currentHealth, maxHealth);
}

/**
 * @brief 描画処理
 * @param[in] spriteBatch スプライトバッチ
 * @param[in] hpPos 描画位置
 * @param[in] hpDrawScale 描画スケール
 * @param[in] hpSize HPバーのサイズ
 * @param[in] health 現在のHP
 * @param[in] maxHealth 最大HP
 */
void EnemyStatusUI::DrawHealthBar(DirectX::SpriteBatch* spriteBatch, 
    const DirectX::SimpleMath::Vector2& hpPos, const DirectX::SimpleMath::Vector2& hpDrawScale,
    const DirectX::SimpleMath::Vector2& hpSize, float health, float maxHealth)
{
    UNREFERENCED_PARAMETER(hpSize);

    // --- 描画順序: 背景 -> 中身 -> 枠 ---

    // 背景の描画
    if (m_back)
    {
        spriteBatch->Draw(
            m_back.Get(),
            hpPos, 
            nullptr, 
            DirectX::Colors::White, 
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero,
            hpDrawScale);
    }

    // 中身の描画 (現在のHP割合に応じて右側をクリッピング)
    float hpRatio = std::max(0.0f, std::min(1.0f, health / maxHealth));

    RECT sourceRect = {};
    sourceRect.left = 0;
    sourceRect.top = 0;
    sourceRect.bottom = static_cast<long>(HP_TEX_HEIGHT);
    sourceRect.right = static_cast<long>(HP_TEX_WIDTH * hpRatio);

    spriteBatch->Draw(
        m_hpBar.Get(), 
        hpPos, 
        &sourceRect, 
        DirectX::Colors::White,
        0.0f, 
        DirectX::SimpleMath::Vector2::Zero,
        hpDrawScale);

    // 外枠（フレーム）の描画
    if (m_frame)
    {
        spriteBatch->Draw(
            m_frame.Get(), 
            hpPos, 
            nullptr, 
            DirectX::Colors::White, 
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero, 
            hpDrawScale);
    }
}