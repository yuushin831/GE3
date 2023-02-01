#pragma once
#include <wrl.h>
#include <d3d12.h>


#include"DirectXCommon.h"
#include <DirectXMath.h>

//�X�v���C�g���ʕ�
class SpriteCommon
{
public:
	//�c�����s�N�Z����
	const size_t textureWidth = 256;
	//�������s�N�Z����
	const size_t textureHeight = 256;
	//�z��̗v�f��
	const size_t imageDataCount = textureWidth + textureHeight;
	//�摜�C���[�W�f�[�^�z��
	
	//�S�s�N�Z���̐F��������
public://�����o�֐�
	//������
	void Initialize(DirectXCommon*_dxCommon);
	//�`��
	void PreDraw();
	//Getter
	DirectXCommon* GetDirectXCommon() { return dxCommon; }
private:
	DirectXCommon* dxCommon;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature;

	//�摜
	DirectX::XMFLOAT4 imageData;

	Microsoft::WRL::ComPtr<ID3D12Resource> texBuff;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;


};

