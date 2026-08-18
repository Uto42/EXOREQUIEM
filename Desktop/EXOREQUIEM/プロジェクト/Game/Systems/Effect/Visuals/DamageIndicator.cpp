/*****************************************************************//**
 * @file    DamageIndicator.cpp
 * @brief   戦闘中のダメージ方向描画制御（インディケーター）の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Effect/Visuals/DamageIndicator.h"
#include <fstream>

// シェーダーファイルのパス
static const wchar_t* SHADER_VS_PATH = L"Resources/Shaders/DamageIndicatorVS.cso";
static const wchar_t* SHADER_PS_PATH = L"Resources/Shaders/DamageIndicatorPS.cso";

/**
 * @brief シェーダーバイナリを読み込むヘルパー関数
 * @param[in] fileName ファイルパス
 * @return std::vector<char> 読み込んだバイナリデータ
 */
static std::vector<char> LoadShaderFile(const wchar_t* fileName)
{
    std::ifstream ifs(fileName, std::ios::in | std::ios::binary);
    if (!ifs) return {};

    ifs.seekg(0, std::ios::end);
    size_t size = (size_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> data(size);
    ifs.read(data.data(), size);
    return data;
}

/**
 * @brief コンストラクタ
 */
DamageIndicator::DamageIndicator()
{
}

/**
 * @brief デストラクタ
 */
DamageIndicator::~DamageIndicator()
{
}

/**
 * @brief 初期化処理
 * @param[in] device ID3D11Device
 */
void DamageIndicator::Initialize(ID3D11Device* device)
{
    // --- 頂点シェーダーのロード ---
    auto vsData = LoadShaderFile(SHADER_VS_PATH);
    if (!vsData.empty())
    {
        device->CreateVertexShader(vsData.data(), vsData.size(), nullptr, m_vs.GetAddressOf());
    }

    // --- ピクセルシェーダーのロード ---
    auto psData = LoadShaderFile(SHADER_PS_PATH);
    if (!psData.empty())
    {
        device->CreatePixelShader(psData.data(), psData.size(), nullptr, m_ps.GetAddressOf());
    }

    // --- 定数バッファの作成 ---
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(ConstantBufferData);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());

    // --- 加算合成用ステートの作成 ---
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    device->CreateBlendState(&blendDesc, m_additiveBlend.GetAddressOf());
}

/**
 * @brief ダメージヒット情報を追加
 * @param[in] playerPos プレイヤーの座標
 * @param[in] playerRotation プレイヤーの回転（クォータニオン）
 * @param[in] enemyPos 敵（攻撃元）の座標
 */
void DamageIndicator::AddHit(const DirectX::SimpleMath::Vector3& playerPos,
    const DirectX::SimpleMath::Vector3& camForward, const DirectX::SimpleMath::Vector3& camRight, 
    const DirectX::SimpleMath::Vector3& enemyPos)
{
    // 敵への方向ベクトル (水平面のみ)
    DirectX::SimpleMath::Vector3 dirToEnemy = enemyPos - playerPos;
    dirToEnemy.y = 0.0f;
    dirToEnemy.Normalize();

    // カメラの向き (水平面のみ)
    DirectX::SimpleMath::Vector3 f = camForward;
    DirectX::SimpleMath::Vector3 r = camRight;
    f.y = 0.0f;
    r.y = 0.0f;
    f.Normalize();
    r.Normalize();

    // 内積でカメラ基準のUI方向を計算
    float x = dirToEnemy.Dot(r);
    float y = dirToEnemy.Dot(f);

    HitData hit;
    hit.direction = DirectX::SimpleMath::Vector2(x, -y);
    hit.timer = 0.0f;

    m_hits.push_back(hit);
}

/**
 * @brief 更新処理
 * @param[in] dt デルタタイム
 */
void DamageIndicator::Update(float dt)
{
    // 寿命が尽きたものを削除する安全なループ
    for (int i = (int)m_hits.size() - 1; i >= 0; --i)
    {
        // 時間を進める (1.0秒で消える設定)
        m_hits[i].timer += dt * FADE_SPEED_MULTIPLIER; // 1.5倍速で消える

        // 寿命が尽きたら削除
        if (m_hits[i].timer >= MAX_LIFETIME)
        {
            m_hits.erase(m_hits.begin() + i);
        }
    }
}

/**
 * @brief 描画処理
 * @param[in] context ID3D11DeviceContext
 */
void DamageIndicator::Render(ID3D11DeviceContext* context)
{
    if (m_hits.empty()) return;

    // --- ステート設定 ---

    // 加算合成ON
    context->OMSetBlendState(m_additiveBlend.Get(), nullptr, 0xFFFFFFFF);

    // シェーダセット
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);

    // トポロジー設定 (重要: 頂点バッファなしで描くので三角形リストを指定)
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 入力レイアウトと頂点バッファを解除
    context->IASetInputLayout(nullptr);
    ID3D11Buffer* nullBuffer = nullptr;
    UINT stride = 0;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

    // --- ビューポートからアスペクト比を取得 ---
    UINT numViewports = 1;
    D3D11_VIEWPORT viewport;
    context->RSGetViewports(&numViewports, &viewport);
    float aspectRatio = viewport.Width / viewport.Height;

    // --- 個別描画 ---
    for (const auto& hit : m_hits)
    {
        // 定数バッファの更新
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            ConstantBufferData* data = (ConstantBufferData*)mappedResource.pData;
            data->direction = hit.direction;
            data->timer = hit.timer;
            data->aspectRatio = aspectRatio;
            context->Unmap(m_constantBuffer.Get(), 0);
        }

        context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        // 1つのインディケーターにつき三角形1枚(3頂点)描画
        context->Draw(INDICATOR_VERTEX_COUNT, 0);
    }

    // --- ステート戻し ---
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}