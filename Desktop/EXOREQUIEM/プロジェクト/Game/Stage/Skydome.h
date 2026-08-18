/*****************************************************************//**
 * @file    Skydome.h
 * @brief   巨大な背景モデル（空）の描画、およびプレイヤー追従による無限遠演出の制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

// 前方宣言
class World;

/**
 * @class Skydome
 * @brief プレイヤーの位置に追従して背景（空）を描画するクラス
 */
class Skydome
{
public:
	// コンストラクタ
    Skydome();
	// デストラクタ
    ~Skydome();
    // 初期化処理
    void Initialize(ID3D11Device* device, World* world, const wchar_t* modelFileName);
    // 更新処理
    void Update();
    // 描画処理
    void Render(
        ID3D11DeviceContext* context,
        DirectX::CommonStates* states,
        const DirectX::SimpleMath::Matrix& view, 
        const DirectX::SimpleMath::Matrix& proj);

private:
    // ワールド行列の更新
    void UpdateWorldMatrix(const DirectX::SimpleMath::Vector3& playerPosition);

private:
    std::unique_ptr<DirectX::Model> m_model;        //< 背景モデルデータ
    DirectX::SimpleMath::Matrix m_worldMatrix;      //< ワールド行列

    World* m_world;                                 //< ワールド管理者への参照

    // --- 定数パラメータ ---
	static constexpr float DOME_SCALE = 1000.0f;     //< ドームの大きさを調整するスケール値
    static constexpr float ROTATION_X_DEGREE = 0.0f; //< モデルの向きを補正するためのX軸回転角度
};