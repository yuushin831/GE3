#pragma once
#include<DirectXMath.h>
#include "SpriteCommon.h"


//�X�v���C�g
class Sprite
{
private:
	struct ConstBuffDataMaterial {

		DirectX::XMFLOAT4 color;//�F(RGBA)
	};
public://�����o�֐�
	//������
	void Initialize(SpriteCommon*_spriteCommon);

	void Draw();
private:
	SpriteCommon* spriteCommon = nullptr;

	DirectX::XMFLOAT4 color = { 1,0,0,0.5f };

	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;

	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffMaterial;
	ConstBuffDataMaterial *constMapMaterial = nullptr;


};

