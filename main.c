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
 * New:  F10������Redqueen������ĺ������ȷ��������data.rrt�ե�����˽��Ϥ���褦�ˤ�����
 * New:  ������ɥ���Υޥ�����������κ�ɸ�򡢲��ۿ��̾�κ�ɸ�˥ޥåԥ󥰤���褦�ˤ�����
 * New:  ��ݤǼ�ͳüȿ�ͤ���褦�ˤ�����
 * 
 * >>> Wed, 04 Aug 2004 00:17:51 +0900 <<<
 * New:  ���ܥ������������ɽ������礭�����ɤ��Ʊ���ˤʤ�褦�ˤ�����
 * New:  ɽ����ο��̤��礭����200x200�θ���ˤ�����
 * New:  �դΤ����褹��褦�ˤ�����
 */


/* <<< What I'm Doing >>>
 */


/* <<< To Do >>>
 * FPS�����Ǥʤ����ݥꥴ���/�á�ĺ����/�ä�ɽ���Ǥ���褦�ˤ���
 * ������ɤ߹���
 * �ǡ����񤭽Ф�
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
    char  *pBgImgPath;      // �طʥ��᡼���Υե�����̾
    int    depthRes;        // �����β�����
    int    riplRadius;      // ȯ�������������Ⱦ��
    double riplDepth;       // ȯ������������κ��翼��
    int    widthRes;        // ���ߥ�졼�Ȥ�����̤����β�����
    int    heightRes;       // ���ߥ�졼�Ȥ�����̤ι⤵�β�����
    int    wndWidth;        // ������ɥ�����
    int    wndHeight;       // ������ɥ��ι⤵
    double attRate;         // ����Ψ
    double scale;
    int    csrIPDiv;        // �����������֤�ʬ��ǹԤ���
    bool   isFullScreen;    // �ե륹���꡼���ݤ�
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
const double PI = 3.1415926535;    // �߼�Ψ
// �׻����Թ������κ�����65536��1/4�������ѤǤ��ʤ�
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
ProgConfig   g_Conf;         // �ץ���������
PosData     *g_pNextData;    // ���ο��̥ǡ���
PosData     *g_pCrntData;    // ���ο��̥ǡ���
PosData     *g_pPrevData;    // ���ο��̥ǡ���
PosData     *g_pCrntRipl;    // ���ߤ�����ǡ����ؤΥݥ���
PosData    **g_pRipples;     // ����ǡ���
Uint16      *g_pRfraTbl;     // ���ޤˤ���Ѱ��̥ơ��֥�
SDL_Surface *g_pScreen;      // �����꡼�󥵡��ե���
SDL_Surface *g_pBgImage;     // �طʥ����ե���


/* Main Function */
int main(int argc, char **argv)
{
    // �ץ��������
    InitProc(argc, argv);
    
    // FPS��¬���ѿ�
    Uint32 startTick = SDL_GetTicks();
    Uint32 endTick = 0;
    Uint32 frameCount = 0;
    
    // ̵�¥롼��
    while(!EventProc())
    {
        // ���̤򥹥��꡼�󥵡��ե��������褹��
        Draw();

        // ���̥ǡ����������ؤ���
        PosData *tmp = g_pPrevData;
        g_pPrevData = g_pCrntData;
        g_pCrntData = g_pNextData;
        g_pNextData = tmp;
        
        // ���̤�׻�
        Calculate();
        
        // FPS��¬
        endTick = SDL_GetTicks();
        frameCount++;
        if (endTick - startTick > 1000)
        {
            // ������ɥ�����ץ�����ɽ�����뤿���ʸ��������
            char cap[16];
            sprintf(cap, "FPS : %#.2f", frameCount * 1000.0 / (endTick - startTick));
            // ʸ����򥦥���ɥ�����ץ���������
            SDL_WM_SetCaption(cap, NULL);
            // �������Ȼ���ե졼����Υꥻ�å�
            startTick = SDL_GetTicks();
            frameCount = 0;
        }
    }
    
    // �ץ���ཪλ����
    ExitProc();
    
    return 0;
}


