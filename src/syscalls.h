#pragma once

// Syscalls
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070
typedef void(*sc_vv)(void);
typedef int(*sc_iv)(void);
extern const unsigned int sc003B[];
extern const unsigned int sc019F[];
extern const unsigned int sc0236[];
#define RTC_GetTicks (*(sc_iv)sc003B)
#define SMEM_Optimization (*(sc_vv)sc019F)
#define RebootOS (*(sc_vv)sc0236)