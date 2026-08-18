/*****************************************************************//**
 * @file    InGameUI.cpp
 * @brief   ゲームプレイ中の情報表示（HP、エネルギー、残弾数、ステート情報）の描画制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/UI/HUD/InGameUI.h"
#include "Game/World/World.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/Camera/Camera.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/UI/HUD/LockOnUI.h"
#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include <WICTextureLoader.h>
#include "Game/GameObjects/Robot/Robot.h"

using Microsoft::WRL::ComPtr;

// 数字テクスチャ内の各数字の切り抜き範囲
static const struct { float x, w; } g_digitRects[10] = 
{
    { 0.0f,    150.0f }, // 0
    { 150.0f,  150.0f }, // 1
    { 300.0f,  160.0f }, // 2
    { 460.0f,  155.0f }, // 3
    { 615.0f,  155.0f }, // 4
    { 770.0f,  155.0f }, // 5
    { 920.0f,  150.0f }, // 6
    { 1090.0f, 170.0f }, // 7
    { 1240.0f, 150.0f }, // 8
    { 1400.0f, 160.0f }  // 9
};

/**
 * @brief コンストラクタ
 */
InGameUI::InGameUI()
    : m_crosshairPosition(DirectX::SimpleMath::Vector2::Zero)
    , m_showCrosshair(true)
    , m_crosshairSize{ 0, 0 }
    , m_currentEnergy(0.0f)
    , m_maxEnergy(0.0f)
    , m_missileAmmo(0)
    , m_gunAmmo(0)
    , m_playerHP(0.0f)
    , m_playerMaxHP(0.0f)
    , m_enemyActive(false)
    , m_enemyHP(0.0f)
    , m_enemyMaxHP(0.0f)
    , m_world(nullptr)
    , m_camera(nullptr)
    , m_lockOnSystem(nullptr)
    , m_lockOnUI(nullptr)
    , m_posPlayerState(DirectX::SimpleMath::Vector2::Zero)
    , m_posPlayerHP(DirectX::SimpleMath::Vector2::Zero)
    , m_posPlayerEnergy(DirectX::SimpleMath::Vector2::Zero)
    , m_posEnemyHP(DirectX::SimpleMath::Vector2::Zero)
    , m_posEnemyState(DirectX::SimpleMath::Vector2::Zero)
    , m_posAmmoMissile(DirectX::SimpleMath::Vector2::Zero)
    , m_posAmmoGun(DirectX::SimpleMath::Vector2::Zero)
    , m_gunIconScale(1.0f)
	, m_shotgunIconScale(1.0f)
    , m_missileIconScale(1.0f)
	, m_context(nullptr)
	, m_guiScale(1.0f)
	, m_screenRect{ 0, 0, 0, 0 }
	, m_playerYaw(0.0f)
{
    m_numberSize = DirectX::SimpleMath::Vector2(NUMBER_TEX_WIDTH, NUMBER_TEX_HEIGHT);

    m_enemyStateText[0] = L'\0';
    m_playerStateString[0] = L'\0';
}

/**
 * @brief デストラクタ
 */
InGameUI::~InGameUI() = default;

/**
 * @brief 初期化処理
 * @param[in] device ID3D11Device
 * @param[in] context ID3D11DeviceContext
 * @param[in] world ワールド（エンティティ管理者）
 * @param[in] camera カメラ
 * @param[in] lockOnSystem ロックオンシステム
 */
void InGameUI::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
    World* world, Camera* camera, LockOnSystem* lockOnSystem, LockOnUI* lockOnUI)
{
    m_radar.Initialize(device);

    m_world = world;
    m_camera = camera;
    m_lockOnSystem = lockOnSystem;
    m_lockOnUI = lockOnUI;

    // 描画リソース作成
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
    m_spriteFont = std::make_unique<DirectX::SpriteFont>(device, L"Resources\\Font\\SegoeUI_18.spritefont");

    CreateDeviceDependentResources(device);
}

/**
 * @brief 更新処理
 * @param[in] dt 経過時間
 * @param[in] screenSize スクリーン矩形
 * @param[in] viewport ビューポート
 */
