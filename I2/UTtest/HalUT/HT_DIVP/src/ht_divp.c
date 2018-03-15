#include <linux/string.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/slab.h>

//#include "mhal_common.h"
#include "mhal_divp_datatype.h"
#include "mhal_cmdq.h"

#include "mhal_divp.h"

#include "ht_common_datatype.h"
#include "ht_common.h"
#include "ht_divp.h"

#define HT_DIVP_MAX_CHN 16
#define HT_DIVP_MAX_CASE 4
#define HT_DIVP_PARAMCNT 7

#ifndef HT_WRITE_DDR
#define HT_DIVP_FRAMECNT 10
#else
#define HT_DIVP_FRAMECNT 3
#endif

#define DIVP_TASK_DUMMY_REG (0x123CF0)
#define DIVP_DEV0_ISR_IDX (0x01)//SC_INT_DIPW

#define HT_DIVP_YUV422_YUYV_1920x1088 "/mnt/YUVFILE/YUV422_YUYV1920_1088.yuv"
#define HT_DIVP_YUV422_YUYV_1280x720  "/mnt/YUVFILE/YUV422_YUYV1280_720.yuv"
#define HT_DIVP_YUV422_YUYV_720x480   "/mnt/YUVFILE/YUV422_YUYV720_480.yuv"
#define HT_DIVP_YUV422_YUYV_640x480   "/mnt/YUVFILE/YUV422_YUYV640_480.yuv"
#define HT_DIVP_YUV420_SP_1920x1088   "/mnt/YUVFILE/YUV420SP_1920_1088.yuv"
#define HT_DIVP_YUV420_SP_1280x720    "/mnt/YUVFILE/YUV420SP_1280_720.yuv"
#define HT_DIVP_YUV420_SP_720x480     "/mnt/YUVFILE/YUV420SP_720_480.yuv"
#define HT_DIVP_YUV420_SP_640x480     "/mnt/YUVFILE/YUV420SP_640_480.yuv"

#define SIZE_1920_1088 3
#define SIZE_1280_720  2
#define SIZE_720_480   1
#define SIZE_640_480   0

#define HT_DIVP_TIMING_WIDTH_4K (3840)
#define HT_DIVP_TIMING_HEIGHT_2K (2160)

extern struct list_head gstOutputBufHead;//outputbuf list

typedef struct HT_size_s{
    HT_U16 Width;
    HT_U16 Height;
}HT_size_t;

typedef struct HT_TnrDiRot_Param_s
{
    MHAL_DIVP_TnrLevel_e eDIVPTnrAttr;
    MHAL_DIVP_DiType_e eDIVPDiAttr;
    MHAL_DIVP_Rotate_e eDIVPRotAttr;
}HT_TnrDiRot_Param_t;

typedef struct HT_DIVP_InPutbuffer_s{
    HT_YUVInfo_t stBuffer;
    HT_Stream_ModuleID eModuleID;
}HT_DIVP_InPutbuffer_t;

typedef struct HT_DIVP_ChnAttr_s
{
    void* pHalCtx;
    HT_BOOL bCreate;
    HT_S64 s64ReadPos;
    HT_S64 s64WritePos;
    HT_BOOL bCapFlag;
    char *pFileName;
    HT_U16 ChnId;
    HT_TnrDiRot_Param_t stTnrDiRot_param;
    MHAL_DIVP_Mirror_t stDIVPMirrorAttr;
    MHAL_DIVP_Window_t stDIVPWinAttr;
    HT_size_t stInPutSize;
    HT_DIVP_InPutbuffer_t stVbInputInfo;
    HT_size_t stOutPutSize;
    HT_VB_Info_t stOutWriteDDR;
    HT_VB_Info_t stCapWriteDDR;
    HT_PxlFmt_e ePixelFormat;
}HT_DIVP_ChnAttr_t;

HT_TnrDiRot_Param_t stTNR_DI_Roate_Param[HT_DIVP_MAX_CASE] =
{
    {E_MHAL_DIVP_TNR_LEVEL_OFF, E_MHAL_DIVP_DI_TYPE_OFF, E_MHAL_DIVP_ROTATE_NONE},
    {E_MHAL_DIVP_TNR_LEVEL_LOW, E_MHAL_DIVP_DI_TYPE_2D, E_MHAL_DIVP_ROTATE_90},
    {E_MHAL_DIVP_TNR_LEVEL_MIDDLE, E_MHAL_DIVP_DI_TYPE_3D, E_MHAL_DIVP_ROTATE_180},
    {E_MHAL_DIVP_TNR_LEVEL_HIGH, E_MHAL_DIVP_DI_TYPE_NUM, E_MHAL_DIVP_ROTATE_270}
};

MHAL_DIVP_Mirror_t stMirror_Param[HT_DIVP_MAX_CASE] =
{
    {FALSE,FALSE},
    {FALSE,TRUE},
    {TRUE,FALSE},
    {FALSE,FALSE}
};

MHAL_DIVP_Window_t stWinparam[4]=
{
    {0,0,640,480},
    {0,0,720,480},
    {0,0,1280,720},
    {0,0,1920,1088},
};

