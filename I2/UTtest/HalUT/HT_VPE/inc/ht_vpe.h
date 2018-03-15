#ifndef __HT_VPE_H__
#define __HT_VPE_H__

#include "ht_common.h"
#include "ht_common_datatype.h"
#include "ht_vb_type.h"

//HT_RESULT HT_VPE(HT_S8 *pszCmd);
HT_RESULT HT_VPE(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt);

void HT_VPE_DisplayHelp(void);



#endif
