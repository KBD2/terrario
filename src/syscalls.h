#pragma once

typedef struct{
  char name[8];
  void*address;
} TAddinArrayItem;

typedef struct {
        short p1;
        short estrip_no;
        TAddinArrayItem*addin_array_addr;
        void*estrip_location;
        void*addin_smem_location;
        void*estrip_icon_location;
} TAddinEstripInformation;

// Syscalls
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070
typedef int(*sc_iv)(void);
typedef int (*sc_iii)(int, int);
typedef void(*sc_vv)(void);
typedef void(*sc_viisp)(int, int, TAddinEstripInformation*);
extern const unsigned int sc003B[];
extern const unsigned int sc003C[];
extern const unsigned int sc0236[];
extern const unsigned int sc019F[];
extern const unsigned int sc000E[];
#define RTC_GetTicks (*(sc_iv)sc003B)
#define RTC_Elapsed_ms (*(sc_iii)sc003C)
#define RebootOS (*(sc_vv)sc0236)
#define SMEM_Optimization (*(sc_vv)sc019F)
#define App_GetAddinEstripInfo (*(sc_viisp)sc000E)

extern long SMEM_optimize_codelength;

void JumpOptimizing();
void GetCallbackAddressPtr( int*p0, int*p1 );
extern int SMEM_optimize_frame(unsigned char* w);
short* APP_EnableRestart();