HT_size_t stInPutSize[HT_DIVP_MAX_CASE]=
{
    {640,480},
    {720,480},
    {1280,720},
    {1920,1088},
};

HT_size_t stOutPutVdecSize[HT_DIVP_MAX_CASE]=
{
    {720,576},
    {1280,720},
    {1920,1088},
    {2048,1520},
};


HT_size_t stOutPutSize[HT_DIVP_MAX_CASE]=
{
    {640,480},
    {720,480},
    {1280,720},
    {1920,1088},
};


HT_RESULT HT_DIVP_Alloc(HT_U8 *u8MMAHeapName, HT_U32 u32Size , HT_U64 *PhyAddr)
{
    HT_VB_Info_t stVbInfo;
    stVbInfo.u32Size = u32Size;

    ExecFunc(HT_Malloc(&stVbInfo),0);

    *PhyAddr = stVbInfo.phyAddr;

    return HT_SUCCESS;
}

HT_RESULT HT_DIVP_Free(HT_U64 phyAddr)
{
    HT_Free(phyAddr);

    return HT_SUCCESS;
}

static HT_RESULT _HT_DIVP_InputConfig(HT_DIVP_ChnAttr_t *pstChnInfo, HT_YUVInfo_t *pstVbInputInfo, MHAL_DIVP_InputInfo_t *pstDIVPInputInfo)
{
    if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        pstDIVPInputInfo->ePxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV;
        pstDIVPInputInfo->u64BufAddr[0] = pstVbInputInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr;
        pstDIVPInputInfo->u16Stride[0] = pstChnInfo->stInPutSize.Width*2;
        pstDIVPInputInfo->u32BufSize = pstVbInputInfo->stYUV422YUYV.stVbInfo_yuv.u32Size;
    }
    else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        pstDIVPInputInfo->ePxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        pstDIVPInputInfo->u64BufAddr[0] = pstVbInputInfo->stYUV420SP.stVbInfo_y.phyAddr;
        pstDIVPInputInfo->u64BufAddr[1] = pstVbInputInfo->stYUV420SP.stVbInfo_uv.phyAddr;
        pstDIVPInputInfo->u16Stride[0] = pstChnInfo->stInPutSize.Width;
        pstDIVPInputInfo->u16Stride[1] = pstChnInfo->stInPutSize.Width;
        pstDIVPInputInfo->u32BufSize = pstVbInputInfo->stYUV420SP.stVbInfo_y.u32Size + pstVbInputInfo->stYUV420SP.stVbInfo_uv.u32Size;
    }

    pstDIVPInputInfo->u16InputHeight = pstChnInfo->stInPutSize.Height;
    pstDIVPInputInfo->u16InputWidth = pstChnInfo->stInPutSize.Width;
   // pstDIVPInputInfo->eFieldType = E_MHAL_DIVP_FIELD_TYPE_NONE;
    pstDIVPInputInfo->eScanMode = E_MHAL_DIVP_SCAN_MODE_MAX;
    pstDIVPInputInfo->eTileMode = E_MHAL_DIVP_TILE_MODE_NONE;
    pstDIVPInputInfo->u64Pts = 0;

    //stDIVPInputInfo->tMfdecInfo=   decoder none

    if(pstChnInfo->stTnrDiRot_param.eDIVPDiAttr == E_MHAL_DIVP_DI_TYPE_OFF)
        pstDIVPInputInfo->stDiSettings.eDiMode = E_MHAL_DIVP_DI_MODE_BOB;
        //stDIVPInputInfo->tDiSettings.bTopFirst
    else if(pstChnInfo->stTnrDiRot_param.eDIVPDiAttr == E_MHAL_DIVP_DI_TYPE_2D)//2.5D
        pstDIVPInputInfo->stDiSettings.eDiMode = E_MHAL_DIVP_DI_MODE_EODI;

    else if(pstChnInfo->stTnrDiRot_param.eDIVPDiAttr == E_MHAL_DIVP_DI_TYPE_3D)//3D
        pstDIVPInputInfo->stDiSettings.eDiMode = E_MHAL_DIVP_DI_MODE_WAVE;

    return HT_SUCCESS;

}

static HT_RESULT _HT_DIVP_OutputConfig(HT_DIVP_ChnAttr_t *pstChnInfo, HT_YUVInfo_t *pstVbOutputInfo, MHAL_DIVP_OutPutInfo_t *stDIVPOutputInfo)
{
    if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        stDIVPOutputInfo->ePxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV;
        stDIVPOutputInfo->u64BufAddr[0] = pstVbOutputInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr;
        stDIVPOutputInfo->u16Stride[0] = pstChnInfo->stOutPutSize.Width*2;
        stDIVPOutputInfo->u32BufSize = pstVbOutputInfo->stYUV422YUYV.stVbInfo_yuv.u32Size;
    }
    else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        stDIVPOutputInfo->ePxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stDIVPOutputInfo->u64BufAddr[0] = pstVbOutputInfo->stYUV420SP.stVbInfo_y.phyAddr;
        stDIVPOutputInfo->u64BufAddr[1] = pstVbOutputInfo->stYUV420SP.stVbInfo_uv.phyAddr;
        stDIVPOutputInfo->u16Stride[0] = pstChnInfo->stOutPutSize.Width;
        stDIVPOutputInfo->u16Stride[1] = pstChnInfo->stOutPutSize.Width;
        stDIVPOutputInfo->u32BufSize = pstVbOutputInfo->stYUV420SP.stVbInfo_y.u32Size + pstVbOutputInfo->stYUV420SP.stVbInfo_uv.u32Size;
    }

    stDIVPOutputInfo->u16OutputHeight = pstChnInfo->stOutPutSize.Height;
    stDIVPOutputInfo->u16OutputWidth = pstChnInfo->stOutPutSize.Width;
    //hal set input output cap config function?????

    return HT_SUCCESS;
}

