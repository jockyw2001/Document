#ifndef __HT_COMMON_H__
#define __HT_COMMON_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include "ht_vb_type.h"


/*malloc buffer*/
HT_U32 HT_Malloc(HT_VB_Info_t* pstVbInfo);

/*free buffer*/
void HT_Free(void *va);

/*write file*/
HT_S32 HT_WriteFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 nLength, HT_S64 nPos);

/*read file*/
HT_S32 HT_ReadFile(const HT_U8 *pszFileName, HT_U8 *pszReadBuf, HT_S32 nLength, HT_S64 nPos);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif // __HT_VB_H__
