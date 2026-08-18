/*****************************************************************//**
 * @file    ParticleManager.h
 * @brief   パーティクル群のメモリ管理、更新、およびジオメトリシェーダを用いた一括描画
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Effect/Particle/Particle.h"
#include <list>
#include <d3d11.h>

/**
 * @class ParticleManager
 * @brief 複数のパーティクルを生成・更新・描画管理するクラス
 */
class ParticleManager
{
private:
    /**
     * @brief 定数バッファ構造体 (HLSL側 cbPerFrame と一致させる)
     */
    struct cbPerFrame
    {
        DirectX::SimpleMath::Matrix matWorld; //< ワールド行列
        DirectX::SimpleMath::Matrix matView;  //< ビュー行列
        DirectX::SimpleMath::Matrix matProj;  //< 射影行列
        DirectX::SimpleMath::Vector4 diffuse; //< ディフューズ色
    };

public:
    /**
     * @brief シェーダに送る頂点データ構造
     */
    struct VertexParticle
    {
        DirectX::SimpleMath::Vector3 pos;   //< 座標
        DirectX::SimpleMath::Color   color; //< 色
        DirectX::SimpleMath::Vector2 size;  //< x:現在のサイズ, y:未使用
    };

    // コンストラクタ
    ParticleManager();
    // デストラクタ
    ~ParticleManager();
    // 初期化 (シェーダー読み込み、バッファ作成)
    void Create(ID3D11Device* device, ID3D11DeviceContext* context);
    // 更新処理
    void Update(float dt);
    // 描画処理
    void Render(ID3D11DeviceContext* context, const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj);

    // テクスチャ読み込み
    void LoadTexture(const wchar_t* path);

    // パーティクル追加
    void SpawnParticle(float life, DirectX::SimpleMath::Vector3 pos, DirectX::SimpleMath::Vector3  velocity,
        DirectX::SimpleMath::Vector3 accele, DirectX::SimpleMath::Vector3 startScale,
        DirectX::SimpleMath::Vector3 endScale, DirectX::SimpleMath::Color startColor,DirectX::SimpleMath::Color endColor);


private:
    // 最大パーティクル数（バッファ確保用）
    static const int MAX_PARTICLES = 2000;

    // --- メンバ変数 ---
    ID3D11Device* m_device  = nullptr;                                  //< D3D11デバイス
    ID3D11DeviceContext* m_context = nullptr;                           //< デバイスコンテキスト

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;          //< 頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;            //< ピクセルシェーダー
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;      //< ジオメトリシェーダー
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;            //< 入力レイアウト

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;                //< 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;              //< 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;         //< パーティクルテクスチャ
    std::unique_ptr<DirectX::CommonStates> m_states;                    //< 共通ステート

    std::list<std::unique_ptr<Particle>> m_particles;                   //< パーティクルリスト
    std::vector<VertexParticle> m_vertexList;                           //< CPU側頂点データ配列
};