static HT_RESULT _HT_DIVP_InputCapConfig(HT_DIVP_ChnAttr_t *pstChnInfo, HT_YUVInfo_t *pstVbCapInputInfo, MHAL_DIVP_CaptureInfo_t *stDIVPCapInfo)
{
     if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        stDIVPCapInfo->eInputPxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV;
        stDIVPCapInfo->eOutputPxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV422_YUYV;
        stDIVPCapInfo->u64BufAddr[0] = pstVbCapInputInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr;
        stDIVPCapInfo->u16Stride[0] = pstChnInfo->stInPutSize.Width*2;
        stDIVPCapInfo->u32BufSize = pstVbCapInputInfo->stYUV422YUYV.stVbInfo_yuv.u32Size;
    }
     else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        stDIVPCapInfo->eInputPxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stDIVPCapInfo->eOutputPxlFmt = E_MHAL_DIVP_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stDIVPCapInfo->u64BufAddr[0] = pstVbCapInputInfo->stYUV420SP.stVbInfo_y.phyAddr;
        stDIVPCapInfo->u64BufAddr[1] = pstVbCapInputInfo->stYUV420SP.stVbInfo_uv.phyAddr;
        stDIVPCapInfo->u16Stride[0] = pstChnInfo->stInPutSize.Width;
        stDIVPCapInfo->u16Stride[1] = pstChnInfo->stInPutSize.Width;
        stDIVPCapInfo->u32BufSize = pstVbCapInputInfo->stYUV420SP.stVbInfo_y.u32Size + pstVbCapInputInfo->stYUV420SP.stVbInfo_uv.u32Size;
    }

    stDIVPCapInfo->u16Height = pstChnInfo->stInPutSize.Height;//same with input
    stDIVPCapInfo->u16Width = pstChnInfo->stInPutSize.Width;

    stDIVPCapInfo->eDispId = E_MHAL_DIVP_Display0;
    stDIVPCapInfo->eCapStage = E_MHAL_DIVP_CAP_STAGE_INPUT;
    stDIVPCapInfo->eRotate = E_MHAL_DIVP_ROTATE_NONE;//snap need close rotate
    stDIVPCapInfo->stMirror.bHMirror = false;
    stDIVPCapInfo->stMirror.bVMirror = false;
    stDIVPCapInfo->stCropWin.u16Height = pstChnInfo->stInPutSize.Height;
    stDIVPCapInfo->stCropWin.u16Width = pstChnInfo->stInPutSize.Width;;
    stDIVPCapInfo->stCropWin.u16X = 0;
    stDIVPCapInfo->stCropWin.u16Y = 0;

    return HT_SUCCESS;
}

/*
irqreturn_t HT_DIVP_Isr(int nIRQ_ID, void *data)
{
    ///clean ISR
    MHalDivpCleanFrameDoneIsr();
   // wake_up(&divp_isr_waitqueue);
    return IRQ_HANDLED;
}*/

HT_RESULT _HT_DIVP_Init(void)
{
    MHAL_DIVP_DeviceId_e eDevId;
    eDevId = E_MHAL_DIVP_Device0;

    ExecFunc(MHAL_DIVP_Init(eDevId),1);
    //request_irq(DIVP_DEV0_ISR_IDX, HT_DIVP_Isr, IRQF_SHARED, "HT_DIVP_Isr", NULL);
    ExecFunc(MHAL_DIVP_EnableFrameDoneIsr(true),1);

    return HT_SUCCESS;
}

HT_RESULT _HT_DIVP_DeInit(void)
{
    MHAL_DIVP_DeviceId_e eDevId;
    eDevId = E_MHAL_DIVP_Device0;
    ExecFunc(MHAL_DIVP_EnableFrameDoneIsr(false),1);
    //free_irq(DIVP_DEV0_ISR_IDX, HT_DIVP_Isr);
    ExecFunc(MHAL_DIVP_DeInit(eDevId),1);
	return HT_SUCCESS;
}


