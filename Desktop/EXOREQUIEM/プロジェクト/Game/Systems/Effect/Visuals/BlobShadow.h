/*****************************************************************//**
 * @file    BlobShadow.h
 * @brief   ユニットの足元に投影される簡易的な円形影（丸影）の描画制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

 /**
  * @class BlobShadow
  * @brief 丸影（Blob Shadow）を描画するクラス
  * プレイヤーや敵の足元に配置して使用する
  */
class BlobShadow
{
public:
	// コンストラクタ
    BlobShadow();
	// デストラクタ
    ~BlobShadow() = default;
    // 初期化処理
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    // 描画処理
    void Render(
        ID3D11DeviceContext* context, 
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj,  
        const DirectX::SimpleMath::Vector3& position,
        float scale = 1.5f,
        float groundY = 0.0f);

private:
	// --- 調整用定数パラメータ ---
	static constexpr int32_t RASTER_DEPTH_BIAS = -2000;                 //< ラスタライザの深度バイアス値
	static constexpr float RASTER_SLOPE_SCALED_DEPTH_BIAS = -4.0f;      //< 深度バイアスのスケーリング値

	static constexpr float GROUND_HEIGHT_OFFSET = 0.01f;		        //< 地面からの微小な高さオフセット

	static constexpr float HEIGHT_ATTENUATION_FACTOR = 0.1f;            //< 高さに応じた影の縮小率

    // --- 描画用リソース ---
    std::unique_ptr<DirectX::GeometricPrimitive> m_plane;               //< 板ポリゴン
    std::unique_ptr<DirectX::BasicEffect> m_effect;                     //< 描画用エフェクト
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;         //< 影テクスチャ
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;            //< レイアウト

    // --- ステート関連 ---
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;              //< 半透明合成用
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;       //< 深度設定用
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;    //< ラスタライザ設定用
};