void InGameUI::Update(float dt, const RECT& screenSize, const DirectX::SimpleMath::Viewport& viewport)
{
    UNREFERENCED_PARAMETER(viewport);

    CreateWindowSizeDependentResources(screenSize);
    if (!m_world || !m_camera || !m_lockOnSystem) return;

    const Player* player = m_world->GetPlayer();

    if (!player) return;

    // --- 分割した関数を呼び出す ---
    UpdatePlayerStatus(player);
    UpdateWeaponStatus(player);

    // LockOnUIが計算した単一の正解座標を取得
    if (m_lockOnUI)
    {
        m_crosshairPosition = m_lockOnUI->GetCrosshairPosition();
    }
    else
    {
        float W = static_cast<float>(screenSize.right - screenSize.left);
        float H = static_cast<float>(screenSize.bottom - screenSize.top);
        m_crosshairPosition = DirectX::SimpleMath::Vector2(W * HALF_RATIO, H * HALF_RATIO);
    }

    m_screenRect = screenSize; // 画面サイズを保存

    // プレイヤーのインスタンスをEntityManager等から取得
    m_playerPos = player->GetPosition();
    m_playerYaw = player->GetRotation().ToEuler().y; // クォータニオンからYaw(Y軸回転)を抽出

    m_radar.Update(dt);
}

/**
 * @brief プレイヤーのステータス情報（HP・エネルギー）の更新
 * @param[in] player プレイヤーへのポインタ
 */
void InGameUI::UpdatePlayerStatus(const Player* player)
{
    // --- プレイヤー情報更新 ---
    m_maxEnergy = player->GetMaxEnergy();
    m_playerHP = player->GetHealth();
    m_playerMaxHP = player->GetMaxHealth();
}

/**
 * @brief 武器の残弾数情報の更新
 * @param[in] player プレイヤーへのポインタ
 */
void InGameUI::UpdateWeaponStatus(const Player* player)
{
    // プレイヤーの存在確認
    if (!player) return;

    // プレイヤーからロボットを取得
    auto* robot = player->GetRobot();
    if (!robot) return;

    // ロボットから武器コントローラーを取得
    auto* weaponCtrl = robot->GetWeapon();

    if (weaponCtrl)
    {
        // 残弾数を取得
        m_missileAmmo = weaponCtrl->GetMissileAmmo();
        m_gunAmmo = weaponCtrl->GetGunAmmo();

        m_currentWeaponType = static_cast<int>(weaponCtrl->GetCurrentWeaponSet());
    }
    else
    {
        m_missileAmmo = 0;
        m_gunAmmo = 0;
    }
}

/**
 * @brief 描画処理
 */
void InGameUI::Render()
{
    if (!m_spriteBatch || !m_spriteFont) return;

    float cameraYaw = CalculateCameraYaw();

    m_spriteBatch->Begin();

	//--- プレイヤー情報の描画 ---
    DrawPlayerInfo();
	//--- 武器アイコンと残弾数の描画 ---
    DrawWeaponIconsAndAmmo();
	//--- リロードバーの描画 ---
    DrawReloadBars();
	// -- 敵情報の描画 ---
    DrawEnemyInfo();
	// -- レーダーの描画 ---
    DrawRadar(cameraYaw);
    // --- ガイド描画 ---
    DrawGuides();

    m_spriteBatch->End();
}

/**
 * @brief カメラの向いている方向（Yaw）を計算する
 * @return カメラのYaw角度
 */
float InGameUI::CalculateCameraYaw() const
{
    // --- カメラの向いている方向（Yaw）を計算 ---
    DirectX::SimpleMath::Matrix view = m_camera->GetViewMatrix();
    DirectX::SimpleMath::Matrix invView = view.Invert();
    DirectX::SimpleMath::Vector3 cameraForward = invView.Forward();

    // atan2f で「カメラの向きの角度」を求めます
    return atan2f(cameraForward.x, cameraForward.z);
}

/**
 * @brief プレイヤーステート情報の描画
 */
void InGameUI::DrawPlayerInfo()
{
    // --- プレイヤーステート ---
    m_spriteFont->DrawString(m_spriteBatch.get(), m_playerStateString,
        m_posPlayerState, DirectX::Colors::White);
}

