
/*
 **************************************************************************
 *
 *  imgscale.c - Water
 *
 *
 *  Copyright  2004  Yuta Taniguchi
 **************************************************************************
*/


/* ���󥯥롼�ɥե����� */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "SDL/SDL.h"
#include "imgscale.h"


/* SDL�����ե����ѥ��᡼������̾��ؿ� */
void ScaleCopySurface(enum ScalingMethod method, SDL_Surface *srcSurface, SDL_Surface *desSurface)
{
    // �ƥ����ե��������ȹ⤵
    Uint32 desWidth = desSurface->w;
    Uint32 desHeight = desSurface->h;
    Uint32 srcWidth = srcSurface->w;
    Uint32 srcHeight = srcSurface->h;
    
    // �����꡼�󥵡��ե������å�
    SDL_LockSurface(srcSurface);
    SDL_LockSurface(desSurface);
    
    // �ǽ�ιԤؤΥݥ��󥿤ǽ����
    Uint32 *srcLine = (Uint32 *)srcSurface->pixels;
    Uint32 *desLine = (Uint32 *)desSurface->pixels;
    
    // �ԥå������
    int srcPitch = srcSurface->pitch / sizeof(Uint32);
    int desPitch = desSurface->pitch / sizeof(Uint32);
    
    // �ԥ������ؤ��ݥ���
    Uint32 *desPixel;
    Uint32 *srcPixel;
    
    // ���르�ꥺ��μ����ʬ��
    switch (method)
    {
        case SM_SIMPLE:
        {   // ����ץ����ˡ with Bresenham(��ɸ���Ѵ����ƾ������ʲ����ڼΤ�)
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
        {   // ���������ˡ
            /* �����������ե�������б�������(vx,vy)��Ϥ�Ǥ⾮������������4ĺ��
             * pix00(����), pix10(����), pix01(����), pix11(����)
             * ����դȲ������� u:(1-u) ����ʬ�������ˡ�����2�Ĥ���ʬ��P,Q�������ʬPQ��� v:(1-v) ����ʬ����
             * ���ʤߤˡ�srcLine�����Ѥǡ������������ե����κǽ������ؤ��Ƥ���
            */
            // 2.3 sec per call
            double xScale = (double)srcWidth / desWidth;
            double yScale = (double)srcHeight / desHeight;
            double vy = 0;    // �����������ե�������б�����Y��ɸ
            double vx = 0;    // �����������ե�������б�����X��ɸ
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
    
    // �����ե����򥢥��å�
    SDL_UnlockSurface(srcSurface);
    SDL_UnlockSurface(desSurface);
}


