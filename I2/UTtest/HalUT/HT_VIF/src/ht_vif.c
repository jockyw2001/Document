#include <linux/completion.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/semaphore.h>

#include "ht_vif.h"
#include "mhal_common.h"
#include "mhal_vif.h"
#include "ht_common.h"

#define HT_VIF_MAX_DEV_NUM 4
#define HT_VIF_MAX_WAY_NUM_PER_DEV 4
#define HT_VIF_MAX_PHYCHN_NUM (HT_VIF_MAX_DEV_NUM*HT_VIF_MAX_WAY_NUM_PER_DEV)
#define HT_VIF_MAX_CHN_OUTPORT 2

#define HT_VIF_FILENAME_LENTH 55
#define HT_VIF_MAX_BUFFERHOLD 1
#define HT_VIF_PORT0 0
#define HT_VIF_PORT1 1

#define SIMULATOR_ENV 1

#ifndef HT_WRITE_DDR
#define HT_VIF_FILE_FRAMES_PER_CHN 10
#else
#define HT_VIF_FILE_FRAMES_PER_CHN 2
#endif

typedef enum
{
    E_HT_VIF_MODE_BT656,
    E_HT_VIF_MODE_DIGITAL_CAMERA,
    E_HT_VIF_MODE_BT1120_STANDARD,
    E_HT_VIF_MODE_BT1120_INTERLEAVED,
    E_HT_VIF_MODE_MAX
}HT_VIF_IntfMode_e;
typedef enum
{
    E_HT_VIF_WORK_MODE_1MULTIPLEX,
    E_HT_VIF_WORK_MODE_2MULTIPLEX,
    E_HT_VIF_WORK_MODE_4MULTIPLEX,
    E_HT_VIF_WORK_MODE_MAX
}HT_VIF_WorkMode_e;
typedef enum
{
    E_HT_VIF_CLK_EDGE_SINGLE_UP,
    E_HT_VIF_CLK_EDGE_SINGLE_DOWN,
    E_HT_VIF_CLK_EDGE_DOUBLE,
    E_HT_VIF_CLK_EDGE_MAX
}HT_VIF_ClkEdge_e;
typedef enum
{
    /*The input sequence of the second component(only contains u and v) in BT.1120 mode */
    E_HT_VIF_INPUT_DATA_VUVU = 0,
    E_HT_VIF_INPUT_DATA_UVUV,
    /* The input sequence for yuv */
    E_HT_VIF_INPUT_DATA_UYVY = 0,
    E_HT_VIF_INPUT_DATA_VYUY,
    E_HT_VIF_INPUT_DATA_YUYV,
    E_HT_VIF_INPUT_DATA_YVYU,
    E_HT_VIF_INPUT_DATA_MAX
}HT_VIF_DataYuvSeq_e;
typedef enum
{
    E_HT_VIF_VSYNC_FIELD,
    E_HT_VIF_VSYNC_PULSE,
    E_HT_VIF_VSYNC_MAX
}HT_VIF_Vsync_e;

typedef enum
{
    E_HT_VIF_VSYNC_NEG_HIGH,
    E_HT_VIF_VSYNC_NEG_LOW,
    E_HT_VIF_VSYNC_NEG_MAX
}HT_VIF_VsyncNeg_e;

typedef enum
{
    E_HT_VIF_HSYNC_VALID_SINGNAL,
    E_HT_VIF_HSYNC_PULSE,
    E_HT_VIF_HSYNC_MAX
}HT_VIF_Hsync_e;

typedef enum
{
    E_HT_VIF_HSYNC_NEG_HIGH,
    E_HT_VIF_HSYNC_NEG_LOW,
    E_HT_VIF_HSYNC_NEG_MAX
}HT_VIF_HsyncNeg_e;

typedef enum
{
    E_HT_VIF_VSYNC_NORM_PULSE,
    E_HT_VIF_VSYNC_VALID_SINGAL,
    E_HT_VIF_VSYNC_VALID_MAX
}HT_VIF_VsyncValid_e;

typedef enum
{
    E_HT_VIF_VSYNC_VALID_NEG_HIGH,
    E_HT_VIF_VSYNC_VALID_NEG_LOW,
    E_HT_VIF_VSYNC_VALID_NEG_MAX
}HT_VIF_VsyncValidNeg_e;

typedef enum
{
    E_HT_VI_DATA_TYPE_YUV,
    E_HT_VI_DATA_TYPE_RGB,
    E_HT_VI_DATA_TYPE_MAX
}HT_VIF_DataType_e;

typedef enum
{
    E_HT_VIF_CAPSEL_TOP,
    E_HT_VIF_CAPSEL_BOTTOM,
    E_HT_VIF_CAPSEL_BOTH,
    E_HT_VIF_CAPSEL_MAX
}HT_VIF_Capsel_e;

