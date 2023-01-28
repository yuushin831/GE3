#pragma once
#include <wrl.h>
#include <d3d12.h>


#include"DirectXCommon.h"

//スプライト共通部
class SpriteCommon
{
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

};

