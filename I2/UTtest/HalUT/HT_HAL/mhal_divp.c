#include <linux/slab.h>
#include <asm/string.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/semaphore.h>

#include "mhal_divp_datatype.h"

#include "MsTypes.h"

#include "mhal_cmdq.h"
#include "mhal_divp.h"

#ifdef USE_GE
#include "mhal_gfx.h"

#define CHECK_NULL_POINTER(func, pointer) \
    do\
    {\
        if(pointer==NULL) \
        {\
            printk("%s Parameter NULL!Line: %d\n", func, __LINE__);\
            return false;\
        }\
    }while(0);
#else
#include "ht_common.h"
#endif

#define DBG_ERR(fmt, args...)  do {printk("[MHAL  ERR]: %s [%d]: ", __FUNCTION__, __LINE__);printk(fmt, ##args);}while(0)
#define DBG_INFO(fmt, args...) do {printk("[MHAL INFO]: %s [%d]: ", __FUNCTION__, __LINE__);printk(fmt, ##args);}while(0)

#define DIVP_CHANNEL_MAX (64)

typedef struct MHalDIVPInfo_s{
    MS_U8 u8ChannelId;
    struct list_head list;
    MHAL_DIVP_CaptureInfo_t stCaptureBuf;
    MHAL_DIVP_InputInfo_t stDivpInputBuf;
    MHAL_DIVP_OutPutInfo_t stDivpOutputBuf;
    MHAL_DIVP_Mirror_t stDIVPMirrorAttr;
    MHAL_DIVP_Window_t stDIVPWinAttr;
    MHAL_DIVP_TnrLevel_e eDIVPTnrAttr;
    MHAL_DIVP_DiType_e eDIVPDiAttr;
    MHAL_DIVP_Rotate_e eDIVPRotAttr;
}MHalDIVPInfo_t;

typedef struct
{
    struct semaphore stMutex;
    struct list_head active_list;
    MS_U32 u32Cnt;
} DivpManager;

static DivpManager DivpManger={
    .u32Cnt = 0,
};

static inline MS_BOOL DIVP_Manager_CheckFull(DivpManager *pMgr)
{
    if ((pMgr->u32Cnt + 1) == DIVP_CHANNEL_MAX)
    {
        return TRUE;
    }
    return FALSE;
}

MS_S32 MHAL_DIVP_Init(MHAL_DIVP_DeviceId_e eDevId)
{
    sema_init(&DivpManger.stMutex, 1);
    INIT_LIST_HEAD(&DivpManger.active_list);
    return 1;
}

MS_S32 MHAL_DIVP_DeInit(MHAL_DIVP_DeviceId_e eDevId)
{
    return 1;
}

MS_S32 MHAL_DIVP_CreateInstance(MHAL_DIVP_DeviceId_e eDevId, MS_U16 u16MaxWidth,        MS_U16 u16MaxHeight, PfnAlloc pfAlloc, PfnFree pfFree, MS_U8 u8ChannelId, void** ppCtx)
{
    MHalDIVPInfo_t *pDivpInfo;
    pDivpInfo= kmalloc(sizeof(MHalDIVPInfo_t),GFP_KERNEL);
    memset(pDivpInfo, 0, sizeof(MHalDIVPInfo_t));

    pDivpInfo->u8ChannelId = u8ChannelId;

    if(DIVP_Manager_CheckFull(&DivpManger) == TRUE)
    {
        printk("channel is Full,create Iq Instance failure\r\n");
        return FALSE;
    }

        down(&DivpManger.stMutex);
       list_add(&pDivpInfo->list, &DivpManger.active_list);
       up(&DivpManger.stMutex);

       *ppCtx =(void *) pDivpInfo;
       DivpManger.u32Cnt ++;

    return 1;
}


