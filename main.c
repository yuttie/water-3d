/*
 *******************************************************************************
 *
 *  main.c - Water-3D
 *
 *  Copyright  2004  Yuta Taniguchi
 *******************************************************************************
 */


/* <<< Changelog >>>
 * >>> Sat, 07 Aug 2004 00:14:51 +0900 <<<
 * New:  F10キーでRedqueen形式の頂点情報と形状情報をdata.rrtファイルに出力するようにした。
 * New:  ウィンドウ上のマウスカーソルの座標を、仮想水面上の座標にマッピングするようにした。
 * New:  水際で自由端反射するようにした。
 * 
 * >>> Wed, 04 Aug 2004 00:17:51 +0900 <<<
 * New:  基本サイズの波紋の表示上の大きさがどれも同じになるようにした。
 * New:  表示上の水面の大きさを200x200の固定にした。
 * New:  辺のみ描画するようにした。
 */


/* <<< What I'm Doing >>>
 */


/* <<< To Do >>>
 * FPSだけでなく、ポリゴン数/秒、頂点数/秒も表示できるようにする
 * 初期値読み込み
 * データ書き出し
 */


/* <<< Max FPS(400x300) - debug >>>
 */


/* <<< Idea >>>
 */


/* <<< Specification >>>
 */


/* Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include "SDL/SDL.h"
#include "imgscale.h"
#include "cpuidutil.h"


/* Type Definition */
typedef Sint16 PosData;
typedef struct __ProgConfig
{
    char  *pBgImgPath;      // 背景イメージのファイル名
    int    depthRes;        // 深さの解像度
    int    riplRadius;      // 発生させる波紋の半径
    double riplDepth;       // 発生させる波紋の最大深さ
    int    widthRes;        // シミュレートする水面の幅の解像度
    int    heightRes;       // シミュレートする水面の高さの解像度
    int    wndWidth;        // ウィンドウの幅
    int    wndHeight;       // ウィンドウの高さ
    double attRate;         // 減衰率
    double scale;
    int    csrIPDiv;        // カーソルの補間を何分割で行うか
    bool   isFullScreen;    // フルスクリーンか否か
} ProgConfig;
typedef struct __Vector3
{
    double x, y, z;
} Vector3;


/* Function Prototype Declaration */
int      main(int argc, char **argv);
void         InitProc(int argc, char **argv);
bool             ParseArgument(int argc, char **argv);
void             InitSDL();
PosData         *CreateRippleData();
Uint16          *CreateRefractionTable();
bool         EventProc();
void             WriteOutRRT(char *fname);
Vector3          Get3DCoordinate(int x, int y);
void             RippleOut(int x, int y);
void         Calculate();
void         Draw();
void         ExitProc();


/* Constant */
const double PI = 3.1415926535;    // 円周率
// 計算の都合で本来の最大値65536の1/4しか利用できない
const int MAX_RESOLUTION = 65536 / 4;

/* TextData */
const char const *MSG_ERROR_OPTION =
    "ERROR: Invalid option '%s' was specified.\n";
const char const *MSG_ERROR_LOAD_IMAGE =
    "ERROR: Can't load image '%s'.\n";
const char const *MSG_ERROR_INIT_SDL =
    "ERROR: Initialization of SDL failed : %s\n";
const char const *MSG_ERROR_INIT_VIDEO =
    "ERROR: Initialization of video failed: %s\n";
const char const *MSG_HELP =
    "<usage>\n"
    "    water [option]\n"
    "<Options>\n"
    "    ** MemorySize(-mN) = HeightResolution(-pN)^2 + 2byte\n"
    "    ** '-rN' is equal to '-r N'\n"
    "    -h                : Show this help.\n"
    "    -f                : Use full-screen mode.\n"
    "    -aR(=0.99)        : Set rate of attenuation as R(0 < R < 1).\n"
    "    -bN(=20.0)        : Set depth of base ripple as N.\n"
    "    -rN(=8)           : Set ripple radius as N.\n"
    "    -dN(=1)           : Set number of interpolating division as N(0 < N).\n"
    "    -iF(=bgimage.bmp) : Use F as background image.\n"
    "    -sWxH(=256x192)   : Simulate water of the size of WxH.\n"
    "    -mN(=512)         : Use N KByte cache. Affect '-p' option.\n"
    "    -pN(=512)         : Set depth resolution as N. Affect '-m' option.\n";


