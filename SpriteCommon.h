#pragma once
#include <wrl.h>
#include <d3d12.h>
#include<array>
#include<string>


#include"DirectXCommon.h"
#include <DirectXMath.h>

//スプライト共通部
class SpriteCommon
{
public:
	//画像イメージデータ配列
	
	//全ピクセルの色を初期化
public://メンバ関数
	//初期化
	void Initialize(DirectXCommon*_dxCommon);
	//描画
	void PreDraw();

	void PostDraw();
	
	void LoadTexture(uint32_t index, const std::string& fileName);

	void SetTextureCommands(uint32_t index);

	//Getter
	DirectXCommon* GetDirectXCommon() { return dxCommon; }
	ID3D12Resource* GetTextureBuffer(uint32_t index) const { return texBuff[index].Get(); }


private://静的メンバ変数

	static const size_t kMaxSRVCount = 2056;

	static std::string kDefaultTextureDirectoryPath;

private:
	DirectXCommon* dxCommon;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature;

	//画像
	//DirectX::XMFLOAT4 imageData;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,kMaxSRVCount> texBuff;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;


};

