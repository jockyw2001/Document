#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/slab.h>
///#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>

#include "mhal_common.h"
#include "mhal_vpe.h"

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
//#define DBG_INFO(fmt, args...) do {printk("[MHAL INFO]: %s [%d]: ", __FUNCTION__, __LINE__);printk(fmt, ##args);}while(0)
#define DBG_INFO(fmt, args...)

#define VPE_PORT_MAX    (4)
#define VPE_CHANNEL_MAX (64)
#define VPE_IRQ_NUM     (104)

typedef struct MHalVpeSCL_s{
    MS_U8 u8ChnnId;
    struct list_head list;
    MHalVpeSclWinSize_t  stMaxWin;
    MHalVpeSclOutputBufferConfig_t  stOutputBuffer;
    MHalVpeSclCropConfig_t   stCropConfig;
    MHalVpeSclOutputDmaConfig_t stDmaConfig;
    MHalVpeSclInputSizeConfig_t  stInputSize;
    MHalVpeSclOutputSizeConfig_t  stOutputSize[4];
    MHalVpeSclOutputMDwinConfig_t  stOutMDwinConfig;
}MHalVpeSCL_t;

typedef struct MHalVpeISP_s{
    MS_U8 u8ChnnId;
    struct list_head list;
    MHalVpeIspVideoInfo_t   stIspVideo;
    MHalVpeIspRotationConfig_t  stIspRoation;
    MHalVpeIspInputConfig_t     stIspInputconfig;
}MHalVpeISP_t;

typedef struct MHalVpeIq_s{
    MS_U8 u8ChnnId;
    struct list_head list;
    MHalVpeIqConfig_t   stIqconfig;
    MHalVpeIqOnOff_t    stIqOnOff;
    MHalVpeIqWdrRoiReport_t  stIqRoidata;
    MHalVpeIqWdrRoiHist_t  stIqRoiConfig;
}MHalVpeIq_t;

typedef struct
{
    MS_U64 u64BitMap;
    struct semaphore stMutex;
    struct list_head active_list;
    MS_U32 u32Cnt;
} VpeManager;

static VpeManager _gIqMgr = {
    .u64BitMap = 0,
    .u32Cnt = 0,
};

static VpeManager _gIspMgr = {
    .u64BitMap = 0,
    .u32Cnt = 0,
};

static VpeManager _gSclMgr = {
    .u64BitMap = 0,
    .u32Cnt = 0,
};

static const MHalVpeSclOutputBufferConfig_t *_gpSclVideo;
static MHalVpeSCL_t * _gpVpeScl = NULL;

static inline MS_BOOL VPE_Manager_CheckFull(VpeManager *pMgr)
{
    if ((pMgr->u32Cnt + 1) == VPE_CHANNEL_MAX)
    {
        return true;
    }
    return false;
}

static inline MS_BOOL VPE_Manager_CheckEmpty(VpeManager *pMgr)
{
    if (pMgr->u32Cnt == 0)
    {
        return true;
    }
    return false;
}

MS_BOOL MHalVpeCreateIqInstance(const MHalAllocPhyMem_t *pstAlloc ,void **pCtx)
{
    MHalVpeIq_t *pIq = NULL;
    int i=0;

    pIq = kmalloc(sizeof(MHalVpeIq_t), GFP_KERNEL);
    memset(pIq,0,sizeof(MHalVpeIq_t));

    CHECK_NULL_POINTER(__FUNCTION__,pstAlloc);
    CHECK_NULL_POINTER(__FUNCTION__,pIq);

    if(VPE_Manager_CheckFull(&_gIqMgr) == true)
    {
        printk("channel is Full,create Iq Instance failure\r\n");
        return false;
    }
    else if(VPE_Manager_CheckEmpty(&_gIqMgr) == true)
    {
        sema_init(&_gIqMgr.stMutex, 1);
        INIT_LIST_HEAD(&_gIqMgr.active_list);
        pIq->u8ChnnId=0;
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            if (0 == ((_gIqMgr).u64BitMap && (1 << i)))
            {
                (pIq)->u8ChnnId = i;
                break;
            }
        }
    }

    down(&_gIqMgr.stMutex);
    _gIqMgr.u32Cnt ++;
    list_add(&pIq->list, &_gIqMgr.active_list);
    up(&_gIqMgr.stMutex);

    *pCtx =(void *) pIq;
    CHECK_NULL_POINTER(__FUNCTION__,pCtx);
    return true;
}

