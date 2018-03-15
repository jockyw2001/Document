#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include "mhal_vif.h"
#include "mhal_vif_datatype.h"
#include "mhal_common.h"
#include "ht_vb_type.h"
#include "ht_common_datatype.h"
#include "ht_common.h"

#ifdef USE_GE
#include "mhal_gfx.h"
#endif

#define HALVIF_YUV420_FILENAME "/mnt/YUVFILE/YUV420SP_1920_1088.yuv"
#define HALVIF_YUV422_FILENAME "/mnt/YUVFILE/YUV422_YUYV1920_1080_40.yuv"
#define MHAL_MAX_RINGBUFF 4
#define MHAL_MAX_CHN_OUTPORT 2

typedef struct MHAL_VIF_RingBufInfo_s
{
    MHal_VIF_RingBufElm_t stRingbuffer;
    MS_BOOL bUsed;
}MHAL_VIF_RingBufInfo_t;

typedef struct MHAL_VIF_PortContext_s
{
    MS_BOOL bEnable;
    MHAL_VIF_RingBufInfo_t stRingBufInfo[MHAL_MAX_RINGBUFF];
}MHAL_VIF_PortContext_t;

typedef struct MHAL_VIF_ChnContext_s
{
    MHal_VIF_ChnCfg_t stChnCfg;
    MHal_VIF_SubChnCfg_t stSubChnCfg;
    MHAL_VIF_PortContext_t stPortCtx[MHAL_MAX_CHN_OUTPORT];
}MHAL_VIF_ChnContext_t;

MHAL_VIF_ChnContext_t *pgstChnCtx;
HT_S64 s64ReadFilePos[16][2];



#ifdef USE_GE
#define DBG_ERR(fmt, args...)  do {printk("[MHAL  ERR]: %s [%d]: ", __FUNCTION__, __LINE__);printk(fmt, ##args);}while(0)
#define DBG_INFO(fmt, args...) do {printk("[MHAL INFO]: %s [%d]: ", __FUNCTION__, __LINE__);printk(fmt, ##args);}while(0)

 static MS_BOOL VIF_ScalingByGe(MS_GFX_Surface_t *pstSrc, MS_GFX_Rect_t *pstSrcRect,
    MS_GFX_Surface_t *pstDst,  MS_GFX_Rect_t *pstDstRect)
 {
    MS_U16 u16Fence = 0xFF;
    MS_GFX_Opt_t stOpt;
    MS_GFX_Opt_t *pstBlitOpt = &stOpt;
    memset(pstBlitOpt, 0, sizeof(*pstBlitOpt));
    MHalGFXOpen();
    pstBlitOpt->bEnGfxRop = FALSE;
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
    MHalGFXWaitAllDone(TRUE, u16Fence);
    MHalGFXClose();
    return MHAL_SUCCESS;
 }
 static MS_BOOL MHAL_VIF_USE_GE(MHal_VIF_CHN s32VifChn, MHal_VIF_PORT s32ChnPort,MHAL_VIF_ChnContext_t * pstChnCtx)
 {
    HT_U32 u32YuvBuffSize;
    HT_U32 u32SrcWidth = 1920;
    HT_U32 u32SrcHeight = 1080;
    HT_YUVInfo_t stSrcVbInfo;
    MS_GFX_Surface_t stSrc ,stDst;
    MS_GFX_Rect_t   stSrcRect,stDstRect;

    stSrcVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV422_YUYV;
    HT_MallocYUVOneFrame(&stSrcVbInfo,u32SrcWidth, u32SrcHeight);
    HT_ReadYUVOneFrameToBuffer(HALVIF_YUV422_FILENAME,&stSrcVbInfo,&s64ReadFilePos[s32VifChn][s32ChnPort]);
    u32YuvBuffSize=stSrcVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size;
    if(s64ReadFilePos[s32VifChn][s32ChnPort]==(u32YuvBuffSize*10))
        s64ReadFilePos[s32VifChn][s32ChnPort]=0;
    stSrc.phyAddr=stSrcVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr;
    stSrc.eColorFmt=E_MS_GFX_FMT_YUV422;
    stSrc.u32Width=u32SrcWidth;
    stSrc.u32Height=u32SrcHeight;
    stSrc.u32Stride=stSrc.u32Width*2;
    stSrcRect.s32Xpos=0;
    stSrcRect.s32Ypos=0;
    stSrcRect.u32Width=u32SrcWidth;
    stSrcRect.u32Height=u32SrcHeight;

    stDst.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[0];
    stDst.eColorFmt=E_MS_GFX_FMT_YUV422;
    stDst.u32Width=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW;
    stDst.u32Height=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH;
    stDst.u32Stride=stDst.u32Width*2;
    stDstRect.s32Xpos=0;
    stDstRect.s32Ypos=0;
    stDstRect.u32Width=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW;
    stDstRect.u32Height=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH;
    VIF_ScalingByGe(&stSrc,&stSrcRect,&stDst,&stDstRect);

    HT_FreeYUVOneFrame(&stSrcVbInfo);
    return MHAL_SUCCESS;
 }