/* Global Variable */
ProgConfig   g_Conf;         // プログラムの設定
PosData     *g_pNextData;    // 次の水面データ
PosData     *g_pCrntData;    // 今の水面データ
PosData     *g_pPrevData;    // 前の水面データ
PosData     *g_pCrntRipl;    // 現在の波紋データへのポインタ
PosData    **g_pRipples;     // 波紋データ
Uint16      *g_pRfraTbl;     // 屈折による変移量テーブル
SDL_Surface *g_pScreen;      // スクリーンサーフェス
SDL_Surface *g_pBgImage;     // 背景サーフェス


/* Main Function */
int main(int argc, char **argv)
{
    // プログラム初期化
    InitProc(argc, argv);
    
    // FPS計測用変数
    Uint32 startTick = SDL_GetTicks();
    Uint32 endTick = 0;
    Uint32 frameCount = 0;
    
    // 無限ループ
    while(!EventProc())
    {
        // 水面をスクリーンサーフェスに描画する
        Draw();

        // 水面データを入れ替える
        PosData *tmp = g_pPrevData;
        g_pPrevData = g_pCrntData;
        g_pCrntData = g_pNextData;
        g_pNextData = tmp;
        
        // 水面を計算
        Calculate();
        
        // FPS計測
        endTick = SDL_GetTicks();
        frameCount++;
        if (endTick - startTick > 1000)
        {
            // ウィンドウキャプションに表示するための文字列を作成
            char cap[16];
            sprintf(cap, "FPS : %#.2f", frameCount * 1000.0 / (endTick - startTick));
            // 文字列をウィンドウキャプションに設定
            SDL_WM_SetCaption(cap, NULL);
            // スタート時刻・フレーム数のリセット
            startTick = SDL_GetTicks();
            frameCount = 0;
        }
    }
    
    // プログラム終了処理
    ExitProc();
    
    return 0;
}


