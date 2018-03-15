#ifndef __HT_TEST_H__
#define __HT_TEST_H__

#include "ht_common_datatype.h"

#define HT_MODULE_NAME_MAX_LENGTH			8
#define HT_HALUT_VALUE_CNT_MAX				100
#define HT_HALUT_VERSION					"v1.0"


typedef struct
{
	HT_S8 szName[HT_MODULE_NAME_MAX_LENGTH];
	HT_U8 u8CmdCnt;
	HT_U16 *pau16CmdValue;
}HT_UtData_t;

typedef enum
{
	E_HT_MODULE_VIF = 0,
	E_HT_MODULE_VPE,
	E_HT_MODULE_VENC,
	E_HT_MODULE_VDEC,
	E_HT_MODULE_DIVP,
	E_HT_MODULE_DISP,
	E_HT_MODULE_AI,
	E_HT_MODULE_AO,
	E_HT_MODULE_HDMI,
	E_HT_MODULE_MAX,
}E_HT_MODULE_e;


#endif // #ifndef __HT_TEST_H__