MS_BOOL MHalVpeDestroyIqInstance(void *pCtx)
{
   MHalVpeIq_t *pIq = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__,pCtx);

    down(&(_gIqMgr).stMutex);
    list_for_each_safe(pos,q,&_gIqMgr.active_list)
    {
        pIq = list_entry(pos,MHalVpeIq_t,list);
        if(pIq == pCtx)
        {
            _gIqMgr.u64BitMap &= ~(1 << (pIq)->u8ChnnId);
            _gIqMgr.u32Cnt--;
            kfree(pIq);
            list_del(pos);
            break;
        }
    }
    up(&(_gIqMgr).stMutex);
    return true;
}

MS_BOOL MHalVpeIqProcess(void *pCtx, const MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    MHalVpeIq_t *pIq = NULL;
    pIq = (MHalVpeIq_t *)pCtx;

    CHECK_NULL_POINTER(__FUNCTION__,pCtx);
    CHECK_NULL_POINTER(__FUNCTION__,pIq);
    CHECK_NULL_POINTER(__FUNCTION__,pstCmdQInfo);

    return true;
}

MS_BOOL MHalVpeIqDbgLevel(void *p)
{
    CHECK_NULL_POINTER(__FUNCTION__,p);

     return true;
}

 MS_BOOL MHalVpeIqConfig(void *pCtx, const MHalVpeIqConfig_t *pCfg)
{
    MHalVpeIq_t *pIq = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__,pCtx);
    CHECK_NULL_POINTER(__FUNCTION__,pCfg);

    down(&(_gIqMgr).stMutex);
    list_for_each_safe(pos,q,&_gIqMgr.active_list)
    {
        pIq = list_entry(pos,MHalVpeIq_t,list);
        if(pIq == pCtx)
        {
            pIq->stIqconfig = *pCfg;
            printk("Iq channid %d , channle cnt %d\r\n",pIq->u8ChnnId,_gIqMgr.u32Cnt);

            break;
        }
    }
    up(&(_gIqMgr).stMutex);

    CHECK_NULL_POINTER(__FUNCTION__,pIq);
    return true;
}

MS_BOOL MHalVpeIqOnOff(void *pCtx, const MHalVpeIqOnOff_t *pCfg)
{
    MHalVpeIq_t *pIq = NULL;
     struct list_head *pos,*q;

     CHECK_NULL_POINTER(__FUNCTION__,pCtx);
     CHECK_NULL_POINTER(__FUNCTION__,pCfg);

     down(&(_gIqMgr).stMutex);
     list_for_each_safe(pos,q,&_gIqMgr.active_list)
     {
         pIq = list_entry(pos,MHalVpeIq_t,list);
         if(pIq == pCtx)
         {
             pIq->stIqOnOff = *pCfg;
             break;
         }
     }
     up(&(_gIqMgr).stMutex);

     CHECK_NULL_POINTER(__FUNCTION__,pIq);
     return true;
}

 MS_BOOL MHalVpeIqGetWdrRoiHist(void *pCtx, MHalVpeIqWdrRoiReport_t * pstRoiReport)
{
     MHalVpeIq_t *pIq = NULL;
     MS_S32 i=0;
     struct list_head *pos,*q;

     CHECK_NULL_POINTER(__FUNCTION__,pCtx);
     CHECK_NULL_POINTER(__FUNCTION__,pstRoiReport);

    for (i = 0; i < ROI_WINDOW_MAX; i++)
    {
        pstRoiReport->u32Y[i] = (MS_U32)((i + 1) * 10);
    }

    down(&(_gIqMgr).stMutex);
    list_for_each_safe(pos,q,&_gIqMgr.active_list)
    {
        pIq = list_entry(pos,MHalVpeIq_t,list);
        if(pIq == pCtx)
        {
         pIq->stIqRoidata = *pstRoiReport;
         break;
        }
    }
    up(&(_gIqMgr).stMutex);

    CHECK_NULL_POINTER(__FUNCTION__,pIq);
    return true;
}

 MS_BOOL MHalVpeIqSetWdrRoiHist(void *pCtx, const MHalVpeIqWdrRoiHist_t *pCfg)
 {
    MHalVpeIq_t *pIq = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__,pCtx);
    CHECK_NULL_POINTER(__FUNCTION__,pCfg);

    down(&(_gIqMgr).stMutex);
    list_for_each_safe(pos,q,&_gIqMgr.active_list)
    {
        pIq = list_entry(pos,MHalVpeIq_t,list);
        if(pIq == pCtx)
        {
            pIq->stIqRoiConfig = *pCfg;
            break;
        }
    }
    up(&(_gIqMgr).stMutex);

    CHECK_NULL_POINTER(__FUNCTION__,pIq);
    return true;
 }