MS_S32 MHAL_DIVP_DestroyInstance(void* pCtx)
{
    struct list_head *pos,*q;
    MHalDIVPInfo_t *pDivpInfo;

    CHECK_NULL_POINTER(__FUNCTION__,pCtx);
    list_for_each_safe(pos,q,&DivpManger.active_list)
    {
        pDivpInfo = list_entry(pos,MHalDIVPInfo_t,list);
        if(pDivpInfo == pCtx)
        {
            printk("divp destory %d channel\r\n",pDivpInfo->u8ChannelId);
            kfree(pDivpInfo);
            list_del(pos);
            break;
        }
    }
    return 1;
}

#ifdef USE_GE
    static MS_BOOL _ScalingByGe(MS_GFX_Surface_t *pstSrc, MS_GFX_Rect_t *pstSrcRect,
      MS_GFX_Surface_t *pstDst,  MS_GFX_Rect_t *pstDstRect)
  {
      MS_U16 u16Fence = 0xFF;
      MS_GFX_Opt_t stOpt;
      MS_GFX_Opt_t *pstBlitOpt = &stOpt;
      memset(pstBlitOpt, 0, sizeof(*pstBlitOpt));
      MHalGFXOpen();
      DBG_INFO("Start bit blit.\n");

      DBG_INFO("Src : Rect = {%d, %d, %d, %d}.\n", pstSrcRect->s32Xpos, pstSrcRect->s32Ypos, pstSrcRect->u32Width, pstSrcRect->u32Height);
      DBG_INFO("Src surface = {.phyAddr: %llx, .eColorFmt: %d, .u32Width: %u, .u32Height: %u, .u32Stride: %u}.\n",
          pstSrc->phyAddr, pstSrc->eColorFmt, pstSrc->u32Width, pstSrc->u32Height, pstSrc->u32Stride);

      DBG_INFO("Dest: Rect = {%d, %d, %d, %d}.\n", pstDstRect->s32Xpos, pstDstRect->s32Ypos, pstDstRect->u32Width, pstDstRect->u32Height);
      DBG_INFO("Dest surface = {.phyAddr: %llx, .eColorFmt: %d, .u32Width: %u, .u32Height: %u, .u32Stride: %u}.\n",
          pstDst->phyAddr, pstDst->eColorFmt, pstDst->u32Width, pstDst->u32Height, pstDst->u32Stride);
      DBG_INFO("Flip: Src: 0x%llx Dest: 0x%llx.\n", pstSrc->phyAddr,pstDst->phyAddr);

      pstBlitOpt->bEnGfxRop = false;
      pstBlitOpt->eRopCode = E_MS_GFX_ROP_NONE;
      pstBlitOpt->eSrcDfbBldOp = E_MS_GFX_DFB_BLD_NONE;
      pstBlitOpt->eDstDfbBldOp = E_MS_GFX_DFB_BLD_NONE;
      pstBlitOpt->eMirror = E_MS_GFX_MIRROR_NONE;
      pstBlitOpt->eRotate = E_MS_GFX_ROTATE_0;
      pstBlitOpt->eSrcYuvFmt = E_MS_GFX_YUV_YUYV;
      pstBlitOpt->eDstYuvFmt = E_MS_GFX_YUV_YUYV;
      pstBlitOpt->stClipRect.s32Xpos = 0;
      pstBlitOpt->stClipRect.s32Ypos = 0;
      pstBlitOpt->stClipRect.u32Width  = 0;
      pstBlitOpt->stClipRect.u32Height = 0;

      MHalGFXBitBlit(pstSrc, pstSrcRect, pstDst, pstDstRect, pstBlitOpt, &u16Fence);
      DBG_INFO("Bit blit done.\n");

      DBG_INFO("Start wait fence: 0x%x.\n", u16Fence);
      MHalGFXWaitAllDone(true, u16Fence);
      DBG_INFO("Wait done.\n");

      MHalGFXClose();

      return true;
  }
#endif


