/*
 *******************************************************************************
 *
 *  cpuidutil.h - *
 *
 *  Copyright  2004  Yuta Taniguchi
 *******************************************************************************
 */


#ifndef __CPUIDUTIL_H
#define __CPUIDUTIL_H

/* Function Prototype Declaration */
bool CheckCPUID();
unsigned int GetEDX();
bool CheckMMX();
bool CheckSSE();
bool CheckSSE2();

/* CPUID命令の有無確認関数 */
bool CheckCPUID()
{
    bool b;
    asm volatile (
        "pushfl\n"
        "pushfl\n"
        "popl %%eax\n"
        "movl %%eax, %%ecx\n"
        "xorl $0x200000, %%eax\n"
        "pushl %%eax\n"
        "popfl\n"
        "pushfl\n"
        "popl %%eax\n"
        "popfl\n"
        "cmpl %%ecx, %%eax\n"
        "jz false\n"
    "true:\n"
        "movb $0x1, %0\n"
        "jmp exit\n"
    "false:\n"
        "movb $0x0, %0\n"
    "exit:\n"
        : "=r" (b)
    );
    return b;
}

/* CPUID命令で得られるEDXレジスタを返す関数 */
unsigned int GetEDX()
{
    unsigned int edx = 0;
    asm volatile (
        "movl $0x1, %%eax\n"
        "cpuid\n"
        "movl %%edx, %0\n"
        : "=r" (edx)
    );
    return edx;
}

/* MMX命令の有無確認関数 */
bool CheckMMX()
{
    bool b = 0;
    asm volatile (
        "movl $0x1, %%eax\n"
        "cpuid\n"
        "bt $23, %%edx\n"
        "setc %0\n"
        : "=r" (b)
    );
    return b;
}

/* SSE命令の有無確認関数 */
bool CheckSSE()
{
    bool b;
    asm volatile (
        "movl $0x1, %%eax\n"
        "cpuid\n"
        "bt $25, %%edx\n"
        "setc %0\n"
        : "=r" (b)
    );
    return b;
}

/* SSE2命令の有無確認関数 */
bool CheckSSE2()
{
    bool b;
    asm volatile (
        "movl $0x1, %%eax\n"
        "cpuid\n"
        "bt $26, %%edx\n"
        "setc %0\n"
        : "=r" (b)
    );
    return b;
}


#endif