/* �ץ���������ؿ� */
void InitProc(int argc, char **argv)
{
    // �ץ���४�ץ����Υǥե�����ͤ�����
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
    
    // ��������
    if (ParseArgument(argc, argv))
    {
        // ��Ĺ�⡼�ɻ� : �����ͤ�ɸ����Ϥ˽��Ϥ���
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
    
    // SDL��������ƥ����꡼�󥵡��ե��������
    InitSDL();
    
    // �طʲ����ɤ߹���
    SDL_Surface *tmpBg = SDL_LoadBMP(g_Conf.pBgImgPath);
    if (tmpBg == NULL)
    {
        // ���顼ɽ���򤷤ƽ�λ
        fprintf(stderr, MSG_ERROR_LOAD_IMAGE, g_Conf.pBgImgPath);
        exit(EXIT_FAILURE);
    }
    
    // �طʥ����ե����򥹥��꡼�󥵡��ե�����Ʊ���������Ѵ�
    tmpBg = SDL_ConvertSurface(tmpBg, g_pScreen->format, SDL_SWSURFACE);
    
    // �ɤ߹�����طʤ���̤��礭���˹�碌��
    g_pBgImage = SDL_CreateRGBSurface(SDL_SWSURFACE,
            g_Conf.widthRes, g_Conf.heightRes, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    ScaleCopySurface(SM_BI_LINEAR, tmpBg, g_pBgImage);
    
    // ���̥ǡ����������
    int arraySize = (1 + g_Conf.widthRes + 1) * (1 + g_Conf.heightRes + 1);
    g_pNextData = calloc(arraySize, sizeof(PosData));
    g_pCrntData = calloc(arraySize, sizeof(PosData));
    g_pPrevData = calloc(arraySize, sizeof(PosData));
    
    // ����ǡ�������
    g_pCrntRipl = CreateRippleData();
    
    // ���ޥơ��֥�����
//    g_pRfraTbl = CreateRefractionTable();
    
    // ���ޥơ��֥륢����������ɬ�פʥ��ե��åȤ�û����Ƥ���
//    g_pRfraTbl += (g_Conf.depthRes * (MaxWaterHeight - MinWaterHeight + 1)) - MinWaterHeight;
}


/* ���٥�Ƚ����ؿ� */
bool EventProc()
{
    int topLimit = g_Conf.riplRadius;
    int bottomLimit = g_Conf.heightRes - g_Conf.riplRadius;
    int leftLimit = g_Conf.riplRadius;
    int rightLimit = g_Conf.widthRes - g_Conf.riplRadius;
    bool exitFlag = false;    // ��λ�׵᤬ͭ�뤫�ݤ�
    static int preCsrX;    // ���Υ��٥��ȯ�����Υޥ������������X��ɸ
    static int preCsrY;    // ���Υ��٥��ȯ�����Υޥ������������Y��ɸ
    SDL_Event event;          // ���٥�Ȥ˴ؤ���������빽¤��(������?)
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            // ���������󥤥٥��
            switch (event.key.keysym.sym)
            {
            case SDLK_F10:
                // �ǡ������ϥ���
                WriteOutRRT("data.rrt");
                break;
            case SDLK_ESCAPE:
            case SDLK_q:
                // ��λ����(ESC, Q)
                exitFlag = true;
                break;
            default:
                // ����¾�Υ���(�ϥ�ɥ뤵��ʤ�����)
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            // �ޥ����ܥ�������󥤥٥��
            // �ޥ����Υܥ��󤬲�����Ƥ����
            if (event.button.state == SDL_PRESSED)
            {
                // 3D���־�κ�ɸ������
                Vector3 pos = Get3DCoordinate(event.button.x, event.button.y);
                // ������ɥ����������̾�ΰ��֤��Ѵ�
                int vx = (pos.x + 100.0) * (g_Conf.widthRes - 1) / 200.0;
                int vy = (pos.z + 100.0) * (g_Conf.heightRes - 1) / 200.0;
                // ������ɥ����������̾�ΰ��֤��Ѵ�
//                vx = (float)g_Conf.widthRes * event.button.x / g_Conf.wndWidth;
//                vy = (float)g_Conf.heightRes * event.button.y / g_Conf.wndHeight;
                // ����ȯ����ǽ�ΰ�ˤ��뤫�ɤ���������å�
                if ((leftLimit < vx) && (vx < rightLimit) &&
                    (topLimit < vy) && (vy < bottomLimit))
                {
                    // �����ȯ��
                    RippleOut(vx, vy);
                    preCsrX = vx;
                    preCsrY = vy;
                }
            }
            break;
        case SDL_MOUSEMOTION:
            // �ޥ������������ư���٥��
            // �ޥ����Υܥ��󤬲�����Ƥ����
            if (event.motion.state == SDL_PRESSED)
            {
                // 3D���־�κ�ɸ������
                Vector3 pos = Get3DCoordinate(event.motion.x, event.motion.y);
                // ������ɥ����������̾�ΰ��֤��Ѵ�
                int vx = (pos.x + 100.0) * (g_Conf.widthRes - 1) / 200.0;
                int vy = (pos.z + 100.0) * (g_Conf.heightRes - 1) / 200.0;
                // ������ɥ����������̾�ΰ��֤��Ѵ�
//                vx = (float)g_Conf.widthRes * event.motion.x / g_Conf.wndWidth;
//                vy = (float)g_Conf.heightRes * event.motion.y / g_Conf.wndHeight;
                // ����ȯ����ǽ�ΰ�ˤ��뤫�ɤ���������å�
                if ((leftLimit < vx) && (vx < rightLimit) &&
                    (topLimit < vy) && (vy < bottomLimit))
                {    // �����ȯ��
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
            // ��λ���٥��
            exitFlag = true;
            break;
        }
    }
    return exitFlag;
}


/* �ץ���ཪλ���θ���� */
void ExitProc()
{
    // ���ޥơ��֥륢����������ɬ�פʥ��ե��åȤ򸺻����Ƥ���
//    g_pRfraTbl -= (-(MinWaterHeight - MaxWaterHeight) * (MaxWaterHeight - MinWaterHeight + 1)) - MinWaterHeight;
    
    // ����β���
    SDL_FreeSurface(g_pBgImage);
    free(g_pNextData);
    free(g_pCrntData);
    free(g_pPrevData);
    free(g_pCrntRipl);
    free(g_pRfraTbl);
    
    // SDL��λ
    SDL_Quit();
}


/* ��ư���������ϴؿ� */
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
                // �Ȥ�����ɽ�����ƽ�λ
                printf(MSG_HELP); exit(EXIT_SUCCESS);
                break;
            case 'v':
                // ��Ĺ��ɽ����Ԥ�
                isVerboseMode = true;
                break;
            case 'f':
                // �ե륹���꡼��⡼�ɤ�����
                g_Conf.isFullScreen = true;
                break;
            case 'a':
                // ����θ���Ψ�����
                if (argv[i][2] == '\0') g_Conf.attRate = atof(&argv[++i][0]);
                else                    g_Conf.attRate = atof(&argv[i][2]);
                break;
            case 'b':
                // ���ܤ�����κ���ο���
                if (argv[i][2] == '\0') g_Conf.riplDepth = atof(&argv[++i][0]);
                else                    g_Conf.riplDepth = atof(&argv[i][2]);
                break;
            case 'r':
                // ������礭�������
                if (argv[i][2] == '\0') g_Conf.riplRadius = atoi(&argv[++i][0]);
                else                    g_Conf.riplRadius = atoi(&argv[i][2]);
                break;
            case 'd':
                // ��ֻ���ʬ��������
                if (argv[i][2] == '\0') g_Conf.csrIPDiv = atoi(&argv[++i][0]);
                else                    g_Conf.csrIPDiv = atoi(&argv[i][2]);
                break;
            case 'i':
                // �طʤΥե�����̾�����
                if (argv[i][2] == '\0') g_Conf.pBgImgPath = &argv[++i][0];
                else                    g_Conf.pBgImgPath = &argv[i][2];
                break;
            case 's':
                // �׻���ο��̤Υ����������
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
                // ������Ѳ�ǽ��(KBñ��)�����
                // ���̤ι⤵������(������)�˱ƶ�����
                int mem;
                if (argv[i][2] == '\0') mem = atoi(&argv[++i][0]);
                else                    mem = atoi(&argv[i][2]);
                
                // ���̤ι⤵�˴�����������
                // �Ȥη׻����ˡ��岼������­������
                // PosData(Sint16)�κ�����32767/4=8191���
                // �ƥԥ�����ι⤵�κ����ͤ�8191���Ǿ��ͤ�-8192
                // �����٤κ����ͤ�8191 - (-8192) + 1=16384
                // ���㺹�κǾ��ͤ�0�������ͤ�8191-(-8192)=16383
                // �Ĥޤꡢ����ϲ�����-1����������
                // ���ޥơ��֥��ѥ���γ����̤�
                // ������=g_Conf.depthRes^2 * 2byte(16bit))
                g_Conf.depthRes = (int)(sqrt(mem * 1024 / 2.0));

                // ���ꤵ�줿�����٤������ͤ�ۤ��Ƥ����齤�����롣
                if (g_Conf.depthRes > MAX_RESOLUTION)
                    g_Conf.depthRes = MAX_RESOLUTION;
                
                //
//                    printf("%d\n", g_Conf.depthRes);
                
                // ����ι⤵��Ʒ׻�
                //g_Conf.riplDepth = (g_Conf.depthRes - 1) / 2;
                
                break;
            }
            case 'p':
                // ���̤ι⤵������(������)�����
                // ��������̤˱ƶ�����
                if (argv[i][2] == '\0') g_Conf.depthRes = atoi(&argv[++i][0]);
                else                    g_Conf.depthRes = atoi(&argv[i][2]);
                
                // ���ꤵ�줿�����٤������ͤ�ۤ��Ƥ����齤�����롣
                if (g_Conf.depthRes > MAX_RESOLUTION)
                    g_Conf.depthRes = MAX_RESOLUTION;
                
                // ����ι⤵��Ʒ׻�
                //g_Conf.riplDepth = (g_Conf.depthRes - 1) / 2;
                
                break;
            default:
                // �����ʰ��������ꤵ�줿��硢���顼ɽ���򤷤ƽ�λ
                fprintf(stderr, MSG_ERROR_OPTION, argv[i]);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // �����ʰ��������ꤵ�줿��硢���顼ɽ���򤷤ƽ�λ
            fprintf(stderr, MSG_ERROR_OPTION, argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    
    // ��Ĺ��ɽ����Ԥ����ݤ����֤�
    return isVerboseMode;
}


/* SDL������ؿ� */
void InitSDL()
{
    // SDL������
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        // ���顼ɽ���򤷤ƽ�λ
        fprintf(stderr, MSG_ERROR_INIT_SDL, SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    // �ǥ����ץ쥤�ǥХ����ν�����ե饰������
    Uint32 screenFlags = SDL_SWSURFACE | SDL_OPENGL;
    if (g_Conf.isFullScreen) screenFlags |= SDL_FULLSCREEN;
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // �ǥ����ץ쥤�ǥХ���������
    g_pScreen = SDL_SetVideoMode(g_Conf.wndWidth, g_Conf.wndHeight, 32, screenFlags);
    
    // ������˼��Ԥ����顢���顼��å�������ɽ�����ƽ�λ
    if (g_pScreen == NULL)
    {
        // ���顼ɽ���򤷤ƽ�λ
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


/* ����ǡ��������ؿ� */
PosData *CreateRippleData()
{
    int radius = g_Conf.riplRadius;
    // ����ǡ�������
    PosData *riplData = calloc((2 * radius + 1) * (2 * radius + 1), sizeof(PosData));
    PosData *pData = riplData;
    
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            // x = 0, y = 0���濴�Ȥ���
            // �濴����ε�Υ���������
            int r2 = (x * x) + (y * y);
            
            if (r2 < radius * radius)
            {   // ����
                double t = PI * (sqrt(r2) / radius);
                *pData = (PosData)((cos(t) + 1) * (g_Conf.depthRes / 2) * (g_Conf.riplDepth / 100.0) * (g_Conf.riplRadius / 8.0) / 2);
            }
            else
            {   // �߳�
                *pData = 0;
            }
            pData++;
        }
    }
    return riplData;
}


/* ���ޥơ��֥�����ؿ� */
Uint16 *CreateRefractionTable()
{
/*
    // 512��ǥե���Ȥ�ʬ��ǽ�Ȥ���ȡ�
    // ��������ʬ��ǽ��p�ܤˤʤ�ȡ�����ι⤵��p�ܤˤʤ�
    // �����ǥԥ����������p�ܤˤ��ʤ��ȡ�p�ܽ�Ĺ������ˤʤäƤ��ޤ���
    // ����ǤϺ���Τǡ��ԥ����������p�ܤ��롣
    // �����ޤǤǡ�����Ū��p�ܤΥ�������Ƿ׻��������ˤʤ롣
    // �����ǡ����Ф��������1/p�ܤ��롣
    const double p = g_Conf.depthRes / 512.0;
    
    // 8��ǥե���Ȥ�Ⱦ�¤Ȥ���ȡ�
    // �����Ⱦ�¤�q�ܤˤʤ�ȡ�����ι⤵�Ϥ��ΤޤޤʤΤǡ�
    // q�ܲ�Ĺ������ˤʤäƤ��ޤ���
    // �����ǡ�����ι⤵��q�ܤ��롣
    // ���Τޤ޷׻�����ȡ�q�ܤΥ�������Ƿ׻��������ˤʤ롣
    // ��äơ����Ф��������1/q�ܤ��롣
    const double q = g_Conf.riplRadius / 8.0;
*/    
    const double PIXEL_WIDTH = 1.0;     // �ԥ��������
    const double REFRACTION_INDEX = 1.33;    // ������ж���Ψ

    // �ơ��֥��ѥ�������(��������̢�g_Conf.depthRes^2 * 2byte(16bit))
    // ����ϡ����̤ι⤵�ȡ����Υԥ�����κ����ޤ��Ͼ岼�ι��㺹�˱ƶ������
    // Left < Right
    Uint16 *refractionTable = calloc(g_Conf.depthRes * g_Conf.depthRes, sizeof(Uint16));
    
    // �׻�����
    int i = 0, d = g_Conf.depthRes / 10;
    for (int heightDiff = 0; heightDiff < g_Conf.depthRes; heightDiff++)
    {
        // ���ͳѤ����
        double t1 = atan((heightDiff * 100.0 / (g_Conf.depthRes / 2.0)) / (2.0 * PIXEL_WIDTH));
        
        // ���޳Ѥ����
        double t2 = asin(sin(t1) / REFRACTION_INDEX);
        
        // �ƿ��̤ι⤵�˱�����������̤����
        // ����ϡ����̤ι⤵�ȡ����Υԥ�����κ����ޤ��Ͼ岼�ι��㺹�˱ƶ������
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
    
    // �ơ��֥�ؤΥݥ��󥿤��֤�
    return refractionTable;
}


/* ���̷׻��ؿ� */
void Calculate()
{
    // ˰�½����Ѿ��/��������
    int maxHeight = (g_Conf.depthRes - 1) / 2;
    PosData upperLimit[4] = { maxHeight,  maxHeight,  maxHeight,  maxHeight};
    PosData lowerLimit[4] = {-maxHeight, -maxHeight, -maxHeight, -maxHeight};
    Sint16 v = g_Conf.attRate * 32768;
    Sint16 mulnum[4] = {v, v, v, v};
    
    // ������֡ߤ˰�ư
    // ����������
    // �ɡ߿���
    // �ɿ����
    // �ɿ����
    int SurfaceWidth = g_Conf.widthRes;
    int SurfaceHeight = g_Conf.heightRes;
    PosData *nextData = g_pNextData;
    PosData *crntData = g_pCrntData;
    PosData *prevData = g_pPrevData;
    nextData += 1 * (SurfaceWidth + 2) + 1;
    crntData += 1 * (SurfaceWidth + 2) + 1;
    prevData += 1 * (SurfaceWidth + 2) + 1;
    
    // ��ͳüȿ�Ͳ�
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
    
    // ���ߥ�졼��
    for (int y = 0; y < SurfaceHeight; y++)
    {
        for (int x = 0; x < SurfaceWidth; x += 4)
        {
            asm volatile (
                // ����������������׻�������������
                // crntData�ξ岼������û�
                "movq -2(%%esi), %%mm0\n"              // crntData�κ����ɤ߹���
                "paddsw 2(%%esi), %%mm0\n"             // crntData�α���û�
                "paddsw (%%esi, %%ebx, 2), %%mm0\n"    // crntData�β���û�
                "neg %%ebx\n"                          // ���ե��åȤ�2�������Ȥ�
                "paddsw (%%esi, %%ebx, 2), %%mm0\n"    // crntData�ξ��û�
                
                // �û�������Τ�2ʬ��1�ˤ����0�����˴ݤ���ǡ�
                "movq %%mm0, %%mm1\n"      // ���ԡ�
                "psrlw $15, %%mm1\n"       // �Ǿ�̥ӥåȤ����
                "paddsw %%mm1, %%mm0\n"    // �Ǿ�̥ӥåȤ�û�
                "psraw $1, %%mm0\n"        // ���ѱ����եȱ黻
                
                // oldWater������򸺻�
                "psubsw (%%eax), %%mm0\n"
                
                // ����������������˰�½�����������
                // ��±ۤ������å���������ɤ߹���
                "movq (%%ecx), %%mm5\n"
                "movq %%mm0, %%mm6\n"       // ��ӤǾ�񤭤����Τǥ��ԡ�
                
                // ��¤�ۤ��Ƥ���ս��õ���ʥޥ���������
                "pcmpgtw %%mm5, %%mm0\n"    // ��¤�ۤ��Ƥ���꤬1��¾��0�ˤʤ�
                
                // ˰�¤����� 
                "pand %%mm0, %%mm5\n"       // ������±ۤ��ս�Τߤˤ���
                "pandn %%mm6, %%mm0\n"      // ��±ۤ��ս��0�ˤ���
                "por %%mm5, %%mm0\n"        // �����¹���
                
                // ���±ۤ������å���������ɤ߹���
                "movq (%%edx), %%mm5\n"
                "movq %%mm5, %%mm6\n"       // ��ӤǾ�񤭤����Τǥ��ԡ�
                
                // ���¤�ۤ��Ƥ���ս��õ���ʥޥ���������
                "pcmpgtw %%mm0, %%mm6\n"    // ���¤�ۤ��Ƥ���꤬1�ˤʤ�
                
                // ˰�¤�����
                "pand %%mm6, %%mm5\n"       // ����򲼸±ۤ��ս�Τߤˤ���
                "pandn %%mm0, %%mm6\n"      // ���±ۤ��ս��0�ˤ���
                "por %%mm5, %%mm6\n"        // �����¹���
                
                // ����˽��᤹
                "movq %%mm6, (%%edi)\n"
            :
            : "S" (crntData),              // ���ο���
              "D" (nextData),              // ����������
              "a" (prevData),              // �Ť�����
              "b" (SurfaceWidth + 2),      // ���̥ǡ�������
              "c" (upperLimit),            // �����С������å�������
              "d" (lowerLimit)             // ������������å�������
            : "memory");
            // <<< 1 - ����դ�16bit�������Ф���n/32768�ܥ��르�ꥺ�� >>>
            // n�ܤ��ơ����2�Х��ȤȲ���2�Х��Ȥ����롣
            // 1.n=32768��ݤ���(1�ܤ���)
            //                   SHHHHHHH LLLLLLLL
            // 2.���2�Х��Ȥ򺸥��եȡ�����2�Х��Ȥ򱦥��ե�
            // S0HHHHHH HLLLLLLL L0000000 00000000
            // 3.���2�Х��ȤȲ���2�Х��Ȥ�OR����
            // SHHHHHHH LLLLLLL0 00000000 0000000L
            // 4.����
            //                   SHHHHHHH LLLLLLLL
            asm volatile (
                // �����������������Ȥ��Ť�������������
                // mm0 
                "movq  (%%edi), %%mm0\n"     // ���̤ι⤵
                "movq  (%%esi), %%mm1\n"     // �ݤ���� n
                "movq   %%mm1,  %%mm2\n"     // �ݤ�����򥳥ԡ�
                "pmulhw %%mm0,  %%mm1\n"     // �ݤ��ƾ��2�Х��Ȥ����
                "pmullw %%mm0,  %%mm2\n"     // �ݤ��Ʋ���2�Х��Ȥ����
                "psllw  $1,     %%mm1\n"     // ���2�Х��Ȥ򺸤�1���ե�
                "psrlw  $15,    %%mm2\n"     // ����2�Х��Ȥ򱦤�15���ե�
                "por    %%mm2,  %%mm1\n"     // ��̥Х��ȤȲ��̥Х��Ȥ�OR����
                "movq   %%mm1, (%%edi)\n"    // ����˽��᤹
            :
            : "S" (mulnum),
              "D" (nextData)
            : "memory");
            // �ݥ��󥿤�ʤ��
            nextData += 4;
            crntData += 4;
            prevData += 4;
        }
        // �ݥ��󥿤�ʤ��
        nextData += 2;
        crntData += 2;
        prevData += 2;
    }
    
    // MMX��λ
    asm ("emms\n");
}


/* RRT����ĺ������񤭽Ф��ؿ� */
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


/* ������ɥ���κ�ɸ����3D���־�κ�ɸ������ؿ� */
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


/* ����ȯ���ؿ� */
void RippleOut(int x, int y)
{
    // �ȥǡ����κ�����ˤ�������֤˰�ư
    int incValue = (g_Conf.widthRes + 2) * ((y + 1) - g_Conf.riplRadius) +
                                           ((x + 1) - g_Conf.riplRadius);
    PosData *nextData = g_pNextData;
    PosData *crntData = g_pCrntData;
    nextData += incValue;
    crntData += incValue;
    PosData *riplData = g_pCrntRipl;
    
    // �����ȯ��������
    incValue = (g_Conf.widthRes + 2) - (2 * g_Conf.riplRadius + 1);
    for (int iy = -g_Conf.riplRadius; iy <= g_Conf.riplRadius; iy++)
    {
        for (int ix = -g_Conf.riplRadius; ix <= g_Conf.riplRadius; ix++)
        {
            if (*riplData != 0)
            {
                // �����ʤ顢���̤�­��
                if (((*nextData >= 0) && (*riplData < 0)) || ((*nextData <= 0) && (*riplData > 0)))
                    *nextData += *riplData;
                // Ʊ��������ι⤵����㤤�ʤ顢�����ޤǰ����夲��
                else if (*nextData < *riplData)
                    *nextData = *riplData;
                // Ʊ��������ι⤵����礭����Ʊ���Ǥ���ʤ顢���⤷�ʤ�
                else
                    ;
                
                // �����ʤ顢���̤�­��
                if (((*crntData >= 0) && (*riplData < 0)) || ((*crntData <= 0) && (*riplData > 0)))
                    *crntData += *riplData;
                // Ʊ��������ι⤵����㤤�ʤ顢�����ޤǰ����夲��
                else if (*crntData < *riplData)
                    *crntData = *riplData;
                // Ʊ��������ι⤵����礭����Ʊ���Ǥ���ʤ顢���⤷�ʤ�
                else
                    ;
            }
            
            // �ݥ��󥿤�ʤ��
            riplData++;
            nextData++;
            crntData++;
        }
        // ���˽񤭤���Ԥ�Ƭ�˰�ư
        nextData += incValue;
        crntData += incValue;
    }
}


/* ��������ؿ� */
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


