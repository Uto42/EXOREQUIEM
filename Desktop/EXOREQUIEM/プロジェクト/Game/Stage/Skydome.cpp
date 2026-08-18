/*****************************************************************//**
 * @file    Skydome.cpp
 * @brief   巨大な背景モデル（空）の描画、およびプレイヤー追従による無限遠演出の制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Skydome.h"
#include "Game/World/World.h"
#include "Game/GameObjects/Player/Player.h"

/**
 * @brief コンストラクタ
 */
Skydome::Skydome()
    : m_world(nullptr)
    , m_worldMatrix(DirectX::SimpleMath::Matrix::Identity)
{
}

/**
 * @brief デストラクタ
 */
Skydome::~Skydome()
{
}

/**
 * @brief 初期化処理
 * @param[in] device D3D11デバイス
 * @param[in] entityMgr エンティティ管理クラス（プレイヤーの位置を取得するため）
 */
void Skydome::Initialize(ID3D11Device* device, World* world, const wchar_t* modelFileName)
{
    m_world = world;

    // モデル読み込み
    DirectX::EffectFactory fx(device);
    fx.SetDirectory(L"Resources\\Models\\Skydome");
    std::wstring fullPath = L"Resources\\Models\\Skydome\\" + std::wstring(modelFileName);
    m_model = DirectX::Model::CreateFromSDKMESH(device, fullPath.c_str(), fx);

    // エフェクト設定（自己発光させてライティングの影響を受けないようにする）
    for (auto& mesh : m_model->meshes)
    {
        for (auto& part : mesh->meshParts)
        {
            auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(part->effect.get());
            if (basicEffect)
            {
                basicEffect->SetLightingEnabled(true);
                // 環境光のみを最大にして、陰影がつかないようにする
                basicEffect->SetAmbientLightColor(DirectX::Colors::LightGray);
            }
        }
    }
}

/**
 * @brief 更新処理
 */
void Skydome::Update()
{
    DirectX::SimpleMath::Vector3 playerPosition = DirectX::SimpleMath::Vector3::Zero;

    // プレイヤーの位置を取得
    if (m_world)
    {
        if (Player* player = m_world->GetPlayer())
        {
            playerPosition = player->GetPosition();
        }
    }

    // 行列計算を補助関数に任せる
    UpdateWorldMatrix(playerPosition);
}

/**
 * @brief ワールド行列の計算
 * @param[in] playerPosition 追従対象（プレイヤー）の座標
 */
void Skydome::UpdateWorldMatrix(const DirectX::SimpleMath::Vector3& playerPosition)
{
    // スカイドームをプレイヤーの位置に追従させる
    // 常にプレイヤーがドームの中心にいるようにすることで、無限遠を表現する
    DirectX::SimpleMath::Vector3 skydomePosition = playerPosition;

    // ワールド行列の計算
    // 拡大縮小(反転) -> 回転-> 平行移動
    m_worldMatrix = 
        DirectX::SimpleMath::Matrix::CreateScale(DOME_SCALE) 
        * DirectX::SimpleMath::Matrix::CreateRotationX(DirectX::XMConvertToRadians(ROTATION_X_DEGREE))
        * DirectX::SimpleMath::Matrix::CreateTranslation(skydomePosition);
}

/**
 * @brief 描画処理
 * @param[in] context Direct3Dのデバイスコンテキスト
 * @param[in] states 共通ステート
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 */
void Skydome::Render(ID3D11DeviceContext* context, DirectX::CommonStates* states,
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
    m_model->Draw(context, *states, m_worldMatrix, view, proj);
}