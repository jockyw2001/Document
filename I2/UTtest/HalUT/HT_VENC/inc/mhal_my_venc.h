#include <linux/module.h>
#include <linux/init.h>
#include "mhal_venc.h"


MS_S32 MHAL_VENC_MyCreateDevice(VOID *pOsDev, VOID** ppBase, MS_S32 *pSize, MHAL_VENC_DEV_HANDLE *phDev);

MS_S32 MHAL_VENC_MyDestroyDevice(MHAL_VENC_DEV_HANDLE hDev);

MS_S32 MHAL_VENC_MyGetDevConfig(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param);

MS_S32 MHAL_VENC_MyCreateInstance(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_INST_HANDLE *phInst);

MS_S32 MHAL_VENC_MyDestroyInstance(MHAL_VENC_INST_HANDLE hInst);

MS_S32 MHAL_VENC_MySetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param);

MS_S32 MHAL_VENC_MyGetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param);

MS_S32 MHAL_VENC_MyEncodeOneFrame(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InOutBuf_t* pInOutBuf);

MS_S32 MHAL_VENC_MyQueryBufSize(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InternalBuf_t *pstSize);

MS_S32 MHAL_VENC_MyEncDone(MHAL_VENC_INST_HANDLE hInst,MHAL_VENC_EncResult_t* pEncRet);