MS_S32 MHAL_DIVP_CaptureTiming(void* pCtx, MHAL_DIVP_CaptureInfo_t* ptCaptureInfo,MHAL_CMDQ_CmdqInterface_t* ptCmdInf)
{
    MHalDIVPInfo_t *pDivpInfo;
    struct list_head *pos,*q;

    list_for_each_safe(pos,q,&DivpManger.active_list)
    {
        pDivpInfo = list_entry(pos,MHalDIVPInfo_t,list);
        if(pDivpInfo == pCtx)
        {
            break;
        }
    }
    pDivpInfo->stCaptureBuf = *ptCaptureInfo;

    if(ptCaptureInfo->eInputPxlFmt == E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV)
    {
#ifdef USE_GE
        MS_GFX_Surface_t stSrc;
        MS_GFX_Rect_t stSrcRect;
        MS_GFX_Surface_t stDst;
        MS_GFX_Rect_t stDstRect;

        CHECK_NULL_POINTER(__FUNCTION__, pCtx);
        CHECK_NULL_POINTER(__FUNCTION__, ptCmdInf);
        CHECK_NULL_POINTER(__FUNCTION__, ptCaptureInfo);

        memset(&stSrc, 0, sizeof(stSrc));
        stSrc.eColorFmt = E_MS_GFX_FMT_YUV422;//
        stSrc.u32Width = pDivpInfo->stDivpInputBuf.u16InputWidth;
        stSrc.u32Height = pDivpInfo->stDivpInputBuf.u16InputHeight;
        stSrc.phyAddr = pDivpInfo->stDivpInputBuf.u64BufAddr[0];
        stSrc.u32Stride =pDivpInfo->stDivpInputBuf.u16Stride[0];

        memset(&stSrcRect, 0, sizeof(stSrcRect));

        stSrcRect.s32Xpos = pDivpInfo->stDIVPWinAttr.u16X;
        stSrcRect.s32Ypos = pDivpInfo->stDIVPWinAttr.u16Y;
        stSrcRect.u32Width = pDivpInfo->stDIVPWinAttr.u16Width;
        stSrcRect.u32Height = pDivpInfo->stDIVPWinAttr.u16Height;

        memset(&stDst, 0, sizeof(stDst));
        stDst.eColorFmt = E_MS_GFX_FMT_YUV422;
        stDst.u32Width = ptCaptureInfo->u16Width;
        stDst.u32Height =ptCaptureInfo->u16Height;
        stDst.phyAddr = ptCaptureInfo->u64BufAddr[0];
        stDst.u32Stride = ptCaptureInfo->u16Stride[0];

        memset(&stDstRect, 0, sizeof(stDstRect));
        stDstRect.s32Xpos = 0;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Width = stDst.u32Width;
        stDstRect.u32Height = stDst.u32Height;
        _ScalingByGe(&stSrc, &stSrcRect, &stDst, &stDstRect);
#else
        {
            MS_U8 *poutviraddr,*pinviraddr;
            MS_U32 Buffersize=0;

            Buffersize = pDivpInfo->stDivpInputBuf.u16InputHeight * pDivpInfo->stDivpInputBuf.u16InputWidth *2;

            pinviraddr = HT_FindVirAddr(pDivpInfo->stDivpInputBuf.u64BufAddr[0]);
            poutviraddr = HT_FindVirAddr(ptCaptureInfo->u64BufAddr[0]);
            memcpy(poutviraddr, pinviraddr, Buffersize);
        }
#endif
    }
    else if(ptCaptureInfo->eInputPxlFmt == E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        MS_U8 *pyoutviraddr,*pyinviraddr;
        MS_U8 *puvoutviraddr,*puvinviraddr;
        MS_U32 Buffersize=0;

        Buffersize = pDivpInfo->stDivpInputBuf.u16InputHeight * pDivpInfo->stDivpInputBuf.u16InputWidth;

        pyinviraddr = HT_FindVirAddr(pDivpInfo->stDivpInputBuf.u64BufAddr[0]);
        pyoutviraddr = HT_FindVirAddr(ptCaptureInfo->u64BufAddr[0]);

        puvinviraddr = HT_FindVirAddr(pDivpInfo->stDivpInputBuf.u64BufAddr[1]);
        puvoutviraddr = HT_FindVirAddr(ptCaptureInfo->u64BufAddr[1]);

        memcpy(pyoutviraddr, pyinviraddr, Buffersize);
        memcpy(puvoutviraddr, puvinviraddr, Buffersize/2);
    }

    return 1;
}