/* プログラム初期化関数 */
void InitProc(int argc, char **argv)
{
    // プログラムオプションのデフォルト値を設定
    g_Conf.pBgImgPath = "bgimage.bmp";
    g_Conf.depthRes = 512;
    g_Conf.riplRadius = 4;
    g_Conf.riplDepth = 20.0;
    g_Conf.widthRes = 80;
    g_Conf.heightRes = 80;
    g_Conf.wndWidth = 640;
    g_Conf.wndHeight = 480;
    g_Conf.attRate = 0.99;
    g_Conf.scale = g_Conf.depthRes / 200.0;
    g_Conf.csrIPDiv = 1;
    g_Conf.isFullScreen = false;
    
    // 引数解析
    if (ParseArgument(argc, argv))
    {
        // 冗長モード時 : 設定値を標準出力に出力する
        printf("BgImagePath : %s\n", g_Conf.pBgImgPath);
        printf("DepthResolution : %d\n", g_Conf.depthRes);
        printf("RippleRadius : %d\n", g_Conf.riplRadius);
        printf("RippleDepth : %f\n", g_Conf.riplDepth);
        printf("WidthResolution : %d\n", g_Conf.widthRes);
        printf("HeightResolution : %d\n", g_Conf.heightRes);
        printf("WindowWidth : %d\n", g_Conf.wndWidth);
        printf("WindowHeight : %d\n", g_Conf.wndHeight);
        printf("AttenuationRate : %f\n", g_Conf.attRate);
        printf("InterpolatingDivide : %d\n", g_Conf.csrIPDiv);
        printf("FullScreen : %s\n", g_Conf.isFullScreen ? "ON" : "OFF");
        /*
        printf("CPUID : %s\n", CheckCPUID() ? "Supported" : "Not Supported");
        printf("MMX : %s\n", CheckMMX() ? "Supported" : "Not Supported");
        printf("SSE : %s\n", CheckSSE() ? "Supported" : "Not Supported");
        printf("SSE2 : %s\n", CheckSSE2() ? "Supported" : "Not Supported");
        printf("EDX : %d\n", GetEDX());
        */
    }
    
    // SDL初期化してスクリーンサーフェスを取得
    InitSDL();
    
    // 背景画像読み込み
    SDL_Surface *tmpBg = SDL_LoadBMP(g_Conf.pBgImgPath);
    if (tmpBg == NULL)
    {
        // エラー表示をして終了
        fprintf(stderr, MSG_ERROR_LOAD_IMAGE, g_Conf.pBgImgPath);
        exit(EXIT_FAILURE);
    }
    
    // 背景サーフェスをスクリーンサーフェスと同じ形式に変換
    tmpBg = SDL_ConvertSurface(tmpBg, g_pScreen->format, SDL_SWSURFACE);
    
    // 読み込んだ背景を水面の大きさに合わせる
    g_pBgImage = SDL_CreateRGBSurface(SDL_SWSURFACE,
            g_Conf.widthRes, g_Conf.heightRes, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    ScaleCopySurface(SM_BI_LINEAR, tmpBg, g_pBgImage);
    
    // 水面データ配列作成
    int arraySize = (1 + g_Conf.widthRes + 1) * (1 + g_Conf.heightRes + 1);
    g_pNextData = calloc(arraySize, sizeof(PosData));
    g_pCrntData = calloc(arraySize, sizeof(PosData));
    g_pPrevData = calloc(arraySize, sizeof(PosData));
    
    // 波紋データ作成
    g_pCrntRipl = CreateRippleData();
    
    // 屈折テーブルを作成
//    g_pRfraTbl = CreateRefractionTable();
    
    // 屈折テーブルアクセス時に必要なオフセットを加算しておく
//    g_pRfraTbl += (g_Conf.depthRes * (MaxWaterHeight - MinWaterHeight + 1)) - MinWaterHeight;
}


/* イベント処理関数 */
bool EventProc()
{
    int topLimit = g_Conf.riplRadius;
    int bottomLimit = g_Conf.heightRes - g_Conf.riplRadius;
    int leftLimit = g_Conf.riplRadius;
    int rightLimit = g_Conf.widthRes - g_Conf.riplRadius;
    bool exitFlag = false;    // 終了要求が有るか否か
    static int preCsrX;    // 前のイベント発生時のマウスカーソルのX座標
    static int preCsrY;    // 前のイベント発生時のマウスカーソルのY座標
    SDL_Event event;          // イベントに関する情報が入る構造体(共用体?)
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            // キーダウンイベント
            switch (event.key.keysym.sym)
            {
            case SDLK_F10:
                // データ出力キー
                WriteOutRRT("data.rrt");
                break;
            case SDLK_ESCAPE:
            case SDLK_q:
                // 終了キー(ESC, Q)
                exitFlag = true;
                break;
            default:
                // その他のキー(ハンドルされないキー)
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            // マウスボタンダウンイベント
            // マウスのボタンが押されている時
            if (event.button.state == SDL_PRESSED)
            {
                // 3D空間上の座標を得る
                Vector3 pos = Get3DCoordinate(event.button.x, event.button.y);
                // ウィンドウ上の点を水面上の位置に変換
                int vx = (pos.x + 100.0) * (g_Conf.widthRes - 1) / 200.0;
                int vy = (pos.z + 100.0) * (g_Conf.heightRes - 1) / 200.0;
                // ウィンドウ上の点を水面上の位置に変換
//                vx = (float)g_Conf.widthRes * event.button.x / g_Conf.wndWidth;
//                vy = (float)g_Conf.heightRes * event.button.y / g_Conf.wndHeight;
                // 波紋発生可能領域にいるかどうかをチェック
                if ((leftLimit < vx) && (vx < rightLimit) &&
                    (topLimit < vy) && (vy < bottomLimit))
                {
                    // 波紋を発生
                    RippleOut(vx, vy);
                    preCsrX = vx;
                    preCsrY = vy;
                }
            }
            break;
        case SDL_MOUSEMOTION:
            // マウスカーソル移動イベント
            // マウスのボタンが押されている時
            if (event.motion.state == SDL_PRESSED)
            {
                // 3D空間上の座標を得る
                Vector3 pos = Get3DCoordinate(event.motion.x, event.motion.y);
                // ウィンドウ上の点を水面上の位置に変換
                int vx = (pos.x + 100.0) * (g_Conf.widthRes - 1) / 200.0;
                int vy = (pos.z + 100.0) * (g_Conf.heightRes - 1) / 200.0;
                // ウィンドウ上の点を水面上の位置に変換
//                vx = (float)g_Conf.widthRes * event.motion.x / g_Conf.wndWidth;
//                vy = (float)g_Conf.heightRes * event.motion.y / g_Conf.wndHeight;
                // 波紋発生可能領域にいるかどうかをチェック
                if ((leftLimit < vx) && (vx < rightLimit) &&
                    (topLimit < vy) && (vy < bottomLimit))
                {    // 波紋を発生
                    for (int i = 0; i < g_Conf.csrIPDiv; i++)
                    {
                        RippleOut((vx * i + preCsrX * (g_Conf.csrIPDiv - i)) / g_Conf.csrIPDiv,
                                  (vy * i + preCsrY * (g_Conf.csrIPDiv - i)) / g_Conf.csrIPDiv);
                    }
                    preCsrX = vx;
                    preCsrY = vy;
                }
            }
            break;
        case SDL_QUIT:
            // 終了イベント
            exitFlag = true;
            break;
        }
    }
    return exitFlag;
}


