/*****************************************************************//**
 * @file    EffectSystem.cpp
 * @brief   各種パーティクルマネージャーを統括するエフェクト生成の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Effect/EffectSystem.h"

/**
 * @brief コンストラクタ
 */
EffectSystem::EffectSystem()
{
}

/**
 * @brief デストラクタ
 */
EffectSystem::~EffectSystem()
{
    Finalize();
}

/**
 * @brief シングルトンインスタンス取得 (Meyers Singleton)
 * @return EffectSystem* インスタンスポインタ
 */
EffectSystem* EffectSystem::Instance()
{
    static EffectSystem instance;
    return &instance;
}

/**
 * @brief 初期化処理
 * @param[in] device  ID3D11Device
 * @param[in] context ID3D11DeviceContext
 */
void EffectSystem::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // 火・光用マネージャー (加算合成を想定)
    m_fireMgr = std::make_unique<ParticleManager>();
    m_fireMgr->Create(device, context);
    m_fireMgr->LoadTexture(L"Resources/Textures/Effects/effect_fire.png"); // 光る画像

    // 煙・塵用マネージャー (半透明合成を想定)
    m_smokeMgr = std::make_unique<ParticleManager>();
    m_smokeMgr->Create(device, context);
    m_smokeMgr->LoadTexture(L"Resources/Textures/Effects/effect_smoke.png"); // 煙画像
}


/**
 * @brief 更新処理
 * @param[in] dt デルタタイム
 */
void EffectSystem::Update(float dt)
{
	// 各マネージャーの更新
    if (m_fireMgr) m_fireMgr->Update(dt);
    if (m_smokeMgr) m_smokeMgr->Update(dt);
}

/**
 * @brief 描画処理
 * @param[in] context ID3D11DeviceContext
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 */
void EffectSystem::Render(ID3D11DeviceContext* context,
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
    // 描画順序: 半透明(煙) -> 加算(火) が一般的
    if (m_smokeMgr) m_smokeMgr->Render(context, view, proj);
    if (m_fireMgr) m_fireMgr->Render(context, view, proj);
}

/**
 * @brief 終了処理
 */
void EffectSystem::Finalize()
{
    m_fireMgr.reset();
    m_smokeMgr.reset();
}

// --- エフェクト発生ロジック ---

/**
 * @brief 爆発エフェクトの発生
 * @param[in] pos 発生座標
 */
void EffectSystem::SpawnExplosion(const DirectX::SimpleMath::Vector3& pos)
{
    if (!m_fireMgr) return;

	// 乱数生成器の設定
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speedDist(ExplosionParam::SparkSpeedMin, ExplosionParam::SparkSpeedMax);

    // コア（中心の閃光）
    m_fireMgr->SpawnParticle(
        ExplosionParam::CoreLife,
        pos, 
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3(ExplosionParam::CoreStartSize),
        DirectX::SimpleMath::Vector3(ExplosionParam::CoreEndSize),
        DirectX::SimpleMath::Color(1.0f, 1.0f, 0.8f, 1.0f),
        DirectX::SimpleMath::Color(1.0f, 0.4f, 0.0f, 0.0f)
    );

    // スパーク（飛び散る火花）
    for (int i = 0; i < ExplosionParam::SparkCount; ++i)
    {
        // 各成分を -1.0 ～ 1.0 で生成して正規化
        DirectX::SimpleMath::Vector3 v(dist(m_gen), dist(m_gen), dist(m_gen));
        v.Normalize();
        float speed = speedDist(m_gen);

		// パーティクルの発生
        m_fireMgr->SpawnParticle(
            ExplosionParam::SparkLife,
            pos, 
            v * speed, 
            DirectX::SimpleMath::Vector3(0, -10.0f, 0),
            DirectX::SimpleMath::Vector3(ExplosionParam::SparkSize),
            DirectX::SimpleMath::Vector3::Zero,
            DirectX::SimpleMath::Color(1.0f, 0.9f, 0.5f, 1.0f),
            DirectX::SimpleMath::Color(1.0f, 0.0f, 0.0f, 0.0f)
        );
    }

    // 爆発炎
    for (int i = 0; i < ExplosionParam::FlameCount; ++i)
    {
        DirectX::SimpleMath::Vector3 v(dist(m_gen), dist(m_gen), dist(m_gen));
        v.Normalize();

		// パーティクルの発生
        m_fireMgr->SpawnParticle(
            ExplosionParam::FlameLife,
            pos, 
            v * 0.5f, 
            DirectX::SimpleMath::Vector3(0, 0.5f, 0),
            DirectX::SimpleMath::Vector3(ExplosionParam::FlameSize),
            DirectX::SimpleMath::Vector3(ExplosionParam::FlameEndSize),
            DirectX::SimpleMath::Color(1.0f, 0.5f, 0.1f, 1.0f),
            DirectX::SimpleMath::Color(0.6f, 0.0f, 0.0f, 0.0f)
        );
    }
}

/**
 * @brief マズルフラッシュの発生
 * @param[in] pos 発生座標
 * @param[in] dir 発射方向
 */
void EffectSystem::SpawnMuzzleFlash(const DirectX::SimpleMath::Vector3& pos, 
    const DirectX::SimpleMath::Vector3& dir)
{
    UNREFERENCED_PARAMETER(dir);

    if (!m_fireMgr) return;

	// マズルフラッシュは発射方向に沿った形状を持つため、位置と方向を考慮してパーティクルを生成
    m_fireMgr->SpawnParticle(
        EnvironmentParam::MuzzleFlashLife,
        pos, 
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3(EnvironmentParam::MuzzleFlashSize),
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Color(1.0f, 1.0f, 0.6f, 1.0f),
        DirectX::SimpleMath::Color(1.0f, 0.4f, 0.0f, 0.0f)
    );
}

