#define WIN32_LEAN_AND_MEAN
#include<math.h>
#include<stdlib.h>
#include <WinSock2.h>
#include<windows.h>
#include"DxLib.h"
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <unordered_map>


#pragma comment( lib, "ws2_32.lib" )
using namespace std;

//ボールの跳ね返りの計算(クリア) 
//ボールを自動で動かす(クリア)
//キーでラケットを動かす(クリア) 
//ラケットで打ち返す()
//スコアの表示(クリア)
//画面切り替え()
//

#pragma pack(push,1) //構造体のパディングを1バイトに設定
struct CIRCLE
{
	int centerX; //円中心X
	int centerY; //円中心Y
	int radius; //半径
	int color; //DiLibのGetColorで作った値
	int id; //送信元クライアントID
};
#pragma pack(pop)


const char* SERVER_IPADDRSS = "127.0.0.1";	//サーバーのIP番号
const unsigned short SERVER_PORT = 8888;	//サーバーのポート番号

static bool set_nonblocking(SOCKET s)// ソケットをノンブロッキングモードに設定
{
	unsigned long arg = 1;
	return ioctlsocket(s, FIONBIO, &arg) != SOCKET_ERROR;// 成功ならtrueを返す
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 定数の定義
    const int WIDTH = 960, HEIGHT = 640; // ウインドウの幅・高さのピクセル数
    const int WHITE = GetColor(255, 255, 255); // よく使用する色
    const int Tx_GTitleFont = 50; // タイトル文字サイズ
    const int Tx_GSystemFont = 30; // システム文字サイズ
    const int Tx_GOverFont = 40; // ゲームオーバー文字サイズ

    const int SenenFont = 50; // シーン変化文字サイズ
    const int SenenChars = 12; // シーン変化文字数

    const int promptFont = 30; // プロンプト文字サイズ
    const int promptChars = 21; // プロンプト文字数

    CIRCLE Circle = { 0, 0, 5, GetColor(0, 255, 0) }; // 送信データ格納用変数
    POINT cursorPos; // マウスカーソルの位置取得変数

    // WinSock2.2 初期化処理
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return -1;
    }

    // TCPリスンソケットの作成
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
    {
        WSACleanup();
        return -1;
    }
    set_nonblocking(listenSock);

    // サーバーのソケットアドレス構造体の作成
    SOCKADDR_IN serverSockAddress{};
    serverSockAddress.sin_family = AF_INET;
    serverSockAddress.sin_port = htons(SERVER_PORT);
    serverSockAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSock, (SOCKADDR*)&serverSockAddress, sizeof(serverSockAddress)) == SOCKET_ERROR)
    {
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    // リスン状態
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    // 2クライアント用のソケットを用意
    SOCKET socks[2] = { INVALID_SOCKET, INVALID_SOCKET };

    // 2クライアント用のソケットアドレス構造体を用意
    SOCKADDR_IN clientSocketAddresses[2];
    SOCKADDR_IN clientSockAddress;
    int clientAddressLength = sizeof(clientSockAddress);

    std::vector<char> recvBufs[2];
    SOCKADDR_IN serverAddr[2]; // サーバーのアドレス格納用変数
    SOCKADDR_IN clientAddr; // クライアントのアドレス格納用変数
    unordered_map<int, CIRCLE> others; // クライアントIDとCIRCLEのマップ（他クライアントの情報管理用）
    int addrLen = sizeof(clientAddr); // アドレス長

    SetWindowText("テニスゲーム"); // ウインドウのタイトル

    SetGraphMode(WIDTH, HEIGHT, 32); // ウインドウサイズとカラービットの指定
    ChangeWindowMode(TRUE); // ウインドウモードで起動
    if (DxLib_Init() == -1) return -1; // ライブラリを初期化する

    SetBackgroundColor(0, 0, 0);
    SetDrawScreen(DX_SCREEN_BACK); // 描画面を裏に

    int imgbg = LoadGraph("image/bg.png"); // 背景画像読み込み
    int bgm = LoadSoundMem("sound/bgm.mp3"); // サウンド・音量設定
    int jin = LoadSoundMem("sound/gameover.mp3");
    int se = LoadSoundMem("sound/hit.mp3");
    ChangeVolumeSoundMem(20, bgm);
    ChangeVolumeSoundMem(20, jin);

    // ボールの座標管理
    int ballX = 5; // ボールの中心(X)
    int ballY = 5; // ボールの中心(Y)
    int ballVx = 5; // Xの速さ
    int ballVy = 5; // Yの速さ
    int ballR = 50; // ボールのサイズ

    // 1Pラケットの座標管理
    int racketX = Circle.centerX; // ラケットの中心(X)
    int racketY = Circle.centerY; // ラケットの中心(Y)
    int racketW = 120; // ラケットの幅
    int racketH = 12; // ラケットの高さ

    // 2Pラケットの座標管理
    int topRacketX = WIDTH / 2;
    int topRacketY = 50; // 画面上から50px
    int topRacketW = 120;
    int topRacketH = 12;

    // 画面切り替え
    enum { TITLE, CONNECT, PLAY, OVER };
    int scene = TITLE;
    int timer = 0;
    int score = 0; // スコア入力
    int highScore = 1000; // ハイスコア入力
    int dx, dy; // ヒットチェックの文

    int TexT_Y = 3; // メイン文字の表示位置固定(Title等)

    int P1PlayAreaX = WIDTH; // 1Pのプレイエリア境界線
    int P1PlayAreaY = HEIGHT / 2; // 1Pのプレイエリア境界線

    const int MAX_CLIENT = 2; // クライアントの最大数
    int clientCount = 0; // 接続中のクライアント数の計測用
    int startTime = 0; // 接続完了後から開始までのカウント用

    while (1) // {}内を繰り返し行う
    {
        ClearDrawScreen(); // 描画クリア
        timer++;

        switch (scene) // 画面の分岐
        {
        case TITLE: // タイトル
            SetFontSize(Tx_GTitleFont);

            if (timer % 60 < 30)
            {
                SetFontSize(Tx_GSystemFont);
            }

            if (CheckHitKey(KEY_INPUT_SPACE)) // スペースキーを押したとき開始する
            {
                ballX = 40;
                ballY = 80;
                ballVx = 5;
                ballVy = 5;
                racketX = WIDTH / 2;
                racketY = HEIGHT - 50;
                score = 0;

                scene = CONNECT;
                PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); // ループ再生
            }
            break;

        case CONNECT:
            if (clientCount < MAX_CLIENT)
            {
                DrawString(WIDTH - 400, HEIGHT - 200, "接続待機中...", WHITE, TRUE);
            }

            for (int i = 0; i < MAX_CLIENT; i++)
            {
                int addressLength = sizeof(clientSocketAddresses[0]);
                SOCKET tmpSock = accept(listenSock, (SOCKADDR*)&clientSocketAddresses[clientCount], &addressLength);
                if (tmpSock == INVALID_SOCKET)
                {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK)
                    {
                        // クライアント未接続→そのまま待つ
                        break;
                    }
                    else
                    {
                        // 本当のエラー
                    }
                }
                else // クライアント接続成功
                {
                    socks[clientCount] = tmpSock;

                    set_nonblocking(socks[clientCount]);
                    recvBufs[clientCount].clear();
                    char ipaddress[20];
                    inet_ntop(AF_INET, &clientSocketAddresses[clientCount].sin_addr, ipaddress, sizeof(ipaddress)); // クライアントのIPアドレスを文字列に変換
                    clientCount++;
                }
            }

            // 全クライアント接続完了
            if (clientCount >= MAX_CLIENT)
            {
                if (startTime == 0) startTime = GetNowCount();

                int elapsed = GetNowCount() - startTime;
                DrawFormatString(WIDTH - 20, HEIGHT - 10, WHITE, "ゲーム開始まで　%d　秒", 3 - elapsed / 1000);
                if (elapsed >= 3000)
                {
                    scene = PLAY;
                    startTime = 0;
                }
            }
            break;

        case PLAY: // ゲームプレイ
            // ボールの跳ね返りの計算
            ballX = ballX + ballVx; // ボールの中心（Ｘ）＝ボールの中心（Ｘ）＋Ｘの速さ
            if (ballX < ballR && ballVx < 0) ballVx = -ballVx; // ボールのX座標＜ボールの半径になる上X座標の速さ＜０の場合

            if (ballX > WIDTH - ballR && ballVx > 0) ballVx = -ballVx; // ボールのX座標＞横幅になる上X座標の速さ＞０の場合

            ballY = ballY + ballVy; // ボールの中心（Ｙ）＝ボールの中心（Ｙ）＋Ｙの速さ
            if (ballY < ballR && ballVy < 0) ballVy = -ballVy; // ボールのY座標＜ボールの半径になる上Y座標の速さ＜０の場合

            if (ballY > HEIGHT) // ボールが下の端に到達したとき終了する
            {
                scene = OVER;
                timer = 0;
                break;
            }
            DrawCircle(ballX, ballY, ballR, GetColor(138, 43, 226), TRUE); // ボールの色（座標にballのX・Y・R座標を入力する）

            char temp[4096];

            for (int i = 0; i < clientCount; ++i)
            {
                if (socks[i] == INVALID_SOCKET) continue;

                // 受信可能な分をできるだけまとめて読む
                for (;;)
                {
                    int ret = recv(socks[i], temp, sizeof(temp), 0);

                    if (ret > 0)
                    {
                        // 受信データを蓄積
                        recvBufs[i].insert(recvBufs[i].end(), temp, temp + ret);

                        // 固定長メッセージ（CIRCLE）単位で切り出す
                        while ((int)recvBufs[i].size() >= (int)sizeof(CIRCLE))
                        {
                            CIRCLE netCircle;
                            memcpy(&netCircle, recvBufs[i].data(), sizeof(CIRCLE));
                            recvBufs[i].erase(recvBufs[i].begin(), recvBufs[i].begin() + sizeof(CIRCLE));

                            // ネットワーク→ホスト変換
                            CIRCLE circle;
                            circle.centerX = ntohl(netCircle.centerX);
                            circle.centerY = ntohl(netCircle.centerY);
                            circle.radius = ntohl(netCircle.radius);
                            circle.color = ntohl(netCircle.color);
                            circle.id = ntohl(netCircle.id);

                            //上側ラケットの座標に反映
                            topRacketX = circle.centerX;
                            topRacketY = circle.centerY;
                        }
                    }
                    else if (ret == 0)
                    {
                        // 相手が orderly shutdown（切断）
                        closesocket(socks[i]);
                        socks[i] = INVALID_SOCKET;
                        recvBufs[i].clear();
                        break; // このソケットのループを抜ける
                    }
                    else
                    {
                        int err = WSAGetLastError();
                        if (err == WSAEWOULDBLOCK)
                        {
                            // 未着：正常
                            break;
                        }
                        else
                        {
                            // 受信エラー：クリーンアップ
                            closesocket(socks[i]);
                            socks[i] = INVALID_SOCKET;
                            recvBufs[i].clear();
                            break;
                        }
                    }
                }

                // ラケット
                if (GetCursorPos(&cursorPos))
                {
                    // マウス位置を取得して更新
                    GetMousePoint(&Circle.centerX, &Circle.centerY);
                    if (Circle.centerY < P1PlayAreaY) Circle.centerY = P1PlayAreaY; // 1Pのプレイエリア制限

                    if (racketX < racketW / 2) racketX = racketW / 2; // ラケットの左端制限
                    // 円の描画
                    DrawCircle(Circle.centerX, Circle.centerY, Circle.radius, Circle.color, TRUE);

                    DrawBox(
                        racketX - racketW / 2,
                        racketY - racketH / 2,
                        racketX + racketW / 2,
                        racketY + racketH / 2,
                        0x0080ff, TRUE);

                    racketX = Circle.centerX; // ラケットの中心(X)
                    racketY = Circle.centerY; // ラケットの中心(Y)
                }

                // ヒットチェック
                dx = ballX - racketX;
                dy = ballY - racketY;
                if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0)
                {
                    ballVy = -5 - rand() % 5;
                    score = score + 100;
                    if (score > highScore) highScore = score;
                }
            }
            break;

        case OVER: // ゲームオーバー
            SetFontSize(Tx_GOverFont);
            DrawString(WIDTH / 2 - (SenenFont * SenenChars) / 4, HEIGHT / TexT_Y, "GAME OVER", 0xff0000);
            if (timer > 60 * 5) scene = TITLE;
            break;
        }

        SetFontSize(Tx_GSystemFont); // スコアとハイスコア表示
        DrawFormatString(10, 10, 0xfffff, "SCORE %d", score);
        DrawFormatString(WIDTH - 200, 10, 0xffff00, "HI-SC %d", highScore);

        ScreenFlip(); // 裏画面を表に反映させる
        WaitTimer(16); // 待機1秒間に16回描画する
        if (ProcessMessage() == -1) break; // 常に情報を受け取りエラーが起きたら終了
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break; // ESCが押されたら終了
    }

    DxLib_End();
    return 0;
}
