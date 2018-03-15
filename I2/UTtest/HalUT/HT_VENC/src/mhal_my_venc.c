#include <linux/module.h>
#include <linux/init.h>
#include "mhal_venc.h"
#include "ht_common.h"

#define MI_OK 0

MS_S32 MHAL_VENC_MyCreateDevice(VOID *pOsDev, VOID** ppBase, MS_S32 *pSize, MHAL_VENC_DEV_HANDLE *phDev)
{
    printk("MHAL_VENC_MyCreateDevice Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyDestroyDevice(MHAL_VENC_DEV_HANDLE hDev)
{
	printk("MHAL_VENC_MyDestroyDevice Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyGetDevConfig(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param)
{
	printk("MHAL_VENC_MyGetDevConfig\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyCreateInstance(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_INST_HANDLE *phInst)
{
    printk("MHAL_VENC_MyCreateInstance Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyDestroyInstance(MHAL_VENC_INST_HANDLE hInst)
{
    printk("MHAL_VENC_MyDestroyInstance Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MySetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param)
{
    printk("MHAL_VENC_MySetParam Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyGetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param)
{
    printk("MHAL_VENC_MyGetParam Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyEncodeOneFrame(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InOutBuf_t* pInOutBuf)
{
    HT_U8 *pu8String = "AAAAAAAAA";
    HT_U8 *pu8ViAddr = NULL;

    printk("MHAL_VENC_MyEncodeOneFrame Done\n");

    pu8ViAddr = HT_FindVirAddr(pInOutBuf->phyOutputBuf);
    memcpy(pu8ViAddr, pu8String, 10);

    return MI_OK;
}

MS_S32 MHAL_VENC_MyQueryBufSize(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InternalBuf_t *pstSize)
{
    pstSize->u32IntrAlBufSize = 10;
    pstSize->u32IntrRefBufSize = 10;

    printk("MHAL_VENC_MyQueryBufSize Done\n");
    return MI_OK;
}

MS_S32 MHAL_VENC_MyEncDone(MHAL_VENC_INST_HANDLE hInst,MHAL_VENC_EncResult_t* pEncRet)
{
    pEncRet->u32OutputBufUsed = 10;
    printk("MHAL_VENC_MyEncDone Done\n");
    return MI_OK;
}