/**
 * @brief 武器アイコンと残弾数の描画
 */
void InGameUI::DrawWeaponIconsAndAmmo()
{
    // ミサイルアイコン
    if (m_missileIconTexture) {
        m_spriteBatch->Draw(m_missileIconTexture.Get(),
            m_posMissileIcon,
            nullptr,
            DirectX::Colors::White,
            0.0f,
            DirectX::SimpleMath::Vector2::Zero,
            DirectX::SimpleMath::Vector2(m_missileIconScale, m_missileIconScale));
    }

	// ミサイルの弾数表示例
    DrawValue(m_missileAmmo, m_posAmmoMissile, AMMO_TEXT_SCALE);

    const bool isStandard = (m_currentWeaponType == 0);
    ID3D11ShaderResourceView* currentIcon = isStandard ? m_gunIconTexture.Get() : m_shotgunIconTexture.Get();
    const DirectX::SimpleMath::Vector2& iconPos = isStandard ? m_posGunIcon : m_posShotgunIcon;
    const float iconScale = isStandard ? m_gunIconScale : m_shotgunIconScale;
    const DirectX::XMVECTORF32& iconColor = isStandard ? DirectX::Colors::Black : DirectX::Colors::White;

    // メイン武器（ガトリング/ショットガン）アイコン
    if (currentIcon) {
        m_spriteBatch->Draw(currentIcon,
            iconPos,
            nullptr,
            iconColor,
            0.0f,
            DirectX::SimpleMath::Vector2::Zero,
            DirectX::SimpleMath::Vector2(iconScale, iconScale));
    }

    // ミサイルの弾数表示例
    DrawValue(m_gunAmmo, m_posAmmoGun, AMMO_TEXT_SCALE);
}

/**
 * @brief 敵情報の描画
 */
void InGameUI::DrawEnemyInfo() const
{
    // --- 敵情報 ---
    if (m_enemyActive)
    {
        /*swprintf_s(buffer, L"Enemy HP: %.0f", m_enemyHP);
        m_spriteFont->DrawString(m_spriteBatch.get(), buffer,
            m_posEnemyHP, Colors::Red);

        m_spriteFont->DrawString(m_spriteBatch.get(), m_enemyStateText,
            m_posEnemyState, Colors::White);*/
    }
}

/**
 * @brief レーダーのデータ準備および描画
 * @param[in] cameraYaw カメラのYaw角度
 */
void InGameUI::DrawRadar(float cameraYaw)
{
    // --- データの準備 ---
    std::vector<DirectX::SimpleMath::Vector3> enemyPositions;
    if (m_world && m_world->GetEnemyManager())
    {
        const auto& enemies = m_world->GetEnemyManager()->GetEnemies();
        for (const auto& enemy : enemies)
        {
            if (enemy && enemy->IsActive() && enemy->GetHealth() > 0.0f)
            {
                enemyPositions.push_back(enemy->GetPosition());
            }
        }
    }

    if (m_world)
    {
        if (auto player = m_world->GetPlayer())
        {
            m_playerPos = player->GetPosition();
            m_playerYaw = player->GetRotation().ToEuler().y;
        }
    }

    // --- 画面サイズに基づいたスケーリング ---
    float guiScale = m_screenRect.bottom / BASE_SCREEN_HEIGHT;

    // 半径もスケールさせる
    float scaledRadius = RADAR_BASE_RADIUS * guiScale;

    float scaledMargin = RADAR_MARGIN * guiScale;

    // 右上からのオフセットを小さくして、より端に寄せる
    DirectX::SimpleMath::Vector2 radarPos(
        m_screenRect.right - (scaledRadius + scaledMargin),
        (scaledRadius + scaledMargin)
    );

    // レーダーの描画
    m_radar.Render(
        m_spriteBatch.get(),
        radarPos,
        scaledRadius, // 半径を渡すように変更
        m_playerPos,
        cameraYaw, // カメラの向き
        enemyPositions
    );
}

/**
 * @brief 操作ガイドUIの描画
 */
