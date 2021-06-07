#include <string.h>

#include <gint/defs/attributes.h>

#include "optimise.h"

typedef struct{
	char name[8];
	void *address;
} TAddinArrayItem;

typedef struct {
	short p1;
	short estrip_no;
	TAddinArrayItem *addin_array_addr;
	void *estrip_location;
	void *addin_smem_location;
	void *estrip_icon_location;
} TAddinEstripInformation;

extern int SMEM_optimise_codelength;
int SMEM_optimise_frame (unsigned char *title);
void App_GetAddinEstripInfo(int addinno, int estripno, TAddinEstripInformation *result);

void GetCallbackAddressPtr( int *p0, int *p1 ){
	TAddinEstripInformation aei;

    App_GetAddinEstripInfo(100, 0, &aei);
    *p0 = (int)aei.addin_array_addr;
    *p1 = *p0 + 4;
}

void JumpOptimising(){
	int p0, p1;
	unsigned char title[8];
	memcpy(title, (void *)0x003001D4, 8);

	GetCallbackAddressPtr( &p0, &p1 );
	*(int *)p1 = 0;
	SMEM_optimise_frame(title);
}