#include"DxLib.h"
#include<math.h>
#include<stdlib.h>
//ボールの跳ね返りの計算(クリア) 
//ボールを自動で動かす(クリア)
//キーでラケットを動かす(クリア) 
//ラケットで打ち返す()
//スコアの表示(クリア)
//画面切り替え()
//

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//定数の定義
	const int WIDTH = 960, HEIGHT = 640;//ウインドウの幅・高さのピクセル数
	const int WHITE = GetColor(255, 255, 255);//よく使用する色

	SetWindowText("テニスゲーム");//ウインドウのタイトル

	SetGraphMode(WIDTH, HEIGHT, 32);//ウインドウサイズとカラービットの指定
	ChangeWindowMode(TRUE);//ウインドウモードで起動
	if (DxLib_Init() == -1)return-1;//ライブラリを初期化する

	SetBackgroundColor(0, 0, 0);
	SetDrawScreen(DX_SCREEN_BACK);//描画面を裏に
	//int timer = 0;//経過時間

	int imgbg = LoadGraph("image/bg.png");//背景画像読み込み
	int bgm = LoadSoundMem("sound/bgm.mp3");//サウンド・音量設定
	int jin = LoadSoundMem("sound/gameover.mp3");
	int se = LoadSoundMem("sound/hit.mp3");
	ChangeVolumeSoundMem(20, bgm);
	ChangeVolumeSoundMem(20, jin);


	//ボールの座標管理
	int ballX = 5;//ボールの中心(X)
	int ballY = 5;//ボールの中心(y)
	int ballVx = 5;//Xの速さ
	int ballVy = 5;//Yの速さ
	int ballR = 50;//ボールのサイズ

	//ラケットの座標管理
	int racketX = WIDTH / 2;//ラケットの中心(X)
	int racketY = HEIGHT - 50;//ラケットの中心(Y)
	int racketW = 120;//ラケットの幅
	int racketH = 12;//ラケットの高さ

	//スコアの入力式
	//int score = 0;
	//int highScore = 1000;

	//画面切り替え
	enum { TITLE, PLAY, OVER };
	int scene = TITLE;
	int timer = 0;
	int score = 0;//スコア入力
	int highScore = 1000;//ハイスコア入力
	int dx, dy;//ヒットチェックの文


	while (1)//{}内を繰り返し行う
	{
		ClearDrawScreen();//描画クリア
		timer++;

		switch (scene) //画面の分岐
		{
		case TITLE://タイトル
			SetFontSize(50);
			DrawString(WIDTH / 2 - 50 / 2 * 12 / 2, HEIGHT / 3, "Tennis Game", 0x00ff00);
			if (timer % 60 < 30)
			{
				SetFontSize(30);
				DrawString(WIDTH / 2 - 30 / 2 * 21 / 2, HEIGHT * 2 / 3, "Press SPACE to start.", 0x00ffff);
			}
			if (CheckHitKey(KEY_INPUT_SPACE) == 1)//スペースキーを押したとき開始する
			{
				ballX = 40;
				ballY = 80;
				ballVx = 5;
				ballVy = 5;
				racketX = WIDTH / 2;
				racketY = HEIGHT - 50;
				score = 0;
				scene = PLAY;
				PlaySoundMem(bgm, DX_PLAYTYPE_LOOP);//ループ再生
			}
			break;

		case PLAY://ゲームプレイ

			//int bollの跳ね返りの計算
			ballX = ballX + ballVx;//ボールの中心（Ｘ）＝ボールの中心（Ｘ）＋Ｘの速さ
			if (ballX < ballR && ballVx < 0)ballVx = -ballVx;//ボールのX座標＜ボールの半径になる上X座標の速さ＜０の場合
			if (ballX > WIDTH - ballR && ballVx > 0)ballVx = -ballVx;//ボールのX座標＞横幅になる上X座標の速さ＞０の場合

			ballY = ballY + ballVy;//ボールの中心（Ｙ）＝ボールの中心（Ｙ）＋Ｙの速さ
			if (ballY < ballR && ballVy < 0)ballVy = -ballVy;//ボールのY座標＜ボールの半径になる上Y座標の速さ＜０の場合
			//if (ballY > HEIGHT && ballVy > 0)ballVy = -ballVy;//ボールのY座標＜ボールの高さになる上Y座標の速さ＜０の場合

			if (ballY > HEIGHT)//ボールが下の端に到達したとき終了する
			{
				scene = OVER;
				timer = 0;
				break;
			}
			DrawCircle(ballX, ballY, ballR, GetColor(138, 43, 226), TRUE);//ボールの色（座標にballのX・Y・R座標を入力する）

			//ラケット

			if (CheckHitKey(KEY_INPUT_LEFT) == 1)//左キー
			{
				racketX = racketX - 10;
				if (racketX < racketW / 2)racketX = racketW / 2;
			}

			if (CheckHitKey(KEY_INPUT_RIGHT) == 1)//右キー
			{
				racketX = racketX + 10;
				if (racketX > WIDTH - racketW / 2)racketX = WIDTH - racketW / 2;
			}

			DrawBox(racketX - racketW / 2, racketY - racketH / 2, racketX + racketW / 2, racketY + racketH / 2, 0x0080ff, TRUE);
			//(説明文付け足し忘れずに)//////////////////////////


			//ヒットチェック
			//int dx = ballX - racketX;
			//int dy = ballY - racketY;
			dx = ballX - racketX;
			dy = ballY - racketY;
			if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0)//ballVy = -5 - rand() % 5;
			{//スコア・ハイスコア
				ballVy = -5 - rand() % 5;
				score = score + 100;
				if (score > highScore) highScore = score;
			}
			break;

		case OVER://ゲームオーバー
			SetFontSize(40);
			DrawString(WIDTH / 2 - 40 / 2 * 9 / 2, HEIGHT / 3, "GAME OVER", 0xff0000);
			if (timer > 60 * 5)scene = TITLE;
			break;
		}


		SetFontSize(30);//スコアとハイスコア表示
		DrawFormatString(10, 10, 0xffffff, "SCORE %d", score);
		DrawFormatString(WIDTH - 200, 10, 0xffff00, "HI-SC %d", highScore);



		//timer++;//時間のカウント
		//DrawFormatString(0, 0, WHITE, "%d", timer);//タイムの描画
		ScreenFlip();//裏画面を表に反映させる
		WaitTimer(16);//待機1秒間に16回描画する
		if (ProcessMessage() == -1)break;//常に情報を受け取りエラーが起きたら終了
		if (CheckHitKey(KEY_INPUT_ESCAPE) == 1)break;//ESCが押されたら終了
	}


	DxLib_End();
	return 0;




}