void InGameUI::DrawGuides()
{
    if (!m_texEscGuide) return;

    DirectX::SimpleMath::Vector2 origin = m_sizeEscGuide * HALF_RATIO;
    float finalScale = m_guiScale * GUIDE_SCALE;

    DirectX::SimpleMath::Vector2 escPos(
        (origin.x * finalScale) + GUIDE_MARGIN_X * m_guiScale,
        (origin.y * finalScale) + GUIDE_MARGIN_Y * m_guiScale
    );

    m_spriteBatch->Draw(m_texEscGuide.Get(), escPos, nullptr, DirectX::Colors::White, 0.0f, origin, finalScale);

    // トレーニングモード時のみ追加ガイドを表示
    if (m_isTrainingMode && m_texTrainingGuide)
    {
        DirectX::SimpleMath::Vector2 trainOrigin = m_sizeTrainingGuide * HALF_RATIO;
        float trainFinalScale = m_guiScale * GUIDE_SCALE;
        float spacing = GUIDE_SPACING * m_guiScale;
        float leftMargin = GUIDE_MARGIN_X * m_guiScale;

        DirectX::SimpleMath::Vector2 trainPos(
            (trainOrigin.x * trainFinalScale) + leftMargin,
            escPos.y + (m_sizeEscGuide.y * HALF_RATIO * finalScale)
            + (m_sizeTrainingGuide.y * HALF_RATIO * trainFinalScale) + spacing
        );

        m_spriteBatch->Draw(
            m_texTrainingGuide.Get(),
            trainPos, 
            nullptr,
            DirectX::Colors::White, 
            0.0f,
            trainOrigin, 
            trainFinalScale);
    }
}

/**
 * @brief クロスヘア周辺の4連リロードバーの描画
 */