// Register write via cmdQ
 MS_BOOL MHalVpeIqSetWdrRoiMask(void *pCtx,const MS_BOOL bEnMask, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
     MHalVpeIq_t *pIq = NULL;
     pIq = (MHalVpeIq_t *)pCtx;

     CHECK_NULL_POINTER(__FUNCTION__,pCtx);
     CHECK_NULL_POINTER(__FUNCTION__,pstCmdQInfo);
     CHECK_NULL_POINTER(__FUNCTION__,pIq);

    return true;
}

// Register write via cmdQ
MS_BOOL MHalVpeIqSetDnrTblMask(void *pCtx,const MS_BOOL bEnMask, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
     MHalVpeIq_t *pIq = NULL;
     pIq = (MHalVpeIq_t *)pCtx;

     CHECK_NULL_POINTER(__FUNCTION__,pCtx);
     CHECK_NULL_POINTER(__FUNCTION__,pstCmdQInfo);
     CHECK_NULL_POINTER(__FUNCTION__,pIq);

    return true;
}


//ROI Buffer

//ISP
MS_BOOL MHalVpeCreateIspInstance(const MHalAllocPhyMem_t *pstAlloc ,void **pCtx)
{
    MHalVpeISP_t *pIsp = NULL;
    int i=0;

    pIsp = kmalloc(sizeof(MHalVpeISP_t), GFP_KERNEL);
    memset(pIsp,0,sizeof(MHalVpeISP_t));

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pstAlloc);
    CHECK_NULL_POINTER(__FUNCTION__, pIsp);

    if(VPE_Manager_CheckFull(&_gIspMgr) == true)
    {
       printk("channel is Full,create Isp Instance failure\r\n");
       return false;
    }
    else if(VPE_Manager_CheckEmpty(&_gIspMgr) == true)
    {
        sema_init(&_gIspMgr.stMutex, 1);
        INIT_LIST_HEAD(&_gIspMgr.active_list);
        pIsp->u8ChnnId=0;
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            if (0 == ((_gIspMgr).u64BitMap && (1 << i)))
            {
                (pIsp)->u8ChnnId = i;
                break;
            }
        }
    }

    down(&_gIspMgr.stMutex);
    list_add(&pIsp->list, &_gIspMgr.active_list);
    _gIspMgr.u32Cnt ++;
    up(&_gIspMgr.stMutex);

    *pCtx =(void *) pIsp;
    return true;
}

