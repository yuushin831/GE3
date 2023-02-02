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

	struct Vertex {
		DirectX::XMFLOAT3 pos;//xyz���W
		DirectX::XMFLOAT2 uv; //uv���W
	};
public://�����o�֐�
	//������
	void Initialize(SpriteCommon*_spriteCommon);

	void Draw();
private:
	static const int verticeCount = 4;
	//���_�f�[�^
	Vertex vertices[verticeCount] = {
	{{-0.5f,-0.5f,0.0f},{0.0f,1.0f}},//����
	{{-0.5f,+0.5f,0.0f},{0.0f,0.0f}},//����
	{{+0.5f,-0.5f,0.0f},{1.0f,1.0f}},//�E��
	{{+0.5f,+0.5f,0.0f},{1.0f,0.0f}},//�E��
	};

	SpriteCommon* spriteCommon = nullptr;

	DirectX::XMFLOAT4 color = { 1,0,0,0.5f };

	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;

	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffMaterial;
	ConstBuffDataMaterial *constMapMaterial = nullptr;


};

