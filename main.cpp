#include"DirectXCommon.h"
#include"Input.h"
#include"WinApp.h"

#include"Sprite.h"
#include"SpriteCommon.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#pragma region 基盤システム初期化
    //WindowsAPI
    WinApp* winApp = nullptr;
    winApp = new WinApp();
    winApp->Initialize();

    //DxCommon
    DirectXCommon* dxCommon = nullptr;
    dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    //Input
    Input* input = nullptr;
    input = new Input();
    input->Initialize(winApp);

    //SpriteCommon
    SpriteCommon* spriteCommon = nullptr;
    spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon);

#pragma endregion 基盤システム初期化

#pragma region 最初のシーンの初期化
    //Sprite
    Sprite* sprite = nullptr;
    sprite = new Sprite();
    sprite->Initialize(spriteCommon);
#pragma endregion 


    // ゲームループ
    while (true) {
#pragma region 基盤システムの更新
        //メッセージの処理
        if (winApp->ProcessMessage() == true)
        {
            //ゲームループを抜ける
            break;
        }
        //入力更新
        input->Update();
#pragma endregion 

#pragma region 最初のシーンの更新

#pragma endregion 
        //描画前処理
        dxCommon->PreDraw();
       
        
#pragma region 最初のシーンの描画
        spriteCommon->PreDraw();
       
#pragma endregion 
        //描画後処理
        dxCommon->PostDraw();
        sprite->Draw();
    }

#pragma region 最初のシーンの終了
    //Sprite
    delete sprite;
    sprite = nullptr;
#pragma endregion

#pragma region 基盤システムのの終了
    //SpriteCommon
    delete spriteCommon;
    spriteCommon = nullptr;

    //Input
    delete input;
    input = nullptr;

    //Dxcommon
    delete dxCommon;
    dxCommon = nullptr;

    //WinAPP
    winApp->Finalize();
    delete winApp;
    winApp = nullptr;
#pragma endregion 
    return 0;
}