MS_BOOL MHalVpeDestroyIspInstance(void *pCtx)
{
    MHalVpeISP_t *pIsp = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);

    down(&_gIspMgr.stMutex);
    list_for_each_safe(pos,q,&_gIspMgr.active_list)
    {
        pIsp = list_entry(pos,MHalVpeISP_t,list);
        if(pIsp == pCtx)
        {
            _gIspMgr.u64BitMap &= ~(1 << (pIsp)->u8ChnnId);
            _gIspMgr.u32Cnt--;
            kfree(pIsp);
            list_del(pos);
            break;
        }
    }
    up(&_gIspMgr.stMutex);

    return true;
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

 MS_BOOL MHalVpeIspProcess(void *pCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, const MHalVpeIspVideoInfo_t *pstVidInfo)
{
    MHalVpeISP_t *pIsp = NULL;
    MS_U8 u8PortId = 0;
    MHalVpeSCL_t *pScl;
    struct list_head *pos,*q;

    down(&_gIspMgr.stMutex);
    list_for_each_safe(pos,q,&_gIspMgr.active_list)
    {
        pIsp = list_entry(pos,MHalVpeISP_t,list);
        if(pIsp == pCtx)
        {
            break;
        }
    }
    up(&_gIspMgr.stMutex);

    pScl = _gpVpeScl;
    pIsp->stIspVideo = *pstVidInfo;

    if(pIsp->stIspInputconfig.ePixelFormat == E_MHAL_PIXEL_FRAME_YUV422_YUYV)
    {
#ifdef USE_GE
        MS_GFX_Surface_t stSrc;
        MS_GFX_Rect_t stSrcRect;
        MS_GFX_Surface_t stDst;
        MS_GFX_Rect_t stDstRect;

        CHECK_NULL_POINTER(__FUNCTION__, pCtx);
        CHECK_NULL_POINTER(__FUNCTION__, pstCmdQInfo);
        CHECK_NULL_POINTER(__FUNCTION__, pstVidInfo);
        CHECK_NULL_POINTER(__FUNCTION__, pIsp);

        memset(&stSrc, 0, sizeof(stSrc));
        stSrc.eColorFmt = E_MS_GFX_FMT_YUV422;//
        stSrc.u32Width = pIsp->stIspInputconfig.u32Width;
        stSrc.u32Height = pIsp->stIspInputconfig.u32Height;
        stSrc.phyAddr = pIsp->stIspVideo.u64PhyAddr[0];
        stSrc.u32Stride = pIsp->stIspVideo.u32Stride[0];


        memset(&stSrcRect, 0, sizeof(stSrcRect));
        if(pScl->stCropConfig.bCropEn == true)
        {
            stSrcRect.s32Xpos = pScl->stCropConfig.stCropWin.u16X;
            stSrcRect.s32Ypos = pScl->stCropConfig.stCropWin.u16Y;
            stSrcRect.u32Width = pScl->stCropConfig.stCropWin.u16Width;
            stSrcRect.u32Height = pScl->stCropConfig.stCropWin.u16Height;
        }
        else
        {
            stSrcRect.s32Xpos = 0;
            stSrcRect.s32Ypos = 0;
            stSrcRect.u32Width = stSrc.u32Width;
            stSrcRect.u32Height =stSrc.u32Height;
        }

        for(u8PortId = 0; u8PortId < VPE_PORT_MAX; u8PortId++)
        {
            if(pScl->stOutputBuffer.stCfg[u8PortId].bEn == false)
                continue;

            memset(&stDst, 0, sizeof(stDst));
            stDst.eColorFmt = E_MS_GFX_FMT_YUV422;
            stDst.u32Width = pScl->stOutputSize[u8PortId].u16Width;
            stDst.u32Height = pScl->stOutputSize[u8PortId].u16Height;
            stDst.phyAddr = _gpSclVideo->stCfg[u8PortId].stBufferInfo.u64PhyAddr[0];
            stDst.u32Stride = _gpSclVideo->stCfg[u8PortId].stBufferInfo.u32Stride[0];

            memset(&stDstRect, 0, sizeof(stDstRect));
            stDstRect.s32Xpos = 0;
            stDstRect.s32Ypos = 0;
            stDstRect.u32Width = stDst.u32Width;
            stDstRect.u32Height = stDst.u32Height;
            _ScalingByGe(&stSrc, &stSrcRect, &stDst, &stDstRect);
        }
#else
    {
        MS_U8 *poutviraddr,*pinviraddr;
        MS_U32 Buffersize=0;

        Buffersize = (pIsp->stIspInputconfig.u32Height) * (pIsp->stIspInputconfig.u32Width)*2;

        pinviraddr = HT_FindVirAddr(pstVidInfo->u64PhyAddr[0]);
        for(u8PortId = 0; u8PortId < VPE_PORT_MAX; u8PortId++)
        {
            if(_gpSclVideo->stCfg[u8PortId].bEn == TRUE)
            {
                poutviraddr = HT_FindVirAddr(_gpSclVideo->stCfg[u8PortId].stBufferInfo.u64PhyAddr[0]);
                memcpy(poutviraddr, pinviraddr, Buffersize);
            }
        }

    }
#endif
    }
    else if(pIsp->stIspInputconfig.ePixelFormat == E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        MS_U8 *pyoutviraddr,*pyinviraddr;
        MS_U8 *puvoutviraddr,*puvinviraddr;
        MS_U32 Buffersize=0;

        Buffersize = (pIsp->stIspInputconfig.u32Height) * (pIsp->stIspInputconfig.u32Width);

        pyinviraddr = HT_FindVirAddr(pstVidInfo->u64PhyAddr[0]);
        puvinviraddr = HT_FindVirAddr(pstVidInfo->u64PhyAddr[1]);
        for(u8PortId = 0; u8PortId < VPE_PORT_MAX; u8PortId++)
        {
            if(_gpSclVideo->stCfg[u8PortId].bEn == TRUE)
            {
                pyoutviraddr = HT_FindVirAddr(_gpSclVideo->stCfg[u8PortId].stBufferInfo.u64PhyAddr[0]);
                puvoutviraddr = HT_FindVirAddr(_gpSclVideo->stCfg[u8PortId].stBufferInfo.u64PhyAddr[1]);
                memcpy(pyoutviraddr, pyinviraddr, Buffersize);
                memcpy(puvoutviraddr, puvinviraddr, Buffersize/2);
            }
        }
    }

    return true;
}

 MS_BOOL MHalVpeIspDbgLevel(void *p)
{
    CHECK_NULL_POINTER(__FUNCTION__, p);
    return true;
}