typedef enum
{
    E_HT_VIF_PIXEL_FORMAT_RGB_1BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_2BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_4BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_8BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_444,

    E_HT_VIF_PIXEL_FORMAT_RGB_4444,
    E_HT_VIF_PIXEL_FORMAT_RGB_555,
    E_HT_VIF_PIXEL_FORMAT_RGB_565,
    E_HT_VIF_PIXEL_FORMAT_RGB_1555,
    /*  9 reserved */
    E_HT_VIF_PIXEL_FORMAT_RGB_888,
    E_HT_VIF_PIXEL_FORMAT_RGB_8888,
    E_HT_VIF_PIXEL_FORMAT_RGB_PLANAR_888,
    E_HT_VIF_PIXEL_FORMAT_RGB_BAYER_8BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_BAYER_10BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_BAYER_12BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_BAYER_14BPP,
    E_HT_VIF_PIXEL_FORMAT_RGB_BAYER,         /* 16 bpp */

    E_HT_VIF_PIXEL_FORMAT_YUV_A422,
    E_HT_VIF_PIXEL_FORMAT_YUV_A444,
    E_HT_VIF_PIXEL_FORMAT_YUV_PLANAR_422,
    E_HT_VIF_PIXEL_FORMAT_YUV_PLANAR_420,
    E_HT_VIF_PIXEL_FORMAT_YUV_PLANAR_444,

    E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420,
    E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_444,

    E_HT_VIF_PIXEL_FORMAT_UYVY_PACKAGE_422,
    E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422,
    E_HT_VIF_PIXEL_FORMAT_VYUY_PACKAGE_422,
    E_HT_VIF_PIXEL_FORMAT_YCbCr_PLANAR,

    E_HT_VIF_PIXEL_FORMAT_YUV_400,
    E_HT_VIF_PIXEL_FORMAT_MAX
}HT_VIF_PixelFormat_e;

typedef enum
{
    E_HT_VIF_SCAN_INTERLACED,   /*隔行*/
    E_HT_VIF_SCAN_PROGRESSIVE,  /*逐行*/
    E_HT_VIF_SCAN_MAX
}HT_VIF_ScanMode_e;

typedef enum
{
    E_HT_VIF_FRAMERATE_FULL,
    E_HT_VIF_FRAMERATE_HALF,
    E_HT_VIF_FRAMERATE_QUARTER,
    E_HT_VIF_FRAMERATE_OCTANT,
    E_HT_VIF_FRAMERATE_THREE_QUARTERS,
    E_HT_VIF_FRAMERATE_MAX
}HT_VIF_FrameRate_e;

typedef struct HT_Size_s
{
    HT_U32 u32Width;
    HT_U32 u32Height;
}HT_Size_t;
typedef struct HT_Rect_s
{
    HT_U32 u32X;//4
    HT_U32 u32Y;//4
    HT_U32 u32Width;//4
    HT_U32 u32Height;//4
}HT_Rect_t;

typedef struct HT_VIF_TimingBlank_s
{
    HT_U32 u32HsyncHfb;
    HT_U32 u32HsyncAct;
    HT_U32 u32HsyncHbb;
    HT_U32 u32VsyncVfb;
    HT_U32 u32VsyncVact;
    HT_U32 u32VsyncVbb;
    HT_U32 u32VsyncVbfb;
    HT_U32 u32VsyncVbact;
    HT_U32 u32VsyncVbbb;
}HT_VIF_TimingBlank_t;

typedef struct HT_VIF_SyncCfg_s
{
    HT_VIF_Vsync_e  eVsync;
    HT_VIF_VsyncNeg_e   eVsyncNeg;
    HT_VIF_Hsync_e  eHsync;
    HT_VIF_HsyncNeg_e   eHsyncNeg;
    HT_VIF_VsyncValid_e  eVsyncValid;
    HT_VIF_VsyncValidNeg_e eVsyncValidNeg;
    HT_VIF_TimingBlank_t  stTimingBlank;

}HT_VIF_SyncCfg_t;

typedef struct HT_VIF_ChnPortAttr_s
{
    HT_Rect_t              stCapRect;
    HT_Size_t              stDestSize;
    HT_VIF_Capsel_e        eCapSel;
    HT_VIF_ScanMode_e      eScanMode;
    HT_VIF_PixelFormat_e   ePixFormat;
    HT_BOOL                bMirror;
    HT_BOOL                bFlip;
    HT_VIF_FrameRate_e     eFrameRate;

} HT_VIF_ChnPortAttr_t;

typedef struct HT_VIF_FileInfo_s
{
    HT_U8 au8FilePath[HT_VIF_FILENAME_LENTH];
    HT_U8 u8FrameCount;
    HT_S64 s64FilePos;
    struct list_head list;

}HT_VIF_FileInfo_t;

typedef struct HT_VIF_PortContext_s
{
    HT_VIF_ChnPortAttr_t stVifChnPortAttr;
    HT_U32 u32HalBufferHold;
    HT_BOOL bEnable;
    HT_VIF_FileInfo_t stOutputFileInfo;
    struct semaphore stBuffQueueMutex;
    struct list_head stOutputBufQueue;

}HT_VIF_PortContext_t;

typedef struct HT_VIF_ChnContext_s
{
    HT_VIF_PortContext_t stPortCtx[HT_VIF_MAX_CHN_OUTPORT];

}HT_VIF_ChnContext_t;

typedef struct HT_VIF_DevAttr_s
{
    HT_VIF_IntfMode_e       eIntfMode;
    HT_VIF_WorkMode_e       eWorkMode;
    HT_U32                  au32CompMask[2];
    HT_VIF_ClkEdge_e        eClkEdge;
    HT_S32                  as32AdChnId[4];
    /* DC模式时以下必配，其它模式时无效*/
    HT_VIF_DataYuvSeq_e     eDataSeq;
    HT_VIF_SyncCfg_t        stSynCfg;
    HT_BOOL                 bDataRev;
} HT_VIF_DevAttr_t;

typedef struct HT_VIF_DevContext_s
{
    HT_VIF_DevAttr_t stVifDevAttr;
    HT_BOOL bEnable;
}HT_VIF_DevContext_t;

