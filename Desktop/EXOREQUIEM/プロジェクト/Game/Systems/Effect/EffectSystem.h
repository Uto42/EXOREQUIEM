/*****************************************************************//**
 * @file    EffectSystem.h
 * @brief   各種パーティクルマネージャーを統括するエフェクト生成窓口（シングルトン）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Effect/Particle/ParticleManager.h"
#include <random>

 /**
  * @class EffectSystem
  * @brief ゲーム内の全エフェクト（爆発、煙、閃光など）を一元管理するシステム
  */
class EffectSystem
{
public:
    // シングルトンインスタンス取得
    static EffectSystem* Instance();
    // デストラクタ
    ~EffectSystem();
    // 初期化処理
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    // 更新処理
    void Update(float dt);
    // 描画処理
    void Render(
        ID3D11DeviceContext* context,
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj);
    // 終了処理
    void Finalize();

    // --- エフェクト発生関数 ---

    // 爆発（ミサイル着弾、撃破時）
    void SpawnExplosion(const DirectX::SimpleMath::Vector3& pos);

    // マズルフラッシュ（銃発射時）
    void SpawnMuzzleFlash(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir);

    // スラスター（通常移動・ブースト）
    void SpawnThruster(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir);
    void SpawnMoveThruster(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir);

    // ミサイルの煙（飛翔中のトレイル）
    void SpawnMissileSmoke(const DirectX::SimpleMath::Vector3& pos);

    // 土煙（地上移動中）
    void SpawnGroundDust(const DirectX::SimpleMath::Vector3& pos);

    // 着地時の土煙（衝撃波）
    void SpawnLandingDust(const DirectX::SimpleMath::Vector3& pos);

private:
	// コンストラクタ
    EffectSystem();
	// コピーコンストラクタを削除
    EffectSystem(const EffectSystem&) = delete;

	// コピー代入演算子を削除
    EffectSystem& operator=(const EffectSystem&) = delete;

private:
	// --- エフェクトパラメータ定数 --- 
	// 爆発のパラメータ
    struct ExplosionParam
    {
		static constexpr int   SparkCount = 10;             //< スパークの数
		static constexpr int   FlameCount = 6;			    //< 炎の数

		static constexpr float CoreLife = 0.1f;             //< コアの寿命
		static constexpr float CoreStartSize = 1.5f;        //< コアの開始サイズ
		static constexpr float CoreEndSize = 0.2f;          //< コアの終了サイズ

		static constexpr float SparkLife = 0.3f;            //< スパークの寿命
		static constexpr float SparkSpeedMin = 5.0f;        //< スパークの最小速度
		static constexpr float SparkSpeedMax = 10.0f;       //< スパークの最大速度
		static constexpr float SparkSize = 0.7f;            //< スパークのサイズ

		static constexpr float FlameLife = 0.5f;            //< 炎の寿命
		static constexpr float FlameSize = 0.8f;            //< 炎のサイズ
		static constexpr float FlameEndSize = 1.5f;         //< 炎の終了サイズ
    };

	// スラスターのパラメータ
    struct ThrusterParam
    {
		static constexpr float Life = 0.1f;                 //< スラスターの寿命
		static constexpr float BoostSpeed = 20.0f;          //< ブースト用スラスターの速度 
		static constexpr float MoveSpeed = 10.0f;		    //< 通常移動用スラスターの速度
		static constexpr float Size = 0.2f;				    //< スラスターのサイズ
    };

	// 環境エフェクトのパラメータ
    struct EnvironmentParam
    {
		static constexpr float MuzzleFlashLife = 0.05f;     //< マズルフラッシュの寿命
		static constexpr float MuzzleFlashSize = 0.4f;      //< マズルフラッシュのサイズ

		static constexpr float MissileSmokeLife = 0.8f;     //< ミサイル煙の寿命
		static constexpr float MissileSmokeStartSize = 0.2f;//< ミサイル煙の開始サイズ
		static constexpr float MissileSmokeEndSize = 0.6f;  //< ミサイル煙の終了サイズ

		static constexpr float GroundDustLife = 0.3f;       //< 土煙の寿命
		static constexpr float GroundDustSpeed = 2.0f;      //< 土煙の速度
		static constexpr float GroundDustStartSize = 0.3f;  //< 土煙の終了サイズ
		static constexpr float GroundDustEndSize = 0.8f;    //< 土煙の終了サイズ
    };

	// 着地エフェクトのパラメータ
    struct LandingParam
    {
		static constexpr int   ParticleCount = 16;          //< パーティクルの数
		static constexpr float SpeedMin = 5.0f;             //< パーティクルの最小速度
		static constexpr float SpeedMax = 8.0f;             //< パーティクルの最大速度
		static constexpr float Life = 0.4f; 			    //< パーティクルの寿命
		static constexpr float StartSize = 0.5f;            //< パーティクルの開始サイズ
		static constexpr float EndSize = 1.2f;              //< パーティクルの終了サイズ
    };

private:
    // --- メンバ変数 ---
    std::unique_ptr<ParticleManager> m_fireMgr;     //< 火・光用マネージャー (加算合成)
    std::unique_ptr<ParticleManager> m_smokeMgr;    //< 煙・塵用マネージャー (半透明合成)

    std::mt19937 m_gen{ std::random_device{}() };   // エンジンを初期化

};