MS_BOOL MHalVpeIspRotationConfig(void *pCtx, const MHalVpeIspRotationConfig_t *pCfg)
{
    MHalVpeISP_t *pIsp = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gIspMgr.stMutex);
    list_for_each_safe(pos,q,&_gIspMgr.active_list)
    {
        pIsp = list_entry(pos,MHalVpeISP_t,list);
        if(pIsp == pCtx)
        {
            pIsp->stIspRoation = *pCfg;
            break;
        }
    }
    up(&_gIspMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pIsp);
    return true;
}

MS_BOOL MHalVpeIspInputConfig(void *pCtx, const MHalVpeIspInputConfig_t *pCfg)
{
    MHalVpeISP_t *pIsp = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gIspMgr.stMutex);
    list_for_each_safe(pos,q,&_gIspMgr.active_list)
    {
        pIsp = list_entry(pos,MHalVpeISP_t,list);
        if(pIsp == pCtx)
        {
            pIsp->stIspInputconfig = *pCfg;
            break;
        }
    }
    up(&_gIspMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pIsp);
    return true;
}


// SCL
MS_BOOL MHalVpeCreateSclInstance(const MHalAllocPhyMem_t *pstAlloc, const MHalVpeSclWinSize_t *pstMaxWin, void **pCtx)
{
    MHalVpeSCL_t *pscl = NULL;
    int i=0;

    pscl = kmalloc(sizeof(MHalVpeSCL_t), GFP_KERNEL);
    memset(pscl,0,sizeof(MHalVpeSCL_t));

    CHECK_NULL_POINTER(__FUNCTION__, pstAlloc);
    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    if(VPE_Manager_CheckFull(&_gSclMgr) == true)
    {
      printk("channel is Full,create Isp Instance failure\r\n");
      return false;
    }
    else if(VPE_Manager_CheckEmpty(&_gSclMgr) == true)
    {
      sema_init(&_gSclMgr.stMutex, 1);
      INIT_LIST_HEAD(&_gSclMgr.active_list);
      pscl->u8ChnnId = 0;
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            if (0 == ((_gSclMgr).u64BitMap && (1 << i)))
            {
                (pscl)->u8ChnnId = i;
                break;
            }
        }
    }

    down(&_gSclMgr.stMutex);
    list_add(&pscl->list, &_gSclMgr.active_list);
    _gSclMgr.u32Cnt ++;
    up(&_gSclMgr.stMutex);

    pscl->stMaxWin = *pstMaxWin;
    *pCtx =(void *) pscl;
    CHECK_NULL_POINTER(__FUNCTION__, pCtx);

    return true;
}