/**
 * @brief スラスター（噴出）の発生
 * @param[in] pos 発生座標
 * @param[in] dir 噴出方向
 */
void EffectSystem::SpawnThruster(const DirectX::SimpleMath::Vector3& pos,
    const DirectX::SimpleMath::Vector3& dir)
{
    if (!m_fireMgr) return;

	// スラスターは噴出方向に沿った形状を持つため、位置と方向を考慮してパーティクルを生成
    m_fireMgr->SpawnParticle(
        ThrusterParam::Life,
        pos, 
        dir * ThrusterParam::BoostSpeed,
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3(ThrusterParam::Size),
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Color(1.0f, 0.1f, 0.1f, 1.0f),
        DirectX::SimpleMath::Color(1.0f, 0.0f, 0.0f, 0.0f)
    );
}

/**
 * @brief 移動用スラスター（ブースト時など）の発生
 * @param[in] pos 発生座標
 * @param[in] dir 噴出方向
 */
void EffectSystem::SpawnMoveThruster(const DirectX::SimpleMath::Vector3& pos, 
    const DirectX::SimpleMath::Vector3& dir)
{
    if (!m_fireMgr) return;

	// 位置と方向を考慮してパーティクルを生成
    m_fireMgr->SpawnParticle(
        ThrusterParam::Life,
        pos, 
        dir * ThrusterParam::MoveSpeed,
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3(ThrusterParam::Size),
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Color(1.0f, 0.2f, 0.0f, 1.0f),
        DirectX::SimpleMath::Color(1.0f, 0.0f, 0.0f, 0.0f)
    );
}

/**
 * @brief ミサイルの煙（トレイル）の発生
 * @param[in] pos 発生座標
 */
void EffectSystem::SpawnMissileSmoke(const DirectX::SimpleMath::Vector3& pos)
{
    if (!m_smokeMgr) return;

	// 速度やサイズの変化を設定してパーティクルを生成
    m_smokeMgr->SpawnParticle(
        EnvironmentParam::MissileSmokeLife,
        pos, 
        DirectX::SimpleMath::Vector3::Zero,
        DirectX::SimpleMath::Vector3(0, 0.5f, 0),
        DirectX::SimpleMath::Vector3(EnvironmentParam::MissileSmokeStartSize),
        DirectX::SimpleMath::Vector3(EnvironmentParam::MissileSmokeEndSize),
        DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 0.8f),
        DirectX::SimpleMath::Color(0.5f, 0.5f, 0.5f, 0.0f)
    );
}

/**
 * @brief 土煙（地上移動用）の発生
 * @param[in] pos 発生座標
 */
void EffectSystem::SpawnGroundDust(const DirectX::SimpleMath::Vector3& pos)
{
    if (!m_smokeMgr) return;

    std::uniform_real_distribution<float> xzDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> yDist(0.0f, 2.0f);

    DirectX::SimpleMath::Vector3 vel(xzDist(m_gen), yDist(m_gen), xzDist(m_gen));

	// パーティクルの発生
    m_smokeMgr->SpawnParticle(
        EnvironmentParam::GroundDustLife,
        pos, 
        vel * EnvironmentParam::GroundDustSpeed,
        DirectX::SimpleMath::Vector3(0, 1.0f, 0),
        DirectX::SimpleMath::Vector3(EnvironmentParam::GroundDustStartSize),
        DirectX::SimpleMath::Vector3(EnvironmentParam::GroundDustEndSize),
        DirectX::SimpleMath::Color(0.6f, 0.5f, 0.4f, 0.3f),
        DirectX::SimpleMath::Color(0.6f, 0.5f, 0.4f, 0.0f)
    );
}

/**
 * @brief 着地時の土煙（衝撃波）の発生
 * @param[in] pos 発生座標
 */
void EffectSystem::SpawnLandingDust(const DirectX::SimpleMath::Vector3& pos)
{
    if (!m_smokeMgr) return;

    // スピードの範囲指定
    std::uniform_real_distribution<float> speedDist(LandingParam::SpeedMin, LandingParam::SpeedMax);

    for (int i = 0; i < LandingParam::ParticleCount; ++i)
    {
		// パーティクルを円形に広げるための角度計算
        float angle = DirectX::XM_2PI * (static_cast<float>(i) / LandingParam::ParticleCount);
        DirectX::SimpleMath::Vector3 dir(cosf(angle), 0.0f, sinf(angle));

        float speed = speedDist(m_gen);

		// パーティクルの発生
        m_smokeMgr->SpawnParticle(
            LandingParam::Life,
            pos
            + DirectX::SimpleMath::Vector3(0, 0.1f, 0),
            dir * speed, 
            DirectX::SimpleMath::Vector3(0, 0.5f, 0),
            DirectX::SimpleMath::Vector3(LandingParam::StartSize),
            DirectX::SimpleMath::Vector3(LandingParam::EndSize),
            DirectX::SimpleMath::Color(0.6f, 0.5f, 0.4f, 0.4f),
            DirectX::SimpleMath::Color(0.6f, 0.5f, 0.4f, 0.0f)
        );
    }
}