/* プログラム終了前の後始末 */
void ExitProc()
{
    // 屈折テーブルアクセス時に必要なオフセットを減算しておく
//    g_pRfraTbl -= (-(MinWaterHeight - MaxWaterHeight) * (MaxWaterHeight - MinWaterHeight + 1)) - MinWaterHeight;
    
    // メモリの解放
    SDL_FreeSurface(g_pBgImage);
    free(g_pNextData);
    free(g_pCrntData);
    free(g_pPrevData);
    free(g_pCrntRipl);
    free(g_pRfraTbl);
    
    // SDL終了
    SDL_Quit();
}


/* 起動時引数解析関数 */
bool ParseArgument(int argc, char **argv)
{
    bool isVerboseMode = false;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
            case 'h':
                // 使い方を表示して終了
                printf(MSG_HELP); exit(EXIT_SUCCESS);
                break;
            case 'v':
                // 冗長な表示を行う
                isVerboseMode = true;
                break;
            case 'f':
                // フルスクリーンモードに設定
                g_Conf.isFullScreen = true;
                break;
            case 'a':
                // 波紋の減衰率を取得
                if (argv[i][2] == '\0') g_Conf.attRate = atof(&argv[++i][0]);
                else                    g_Conf.attRate = atof(&argv[i][2]);
                break;
            case 'b':
                // 一倍の波紋の最大の深さ
                if (argv[i][2] == '\0') g_Conf.riplDepth = atof(&argv[++i][0]);
                else                    g_Conf.riplDepth = atof(&argv[i][2]);
                break;
            case 'r':
                // 波紋の大きさを取得
                if (argv[i][2] == '\0') g_Conf.riplRadius = atoi(&argv[++i][0]);
                else                    g_Conf.riplRadius = atoi(&argv[i][2]);
                break;
            case 'd':
                // 補間時の分割数を取得
                if (argv[i][2] == '\0') g_Conf.csrIPDiv = atoi(&argv[++i][0]);
                else                    g_Conf.csrIPDiv = atoi(&argv[i][2]);
                break;
            case 'i':
                // 背景のファイル名を取得
                if (argv[i][2] == '\0') g_Conf.pBgImgPath = &argv[++i][0];
                else                    g_Conf.pBgImgPath = &argv[i][2];
                break;
            case 's':
                // 計算上の水面のサイズを取得
                if (argv[i][2] == '\0')
                {
                    char *endPtr;
                    g_Conf.widthRes = (int)strtoul(&argv[++i][0], &endPtr, 10);
                    g_Conf.heightRes = (int)strtoul(++endPtr, NULL, 10);
                }
                else
                {
                    char *endPtr;
                    g_Conf.widthRes = (int)strtoul(&argv[i][2], &endPtr, 10);
                    g_Conf.heightRes = (int)strtoul(++endPtr, NULL, 10);
                }
                g_Conf.wndWidth = g_Conf.widthRes;
                g_Conf.wndHeight = g_Conf.heightRes;
                break;
            case 'm':
            {
                // メモリ使用可能量(KB単位)を取得
                // 水面の高さの精度(解像度)に影響する
                int mem;
                if (argv[i][2] == '\0') mem = atoi(&argv[++i][0]);
                else                    mem = atoi(&argv[i][2]);
                
                // 水面の高さに換算し、補正
                // 波の計算時に、上下左右を足すから
                // PosData(Sint16)の最大値32767/4=8191より
                // 各ピクセルの高さの最大値は8191、最小値は-8192
                // 解像度の最大値は8191 - (-8192) + 1=16384
                // 高低差の最小値は0、最大値は8191-(-8192)=16383
                // つまり、これは解像度-1に等しい。
                // 屈折テーブル用メモリの確保量は
                // 確保量=g_Conf.depthRes^2 * 2byte(16bit))
                g_Conf.depthRes = (int)(sqrt(mem * 1024 / 2.0));

                // 指定された解像度が最大値を越えていたら修正する。
                if (g_Conf.depthRes > MAX_RESOLUTION)
                    g_Conf.depthRes = MAX_RESOLUTION;
                
                //
//                    printf("%d\n", g_Conf.depthRes);
                
                // 波紋の高さを再計算
                //g_Conf.riplDepth = (g_Conf.depthRes - 1) / 2;
                
                break;
            }
            case 'p':
                // 水面の高さの精度(解像度)を取得
                // メモリ使用量に影響する
                if (argv[i][2] == '\0') g_Conf.depthRes = atoi(&argv[++i][0]);
                else                    g_Conf.depthRes = atoi(&argv[i][2]);
                
                // 指定された解像度が最大値を越えていたら修正する。
                if (g_Conf.depthRes > MAX_RESOLUTION)
                    g_Conf.depthRes = MAX_RESOLUTION;
                
                // 波紋の高さを再計算
                //g_Conf.riplDepth = (g_Conf.depthRes - 1) / 2;
                
                break;
            default:
                // 不正な引数が指定された場合、エラー表示をして終了
                fprintf(stderr, MSG_ERROR_OPTION, argv[i]);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // 不正な引数が指定された場合、エラー表示をして終了
            fprintf(stderr, MSG_ERROR_OPTION, argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    
    // 冗長な表示を行うか否かを返す
    return isVerboseMode;
}


/* SDL初期化関数 */
void InitSDL()
{
    // SDLを初期化
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        // エラー表示をして終了
        fprintf(stderr, MSG_ERROR_INIT_SDL, SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    // ディスプレイデバイスの初期化フラグを設定
    Uint32 screenFlags = SDL_SWSURFACE | SDL_OPENGL;
    if (g_Conf.isFullScreen) screenFlags |= SDL_FULLSCREEN;
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // ディスプレイデバイスを初期化
    g_pScreen = SDL_SetVideoMode(g_Conf.wndWidth, g_Conf.wndHeight, 32, screenFlags);
    
    // 初期化に失敗したら、エラーメッセージを表示して終了
    if (g_pScreen == NULL)
    {
        // エラー表示をして終了
        fprintf(stderr, MSG_ERROR_INIT_VIDEO, SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    // GL Setup
    // Depth Buffer
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    
    // Culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    
    // Clear Color
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // Viewport
    glViewport(0, 0, g_Conf.wndWidth, g_Conf.wndHeight);

    // Projection Matrix and set our viewing volume.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = (float)g_Conf.wndWidth / (float)g_Conf.wndHeight;
    gluPerspective(60.0, ratio, 1.0, 1024.0);
}


/* 波紋データ作成関数 */
PosData *CreateRippleData()
{
    int radius = g_Conf.riplRadius;
    // 波紋データ作成
    PosData *riplData = calloc((2 * radius + 1) * (2 * radius + 1), sizeof(PosData));
    PosData *pData = riplData;
    
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            // x = 0, y = 0を中心とする
            // 中心からの距離の二乗を求める
            int r2 = (x * x) + (y * y);
            
            if (r2 < radius * radius)
            {   // 円内
                double t = PI * (sqrt(r2) / radius);
                *pData = (PosData)((cos(t) + 1) * (g_Conf.depthRes / 2) * (g_Conf.riplDepth / 100.0) * (g_Conf.riplRadius / 8.0) / 2);
            }
            else
            {   // 円外
                *pData = 0;
            }
            pData++;
        }
    }
    return riplData;
}


/* 屈折テーブル作成関数 */
Uint16 *CreateRefractionTable()
{
/*
    // 512をデフォルトの分解能とすると、
    // 縦方向の分解能がp倍になると、波紋の高さもp倍になる
    // ここでピクセルの幅をp倍にしないと、p倍縦長の波紋になってしまう。
    // それでは困るので、ピクセルの幅をp倍する。
    // ここまでで、仮想的にp倍のスケールで計算した事になる。
    // そこで、算出したずれを1/p倍する。
    const double p = g_Conf.depthRes / 512.0;
    
    // 8をデフォルトの半径とすると、
    // 波紋の半径がq倍になると、波紋の高さはそのままなので、
    // q倍横長の波紋になってしまう。
    // そこで、波紋の高さをq倍する。
    // このまま計算すると、q倍のスケールで計算した事になる。
    // よって、算出したずれを1/q倍する。
    const double q = g_Conf.riplRadius / 8.0;
*/    
    const double PIXEL_WIDTH = 1.0;     // ピクセルの幅
    const double REFRACTION_INDEX = 1.33;    // 水の絶対屈折率

    // テーブル用メモリを確保(メモリ使用量≒g_Conf.depthRes^2 * 2byte(16bit))
    // ずれは、水面の高さと、そのピクセルの左右または上下の高低差に影響される
    // Left < Right
    Uint16 *refractionTable = calloc(g_Conf.depthRes * g_Conf.depthRes, sizeof(Uint16));
    
    // 計算開始
    int i = 0, d = g_Conf.depthRes / 10;
    for (int heightDiff = 0; heightDiff < g_Conf.depthRes; heightDiff++)
    {
        // 入射角を求める
        double t1 = atan((heightDiff * 100.0 / (g_Conf.depthRes / 2.0)) / (2.0 * PIXEL_WIDTH));
        
        // 屈折角を求める
        double t2 = asin(sin(t1) / REFRACTION_INDEX);
        
        // 各水面の高さに応じたずれの量を求める
        // ずれは、水面の高さと、そのピクセルの左右または上下の高低差に影響される
        for (int height = 0; height < g_Conf.depthRes; height++)
        {
            int index = g_Conf.depthRes * heightDiff + height;
            double delta = (height * 100.0 / (g_Conf.depthRes / 2.0)) * tan(t1 - t2);
            refractionTable[index] = (Uint16)(delta);
        }
        if (i < heightDiff)
        {
            char cap[32];
            sprintf(cap, "Creating Cache : %#.2f%%...", 100.0 * heightDiff / g_Conf.depthRes);
            SDL_WM_SetCaption(cap, NULL);
            i += d;
        }
    }
    
    // テーブルへのポインタを返す
    return refractionTable;
}


/* 水面計算関数 */
void Calculate()
{
    // 飽和処理用上限/下限配列
    int maxHeight = (g_Conf.depthRes - 1) / 2;
    PosData upperLimit[4] = { maxHeight,  maxHeight,  maxHeight,  maxHeight};
    PosData lowerLimit[4] = {-maxHeight, -maxHeight, -maxHeight, -maxHeight};
    Sint16 v = g_Conf.attRate * 32768;
    Sint16 mulnum[4] = {v, v, v, v};
    
    // 初期位置×に移動
    // 壁壁壁壁壁
    // 壁×水水水
    // 壁水水水水
    // 壁水水水水
    int SurfaceWidth = g_Conf.widthRes;
    int SurfaceHeight = g_Conf.heightRes;
    PosData *nextData = g_pNextData;
    PosData *crntData = g_pCrntData;
    PosData *prevData = g_pPrevData;
    nextData += 1 * (SurfaceWidth + 2) + 1;
    crntData += 1 * (SurfaceWidth + 2) + 1;
    prevData += 1 * (SurfaceWidth + 2) + 1;
    
    // 自由端反射化
    PosData *tmpMain = g_pCrntData;
    PosData *tmpSub = g_pCrntData + (SurfaceWidth + 2);
    for (int x = 1; x < SurfaceWidth + 1; x++)
        tmpMain[x] = tmpSub[x];
    for (int y = 1; y < SurfaceHeight + 1; y++)
    {
        tmpMain += (SurfaceWidth + 2);
        tmpSub += (SurfaceWidth + 2);
        tmpMain[0] = tmpMain[1];
        tmpMain[SurfaceWidth + 1] = tmpMain[SurfaceWidth];
    }
    for (int x = 1; x < SurfaceWidth + 1; x++)
        tmpSub[x] = tmpMain[x];
    
    // シミュレート
    for (int y = 0; y < SurfaceHeight; y++)
    {
        for (int x = 0; x < SurfaceWidth; x += 4)
        {
            asm volatile (
                // ■□■□ここから計算処理□■□■
                // crntDataの上下左右を加算
                "movq -2(%%esi), %%mm0\n"              // crntDataの左を読み込む
                "paddsw 2(%%esi), %%mm0\n"             // crntDataの右を加算
                "paddsw (%%esi, %%ebx, 2), %%mm0\n"    // crntDataの下を加算
                "neg %%ebx\n"                          // オフセットの2の補数をとる
                "paddsw (%%esi, %%ebx, 2), %%mm0\n"    // crntDataの上を加算
                
                // 加算したものを2分の1にする（0方向に丸める版）
                "movq %%mm0, %%mm1\n"      // コピー
                "psrlw $15, %%mm1\n"       // 最上位ビットを抽出
                "paddsw %%mm1, %%mm0\n"    // 最上位ビットを加算
                "psraw $1, %%mm0\n"        // 算術右シフト演算
                
                // oldWaterの中央を減算
                "psubsw (%%eax), %%mm0\n"
                
                // ■□■□ここから飽和処理□■□■
                // 上限越えチェック用配列を読み込む
                "movq (%%ecx), %%mm5\n"
                "movq %%mm0, %%mm6\n"       // 比較で上書きされるのでコピー
                
                // 上限を越えている箇所を探す（マスク作成）
                "pcmpgtw %%mm5, %%mm0\n"    // 上限を越えている所が1、他は0になる
                
                // 飽和させる 
                "pand %%mm0, %%mm5\n"       // 配列を上限越え箇所のみにする
                "pandn %%mm6, %%mm0\n"      // 上限越え箇所を0にする
                "por %%mm5, %%mm0\n"        // 論理和合成
                
                // 下限越えチェック用配列を読み込む
                "movq (%%edx), %%mm5\n"
                "movq %%mm5, %%mm6\n"       // 比較で上書きされるのでコピー
                
                // 下限を越えている箇所を探す（マスク作成）
                "pcmpgtw %%mm0, %%mm6\n"    // 下限を越えている所が1になる
                
                // 飽和させる
                "pand %%mm6, %%mm5\n"       // 配列を下限越え箇所のみにする
                "pandn %%mm0, %%mm6\n"      // 下限越え箇所を0にする
                "por %%mm5, %%mm6\n"        // 論理和合成
                
                // メモリに書き戻す
                "movq %%mm6, (%%edi)\n"
            :
            : "S" (crntData),              // 今の水面
              "D" (nextData),              // 新しい水面
              "a" (prevData),              // 古い水面
              "b" (SurfaceWidth + 2),      // 水面データの幅
              "c" (upperLimit),            // オーバーチェック用配列
              "d" (lowerLimit)             // アンダーチェック用配列
            : "memory");
            // <<< 1 - 符号付き16bit整数に対するn/32768倍アルゴリズム >>>
            // n倍して、上位2バイトと下位2バイトを得る。
            // 1.n=32768を掛ける(1倍の例)
            //                   SHHHHHHH LLLLLLLL
            // 2.上位2バイトを左シフト、下位2バイトを右シフト
            // S0HHHHHH HLLLLLLL L0000000 00000000
            // 3.上位2バイトと下位2バイトをOR合成
            // SHHHHHHH LLLLLLL0 00000000 0000000L
            // 4.完成
            //                   SHHHHHHH LLLLLLLL
            asm volatile (
                // ■□■□ここから波を静める処理□■□■
                // mm0 
                "movq  (%%edi), %%mm0\n"     // 水面の高さ
                "movq  (%%esi), %%mm1\n"     // 掛ける数 n
                "movq   %%mm1,  %%mm2\n"     // 掛ける数をコピー
                "pmulhw %%mm0,  %%mm1\n"     // 掛けて上位2バイトを取得
                "pmullw %%mm0,  %%mm2\n"     // 掛けて下位2バイトを取得
                "psllw  $1,     %%mm1\n"     // 上位2バイトを左へ1シフト
                "psrlw  $15,    %%mm2\n"     // 下位2バイトを右へ15シフト
                "por    %%mm2,  %%mm1\n"     // 上位バイトと下位バイトをOR合成
                "movq   %%mm1, (%%edi)\n"    // メモリに書き戻す
            :
            : "S" (mulnum),
              "D" (nextData)
            : "memory");
            // ポインタを進める
            nextData += 4;
            crntData += 4;
            prevData += 4;
        }
        // ポインタを進める
        nextData += 2;
        crntData += 2;
        prevData += 2;
    }
    
    // MMX終了
    asm ("emms\n");
}


/* RRT形式頂点情報書き出し関数 */
void WriteOutRRT(char *fname)
{
    int pitch = g_Conf.widthRes + 2;
    PosData *waterMain = g_pNextData;
    PosData *waterSub = g_pNextData + pitch;
    waterMain += pitch;
    waterSub += pitch;
    float scale = 50.0 * 100.0 / g_Conf.riplDepth;
    FILE *out = fopen(fname, "wb");
    fprintf(out, "<vertices>\n");
    for (int y = 1; y < g_Conf.heightRes + 1; y++)
    {
        float vy = -100.0 + y * 200.0 / (g_Conf.heightRes - 1);
        for (int x = 1; x < g_Conf.widthRes + 1; x++)
        {
            float vx = -100.0 + x * 200.0 / (g_Conf.widthRes - 1);
            fprintf(out, "    <vertex x=\"%f\" y=\"%f\" z=\"%f\" />\n", vx, vy, waterMain[x] * scale / g_Conf.depthRes);
        }
        waterMain += pitch;
        waterSub += pitch;
    }
    fprintf(out, "</vertices>\n");
    fprintf(out, "<triangles>\n");
    for (int y = 0; y < g_Conf.heightRes; y++)
    {
        for (int x = 0; x < g_Conf.widthRes; x++)
        {
            int n = x + y * g_Conf.widthRes;
            fprintf(out, "    <triangle id0=\"%d\" id1=\"%d\" id2=\"%d\" />\n", n, n + g_Conf.widthRes, n + 1);
            fprintf(out, "    <triangle id0=\"%d\" id1=\"%d\" id2=\"%d\" />\n", n + 1, n + g_Conf.widthRes, n + 1 + g_Conf.widthRes);
        }
        waterMain += pitch;
        waterSub += pitch;
    }
    fprintf(out, "</triangles>\n");
    fclose(out);
}


/* ウィンドウ上の座標から3D空間上の座標を得る関数 */
Vector3 Get3DCoordinate(int x, int y)
{
    GLdouble model[16], proj[16];
    GLint view[4];
    GLfloat z;
    Vector3 pos;

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);

    glReadPixels(x, g_Conf.wndHeight - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
    gluUnProject(x, g_Conf.wndHeight - y, z, model, proj, view, &pos.x, &pos.y, &pos.z);

    return pos;
}


/* 波紋発生関数 */
void RippleOut(int x, int y)
{
    // 波データの左上隅にあたる位置に移動
    int incValue = (g_Conf.widthRes + 2) * ((y + 1) - g_Conf.riplRadius) +
                                           ((x + 1) - g_Conf.riplRadius);
    PosData *nextData = g_pNextData;
    PosData *crntData = g_pCrntData;
    nextData += incValue;
    crntData += incValue;
    PosData *riplData = g_pCrntRipl;
    
    // 波紋を発生させる
    incValue = (g_Conf.widthRes + 2) - (2 * g_Conf.riplRadius + 1);
    for (int iy = -g_Conf.riplRadius; iy <= g_Conf.riplRadius; iy++)
    {
        for (int ix = -g_Conf.riplRadius; ix <= g_Conf.riplRadius; ix++)
        {
            if (*riplData != 0)
            {
                // 異符号なら、普通に足す
                if (((*nextData >= 0) && (*riplData < 0)) || ((*nextData <= 0) && (*riplData > 0)))
                    *nextData += *riplData;
                // 同符号で波紋の高さより低いなら、そこまで引き上げる
                else if (*nextData < *riplData)
                    *nextData = *riplData;
                // 同符号で波紋の高さより大きいか同じであるなら、何もしない
                else
                    ;
                
                // 異符号なら、普通に足す
                if (((*crntData >= 0) && (*riplData < 0)) || ((*crntData <= 0) && (*riplData > 0)))
                    *crntData += *riplData;
                // 同符号で波紋の高さより低いなら、そこまで引き上げる
                else if (*crntData < *riplData)
                    *crntData = *riplData;
                // 同符号で波紋の高さより大きいか同じであるなら、何もしない
                else
                    ;
            }
            
            // ポインタを進める
            riplData++;
            nextData++;
            crntData++;
        }
        // 次に書きこむ行の頭に移動
        nextData += incValue;
        crntData += incValue;
    }
}


/* 水面描画関数 */
void Draw()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set shading model
    glShadeModel(GL_FLAT);
    
    // Set modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 80.0, 160.0,   // Position of the eye point
              0.0, 0.0, 0.0,     // Position of the reference point
              0.0, 1.0, 0.0);    // Up vector

    // Start draw
    int pitch = g_Conf.widthRes + 2;
    PosData *waterMain = g_pNextData;
    PosData *waterSub = g_pNextData + pitch;
    waterMain += pitch;
    waterSub += pitch;
    float scale = 50.0 * 100.0 / g_Conf.riplDepth;
    for (int y = 1; y < g_Conf.heightRes + 1; y++)
    {
        float vyMain = -100.0 + y * 200.0 / (g_Conf.heightRes - 1);
        float vySub = -100.0 + (y + 1) * 200.0 / (g_Conf.heightRes - 1);

        // Draw polygons' surface
        // Set polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_QUAD_STRIP);
        {
            glColor3f(0.0, 0.0, 0.0);
            for (int x = 1; x < g_Conf.widthRes + 1; x++)
            {
                float vx = -100.0 + x * 200.0 / (g_Conf.widthRes - 1);
                glVertex3f(vx, waterMain[x] * scale / g_Conf.depthRes, vyMain);
                glVertex3f(vx, waterSub[x] * scale / g_Conf.depthRes, vySub);
            }
        }
        glEnd();

        // Draw polygons' edge
        // Set polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin(GL_QUAD_STRIP);
        {
            glColor3f(1.0, 1.0, 1.0);
            for (int x = 1; x < g_Conf.widthRes + 1; x++)
            {
                float vx = -100.0 + x * 200.0 / (g_Conf.widthRes - 1);
                glVertex3f(vx, 1.0 + waterMain[x] * scale / g_Conf.depthRes, vyMain);
                glVertex3f(vx, 1.0 + waterSub[x] * scale / g_Conf.depthRes, vySub);
            }
        }
        glEnd();
        waterMain += pitch;
        waterSub += pitch;
    }
    
    // Update screen
    SDL_GL_SwapBuffers();
}


