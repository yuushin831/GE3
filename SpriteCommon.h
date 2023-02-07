#pragma once
#include <wrl.h>
#include <d3d12.h>
#include<array>
#include<string>


#include"DirectXCommon.h"
#include <DirectXMath.h>

//�X�v���C�g���ʕ�
class SpriteCommon
{
public:
	//�摜�C���[�W�f�[�^�z��
	
	//�S�s�N�Z���̐F��������
public://�����o�֐�
	//������
	void Initialize(DirectXCommon*_dxCommon);
	//�`��
	void PreDraw();

	void PostDraw();
	
	void LoadTexture(uint32_t index, const std::string& fileName);

	void SetTextureCommands(uint32_t index);

	//Getter
	DirectXCommon* GetDirectXCommon() { return dxCommon; }
	ID3D12Resource* GetTextureBuffer(uint32_t index) const { return texBuff[index].Get(); }


private://�ÓI�����o�ϐ�

	static const size_t kMaxSRVCount = 2056;

	static std::string kDefaultTextureDirectoryPath;

private:
	DirectXCommon* dxCommon;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature;

	//�摜
	//DirectX::XMFLOAT4 imageData;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,kMaxSRVCount> texBuff;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;


};

