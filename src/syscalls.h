#pragma once

// Syscalls
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070
typedef int(*sc_iv)(void);
typedef int (*sc_iii)(int, int);
typedef void(*sc_vv)(void);
extern const unsigned int sc003B[];
extern const unsigned int sc003C[];
extern const unsigned int sc0236[];
extern const unsigned int sc019F[];
#define RTC_GetTicks (*(sc_iv)sc003B)
#define RTC_Elapsed_ms (*(sc_iii)sc003C)
#define RebootOS (*(sc_vv)sc0236)
#define SMEM_Optimization (*(sc_vv)sc019F)
