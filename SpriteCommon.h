#pragma once
#include <wrl.h>
#include <d3d12.h>


#include"DirectXCommon.h"
#include <DirectXMath.h>

//スプライト共通部
class SpriteCommon
{
public:
	//縦方向ピクセル数
	const size_t textureWidth = 256;
	//横方向ピクセル数
	const size_t textureHeight = 256;
	//配列の要素数
	const size_t imageDataCount = textureWidth + textureHeight;
	//画像イメージデータ配列
	
	//全ピクセルの色を初期化
public://メンバ関数
	//初期化
	void Initialize(DirectXCommon*_dxCommon);
	//描画
	void PreDraw();
	//Getter
	DirectXCommon* GetDirectXCommon() { return dxCommon; }
private:
	DirectXCommon* dxCommon;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature;

	//画像
	DirectX::XMFLOAT4 imageData;

	Microsoft::WRL::ComPtr<ID3D12Resource> texBuff;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;


};