#endif

MS_S32 MHal_VIF_Init(void)
{
	pgstChnCtx = (MHAL_VIF_ChnContext_t *)kmalloc((sizeof(MHAL_VIF_ChnContext_t)) * 16, GFP_KERNEL);
	memset(pgstChnCtx,0,(sizeof(MHAL_VIF_ChnContext_t)) * 16);
	memset(s64ReadFilePos,0,sizeof(s64ReadFilePos));
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_Deinit(void)
{
    memset(pgstChnCtx,0,sizeof(MHAL_VIF_ChnContext_t)*16);
    memset(s64ReadFilePos,0,sizeof(s64ReadFilePos));
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_Reset(void)
{
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_DevSetConfig(MHal_VIF_DEV s32VifDev, MHal_VIF_DevCfg_t *pstDevAttr)
{
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_DevEnable(MHal_VIF_DEV s32VifDev)
{
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_DevDisable(MHal_VIF_DEV s32VifDev)
{
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_ChnSetConfig(MHal_VIF_CHN s32VifChn, MHal_VIF_ChnCfg_t *pstAttr)
{
	MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stChnCfg=*pstAttr;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_ChnEnable(MHal_VIF_CHN s32VifChn)
{
	MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stPortCtx[0].bEnable=TRUE;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_ChnDisable(MHal_VIF_CHN s32VifChn)
{
	MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stPortCtx[0].bEnable=FALSE;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_ChnQuery(MHal_VIF_CHN s32VifChn, MHal_VIF_ChnStat_t *pstStat)
{
    return MHAL_SUCCESS;
}

MS_S32 MHal_VIF_SubChnSetConfig(MHal_VIF_CHN s32VifChn, MHal_VIF_SubChnCfg_t *pstAttr)
{
    MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stSubChnCfg = *pstAttr;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}

MS_S32 MHal_VIF_SubChnEnable(MHal_VIF_CHN s32VifChn)
{
    MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stPortCtx[1].bEnable=TRUE;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_SubChnDisable(MHal_VIF_CHN s32VifChn)
{
    MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		pstChnCtx->stPortCtx[1].bEnable=FALSE;
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_SubChnQuery(MHal_VIF_CHN s32VifChn, MHal_VIF_ChnStat_t *pstStat)
{
    return MHAL_SUCCESS;
}

MS_S32 MHal_VIF_QueueFrameBuffer(MHal_VIF_CHN s32VifChn, MHal_VIF_PORT s32ChnPort, const MHal_VIF_RingBufElm_t *pstFbInfo)
{
    MS_U8 i;
    MHAL_VIF_ChnContext_t *pstChnCtx;
	if(s32VifChn < 16)
	{
		pstChnCtx = pgstChnCtx+ s32VifChn;
		for(i=0;i<MHAL_MAX_RINGBUFF;i++)
		{
		    if(pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[i].bUsed ==FALSE)
		    {
                pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[i].stRingbuffer=*pstFbInfo;
                pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[i].bUsed=TRUE;
                break;
		    }
		}
        if(i >= 4)
            printk("Channel:%d,Port:%d the ringbuffer is fulled!\r\n",s32VifChn,s32ChnPort);
	}
	else
	    return MHAL_FAILURE;
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_QueryFrames(MHal_VIF_CHN s32VifChn, MHal_VIF_PORT s32ChnPort, MS_U32 *pNumBuf)
{
    HT_U32 u32YuvBuffSize;
	MHAL_VIF_ChnContext_t *pstChnCtx;
	HT_YUVInfo_t stYuvVbInfo;

    pstChnCtx = pgstChnCtx + s32VifChn;
	if(pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].bUsed==TRUE)
	{
	    #ifdef USE_GE
	    if(s32ChnPort==0)
	    {
            if(pstChnCtx->stChnCfg.ePixFormat==E_MHAL_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
	        {
	            stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                stYuvVbInfo.stYUV420SP.stVbInfo_y.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[0];
                stYuvVbInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV420SP.stVbInfo_y.phyAddr);
                stYuvVbInfo.stYUV420SP.stVbInfo_y.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW;
                stYuvVbInfo.stYUV420SP.stVbInfo_uv.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[1];
                stYuvVbInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV420SP.stVbInfo_uv.phyAddr);
                stYuvVbInfo.stYUV420SP.stVbInfo_uv.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW/2;
                HT_ReadYUVOneFrameToBuffer(HALVIF_YUV420_FILENAME, &stYuvVbInfo, &s64ReadFilePos[s32VifChn][s32ChnPort]);
                u32YuvBuffSize=stYuvVbInfo.stYUV420SP.stVbInfo_y.u32Size+stYuvVbInfo.stYUV420SP.stVbInfo_uv.u32Size;
                if(s64ReadFilePos[s32VifChn][s32ChnPort]==(u32YuvBuffSize*10))
                    s64ReadFilePos[s32VifChn][s32ChnPort]=0;
	        }
    	    else if(pstChnCtx->stChnCfg.ePixFormat==E_MHAL_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
    	    {
    	        stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV422_YUYV;
                stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[0];
                stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr);
                stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW*2;
                HT_ReadYUVOneFrameToBuffer(HALVIF_YUV422_FILENAME,&stYuvVbInfo,&s64ReadFilePos[s32VifChn][s32ChnPort]);
                u32YuvBuffSize=stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size;
                if(s64ReadFilePos[s32VifChn][s32ChnPort]==(u32YuvBuffSize*10))
                    s64ReadFilePos[s32VifChn][s32ChnPort]=0;
    	    }
    	    *pNumBuf = 1;
	    }
	    else if(s32ChnPort==1)
	    {
            MHAL_VIF_USE_GE(s32VifChn,s32ChnPort,pstChnCtx);
            *pNumBuf = 1;
            return MHAL_FAILURE;
	    }
	    #else
        if(pstChnCtx->stChnCfg.ePixFormat==E_MHAL_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
        {
            stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            stYuvVbInfo.stYUV420SP.stVbInfo_y.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[0];
            stYuvVbInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV420SP.stVbInfo_y.phyAddr);
            stYuvVbInfo.stYUV420SP.stVbInfo_y.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW;
            stYuvVbInfo.stYUV420SP.stVbInfo_uv.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[1];
            stYuvVbInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV420SP.stVbInfo_uv.phyAddr);
            stYuvVbInfo.stYUV420SP.stVbInfo_uv.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW/2;
            HT_ReadYUVOneFrameToBuffer(HALVIF_YUV420_FILENAME, &stYuvVbInfo, &s64ReadFilePos[s32VifChn][s32ChnPort]);
            u32YuvBuffSize=stYuvVbInfo.stYUV420SP.stVbInfo_y.u32Size+stYuvVbInfo.stYUV420SP.stVbInfo_uv.u32Size;
            if(s64ReadFilePos[s32VifChn][s32ChnPort]==(u32YuvBuffSize*10))
                s64ReadFilePos[s32VifChn][s32ChnPort]=0;
        }
	    else if(pstChnCtx->stChnCfg.ePixFormat==E_MHAL_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
	    {
	        stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV422_YUYV;
            stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.u64PhyAddr[0];
            stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr=HT_FindVirAddr(stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr);
            stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size=pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropH*pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer.nCropW*2;
            HT_ReadYUVOneFrameToBuffer(HALVIF_YUV422_FILENAME,&stYuvVbInfo,&s64ReadFilePos[s32VifChn][s32ChnPort]);
            u32YuvBuffSize=stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size;
            if(s64ReadFilePos[s32VifChn][s32ChnPort]==(u32YuvBuffSize*10))
                s64ReadFilePos[s32VifChn][s32ChnPort]=0;
	    }
	    *pNumBuf = 1;
	    #endif
	}
    return MHAL_SUCCESS;
}
MS_S32 MHal_VIF_DequeueFrameBuffer(MHal_VIF_CHN s32VifChn, MHal_VIF_PORT s32ChnPort, MHal_VIF_RingBufElm_t *ptFbInfo)
{
	MHAL_VIF_ChnContext_t *pstChnCtx;

	if(s32VifChn <16)
	{
		pstChnCtx = pgstChnCtx + s32VifChn;
		*ptFbInfo = pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].stRingbuffer;
        pstChnCtx->stPortCtx[s32ChnPort].stRingBufInfo[0].bUsed = FALSE;
	}
	else
	    return HT_FAILURE;
    return MHAL_SUCCESS;
}