void InGameUI::DrawReloadBars()
{
    if (!m_texReloadFrame || !m_texReloadBar) return;
    if (!m_world || !m_world->GetPlayer() || !m_world->GetPlayer()->GetRobot()) return;

    auto* weaponCtrl = m_world->GetPlayer()->GetRobot()->GetWeapon();
    if (!weaponCtrl) return;

    // 基準となる座標とオフセットの計算
    DirectX::SimpleMath::Vector2 center = m_crosshairPosition;

    // 照準からの距離
    float offsetX = RELOAD_BAR_OFFSET_X * m_guiScale;
    float offsetY = RELOAD_BAR_OFFSET_Y * m_guiScale;
    float spacingX = RELOAD_BAR_SPACING_X * m_guiScale;
    float scale = RELOAD_BAR_SCALE_BASE * m_guiScale;

    // バーを横に並べるときの実際の移動距離（バーの幅 ＋ 隙間）
    float totalSpacingX = (m_sizeReloadBar.x * scale) + spacingX;

    // 1本のバーを描画する便利なラムダ式
    auto DrawBar = [&](WeaponSet set, bool isPrimary, DirectX::SimpleMath::Vector2 pos)
        {
            // データの取得
            int ammo = isPrimary ? weaponCtrl->GetPrimaryAmmo(set) : weaponCtrl->GetSecondaryAmmo(set);
            int maxAmmo = isPrimary ? weaponCtrl->GetPrimaryMaxAmmo(set) : weaponCtrl->GetSecondaryMaxAmmo(set);

            // リロード中かどうかの判定とリロード率の取得
            bool isReloading = isPrimary
                ? weaponCtrl->IsPrimaryReloading(set) : weaponCtrl->IsSecondaryReloading(set);
            float reloadRate = isPrimary
                ? weaponCtrl->GetPrimaryReloadTimeRate(set) : weaponCtrl->GetSecondaryReloadTimeRate(set);

            // 割合の計算（リロード中は進捗率、通常時は残弾率）
            float ammoRatio = (maxAmmo > 0) ? (static_cast<float>(ammo) / static_cast<float>(maxAmmo)) : 0.0f;
            float ratio = isReloading ? reloadRate : ammoRatio;
            ratio = std::clamp(ratio, 0.0f, 1.0f);

            // 色と透明度の決定
            DirectX::SimpleMath::Color color(DirectX::Colors::Gray);

            // リロード中、または残弾が閾値以下なら赤色にする
            if (isReloading || ammoRatio <= RELOAD_BAR_LOW_AMMO_THRESHOLD)
            {
                color = DirectX::SimpleMath::Color(DirectX::Colors::Red);
            }

            // アクティブなセットは不透明、裏に回っているセットは半透明にする
            bool isActive = (weaponCtrl->GetCurrentWeaponSet() == set);
            float alpha = isActive ? 1.0f : RELOAD_BAR_INACTIVE_ALPHA;
            color.w = alpha; // バーの透明度を適用

            // バーの中身の描画（下端固定で、水が減るように上から下へ空にしていく）
            long originalHeight = static_cast<long>(m_sizeReloadBar.y);
            long cutHeight = static_cast<long>(m_sizeReloadBar.y * ratio);

            if (cutHeight > 0) // 高さが0の時は描画しない
            {
                RECT srcRect = {
                    0,
                    originalHeight - cutHeight,
                    static_cast<long>(m_sizeReloadBar.x),
                    originalHeight
                };

                DirectX::SimpleMath::Vector2 bottomPos =
                    pos + DirectX::SimpleMath::Vector2(0.0f, (m_sizeReloadBar.y * HALF_RATIO) * scale);
                DirectX::SimpleMath::Vector2 origin(m_sizeReloadBar.x * HALF_RATIO, static_cast<float>(cutHeight));

                // 背面に中身を描画
                m_spriteBatch->Draw(m_texReloadBar.Get(), bottomPos, &srcRect, color, 0.0f, origin, scale);
            }

            // 枠の描画（中身の上に被せる）
            m_spriteBatch->Draw(m_texReloadFrame.Get(), pos, nullptr,
                DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, alpha), 0.0f,
                DirectX::SimpleMath::Vector2(m_sizeReloadBar.x * HALF_RATIO, m_sizeReloadBar.y * HALF_RATIO),
                scale);
        };

    // 4つのバーを描画（高さYは全て統一し、左右対称に配置）
    // 左側（腕武器）
    DrawBar(WeaponSet::Standard, true, center + DirectX::SimpleMath::Vector2(-offsetX, offsetY));               // 内側
    DrawBar(WeaponSet::Heavy, true, center + DirectX::SimpleMath::Vector2(-offsetX - totalSpacingX, offsetY));  // 外側

    // 右側（肩武器）
    DrawBar(WeaponSet::Standard, false, center + DirectX::SimpleMath::Vector2(offsetX, offsetY));               // 内側
    DrawBar(WeaponSet::Heavy, false, center + DirectX::SimpleMath::Vector2(offsetX + totalSpacingX, offsetY));  // 外側
}

/**
 * @brief 数字（フォントではなく画像）の描画
 * @param[in] value 数値
 * @param[in] pos 座標
 * @param[in] scale 拡大率
 * @param[in] color 色
 */
void InGameUI::DrawNumber(int number, DirectX::SimpleMath::Vector2 position, float scale)
{
    if (!m_texNumbers) return;

    float sw = m_numberSize.x / 10.0f; // 数字1つ分の幅
    float sh = m_numberSize.y;         // 高さ

    // 表示したい数字に合わせて読み取り範囲を計算
    RECT srcRect = {};
    srcRect.left = static_cast<long>(sw * number);
    srcRect.top = 0;
    srcRect.right = static_cast<long>(sw * (number + 1));
    srcRect.bottom = static_cast<long>(sh);

    m_spriteBatch->Draw(
        m_texNumbers.Get(), 
        position, 
        &srcRect, 
        DirectX::Colors::White, 
        0.0f, 
        DirectX::SimpleMath::Vector2::Zero, scale);
}

/**
 * @brief ゲージの描画
 * @param[in] batch SpriteBatchポインタ
 * @param[in] pos 座標
 * @param[in] ratio 割合(0.0～1.0)
 * @param[in] color バーの色
 * @param[in] width 最大幅
 * @param[in] height 高さ
 */
