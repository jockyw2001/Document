#ifndef __HT_TEST_H__
#define __HT_TEST_H__

#include "inc/ht_common_datatype.h"

#define HT_HALUT_NODE_NAME_MAX			8
#define HT_HALUT_NODE_VALUE_MAX			24
#define HT_HALUT_NODE_COUNT_MAX 		8
#define HT_HALUT_VERSION				"v1.0"


typedef struct
{
	HT_S8 szName[HT_HALUT_NODE_NAME_MAX];
	HT_S8 szValue[HT_HALUT_NODE_VALUE_MAX];
	HT_U32 nValueCnt;
}HT_HALUT_UtData_t;

typedef enum
{
	E_HT_HALUT_VIF = 0,
	E_HT_HALUT_VPE,
	E_HT_HALUT_VENC,
	E_HT_HALUT_VDEC,
	E_HT_HALUT_DVIP,
	E_HT_HALUT_DSIP,
	E_HT_HALUT_AI,
	E_HT_HALUT_AO,
	E_HT_HALUT_NODE_MAX,
}E_HT_HALUT_NODE_e;


#endif // #ifndef __HT_TEST_H__
