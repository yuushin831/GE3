#pragma once
#include<windows.h>
#include<wrl.h>

#define DIRECTINPUT_VERSION     0x0800   // DirectInput�̃o�[�W�����w��
#include <dinput.h>
class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	void Initialize(HINSTANCE hInstance,HWND hwnd);

	void Update();

private:
	// �L�[�{�[�h�f�o�C�X�̐���
	ComPtr<IDirectInputDevice8> keyboard;
};