void InGameUI::DrawValue(int value, DirectX::SimpleMath::Vector2 position, float baseScale)
{
    if (!m_texNumbers) return;

    std::string s = std::to_string(value);

    // 画面の拡大率（m_guiScale）をしっかり掛ける
    float finalScale = baseScale * m_guiScale; 

    float totalWidth = 0.0f;
    for (char c : s) {
        int num = c - '0';
        if (num < 0 || num > 9) continue;
        // 各数字の幅 + 文字間隔(10px分) を加算
        totalWidth += (g_digitRects[num].w + NUMBER_SPACING) * finalScale;
    }

    // 指定された座標(position.x)を「右端」にするため、全体の幅を引く
    float currentX = position.x - totalWidth;

    for (char c : s) {
        int num = c - '0';
        if (num < 0 || num > 9) continue;

        // 先頭で定義した g_digitRects（個別サイズ）を使う
        const auto& rectData = g_digitRects[num];

        RECT srcRect = {};
        srcRect.left = static_cast<long>(rectData.x);
        srcRect.top = 0;
        srcRect.right = static_cast<long>(rectData.x + rectData.w);
        srcRect.bottom = static_cast<long>(m_numberSize.y);

        m_spriteBatch->Draw(
            m_texNumbers.Get(), 
            DirectX::SimpleMath::Vector2(currentX, position.y),
            &srcRect,
            DirectX::Colors::White, 
            0.0f, 
            DirectX::SimpleMath::Vector2::Zero, 
            finalScale);

        // 次の数字へ（描画した数字の幅分だけ右にずらす）
        currentX += (rectData.w + NUMBER_SPACING) * finalScale;
    }
}

/**
 * @brief 画像リソースのロード
 * @param[in] device ID3D11Device
 */
void InGameUI::CreateDeviceDependentResources(ID3D11Device* device)
{
    // 各種テクスチャの読み込み
    // クロスヘアテクスチャ読み込み
    DX::ThrowIfFailed(
        DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_crosshair.png",
            nullptr, m_crosshairTexture.ReleaseAndGetAddressOf())
    );

    // テクスチャサイズ取得
    if (m_crosshairTexture)
    {
        ComPtr<ID3D11Resource> resource;
        m_crosshairTexture->GetResource(resource.GetAddressOf());

        ComPtr<ID3D11Texture2D> texture2D;
        if (SUCCEEDED(resource.As(&texture2D)))
        {
            D3D11_TEXTURE2D_DESC desc;
            texture2D->GetDesc(&desc);
            m_crosshairSize.x = desc.Width;
            m_crosshairSize.y = desc.Height;
        }
        else
        {
            m_crosshairSize.x = 32;
            m_crosshairSize.y = 32;
        }
    }

    // ガイドUIの読み込み用ヘルパー
    auto LoadGuideTex = 
        [&](const wchar_t* path, ComPtr<ID3D11ShaderResourceView>& tex, DirectX::SimpleMath::Vector2& sizeOut)
    {
        if (SUCCEEDED(DirectX::CreateWICTextureFromFile(device, path, nullptr, tex.ReleaseAndGetAddressOf())))
        {
            ComPtr<ID3D11Resource> res;
            tex->GetResource(res.GetAddressOf());
            ComPtr<ID3D11Texture2D> tex2D;
            if (SUCCEEDED(res.As(&tex2D)))
            {
                D3D11_TEXTURE2D_DESC desc;
                tex2D->GetDesc(&desc);
                sizeOut = DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
            }
        }
    };

    LoadGuideTex(L"Resources/Textures/inGame/inGame_font_pausemenu.png", m_texEscGuide, m_sizeEscGuide);
    LoadGuideTex(L"Resources/Textures/inGame/inGame_training_guide.png", m_texTrainingGuide, m_sizeTrainingGuide);

    // --- リロードUIの読み込み ---
    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device, L"Resources/Textures/InGame/inGame_reload_frame.png",
        nullptr, m_texReloadFrame.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device, L"Resources/Textures/InGame/inGame_reload_bar.png",
        nullptr, m_texReloadBar.ReleaseAndGetAddressOf()));

    // バー画像のサイズを取得（srcRectで切り抜くため）
    if (m_texReloadBar)
    {
        Microsoft::WRL::ComPtr<ID3D11Resource> res;
        m_texReloadBar->GetResource(res.GetAddressOf());
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
        if (SUCCEEDED(res.As(&tex2D)))
        {
            D3D11_TEXTURE2D_DESC desc;
            tex2D->GetDesc(&desc);
            m_sizeReloadBar.x = static_cast<float>(desc.Width);
            m_sizeReloadBar.y = static_cast<float>(desc.Height);
        }
    }

    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device, L"Resources/Textures/Number/numbers.png", 
        nullptr, 
        m_texNumbers.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device, 
        L"Resources/Textures/InGame/inGame_icon_missile.png",
        nullptr,
        m_missileIconTexture.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device, 
        L"Resources/Textures/InGame/inGame_icon_gun.png",
        nullptr, 
        m_gunIconTexture.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(
        device,
        L"Resources/Textures/InGame/inGame_icon_shotgun.png",
        nullptr,
        m_shotgunIconTexture.ReleaseAndGetAddressOf()));
}