MS_S32 MHAL_DIVP_ProcessDramData(void* pCtx, MHAL_DIVP_InputInfo_t* ptDivpInputInfo,MHAL_DIVP_OutPutInfo_t* ptDivpOutputInfo, MHAL_CMDQ_CmdqInterface_t* ptCmdInf)
{

    MHalDIVPInfo_t *pDivpInfo;
    struct list_head *pos,*q;

    list_for_each_safe(pos,q,&DivpManger.active_list)
    {
        pDivpInfo = list_entry(pos,MHalDIVPInfo_t,list);
        if(pDivpInfo == pCtx)
        {
            break;
        }
    }

    pDivpInfo->stDivpInputBuf = *ptDivpInputInfo;
    pDivpInfo->stDivpOutputBuf = *ptDivpOutputInfo;

    if(ptDivpInputInfo->ePxlFmt == E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV)
    {
#ifdef USE_GE
    MS_GFX_Surface_t stSrc;
    MS_GFX_Rect_t stSrcRect;
    MS_GFX_Surface_t stDst;
    MS_GFX_Rect_t stDstRect;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, ptCmdInf);
    CHECK_NULL_POINTER(__FUNCTION__, ptDivpInputInfo);
    CHECK_NULL_POINTER(__FUNCTION__, ptDivpOutputInfo);

    memset(&stSrc, 0, sizeof(stSrc));
    stSrc.eColorFmt = E_MS_GFX_FMT_YUV422;//
    stSrc.u32Width = ptDivpInputInfo->u16InputWidth;
    stSrc.u32Height = ptDivpInputInfo->u16InputHeight;
    stSrc.phyAddr = ptDivpInputInfo->u64BufAddr[0];
    stSrc.u32Stride =ptDivpInputInfo->u16Stride[0];

    memset(&stSrcRect, 0, sizeof(stSrcRect));

    stSrcRect.s32Xpos = pDivpInfo->stDIVPWinAttr.u16X;
    stSrcRect.s32Ypos = pDivpInfo->stDIVPWinAttr.u16Y;
    stSrcRect.u32Width = pDivpInfo->stDIVPWinAttr.u16Width;
    stSrcRect.u32Height = pDivpInfo->stDIVPWinAttr.u16Height;

    memset(&stDst, 0, sizeof(stDst));
    stDst.eColorFmt = E_MS_GFX_FMT_YUV422;
    stDst.u32Width = ptDivpOutputInfo->u16OutputWidth;
    stDst.u32Height =ptDivpOutputInfo->u16OutputHeight;
    stDst.phyAddr = ptDivpOutputInfo->u64BufAddr[0];
    stDst.u32Stride = ptDivpOutputInfo->u16Stride[0];

    memset(&stDstRect, 0, sizeof(stDstRect));
    stDstRect.s32Xpos = 0;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = stDst.u32Width;
    stDstRect.u32Height = stDst.u32Height;
    _ScalingByGe(&stSrc, &stSrcRect, &stDst, &stDstRect);

#else
        {
            MS_U8 *poutviraddr,*pinviraddr;
            MS_U32 Buffersize=0;

            Buffersize = (ptDivpInputInfo->u16InputWidth) * (ptDivpInputInfo->u16InputHeight)*2;
            printk("[INFO]input buffer size is %d\r\n",Buffersize);

            pinviraddr = HT_FindVirAddr(ptDivpInputInfo->u64BufAddr[0]);
            poutviraddr = HT_FindVirAddr(ptDivpOutputInfo->u64BufAddr[0]);
            memcpy(poutviraddr, pinviraddr, Buffersize);
        }

#endif
    }
    else if(ptDivpInputInfo->ePxlFmt == E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
            MS_U8 *pyoutviraddr,*pyinviraddr;
            MS_U8 *puvoutviraddr,*puvinviraddr;
            MS_U32 Buffersize=0;

            Buffersize = (ptDivpInputInfo->u16InputWidth) * (ptDivpInputInfo->u16InputHeight);
            printk("[INFO]input buffer size is %d\r\n",Buffersize);

            pyinviraddr = HT_FindVirAddr(ptDivpInputInfo->u64BufAddr[0]);
            puvinviraddr = HT_FindVirAddr(ptDivpInputInfo->u64BufAddr[1]);

            pyoutviraddr = HT_FindVirAddr(ptDivpOutputInfo->u64BufAddr[0]);
            puvoutviraddr = HT_FindVirAddr(ptDivpOutputInfo->u64BufAddr[1]);

            memcpy(pyoutviraddr, pyinviraddr, Buffersize);
            memcpy(puvoutviraddr, puvinviraddr, Buffersize/2);
    }

    return 1;
}