static HT_RESULT _HT_DIVP_SetChnAttr(HT_DIVP_ChnAttr_t *pstChnInfo, MHAL_CMDQ_CmdqInterface_t *pCmdInf)
{
    ExecFunc(MHAL_DIVP_SetAttr(pstChnInfo->pHalCtx, E_MHAL_DIVP_ATTR_TNR, &pstChnInfo->stTnrDiRot_param.eDIVPTnrAttr, pCmdInf),1);
    ExecFunc(MHAL_DIVP_SetAttr(pstChnInfo->pHalCtx, E_MHAL_DIVP_ATTR_DI, &pstChnInfo->stTnrDiRot_param.eDIVPDiAttr, pCmdInf),1);
    ExecFunc(MHAL_DIVP_SetAttr(pstChnInfo->pHalCtx, E_MHAL_DIVP_ATTR_ROTATE, &pstChnInfo->stTnrDiRot_param.eDIVPRotAttr, pCmdInf),1);
    ExecFunc(MHAL_DIVP_SetAttr(pstChnInfo->pHalCtx, E_MHAL_DIVP_ATTR_MIRROR, &pstChnInfo->stDIVPMirrorAttr, pCmdInf),1);
    ExecFunc(MHAL_DIVP_SetAttr(pstChnInfo->pHalCtx, E_MHAL_DIVP_ATTR_CROP, &pstChnInfo->stDIVPWinAttr, pCmdInf),1);

    return HT_SUCCESS;
}