typedef struct HT_VIF_TestValue_s
{
    HT_S32 s32DevId;
    HT_S32 s32ChnId;
    HT_S32 s32PortId;
    HT_U32 u32Width;
    HT_U32 u32Height;
    HT_VIF_PixelFormat_e ePixFormat;

}HT_VIF_TestValue_t;

typedef HT_S32 HT_VIF_DEV;
typedef HT_S32 HT_VIF_CHN;
typedef HT_S32 HT_VIF_PORT;

static HT_VIF_DevContext_t gastVifDevCtx[HT_VIF_MAX_DEV_NUM];
static HT_VIF_ChnContext_t gastVifChnCtx[HT_VIF_MAX_PHYCHN_NUM];
static HT_VIF_TestValue_t gastTestValue[HT_VIF_MAX_PHYCHN_NUM];

#ifdef HT_WRITE_DDR
static HT_VB_Info_t gastVbInfo[HT_VIF_MAX_PHYCHN_NUM][HT_VIF_MAX_CHN_OUTPORT];
#endif

static struct timer_list gstVifTimer;
static struct completion gstVifWorkTaskWakeup;
static struct completion gstVifJobFinishWakeup;
static struct task_struct *gpstVifWorkTask=NULL;
static struct task_struct *gpstVifProcessWriteFile=NULL;

HT_U8 gu8RealTestChnNum=0;

static HT_RESULT _HT_VIF_Check_WriteFileFinish(void)
{
    HT_RESULT ret=HT_FAILURE;
    HT_U8 u8ChnIdx=0;
    HT_U8 u8PortIdx=0;
    HT_U8 u8WriteFinishChnCnt=0;
    HT_VIF_PortContext_t *pstPortCtx;
    for(u8ChnIdx=0;u8ChnIdx<HT_VIF_MAX_PHYCHN_NUM;u8ChnIdx++)
    {
        for(u8PortIdx=0;u8PortIdx<HT_VIF_MAX_CHN_OUTPORT;u8PortIdx++)
        {
            pstPortCtx=&gastVifChnCtx[u8ChnIdx].stPortCtx[u8PortIdx];
            if(pstPortCtx->bEnable==FALSE)
                continue;
            if(pstPortCtx->stOutputFileInfo.u8FrameCount==HT_VIF_FILE_FRAMES_PER_CHN)
            {
                u8WriteFinishChnCnt++;
            }
        }
    }
    if(u8WriteFinishChnCnt==gu8RealTestChnNum)
    {
        ret=HT_SUCCESS;
    }
    return ret;
}