/**
 * @brief ウィンドウサイズに基づいた座標計算
 * @param[in] screenSize スクリーン矩形
 */
void InGameUI::CreateWindowSizeDependentResources(const RECT& screenSize)
{
    float W = static_cast<float>(screenSize.right - screenSize.left);
    float H = static_cast<float>(screenSize.bottom - screenSize.top);

    // 基準解像度（1080p）をもとにスケール倍率を計算
    m_guiScale = H / BASE_SCREEN_HEIGHT;

    // 左上グループ（プレイヤー情報）
    m_posPlayerState = DirectX::SimpleMath::Vector2(W * LAYOUT_LEFT, H * LAYOUT_STATE_Y);
    m_posPlayerHP = DirectX::SimpleMath::Vector2(W * LAYOUT_LEFT, H * LAYOUT_HP_Y);
    m_posPlayerEnergy = DirectX::SimpleMath::Vector2(W * LAYOUT_LEFT, H * LAYOUT_ENERGY_Y);

    // 右上グループ（敵情報）
    m_posEnemyHP = DirectX::SimpleMath::Vector2(W * LAYOUT_ENEMY_X, H * LAYOUT_ENEMY_HP_Y);
    m_posEnemyState = DirectX::SimpleMath::Vector2(W * LAYOUT_ENEMY_X, H * LAYOUT_ENEMY_STATE_Y);

    // 右下グループ（弾薬表示の基本位置）
    m_posAmmoMissile = DirectX::SimpleMath::Vector2(W * LAYOUT_AMMO_X, H * LAYOUT_MISSILE_Y);
    m_posAmmoGun = DirectX::SimpleMath::Vector2(W * LAYOUT_AMMO_X, H * LAYOUT_GUN_Y);

    // 数字を右端に寄せるための微調整
    float numberShiftRight = AMMO_NUMBER_SHIFT_X * m_guiScale;
    m_posAmmoMissile.x += numberShiftRight;
    m_posAmmoGun.x += numberShiftRight;

    // マシンガンアイコンの位置調整
    m_posGunIcon = m_posAmmoGun
        - DirectX::SimpleMath::Vector2(GUN_ICON_OFFSET_X * m_guiScale, GUN_ICON_OFFSET_Y * m_guiScale);

    // ショットガンアイコンの位置調整
    m_posShotgunIcon = m_posAmmoGun
        - DirectX::SimpleMath::Vector2(SHOTGUN_ICON_OFFSET_X * m_guiScale, SHOTGUN_ICON_OFFSET_Y * m_guiScale);

    // ミサイルアイコンの位置調整
    m_posMissileIcon = m_posAmmoMissile
        - DirectX::SimpleMath::Vector2(MISSILE_ICON_OFFSET_X * m_guiScale, MISSILE_ICON_OFFSET_Y * m_guiScale);

    // 各武器アイコンのスケール計算
    m_gunIconScale = (H / BASE_ICON_HEIGHT) * SCALE_GUN_BASE;
    m_shotgunIconScale = (H / BASE_ICON_HEIGHT) * SCALE_SHOTGUN_BASE;
    m_missileIconScale = (H / BASE_ICON_HEIGHT) * SCALE_MISSILE_BASE;
}