MS_BOOL MHalVpeDestroySclInstance(void *pCtx)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;
    CHECK_NULL_POINTER(__FUNCTION__, pCtx);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            _gSclMgr.u64BitMap &= ~(1 << (pscl)->u8ChnnId);
            _gSclMgr.u32Cnt--;
            kfree(pscl);
            list_del(pos);
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    return true;
}

MS_BOOL MHalVpeSclProcess(void *pCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, const MHalVpeSclOutputBufferConfig_t *pBuffer)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pBuffer);
    CHECK_NULL_POINTER(__FUNCTION__, pstCmdQInfo);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            pscl->stOutputBuffer = *pBuffer;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    _gpSclVideo = pBuffer;
    _gpVpeScl = pscl;

    return true;
}

MS_BOOL MHalVpeSclDbgLevel(void *p)
{
    CHECK_NULL_POINTER(__FUNCTION__, p);
    return true;
}

MS_BOOL MHalVpeSclCropConfig(void *pCtx, const MHalVpeSclCropConfig_t *pCfg)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            pscl->stCropConfig = *pCfg;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    return true;
}

MS_BOOL MHalVpeSclOutputDmaConfig(void *pCtx, const MHalVpeSclOutputDmaConfig_t *pCfg)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            pscl->stDmaConfig = *pCfg;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    return true;
}

MS_BOOL MHalVpeSclInputConfig(void *pCtx, const MHalVpeSclInputSizeConfig_t *pCfg)
{
    MHalVpeSCL_t *pscl = NULL;
   struct list_head *pos,*q;

   CHECK_NULL_POINTER(__FUNCTION__, pCtx);
   CHECK_NULL_POINTER(__FUNCTION__, pCfg);

   down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            pscl->stInputSize = *pCfg;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    return true;
}

MS_BOOL MHalVpeSclOutputSizeConfig(void *pCtx, const MHalVpeSclOutputSizeConfig_t *pCfg)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
             MS_U8 port = pCfg->enOutPort;
             pscl->stOutputSize[port] = *pCfg;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    return true;
}

MS_BOOL MHalVpeSclOutputMDWinConfig(void *pCtx, const MHalVpeSclOutputMDwinConfig_t *pCfg)
{
    MHalVpeSCL_t *pscl = NULL;
    struct list_head *pos,*q;

    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pCfg);

    down(&_gSclMgr.stMutex);
    list_for_each_safe(pos,q,&_gSclMgr.active_list)
    {
        pscl = list_entry(pos,MHalVpeSCL_t,list);
        if(pscl == pCtx)
        {
            pscl->stOutMDwinConfig = *pCfg;
            break;
        }
    }
    up(&_gSclMgr.stMutex);

    CHECK_NULL_POINTER(__FUNCTION__, pscl);
    return true;
}

// SCL vpe irq.
MS_BOOL MHalVpeSclGetIrqNum(unsigned int *pIrqNum)
{
    CHECK_NULL_POINTER(__FUNCTION__, pIrqNum);
   *pIrqNum = VPE_IRQ_NUM;

    return true;
}

// RIU write register
MS_BOOL MHalVpeSclEnableIrq(MS_BOOL bOn)
{

    return true;
}

// RIU write register
MS_BOOL MHalVpeSclClearIrq(void)
{
    return true;
}

MS_BOOL MHalVpeSclCheckIrq(void)
{
    return true;
}


// SCL sw trigger irq by CMDQ or Sc
MS_BOOL MHalVpeSclSetSwTriggerIrq(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    CHECK_NULL_POINTER(__FUNCTION__, pstCmdQInfo);
    return true;
}

// SCL in irq bottom: Read 3DNR register
MS_BOOL MHalVpeIqRead3DNRTbl(void *pCtx)
{
    CHECK_NULL_POINTER(__FUNCTION__, pCtx);
    return true;
}

// SCL polling MdwinDone
MS_BOOL MHalVpeSclSetWaitMdwinDone(void *pSclCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    CHECK_NULL_POINTER(__FUNCTION__, pSclCtx);
    CHECK_NULL_POINTER(__FUNCTION__, pstCmdQInfo);

    return true;
}

