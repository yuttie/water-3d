
/*
 **************************************************************************
 *
 *  imgscale.c - Water
 *
 *
 *  Copyright  2004  Yuta Taniguchi
 **************************************************************************
*/


/* インクルードファイル */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "SDL/SDL.h"
#include "imgscale.h"


/* SDLサーフェス用イメージ拡大縮小関数 */
void ScaleCopySurface(enum ScalingMethod method, SDL_Surface *srcSurface, SDL_Surface *desSurface)
{
    // 各サーフェスの幅と高さ
    Uint32 desWidth = desSurface->w;
    Uint32 desHeight = desSurface->h;
    Uint32 srcWidth = srcSurface->w;
    Uint32 srcHeight = srcSurface->h;
    
    // スクリーンサーフェスをロック
    SDL_LockSurface(srcSurface);
    SDL_LockSurface(desSurface);
    
    // 最初の行へのポインタで初期化
    Uint32 *srcLine = (Uint32 *)srcSurface->pixels;
    Uint32 *desLine = (Uint32 *)desSurface->pixels;
    
    // ピッチを取得
    int srcPitch = srcSurface->pitch / sizeof(Uint32);
    int desPitch = desSurface->pitch / sizeof(Uint32);
    
    // ピクセルを指すポインタ
    Uint32 *desPixel;
    Uint32 *srcPixel;
    
    // アルゴリズムの種類で分岐
    switch (method)
    {
        case SM_SIMPLE:
        {   // シンプルな方法 with Bresenham(座標を変換して小数点以下を切捨て)
            // 0.12 sec per call
            int ex = 0, ey = 0;
            for (Uint32 y = 0; y < desHeight; y++)
            {
                srcPixel = srcLine;
                desPixel = desLine;
                for (Uint32 x = 0; x < desWidth; x++)
                {
                    *desPixel = *srcPixel;
                    ex += srcWidth;
                    while (ex > desWidth)
                    {
                        srcPixel++;
                        ex -= desWidth;
                    }
                    desPixel++;
                }
                ey += srcHeight;
                while (ey > desHeight)
                {
                    srcLine += srcPitch;
                    ey -= desHeight;
                }
                desLine += desPitch;
            }
#if 0
            for (Uint32 y = 0; y < desHeight; y++)
            {
                desPixel = desLine;
                for (Uint32 x = 0; x < desWidth; x++)
                {
                    *desPixel = srcLine[srcPitch * (y * srcHeight / desHeight) + x * srcWidth / desWidth];
                    desPixel++;
                }
                desLine += desPitch;
            }
#endif
            break;
        }
        case SM_BI_LINEAR:
        {   // 双線形補間法
            /* ソースサーフェス上の対応する点(vx,vy)を囲む最も小さい正方形の4頂点
             * pix00(左上), pix10(右上), pix01(左下), pix11(右下)
             * 上の辺と下の点を u:(1-u) に内分し、次に、その2つの内分点P,Qを結んだ線分PQ上で v:(1-v) に内分する
             * ちなみに、srcLineは不変で、ソースサーフェスの最初の点を指している
            */
            // 2.3 sec per call
            double xScale = (double)srcWidth / desWidth;
            double yScale = (double)srcHeight / desHeight;
            double vy = 0;    // ソースサーフェス上の対応するY座標
            double vx = 0;    // ソースサーフェス上の対応するX座標
            for (int y = 0; y < desHeight; y++)
            {
                desPixel = desLine;
                vy += yScale;
                vx = 0;
                int y00 = (int)vy;
                double py = vy - y00;
                srcPixel = srcLine + srcPitch * y00;
                for (int x = 0; x < desWidth; x++)
                {
                    // 6.41
                    int x00 = (int)vx;
                    double px = vx - x00;
                    Uint32 pix00 = srcPixel[x00];
                    Uint32 pix10 = srcPixel[x00 + 1];
                    Uint32 pix01 = srcPixel[srcPitch + x00];
                    Uint32 pix11 = srcPixel[srcPitch + x00 + 1];
                    unsigned char r = (1 - py) * ((1 - px) * ((pix00 >> 16) & 0xFF) + px * ((pix10 >> 16) & 0xFF)) +
                                            py * ((1 - px) * ((pix01 >> 16) & 0xFF) + px * ((pix11 >> 16) & 0xFF));
                    unsigned char g = (1 - py) * ((1 - px) * ((pix00 >>  8) & 0xFF) + px * ((pix10 >>  8) & 0xFF)) +
                                            py * ((1 - px) * ((pix01 >>  8) & 0xFF) + px * ((pix11 >>  8) & 0xFF));
                    unsigned char b = (1 - py) * ((1 - px) * ((pix00 >>  0) & 0xFF) + px * ((pix10 >>  0) & 0xFF)) +
                                            py * ((1 - px) * ((pix01 >>  0) & 0xFF) + px * ((pix11 >>  0) & 0xFF));
                    *desPixel = (r << 16) | (g << 8) | (b << 0);
                    desPixel++;
                    vx += xScale;
                }
                desLine += desPitch;
            }
            break;
        }
    }
    
    // サーフェスをアンロック
    SDL_UnlockSurface(srcSurface);
    SDL_UnlockSurface(desSurface);
}