// cap stage in
static HT_RESULT _HT_DIVP_HandleEachFrame(HT_U32 u32FrameId, HT_DIVP_ChnAttr_t *pstChnInfo)
{
    HT_YUVInfo_t stVbOutputInfo;
    HT_YUVInfo_t stVbCapInputInfo;

    MHAL_DIVP_InputInfo_t stDIVPInputInfo;
    MHAL_DIVP_OutPutInfo_t stDIVPOutputInfo;
    MHAL_DIVP_CaptureInfo_t stDIVPInputCapInfo;

    MHAL_CMDQ_CmdqInterface_t *pCmdInf;
    MHAL_DIVP_DeviceId_e eDevId = E_MHAL_DIVP_Device0;

    HT_U32 u32Width = pstChnInfo->stOutPutSize.Width;
    HT_U32 u32Height = pstChnInfo->stOutPutSize.Height;

    HT_U8 u8ChnId=pstChnInfo->ChnId;

    MHAL_CMDQ_BufDescript_t cmdq_buff;
    cmdq_buff.u32CmdqBufSize = 1024;
    cmdq_buff.u32CmdqBufSizeAlign = 0;
    cmdq_buff.u32MloadBufSize = 1024;
    cmdq_buff.u16MloadBufSizeAlign = 0;

    printk("\r\n[CHANNEL ID %d Frame ID %d] begin \r\n",u8ChnId,u32FrameId);

    pCmdInf = MHAL_CMDQ_GetSysCmdqService(E_MHAL_CMDQ_ID_DIVP, &cmdq_buff, false);

    if(pstChnInfo->bCreate == false)
    {
        ExecFunc(MHAL_DIVP_CreateInstance(eDevId,HT_DIVP_TIMING_WIDTH_4K,HT_DIVP_TIMING_HEIGHT_2K, HT_DIVP_Alloc, HT_DIVP_Free, pstChnInfo->ChnId, &pstChnInfo->pHalCtx),1);
        pstChnInfo->bCreate = true;
        printk("[CHANNEL ID %d Frame ID %d] Create Channel done phalctx addr is %p \r\n",u8ChnId,u32FrameId,pstChnInfo->pHalCtx);
    }

    ExecFunc(_HT_DIVP_SetChnAttr(pstChnInfo,pCmdInf),HT_SUCCESS);

    stVbOutputInfo.ePixelFormat = pstChnInfo->ePixelFormat;
    ExecFunc(HT_MallocYUVOneFrame(&stVbOutputInfo, u32Width, u32Height),HT_SUCCESS);

    ExecFunc(_HT_DIVP_InputConfig(pstChnInfo, &pstChnInfo->stVbInputInfo.stBuffer, &stDIVPInputInfo),HT_SUCCESS);

    ExecFunc(_HT_DIVP_OutputConfig(pstChnInfo, &stVbOutputInfo, &stDIVPOutputInfo),HT_SUCCESS);

    ExecFunc(MHAL_DIVP_ProcessDramData(pstChnInfo->pHalCtx, &stDIVPInputInfo, &stDIVPOutputInfo, pCmdInf), 1);

    if(pstChnInfo->bCapFlag == TRUE)
    {
        char pfilenamesnap[40]="";
        HT_U32 u32Width = pstChnInfo->stInPutSize.Width;
        HT_U32 u32Height = pstChnInfo->stInPutSize.Height;
        if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            sprintf(pfilenamesnap,"HT_DIVPSnapChn%d_frame%d_YUV420SP.yuv",u8ChnId,u32FrameId);
        else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
            sprintf(pfilenamesnap,"HT_DIVPSnapChn%d_frame%d_YUYV.yuv",u8ChnId,u32FrameId);

        printk("[CHANNEL ID %d Frame ID %d] Begin Cap one Frame\r\n",u8ChnId,u32FrameId);

        stVbCapInputInfo.ePixelFormat = pstChnInfo->ePixelFormat;
        ExecFunc(HT_MallocYUVOneFrame(&stVbCapInputInfo, u32Width, u32Height), HT_SUCCESS);

        ExecFunc(_HT_DIVP_InputCapConfig(pstChnInfo, &stVbCapInputInfo, &stDIVPInputCapInfo), HT_SUCCESS);

        ExecFunc(MHAL_DIVP_CaptureTiming(pstChnInfo->pHalCtx, &stDIVPInputCapInfo, pCmdInf), 1);

        #ifndef HT_WRITE_DDR
        {
            HT_S64 s64WritePos=0;
            ExecFunc(HT_WriteYUVOneFrameToFile(pfilenamesnap, &stVbCapInputInfo, &s64WritePos), HT_SUCCESS);
        }
        #else
        {
            HT_U32 u32CapDDRsize = 0;
            if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
            {
                u32CapDDRsize = pstChnInfo->stInPutSize.Height* pstChnInfo->stInPutSize.Width *2;
                pstChnInfo->stCapWriteDDR.u32Size = u32CapDDRsize;
                HT_Malloc(&pstChnInfo->stCapWriteDDR);
                memcpy(pstChnInfo->stCapWriteDDR.pu8VirtAddr, stVbCapInputInfo.stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, stVbCapInputInfo.stYUV422YUYV.stVbInfo_yuv.u32Size);
            }
            else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            {
                HT_S64 s64WritePos =0;
                u32CapDDRsize = pstChnInfo->stInPutSize.Height* pstChnInfo->stInPutSize.Width *3/2;
                pstChnInfo->stCapWriteDDR.u32Size = u32CapDDRsize;
                HT_Malloc(&pstChnInfo->stCapWriteDDR);
                memcpy(pstChnInfo->stCapWriteDDR.pu8VirtAddr, stVbCapInputInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr, stVbCapInputInfo.stYUV420SP.stVbInfo_y.u32Size);
                s64WritePos += stVbCapInputInfo.stYUV420SP.stVbInfo_y.u32Size;
                memcpy(pstChnInfo->stCapWriteDDR.pu8VirtAddr + s64WritePos, stVbCapInputInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr, stVbCapInputInfo.stYUV420SP.stVbInfo_uv.u32Size);
            }
        }
        #endif
        ExecFunc(HT_FreeYUVOneFrame(&stVbCapInputInfo), HT_SUCCESS);
    }

    {
        #ifndef HT_WRITE_DDR
        char pfilename[25]="";
        if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
         sprintf(pfilename, "HT_DIVPChn%dYUV420SP.yuv", pstChnInfo->ChnId);
        else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
         sprintf(pfilename, "HT_DIVPChn%dYUYV.yuv", pstChnInfo->ChnId);
        ExecFunc(HT_WriteYUVOneFrameToFile(pfilename, &stVbOutputInfo, &pstChnInfo->s64WritePos), HT_SUCCESS);

        #else
        if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
        {
            memcpy(pstChnInfo->stOutWriteDDR.pu8VirtAddr + pstChnInfo->s64WritePos, stVbOutputInfo.stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, stVbOutputInfo.stYUV422YUYV.stVbInfo_yuv.u32Size);
            pstChnInfo->s64WritePos += stVbOutputInfo.stYUV422YUYV.stVbInfo_yuv.u32Size;
        }
        else if(pstChnInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
        {
            memcpy(pstChnInfo->stOutWriteDDR.pu8VirtAddr + pstChnInfo->s64WritePos, stVbOutputInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr, stVbOutputInfo.stYUV420SP.stVbInfo_y.u32Size);
            pstChnInfo->s64WritePos += stVbOutputInfo.stYUV420SP.stVbInfo_y.u32Size;

            memcpy(pstChnInfo->stOutWriteDDR.pu8VirtAddr + pstChnInfo->s64WritePos, stVbOutputInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr, stVbOutputInfo.stYUV420SP.stVbInfo_uv.u32Size);
            pstChnInfo->s64WritePos += stVbOutputInfo.stYUV420SP.stVbInfo_uv.u32Size;
        }
        #endif
    }

    ExecFunc(HT_FreeYUVOneFrame(&stVbOutputInfo), HT_SUCCESS);
    ExecFunc(HT_FreeYUVOneFrame(&pstChnInfo->stVbInputInfo.stBuffer), HT_SUCCESS);

	return HT_SUCCESS;
}

HT_RESULT HT_DIVP_SetUserConfig(const HT_U16 *pu16Cmd, const HT_U8 u8CmdCnt, HT_DIVP_ChnAttr_t *pstChnInfo)
{
    HT_U16 i=0,j=0;
    HT_U16 u8ChnID=0;
    HT_U8 CaseId=0;
    HT_OutputBufList_t *pstInputlist;
    HT_U8 ListInit = FALSE;
    HT_U8 u8ChnCnt=u8CmdCnt/HT_DIVP_PARAMCNT;

    for(i=0; i < u8CmdCnt; i+=HT_DIVP_PARAMCNT)
    {
        pstChnInfo[u8ChnID].ChnId=pu16Cmd[i];
        for(j=u8ChnID; j>0; j--)
        {
            if(pstChnInfo[u8ChnID].ChnId == pstChnInfo[j-1].ChnId )
            {
                printk("%d and %d channel ID same\r\n",u8ChnID,j);
                return HT_FAILURE;
            }
        }
        CaseId = pu16Cmd[i+1];
        pstChnInfo[u8ChnID].stTnrDiRot_param=stTNR_DI_Roate_Param[CaseId];

        CaseId = pu16Cmd[i+2];
        pstChnInfo[u8ChnID].stDIVPMirrorAttr=stMirror_Param[CaseId];

        CaseId = pu16Cmd[i+3];
        pstChnInfo[u8ChnID].stDIVPWinAttr=stWinparam[CaseId];

        CaseId = pu16Cmd[i+4];
        pstChnInfo[u8ChnID].stInPutSize = stInPutSize[CaseId];
        pstChnInfo[u8ChnID].ePixelFormat = pu16Cmd[i+6];
        if(CaseId <4)
        {
            if(ListInit == FALSE)
            {
                INIT_LIST_HEAD(&gstOutputBufHead);
                ListInit =TRUE;
            }

            if(E_HT_PIXEL_FORMAT_YUV422_YUYV == pstChnInfo[u8ChnID].ePixelFormat)
            {
                switch(CaseId)
                {
                    case SIZE_1920_1088:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV422_YUYV_1920x1088;
                        break;
                    case SIZE_1280_720:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV422_YUYV_1280x720;
                        break;
                    case SIZE_720_480:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV422_YUYV_720x480;
                        break;
                    case SIZE_640_480:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV422_YUYV_640x480;
                        break;
                    default:
                    {
                        printk("no file for your input size\r\n");
                        printk("input size 3 is 1920*1080\r\n");
                        printk("input size 2 is 1280*720\r\n");
                        printk("input size 1 is 720*480\r\n");
                        printk("input size 0 is 640*480\r\n");
                        return HT_FAILURE;
                    }
                }
            }
            else if(E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420 ==  pstChnInfo[u8ChnID].ePixelFormat)
            {
                switch(CaseId)
               {
                    case SIZE_1920_1088:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV420_SP_1920x1088;
                        break;
                    case SIZE_1280_720:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV420_SP_1280x720;
                        break;
                    case SIZE_720_480:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV420_SP_720x480;
                        break;
                    case SIZE_640_480:
                        pstChnInfo[u8ChnID].pFileName = HT_DIVP_YUV420_SP_640x480;
                        break;
                    default:
                    {
                        printk("no file for your input size\r\n");
                        printk("input size 3 is 1920*1080\r\n");
                        printk("input size 2 is 1280*720\r\n");
                        printk("input size 1 is 720*480\r\n");
                        printk("input size 0 is 640*480\r\n");
                        return HT_FAILURE;
                    }
               }
            }
            else
            {
                printk("pixel fomat set error \r\n");
                return HT_FAILURE;
            }

            CaseId = pu16Cmd[i+5];
            pstChnInfo[u8ChnID].stOutPutSize = stOutPutSize[CaseId];
        }
        else
        {
            CaseId = pu16Cmd[i+5];
            pstChnInfo[u8ChnID].stOutPutSize = stOutPutVdecSize[CaseId];
        }

        printk("[CHANNEL %d]width %d ,height %d\r\n",  pstChnInfo[u8ChnID].ChnId, pstChnInfo[u8ChnID].stInPutSize.Width,  pstChnInfo[u8ChnID].stInPutSize.Height);

        if(u8ChnID <= u8ChnCnt)
            u8ChnID++;
        else
        {
            printk("HT_DIVP_SetUserConfig param num is error\r\n");
            return HT_FAILURE;
        }
    }

    if(ListInit == TRUE)
    {
        for(j=0; j<HT_DIVP_FRAMECNT ;j++)
        {
            for(i=0;i<u8ChnCnt;i++)
            {
                pstInputlist=(HT_OutputBufList_t *)kmalloc(sizeof(HT_OutputBufList_t),GFP_KERNEL);
                pstInputlist->eModuleID = E_HT_STREAM_MODULEID_NULL;
                pstInputlist->stYuvVbInfo.ePixelFormat = pstChnInfo[i].ePixelFormat;
                pstInputlist->Width = pstChnInfo[i].stInPutSize.Width;
                pstInputlist->Height = pstChnInfo[i].stInPutSize.Height;
                pstInputlist->u8Chnidx = pstChnInfo[i].ChnId;
                list_add_tail(&(pstInputlist->list),&(gstOutputBufHead));
            }
        }
    }
    return HT_SUCCESS;
}

static HT_DIVP_ChnAttr_t *pstChnInfo;

HT_RESULT HT_DIVP(HT_U16 *pu16Cmd,HT_U8 u8CmdCnt)
{
    HT_U32 u32FrameId = 0;
    HT_U8  u8ChnId = 0;
    HT_OutputBufList_t *pos,*posN;

    #ifndef HT_WRITE_DDR
    HT_U32 u16CapFramId[HT_DIVP_MAX_CHN]={5,2,6,8,3,2,4,5,9,0,1};//every channel cap frame ID
    #else
    HT_U32 u16CapFramId[HT_DIVP_MAX_CHN]={0,1,2,0,1,2,0,1,2,0,1};
    #endif

    HT_U8  u8ChnCnt_Work = u8CmdCnt/HT_DIVP_PARAMCNT;

    if(u8ChnCnt_Work > HT_DIVP_MAX_CHN || 0 != u8CmdCnt%HT_DIVP_PARAMCNT)
    {
        printk("channel work %d cmdcnt %d\r\n",u8ChnCnt_Work,u8CmdCnt);
        return HT_FAILURE;
    }

    if(pstChnInfo != NULL)
    {
        printk("channel addr %p, port 0 phyaddr %llx\r\n", pstChnInfo, pstChnInfo[0].stOutWriteDDR.phyAddr);
        for(u8ChnId =0; u8ChnId < HT_DIVP_MAX_CHN; u8ChnId++)
        {
            HT_Free(pstChnInfo[u8ChnId].stOutWriteDDR.phyAddr);
            HT_Free(pstChnInfo[u8ChnId].stCapWriteDDR.phyAddr);
        }
        kfree(pstChnInfo);
    }

    pstChnInfo = (HT_DIVP_ChnAttr_t *)kmalloc(sizeof(HT_DIVP_ChnAttr_t)*u8ChnCnt_Work,GFP_KERNEL);
    memset(pstChnInfo, 0, sizeof(HT_DIVP_ChnAttr_t)*u8ChnCnt_Work);

    ExecFunc(HT_DIVP_SetUserConfig(pu16Cmd, u8CmdCnt, pstChnInfo), HT_SUCCESS);
    ExecFunc(_HT_DIVP_Init(),HT_SUCCESS);

    #ifdef HT_WRITE_DDR
    {
        HT_U32 u32OutDDRsize = 0;
        for(u8ChnId =0; u8ChnId<u8ChnCnt_Work; u8ChnId++)
        {
            if(pstChnInfo[u8ChnId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
            {
                u32OutDDRsize = pstChnInfo[u8ChnId].stOutPutSize.Height * pstChnInfo[u8ChnId].stOutPutSize.Width *2;
            }
            else if(pstChnInfo[u8ChnId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            {
                u32OutDDRsize = pstChnInfo[u8ChnId].stOutPutSize.Height * pstChnInfo[u8ChnId].stOutPutSize.Width *3/2;
            }
            pstChnInfo[u8ChnId].stOutWriteDDR.u32Size = u32OutDDRsize * HT_DIVP_FRAMECNT;

            HT_Malloc(&pstChnInfo[u8ChnId].stOutWriteDDR);
        }
    }
    #endif

    {
        u8ChnId = 0;
        list_for_each_entry_safe(pos,posN,&gstOutputBufHead, list)
        {
            if(pos ==NULL)
                return HT_FAILURE;

            if(u32FrameId == u16CapFramId[pos->u8Chnidx])
                pstChnInfo[u8ChnId].bCapFlag = TRUE;
            else
                pstChnInfo[u8ChnId].bCapFlag = FALSE;

            pstChnInfo[u8ChnId].ChnId = pos->u8Chnidx;
            pstChnInfo[u8ChnId].stVbInputInfo.eModuleID = pos->eModuleID;
            pstChnInfo[u8ChnId].stInPutSize.Height = pos->Height;
            pstChnInfo[u8ChnId].stInPutSize.Width = pos->Width;
            pstChnInfo[u8ChnId].ePixelFormat = pos->stYuvVbInfo.ePixelFormat;

            if(pstChnInfo[u8ChnId].stVbInputInfo.eModuleID == E_HT_STREAM_MODULEID_NULL)
            {
                HT_U32 u32Width = pstChnInfo[u8ChnId].stInPutSize.Width;
                HT_U32 u32Height = pstChnInfo[u8ChnId].stInPutSize.Height;
                printk("[CHANNEL %d]width %d ,height %d\r\n",  pstChnInfo[u8ChnId].ChnId, pstChnInfo[u8ChnId].stInPutSize.Width,  pstChnInfo[u8ChnId].stInPutSize.Height);
                pstChnInfo[u8ChnId].stVbInputInfo.stBuffer.ePixelFormat =  pos->stYuvVbInfo.ePixelFormat;
                ExecFunc(HT_MallocYUVOneFrame(&pstChnInfo[u8ChnId].stVbInputInfo.stBuffer, u32Width, u32Height),HT_SUCCESS);
                ExecFunc(HT_ReadYUVOneFrameToBuffer(pstChnInfo[u8ChnId].pFileName, &pstChnInfo[u8ChnId].stVbInputInfo.stBuffer, &pstChnInfo[u8ChnId].s64ReadPos),HT_SUCCESS);
            }
            else if(pstChnInfo[u8ChnId].stVbInputInfo.eModuleID == E_HT_STREAM_MODULEID_VDEC)
               pstChnInfo[u8ChnId].stVbInputInfo.stBuffer = pos->stYuvVbInfo;
            else
            {
                printk("input buffer module id %d is error \r\n",pstChnInfo[u8ChnId].stVbInputInfo.eModuleID);
                return HT_FAILURE;
            }

            ExecFunc(_HT_DIVP_HandleEachFrame(u32FrameId, &pstChnInfo[u8ChnId]), HT_SUCCESS);
            list_del(&pos->list);
            kfree(pos);
            u8ChnId ++;
            if(u8ChnId >u8ChnCnt_Work -1)
            {
                u8ChnId =0;
                u32FrameId ++;
            }
        }
    }

    for(u8ChnId = 0; u8ChnId < u8ChnCnt_Work; u8ChnId++)
    {
        #ifdef HT_WRITE_DDR

        #if 1
        char pfilename[25]="";
        char pcapfilename[30]="";
        if( pstChnInfo[u8ChnId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
        {
            sprintf(pfilename, "HT_DIVPChn%dYUYV.yuv", u8ChnId);
            sprintf(pcapfilename, "HT_DIVPCapChn%dYUYV.yuv", u8ChnId);
        }
        else if( pstChnInfo[u8ChnId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
        {
            sprintf(pfilename, "HT_DIVPChn%d420SP.yuv", u8ChnId);
            sprintf(pcapfilename, "HT_DIVPCapChn%d420SP.yuv", u8ChnId);
        }
        HT_WriteFile(pfilename, pstChnInfo[u8ChnId].stOutWriteDDR.pu8VirtAddr, pstChnInfo[u8ChnId].stOutWriteDDR.u32Size, 0);
        HT_WriteFile(pcapfilename, pstChnInfo[u8ChnId].stCapWriteDDR.pu8VirtAddr, pstChnInfo[u8ChnId].stCapWriteDDR.u32Size, 0);
        #endif
        {
            printk("[CHANNEL %d]output viraddr %p, cap viraddr %p\r\n", u8ChnId, pstChnInfo[u8ChnId].stOutWriteDDR.pu8VirtAddr, pstChnInfo[u8ChnId].stCapWriteDDR.pu8VirtAddr);
            printk("[CHANNEL %d]output phyaddr %llx, cap phyaddr %llx,size is %d\r\n", u8ChnId, pstChnInfo[u8ChnId].stOutWriteDDR.phyAddr, pstChnInfo[u8ChnId].stCapWriteDDR.phyAddr, pstChnInfo[u8ChnId].stCapWriteDDR.u32Size);
        }
        #endif
        ExecFunc(MHAL_DIVP_DestroyInstance(pstChnInfo[u8ChnId].pHalCtx),1);
    }

    for(u8ChnId = 0;u8ChnId < u8ChnCnt_Work;u8ChnId++)
    {
        HT_U8  channelID = pstChnInfo[u8ChnId].ChnId;
        HT_U16 cropx = pstChnInfo[u8ChnId].stDIVPWinAttr.u16X;
        HT_U16 cropy = pstChnInfo[u8ChnId].stDIVPWinAttr.u16Y;
        HT_U16 crop_width = pstChnInfo[u8ChnId].stDIVPWinAttr.u16Width;
        HT_U16 crop_height = pstChnInfo[u8ChnId].stDIVPWinAttr.u16Height;
        HT_U32 inputsize_width = pstChnInfo[u8ChnId].stInPutSize.Width;
        HT_U32 inputsize_height = pstChnInfo[u8ChnId].stInPutSize.Height;
        HT_U32 outputsize_width = pstChnInfo[u8ChnId].stOutPutSize.Width;
        HT_U32 outputsize_height = pstChnInfo[u8ChnId].stOutPutSize.Height;

        printk("channelID %d   Crop %d %d %d %d  input size %d*%d  outputsize %d*%d \r\n",  \
        channelID,cropx,cropy,crop_width,crop_height,inputsize_width,inputsize_height,outputsize_width,outputsize_height);
    }
    ExecFunc(_HT_DIVP_DeInit(),HT_SUCCESS);

    #ifndef HT_WRITE_DDR
    kfree(pstChnInfo);
    //memset(pstChnInfo,0x00,u8ChnCnt*sizeof(HT_DIVP_ChnAttr_t));
    pstChnInfo = NULL;
    #endif

    return HT_SUCCESS;
}

void HT_DIVP_DisplayHelp(void)
{
	printk("echo > ChnID,Tnr_Di_Rote,mirror,win,inputsize,inputpixelformat,outputsize...... \r\n");
    printk("ChnID  		 0~16 \r\n");
    printk("Tnr_Di_Rote  0~4 \r\n");
    printk("mirror  	 0~4 \r\n");
    printk("win size   [0](0,0,1920,1088)\r\n");
	printk("           [1](0,0,1280,720)\r\n");
	printk("           [2](0,0,720,480)\r\n");
	printk("           [3](0,0,640,480)\r\n");
	printk("input size [0](0,0,1920,1088)\r\n");
	printk("           [1](0,0,1280,720)\r\n");
	printk("           [2](0,0,720,480)\r\n");
	printk("           [3](0,0,640,480)\r\n");
	printk("pixel foramt [0]YUYV\r\n");
	printk("             [9]YUV420SP\r\n");
	printk("output size[0](0,0,1920,1088)\r\n");
	printk("           [1](0,0,1280,720)\r\n");
	printk("           [2](0,0,720,480)\r\n");
	printk("           [3](0,0,640,480)\r\n");
}

