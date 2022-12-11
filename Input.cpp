#include"Input.h"
#include<cassert>
//#include<wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


//using namespace Microsoft::WRL;
void Input::Initialize(WinApp* winApp)
{
    HRESULT result;
    //借りてきたwinAppのインスタンスを記録
    this->winApp = winApp;




    // DirectInputの初期化
    result = DirectInput8Create(
        this->winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
    assert(SUCCEEDED(result));



   
    result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    // 入力データ形式のセット
    result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
    assert(SUCCEEDED(result));
    // 排他制御レベルのセット
    result = keyboard->SetCooperativeLevel(
        this->winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(result));
}

void Input::Update()
{
    HRESULT result;

    memcpy(keyPre, key, sizeof(key));
    // キーボード情報の取得開始
    result = keyboard->Acquire();
    // 全キーの入力状態を取得する
    result = keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE KeyNumber)
{
    if (key [KeyNumber])
    {
        return true;
    }
    return false;
}

bool Input::TriggerKey(BYTE KeyNumber)
{
    if (key[KeyNumber]&&!keyPre[KeyNumber])
    {
        return true;
    }
    return false;
}