MS_S32 MHAL_DIVP_SetAttr(void* pCtx, MHAL_DIVP_AttrType_e eAttrType, const void* pAttr,MHAL_CMDQ_CmdqInterface_t* ptCmdInf)
{
    MHAL_DIVP_TnrLevel_e *eDIVPTnrAttr;
    MHAL_DIVP_DiType_e   *eDIVPDiAttr;
    MHAL_DIVP_Rotate_e   *eDIVPRotAttr;
    MHAL_DIVP_Mirror_t   *stDIVPMirrorAttr;
    MHAL_DIVP_Window_t   *stDIVPWinAttr;

    struct list_head *pos,*q;
    MHalDIVPInfo_t *pDivpInfo;

    list_for_each_safe(pos,q,&DivpManger.active_list)
    {
        pDivpInfo = list_entry(pos,MHalDIVPInfo_t,list);
        if(pDivpInfo == pCtx)
        {
            if(eAttrType == E_MHAL_DIVP_ATTR_TNR)
            {
               eDIVPTnrAttr = (MHAL_DIVP_TnrLevel_e*)pAttr;
               pDivpInfo->eDIVPTnrAttr = *eDIVPTnrAttr;
            }
            if(eAttrType == E_MHAL_DIVP_ATTR_DI)
            {
               eDIVPDiAttr = (MHAL_DIVP_DiType_e*)pAttr;
               pDivpInfo->eDIVPDiAttr = *eDIVPDiAttr;
            }
            if(eAttrType == E_MHAL_DIVP_ATTR_ROTATE)
            {
                eDIVPRotAttr = (MHAL_DIVP_Rotate_e*)pAttr;
                pDivpInfo->eDIVPRotAttr = *eDIVPRotAttr;
            }
            if(eAttrType == E_MHAL_DIVP_ATTR_CROP)
            {
                stDIVPWinAttr = (MHAL_DIVP_Window_t*)pAttr;
                pDivpInfo->stDIVPWinAttr = *stDIVPWinAttr;
            }
            if(eAttrType == E_MHAL_DIVP_ATTR_MIRROR)
            {
                stDIVPMirrorAttr = (MHAL_DIVP_Mirror_t*)pAttr;
                pDivpInfo->stDIVPMirrorAttr = *stDIVPMirrorAttr;
            }
        }
    }
    return 1;
}

MS_S32 MHAL_DIVP_EnableFrameDoneIsr(MS_BOOL bEnable)
{
    return 1;
}

MS_S32 MHAL_DIVP_CleanFrameDoneIsr(void)
{
    return 1;
}