static HT_RESULT _HT_VIF_Process_In(void)
{
    HT_RESULT_e Ret=HT_FAILURE;
    HT_U8 u8ChnIndex=0;
    HT_U8 u8PortIndex=0;
    HT_U32 u32Width=0,u32Height=0;
    HT_VIF_PortContext_t *pstPortCtx;
    HT_YUVInfo_t stYUVvbInfo;
    MHal_VIF_RingBufElm_t stFbInfo;

    for(u8ChnIndex=0;u8ChnIndex<HT_VIF_MAX_PHYCHN_NUM;u8ChnIndex++)
    {
        for(u8PortIndex=0;u8PortIndex<HT_VIF_MAX_CHN_OUTPORT;u8PortIndex++)
        {
            pstPortCtx=&gastVifChnCtx[u8ChnIndex].stPortCtx[u8PortIndex];
            if(pstPortCtx->bEnable==FALSE)
                continue;
            stFbInfo.nCropW=pstPortCtx->stVifChnPortAttr.stCapRect.u32Width;
            stFbInfo.nCropH=pstPortCtx->stVifChnPortAttr.stCapRect.u32Height;
            stFbInfo.nCropX=pstPortCtx->stVifChnPortAttr.stCapRect.u32X;
            stFbInfo.nCropY=pstPortCtx->stVifChnPortAttr.stCapRect.u32Y;
            if(pstPortCtx->stVifChnPortAttr.ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            {
                stFbInfo.u32Stride[0]=pstPortCtx->stVifChnPortAttr.stDestSize.u32Width;
                stFbInfo.u32Stride[1]=pstPortCtx->stVifChnPortAttr.stDestSize.u32Width;
                u32Width = pstPortCtx->stVifChnPortAttr.stDestSize.u32Width;
                u32Height = pstPortCtx->stVifChnPortAttr.stDestSize.u32Height;
                stYUVvbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                Ret=HT_MallocYUVOneFrame(&stYUVvbInfo,u32Width,u32Height);
                if(Ret!=HT_SUCCESS)
                {
                    printk("Malloc yuv420 buffer FAILURE\r\n");
                    return HT_FAILURE;
                }
                stFbInfo.u64PhyAddr[0]=stYUVvbInfo.stYUV420SP.stVbInfo_y.phyAddr;
                stFbInfo.u64PhyAddr[1]=stYUVvbInfo.stYUV420SP.stVbInfo_uv.phyAddr;
                ExecFunc(MHal_VIF_QueueFrameBuffer(u8ChnIndex,u8PortIndex,&stFbInfo),MHAL_SUCCESS);
            }
            else if(pstPortCtx->stVifChnPortAttr.ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
            {
                stFbInfo.u32Stride[0]=pstPortCtx->stVifChnPortAttr.stDestSize.u32Width*2;
                u32Width = pstPortCtx->stVifChnPortAttr.stDestSize.u32Width;
                u32Height = pstPortCtx->stVifChnPortAttr.stDestSize.u32Height;
                stYUVvbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV422_YUYV;
                Ret=HT_MallocYUVOneFrame(&stYUVvbInfo,u32Width,u32Height);
                if(Ret!=HT_SUCCESS)
                {
                    printk("Malloc Port1 yuv420 buffer FAILURE\r\n");
                    return HT_FAILURE;
                }
                stFbInfo.u64PhyAddr[0]=stYUVvbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr;
                ExecFunc(MHal_VIF_QueueFrameBuffer(u8ChnIndex,u8PortIndex,&stFbInfo),MHAL_SUCCESS);
            }
        }
    }
    return HT_SUCCESS;
}

static HT_RESULT _HT_VIF_Process_Out(void)
{
    HT_U8 u8ChnIndex=0;
    HT_U8 u8PortIndex=0;
    HT_U32 u32BufferCount=0;
    HT_OutputBufList_t *pstOutputBufInfo;
    MHal_VIF_RingBufElm_t stFbInfo;
    HT_VIF_PortContext_t *pstPortCtx;

    for(u8ChnIndex=0;u8ChnIndex<HT_VIF_MAX_PHYCHN_NUM;u8ChnIndex++)
    {
        for(u8PortIndex=0;u8PortIndex<HT_VIF_MAX_CHN_OUTPORT;u8PortIndex++)
        {
            pstPortCtx=&gastVifChnCtx[u8ChnIndex].stPortCtx[u8PortIndex];
            if(pstPortCtx->bEnable==FALSE)
                continue;
            MHal_VIF_QueryFrames(u8ChnIndex,u8PortIndex,&u32BufferCount);
            while(u32BufferCount)
            {
                u32BufferCount--;
                pstOutputBufInfo=(HT_OutputBufList_t*)kmalloc(sizeof(HT_OutputBufList_t),GFP_KERNEL);

                ExecFunc(MHal_VIF_DequeueFrameBuffer(u8ChnIndex,u8PortIndex,&stFbInfo),MHAL_SUCCESS);

                pstOutputBufInfo->u8Chnidx=u8ChnIndex;
                pstOutputBufInfo->u8Portidx=u8PortIndex;
                pstOutputBufInfo->Width=stFbInfo.nCropW;
                pstOutputBufInfo->Height=stFbInfo.nCropH;
                pstOutputBufInfo->eModuleID=E_HT_STREAM_MODULEID_VIF;
                pstOutputBufInfo->bWriteFileFlag=0;

                if(pstPortCtx->stVifChnPortAttr.ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
                {
                    pstOutputBufInfo->stYuvVbInfo.ePixelFormat=E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_y.phyAddr=stFbInfo.u64PhyAddr[0];
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr=HT_FindVirAddr(stFbInfo.u64PhyAddr[0]);
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_y.u32Size=stFbInfo.nCropH*stFbInfo.nCropW;
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_uv.phyAddr=stFbInfo.u64PhyAddr[1];
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr=HT_FindVirAddr(stFbInfo.u64PhyAddr[1]);
                    pstOutputBufInfo->stYuvVbInfo.stYUV420SP.stVbInfo_uv.u32Size=stFbInfo.nCropH*stFbInfo.nCropW/2;
                }
                else if(pstPortCtx->stVifChnPortAttr.ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
                {
                    pstOutputBufInfo->stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV422_YUYV;
                    pstOutputBufInfo->stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr=stFbInfo.u64PhyAddr[0];
                    pstOutputBufInfo->stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr=HT_FindVirAddr(stFbInfo.u64PhyAddr[0]);
                    pstOutputBufInfo->stYuvVbInfo.stYUV422YUYV.stVbInfo_yuv.u32Size=stFbInfo.nCropH*stFbInfo.nCropW*2;
                }
                down(&pstPortCtx->stBuffQueueMutex);
                list_add_tail(&pstOutputBufInfo->list,&pstPortCtx->stOutputBufQueue);
                up(&pstPortCtx->stBuffQueueMutex);
            }
        }
    }
    return HT_SUCCESS;
}

static int _HT_VIF_WorkTask_Thread(void *data)
{
    HT_U8 u8IsrCnt;
    while((u8IsrCnt++)<HT_VIF_FILE_FRAMES_PER_CHN)
    {
        _HT_VIF_Process_In();
        _HT_VIF_Process_Out();
        wait_for_completion(&gstVifWorkTaskWakeup);
    }
    return HT_SUCCESS;
}

static int _HT_VIF_WriteFile_Thread(void* data)
{
    HT_U8 u8ChnIndex=0;
    HT_U8 u8PortIndex=0;
    HT_U8 u8BufIdx=0;
    HT_S32 s32BufCnt=0;
    HT_VIF_PortContext_t *pstPortCtx;
    HT_YUVInfo_t stTmpBufInfo[HT_VIF_FILE_FRAMES_PER_CHN];
#ifdef HT_WRITE_DDR
    HT_U32 u32BuffSize=0;
    HT_U32 u32BuffOffset=0;
    HT_U8 *pu8SrcVirtAddr;
    HT_U8 *pu8DstVirtAddr;
#endif

    while(!kthread_should_stop())
    {
        for(u8ChnIndex=0;u8ChnIndex<HT_VIF_MAX_PHYCHN_NUM;u8ChnIndex++)
        {
            for(u8PortIndex=0;u8PortIndex<HT_VIF_MAX_CHN_OUTPORT;u8PortIndex++)
            {
                pstPortCtx=&gastVifChnCtx[u8ChnIndex].stPortCtx[u8PortIndex];
                if(pstPortCtx->bEnable==FALSE)
                    continue;
                s32BufCnt=0;
                down(&pstPortCtx->stBuffQueueMutex);
                if(!list_empty(&pstPortCtx->stOutputBufQueue))
                {
                    struct list_head *stPos;
                    struct list_head *n;
                    HT_OutputBufList_t *pstOutputBufInfo;
                    list_for_each_safe(stPos,n,&pstPortCtx->stOutputBufQueue)
                    {
                        pstOutputBufInfo=list_entry(stPos,HT_OutputBufList_t,list);
                        stTmpBufInfo[s32BufCnt++]=pstOutputBufInfo->stYuvVbInfo;
                        list_del(stPos);
                        kfree(pstOutputBufInfo);
                    }
                }
                up(&pstPortCtx->stBuffQueueMutex);

                for(u8BufIdx=0;u8BufIdx<s32BufCnt;u8BufIdx++)
                {
                    HT_WriteYUVOneFrameToFile(pstPortCtx->stOutputFileInfo.au8FilePath,&stTmpBufInfo[u8BufIdx],&pstPortCtx->stOutputFileInfo.s64FilePos);
                    pstPortCtx->stOutputFileInfo.u8FrameCount++;
#ifdef HT_WRITE_DDR
                    pu8DstVirtAddr=gastVbInfo[u8ChnIndex][u8PortIndex].pu8VirtAddr;
                    if(stTmpBufInfo[u8BufIdx].ePixelFormat==E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
                    {
                        pu8SrcVirtAddr=stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_y.pu8VirtAddr;
                        u32BuffSize=stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_y.u32Size;
                        u32BuffOffset=(stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_y.u32Size+stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_uv.u32Size)*(pstPortCtx->stOutputFileInfo.u8FrameCount-1);
                        memcpy(pu8DstVirtAddr+u32BuffOffset,pu8SrcVirtAddr,u32BuffSize);
                        u32BuffOffset+=u32BuffSize;
                        pu8SrcVirtAddr=stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_uv.pu8VirtAddr;
                        u32BuffSize=stTmpBufInfo[u8BufIdx].stYUV420SP.stVbInfo_uv.u32Size;
                        memcpy(pu8DstVirtAddr+u32BuffOffset,pu8SrcVirtAddr,u32BuffSize);
                    }
                    else if(stTmpBufInfo[u8BufIdx].ePixelFormat==E_HT_PIXEL_FORMAT_YUV422_YUYV)
                    {
                        pu8SrcVirtAddr=stTmpBufInfo[u8BufIdx].stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr;
                        u32BuffSize=stTmpBufInfo[u8BufIdx].stYUV422YUYV.stVbInfo_yuv.u32Size;
                        u32BuffOffset=stTmpBufInfo[u8BufIdx].stYUV422YUYV.stVbInfo_yuv.u32Size*(pstPortCtx->stOutputFileInfo.u8FrameCount-1);
                        memcpy(pu8DstVirtAddr+u32BuffOffset,pu8SrcVirtAddr,u32BuffSize);
                    }
#endif
                    HT_FreeYUVOneFrame(&stTmpBufInfo[u8BufIdx]);

                }
            }
        }
        if(_HT_VIF_Check_WriteFileFinish()==HT_SUCCESS)
        {
            complete(&gstVifJobFinishWakeup);
            return HT_SUCCESS;
        }
    }
    return HT_SUCCESS;
}

#if (SIMULATOR_ENV==0)
static irqreturn_t _HT_VIF_8051_ISR(int irq, void *data)
{
    complete(&gstVifWorkTaskWakeup);
    return IRQ_HANDLED;
}
#else
static void _HT_VIF_Timer_Func(HT_VIRT ulArg)
{
    complete(&gstVifWorkTaskWakeup);
    mod_timer(&gstVifTimer,jiffies+HZ/100);//set 10ms timeout
}
#endif

static HT_RESULT _HT_VIF_SetDevAttr(HT_VIF_TestValue_t* pastTestValue)
{
    HT_U8 i;
    HT_VIF_DEV s32VifDev;
    HT_VIF_DevAttr_t stVifDevAttr;
    MHal_VIF_DevCfg_t stVifDevCfg;
    for(i=0;i<gu8RealTestChnNum;i++)
    {
        s32VifDev=pastTestValue[i].s32DevId;
        stVifDevAttr.eIntfMode=E_HT_VIF_MODE_BT1120_STANDARD;
        stVifDevAttr.eWorkMode=E_HT_VIF_WORK_MODE_1MULTIPLEX;
        stVifDevAttr.au32CompMask[0]=0xFF000000;
        stVifDevAttr.au32CompMask[1]=0xFF0000;
        stVifDevAttr.eClkEdge=E_HT_VIF_CLK_EDGE_SINGLE_UP;
        stVifDevAttr.as32AdChnId[0]=-1;
        stVifDevAttr.as32AdChnId[1]=-1;
        stVifDevAttr.as32AdChnId[2]=-1;
        stVifDevAttr.as32AdChnId[3]=-1;

        stVifDevCfg.eIntfMode=stVifDevAttr.eIntfMode;
        stVifDevCfg.eWorkMode=stVifDevAttr.eWorkMode;
        stVifDevCfg.eClkEdge=stVifDevAttr.eClkEdge;
        memcpy(stVifDevCfg.au32CompMask,stVifDevAttr.au32CompMask,sizeof(stVifDevCfg.au32CompMask));
        memcpy(stVifDevCfg.as32AdChnId,stVifDevAttr.as32AdChnId,sizeof(stVifDevCfg.as32AdChnId));
        ExecFunc(MHal_VIF_DevSetConfig(s32VifDev,&stVifDevCfg),MHAL_SUCCESS);
    	ExecFunc(MHal_VIF_DevEnable(s32VifDev),MHAL_SUCCESS);

    	memcpy(&gastVifDevCtx[s32VifDev].stVifDevAttr,&stVifDevAttr,sizeof(HT_VIF_DevAttr_t));
    	gastVifDevCtx[s32VifDev].bEnable=TRUE;
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_SetChnPortAttr(HT_VIF_TestValue_t* pastTestValue)
{
    HT_U8 i;
    HT_VIF_CHN s32VifChn;
    HT_VIF_PORT s32VifPort;
    HT_U32 u32Width;
    HT_U32 u32Height;
    MHal_VIF_PixelFormat_e ePixFormat;
    MHal_VIF_ChnCfg_t stVifChnCfg;
    MHal_VIF_SubChnCfg_t stVifSubChnCfg;
    HT_VIF_PortContext_t stPortCtx;

    for(i=0;i<gu8RealTestChnNum;i++)
    {
        s32VifChn=pastTestValue[i].s32ChnId;
        s32VifPort=pastTestValue[i].s32PortId;
        u32Height=pastTestValue[i].u32Height;
        u32Width=pastTestValue[i].u32Width;
        ePixFormat=pastTestValue[i].ePixFormat;

        stPortCtx.stVifChnPortAttr.eCapSel=E_HT_VIF_CAPSEL_BOTH;
        stPortCtx.stVifChnPortAttr.eFrameRate=E_HT_VIF_FRAMERATE_FULL;
        stPortCtx.stVifChnPortAttr.ePixFormat=ePixFormat;
        stPortCtx.stVifChnPortAttr.eScanMode=E_HT_VIF_SCAN_PROGRESSIVE;
        stPortCtx.stVifChnPortAttr.stCapRect.u32X=0;
        stPortCtx.stVifChnPortAttr.stCapRect.u32Y=0;
        stPortCtx.stVifChnPortAttr.stCapRect.u32Width=u32Width;
        stPortCtx.stVifChnPortAttr.stCapRect.u32Height=u32Height;
        stPortCtx.stVifChnPortAttr.stDestSize.u32Width=u32Width;
        stPortCtx.stVifChnPortAttr.stDestSize.u32Height=u32Height;
        stPortCtx.bEnable=TRUE;
        if(s32VifPort==0)
        {
            stVifChnCfg.eCapSel=stPortCtx.stVifChnPortAttr.eCapSel;
            stVifChnCfg.eFrameRate=stPortCtx.stVifChnPortAttr.eFrameRate;
            stVifChnCfg.ePixFormat=stPortCtx.stVifChnPortAttr.ePixFormat;
            stVifChnCfg.eScanMode=stPortCtx.stVifChnPortAttr.eScanMode;
            stVifChnCfg.stCapRect.u32X=stPortCtx.stVifChnPortAttr.stCapRect.u32X;
            stVifChnCfg.stCapRect.u32Y=stPortCtx.stVifChnPortAttr.stCapRect.u32Y;
            stVifChnCfg.stCapRect.u32Width=stPortCtx.stVifChnPortAttr.stCapRect.u32Width;
            stVifChnCfg.stCapRect.u32Height=stPortCtx.stVifChnPortAttr.stCapRect.u32Height;
            ExecFunc(MHal_VIF_ChnSetConfig(s32VifChn,&stVifChnCfg),MHAL_SUCCESS);
            ExecFunc(MHal_VIF_ChnEnable(s32VifChn),MHAL_SUCCESS);
        }
        else if(s32VifPort==1)
        {
            stVifSubChnCfg.eFrameRate=stPortCtx.stVifChnPortAttr.eFrameRate;
            ExecFunc(MHal_VIF_SubChnSetConfig(s32VifChn,&stVifSubChnCfg),MHAL_SUCCESS);
            ExecFunc(MHal_VIF_SubChnEnable(s32VifChn),MHAL_SUCCESS);
        }
        sema_init(&gastVifChnCtx[s32VifChn].stPortCtx[s32VifPort].stBuffQueueMutex,1);
        INIT_LIST_HEAD(&gastVifChnCtx[s32VifChn].stPortCtx[s32VifPort].stOutputBufQueue);
        memcpy(&gastVifChnCtx[s32VifChn].stPortCtx[s32VifPort],&stPortCtx,sizeof(HT_VIF_PortContext_t));
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_DevDisable(HT_VIF_TestValue_t* pastTestValue)
{
    HT_U8 i;
    HT_VIF_DEV s32VifDev;
    for(i=0;i<gu8RealTestChnNum;i++)
    {
        s32VifDev=pastTestValue[i].s32DevId;
        ExecFunc(MHal_VIF_DevDisable(s32VifDev),MHAL_SUCCESS);
        gastVifDevCtx[s32VifDev].bEnable=FALSE;
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_ChnDisable(HT_VIF_TestValue_t* pastTestValue)
{
    HT_U8 i;
    HT_VIF_CHN s32VifChn;
    HT_VIF_PORT s32VifPort;
    for(i=0;i<gu8RealTestChnNum;i++)
    {
        s32VifChn=pastTestValue[i].s32ChnId;
        s32VifPort=pastTestValue[i].s32PortId;
        switch(s32VifPort)
        {
            case HT_VIF_PORT0:
                ExecFunc(MHal_VIF_ChnDisable(s32VifChn),MHAL_SUCCESS);break;
            case HT_VIF_PORT1:
                ExecFunc(MHal_VIF_SubChnDisable(s32VifChn),MHAL_SUCCESS);break;
            default:
                break;
        }
        gastVifChnCtx[s32VifChn].stPortCtx[s32VifPort].bEnable=FALSE;
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_Init(HT_VIF_TestValue_t* pastTestValue)
{
    HT_RESULT_e Ret=HT_FAILURE;
    ExecFunc(MHal_VIF_Init(),MHAL_SUCCESS);
    Ret=_HT_VIF_SetDevAttr(pastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF SET DEV FAILURE\r\n");
        return HT_FAILURE;
    }
    Ret=_HT_VIF_SetChnPortAttr(pastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF SET CHN PORT FAILURE\r\n");
        return HT_FAILURE;
    }
    return HT_SUCCESS;
}

static HT_RESULT _HT_VIF_DeInit(HT_VIF_TestValue_t* pastTestValue)
{
    HT_RESULT_e Ret=HT_FAILURE;
    Ret=_HT_VIF_DevDisable(pastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF DISABLE DEV FAILURE\r\n");
        return HT_FAILURE;
    }
    Ret=_HT_VIF_ChnDisable(pastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF DISABLE CHN FAILURE\r\n");
        return HT_FAILURE;
    }
    ExecFunc(MHal_VIF_Deinit(),MHAL_SUCCESS);
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_GetDevId(HT_S32 s32ChnId,HT_VIF_TestValue_t *pstTestValue)
{
    if(s32ChnId>=0&&s32ChnId<=3)
        pstTestValue->s32DevId=0;
    else if(s32ChnId>=4&&s32ChnId<=7)
        pstTestValue->s32DevId=1;
    else if(s32ChnId>=8&&s32ChnId<=11)
        pstTestValue->s32DevId=2;
    else if(s32ChnId>=12&&s32ChnId<=15)
        pstTestValue->s32DevId=3;
    else
        return HT_FAILURE;
    return HT_SUCCESS;
}
static void _HT_VIF_Creat_OutputFile(HT_S32 s32ChnId,HT_S32 s32PortId,HT_VIF_PixelFormat_e ePixFormat)
{
    HT_U8 au8FilePath[HT_VIF_FILENAME_LENTH];
    HT_VIF_PortContext_t *pstPortCtx=&gastVifChnCtx[s32ChnId].stPortCtx[s32PortId];

    if(ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        sprintf(au8FilePath,"/mnt/YUVFILE/outputfile/HT_VIF_YUV420SP_chn%d_port%d.yuv",s32ChnId,s32PortId);
    }
    else if(ePixFormat==E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
    {
        sprintf(au8FilePath,"/mnt/YUVFILE/outputfile/HT_VIF_YUV422_chn%d_port%d.yuv",s32ChnId,s32PortId);
    }
    memcpy(pstPortCtx->stOutputFileInfo.au8FilePath,au8FilePath,sizeof(au8FilePath));

    printk("creat chn:%d port:%d file success\n",s32ChnId,s32PortId);
}

static HT_RESULT _HT_VIF_ParseStrings(HT_U16 *pau16CmdValue,HT_U8 u8CmdCnt,HT_VIF_TestValue_t *pastTestValue,HT_U8 *pu8RealTestChnNum)
{
    HT_U8 i;
    HT_RESULT_e Ret=HT_SUCCESS;
    HT_U8 u8RealTestChnNum;
    if(u8CmdCnt%5!=0||u8CmdCnt==0)
    {
        printk("Parse Counts is Invalid\r\n");
        HT_VIF_DisplayHelp();
        return HT_FAILURE;
    }
    u8RealTestChnNum=u8CmdCnt/5;
    *pu8RealTestChnNum=u8RealTestChnNum;
    for(i=0;i<u8RealTestChnNum;i++)
    {
        pastTestValue[i].s32ChnId=pau16CmdValue[i*5+0];
        pastTestValue[i].s32PortId=pau16CmdValue[i*5+1];
        if(pau16CmdValue[i*5+2]==0)
            pastTestValue[i].ePixFormat=E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        else if(pau16CmdValue[i*5+2]==1)
            pastTestValue[i].ePixFormat=E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422;
        pastTestValue[i].u32Width=pau16CmdValue[i*5+3];
        pastTestValue[i].u32Height=pau16CmdValue[i*5+4];
        Ret=_HT_VIF_GetDevId(pastTestValue[i].s32ChnId,&(pastTestValue[i]));
        if(Ret!=HT_SUCCESS)
        {
            printk("ChnId is Invalid\r\n");
            return HT_FAILURE;
        }
		printk("group%d devid:%d\r\n",i,pastTestValue[i].s32DevId);
	    printk("group%d chnid:%d\r\n",i,pastTestValue[i].s32ChnId);
	    printk("group%d portid:%d\r\n",i,pastTestValue[i].s32PortId);
	    printk("group%d pixformat:%d \r\n",i,pastTestValue[i].ePixFormat);
	    printk("group%d width:%d\r\n",i,pastTestValue[i].u32Width);
	    printk("group%d height:%d\r\n",i,pastTestValue[i].u32Height);
	    _HT_VIF_Creat_OutputFile(pastTestValue[i].s32ChnId,pastTestValue[i].s32PortId,pastTestValue[i].ePixFormat);
    }
    return Ret;
}
#ifdef HT_WRITE_DDR
static HT_RESULT _HT_VIF_Malloc_WriteDDRBuff(HT_VB_Info_t astVbInfo[][HT_VIF_MAX_CHN_OUTPORT])
{
    HT_U8 u8ChnIdx=0;
    HT_U8 u8PortIdx=0;
    HT_U32 u32Width=0;
    HT_U32 u32Height=0;
    HT_VIF_PixelFormat_e ePixFmt;
    HT_VIF_PortContext_t *pstPortCtx;
    for(u8ChnIdx=0;u8ChnIdx<HT_VIF_MAX_PHYCHN_NUM;u8ChnIdx++)
    {
        for(u8PortIdx=0;u8PortIdx<HT_VIF_MAX_CHN_OUTPORT;u8PortIdx++)
        {
            pstPortCtx=&gastVifChnCtx[u8ChnIdx].stPortCtx[u8PortIdx];
            if(pstPortCtx->bEnable==FALSE)
                continue;
            ePixFmt=pstPortCtx->stVifChnPortAttr.ePixFormat;
            u32Width=pstPortCtx->stVifChnPortAttr.stDestSize.u32Width;
            u32Height=pstPortCtx->stVifChnPortAttr.stDestSize.u32Height;
            if(ePixFmt==E_HT_VIF_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            {
                astVbInfo[u8ChnIdx][u8PortIdx].u32Size=u32Width*u32Height*15/10*HT_VIF_FILE_FRAMES_PER_CHN;
                HT_Malloc(&astVbInfo[u8ChnIdx][u8PortIdx]);
            }
            else if(ePixFmt==E_HT_VIF_PIXEL_FORMAT_YUYV_PACKAGE_422)
            {
                astVbInfo[u8ChnIdx][u8PortIdx].u32Size=u32Width*u32Height*2*HT_VIF_FILE_FRAMES_PER_CHN;
                HT_Malloc(&astVbInfo[u8ChnIdx][u8PortIdx]);
            }
        }
    }
    printk("### Malloc Write DDR Buff Success ###\n");
    return HT_SUCCESS;
}
static HT_RESULT _HT_VIF_Free_WriteDDRBuff(HT_VB_Info_t astVbInfo[][HT_VIF_MAX_CHN_OUTPORT])
{
    HT_U8 u8ChnIdx=0;
    HT_U8 u8PortIdx=0;
    for(u8ChnIdx=0;u8ChnIdx<HT_VIF_MAX_PHYCHN_NUM;u8ChnIdx++)
    {
        for(u8PortIdx=0;u8PortIdx<HT_VIF_MAX_CHN_OUTPORT;u8PortIdx++)
        {
            if(astVbInfo[u8ChnIdx][u8PortIdx].pu8VirtAddr!=NULL)
            {
                HT_Free(astVbInfo[u8ChnIdx][u8PortIdx].phyAddr);
            }
        }
    }
    return HT_SUCCESS;
}
#endif

static void _HT_VIF_InitVar(void)
{
#ifdef HT_WRITE_DDR
    _HT_VIF_Free_WriteDDRBuff(gastVbInfo);
    memset(gastVbInfo,0,sizeof(gastVbInfo));
#endif
    gu8RealTestChnNum=0;
    memset(gastVifDevCtx,0,sizeof(gastVifDevCtx));
    memset(gastVifChnCtx,0,sizeof(gastVifChnCtx));
    memset(gastTestValue,0,sizeof(gastTestValue));
    init_completion(&gstVifWorkTaskWakeup);
    init_completion(&gstVifJobFinishWakeup);
}
void HT_VIF_DisplayHelp(void)
{
    printk("param 1: ChnId(0-15)    \r\n");
    printk("param 2: PortId(0-1)    \r\n");
    printk("param 3: Width   \r\n");
    printk("param 4: Height   \r\n");
    printk("param 5: PixFormat (0->E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420 1->E_HT_PIXEL_FORMAT_YUV422_YUYV)");
}



HT_RESULT HT_VIF(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_RESULT_e Ret=HT_FAILURE;
    CHECK_NULL_POINTER(__FUNCTION__,pau16CmdValue);
    _HT_VIF_InitVar();
    Ret=_HT_VIF_ParseStrings(pau16CmdValue,u8CmdCnt,gastTestValue,&gu8RealTestChnNum);
    if(Ret!=HT_SUCCESS)
    {
        printk("Parse is Invalid\r\n");
        return HT_FAILURE;
    }
    Ret=_HT_VIF_Init(gastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF INIT FAILURE\r\n");
        return HT_FAILURE;
    }

#if (SIMULATOR_ENV==1)
    init_timer(&gstVifTimer);
    gstVifTimer.function=_HT_VIF_Timer_Func;
    add_timer(&gstVifTimer);
    mod_timer(&gstVifTimer,jiffies+HZ);//set 1s timeout
#else
    request_irq(VIF_8051_ISR_IDX,_HT_VIF_8051_ISR,NULL); //register ISR
#endif

#ifdef HT_WRITE_DDR
    _HT_VIF_Malloc_WriteDDRBuff(gastVbInfo);
#endif

    gpstVifWorkTask=kthread_run(_HT_VIF_WorkTask_Thread,NULL,"ht_vif_worktask");
    gpstVifProcessWriteFile=kthread_run(_HT_VIF_WriteFile_Thread,NULL,"ht_vif_process_writefile");

    wait_for_completion(&gstVifJobFinishWakeup);
    del_timer(&gstVifTimer);
    Ret=_HT_VIF_DeInit(gastTestValue);
    if(Ret!=HT_SUCCESS)
    {
        printk("VIF DeInit FAILURE\r\n");
        return HT_FAILURE;
    }
    return HT_SUCCESS;
}


