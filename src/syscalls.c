#include <gint/std/string.h>

#include "syscalls.h"

#include <gint/display.h>

#define CODESIZE 0x100

void JumpOptimizing()
{
	int p0, p1;
	unsigned char w[] = "Terrario";

	//APP_EnableRestart();
    GetCallbackAddressPtr( &p0, &p1 );
    *(int*)p1 = 0;
	p0 = SMEM_optimize_frame(w);

	dclear(C_WHITE);dprint(0,0,C_BLACK,"%d",p0);dupdate();while(1){}
}

void GetCallbackAddressPtr( int*p0, int*p1 )
{
	TAddinEstripInformation aei;

    App_GetAddinEstripInfo( 100, 0, &aei );
    *p0 = (int)aei.addin_array_addr;
    *p1 = *p0 + 4;
}

short* APP_EnableRestart(){
    short* pEnableRestartFlag;
    pEnableRestartFlag =  (short*)0x8800773C;
    if ( pEnableRestartFlag ) *pEnableRestartFlag = 1;
    return pEnableRestartFlag;
}