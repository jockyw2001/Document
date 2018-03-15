#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "mhal_common.h"
#include "mhal_vpe.h"

#include "ht_vpe.h"

#define HT_VPE_MAXCHANNEL   16
#define HT_VPE_MAXPORT       4
#define HT_PARAMNUM_CHN      7
#define HT_ROI_FRAMID        1

#define HT_VPE_INPUT_MAXWIDTH   1920
#define HT_VPE_INPUT_MAXHEIGHT   1088

#ifdef HT_WRITE_DDR
#define HT_INPUTFRAME_NUM    2//MAX 10
#else
#define HT_INPUTFRAME_NUM    10//MAX 10
#endif

#define HT_DIVP_MAX_CASE 4

typedef struct HT_size_s{
    HT_U32 Width;
    HT_U32 Height;
}HT_size_t;

typedef struct HT_VPE_InPutbuffer_s{
    HT_YUVInfo_t stBuffer;
    HT_Stream_ModuleID eModuleID;
}HT_VPE_InPutbuffer_t;

typedef struct HT_VPE_ChannelInfo_s
{
    void *pIspCtx;                                            // HAL layer: ISP context pointer
    void *pIqCtx;                                             // HAL layer: IQ context pointer
    void *pSclCtx;                                            // HAL layer: SCL context pointer
    HT_BOOL bCreate;
    HT_U8   VpeCh;
    HT_S64 s64ReadPos;
    HT_S64 s64WritePos[HT_VPE_MAXPORT];
    char *pFileName;
    /*User Set*/
    MHalVpeIqConfig_t stIqConfig;                               // Channel Iq param
    MHalVpeIqOnOff_t stIqOnOff;
    MHalVpeIspRotationType_e eRoationConfig;                   // Channel roation parameter
    MHalVpeSclCropConfig_t stCropConfig;                       // Channel crop param
    HT_size_t stInPutSize;                                    // input port size
    HT_size_t stOutPutSize[HT_VPE_MAXPORT];                   // output size
    HT_VPE_InPutbuffer_t stInputBuffer;                       // vif output buffer
    HT_VB_Info_t stOutWriteDDR[HT_VPE_MAXPORT];
    HT_PxlFmt_e ePixelFormat;
} HT_VPE_ChannelInfo_t;

#define HT_VPE_YUV422_YUYV_1920x1088 "/mnt/YUVFILE/YUV422_YUYV1920_1088.yuv"
#define HT_VPE_YUV422_YUYV_1280x720  "/mnt/YUVFILE/YUV422_YUYV1280_720.yuv"
#define HT_VPE_YUV422_YUYV_720x480   "/mnt/YUVFILE/YUV422_YUYV720_480.yuv"
#define HT_VPE_YUV422_YUYV_640x480   "/mnt/YUVFILE/YUV422_YUYV640_480.yuv"
#define HT_VPE_YUV420_SP_1920x1088   "/mnt/YUVFILE/YUV420SP_1920_1088.yuv"
#define HT_VPE_YUV420_SP_1280x720    "/mnt/YUVFILE/YUV420SP_1280_720.yuv"
#define HT_VPE_YUV420_SP_720x480     "/mnt/YUVFILE/YUV420SP_720_480.yuv"
#define HT_VPE_YUV420_SP_640x480     "/mnt/YUVFILE/YUV420SP_640_480.yuv"


#define SIZE_1920_1088 3
#define SIZE_1280_720  2
#define SIZE_720_480   1
#define SIZE_640_480   0

extern struct list_head gstOutputBufHead;//outputbuf list


const MHalVpeIqConfig_t stIqConfig[HT_DIVP_MAX_CASE] =
{
    {255,255,255,255,14,15,16,16,16,{255,255,255,255,255,0},255},
    {80,60,80,60,4,8,16,4,0,{0,20,80,120,160,160},0},
    {0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0},128}
};

const MHalVpeIqOnOff_t stIqOnOff[HT_DIVP_MAX_CASE] =
{
    {FALSE,FALSE,FALSE,FALSE,FALSE},
    {TRUE,TRUE,TRUE,TRUE,TRUE}
};

const HalSclCropWindowConfig_t stCropWin[HT_DIVP_MAX_CASE]=
{
    {0,0,640,480},
    {0,0,720,480},
    {0,0,1280,720},
    {0,0,1920,1088},
};

HT_size_t stInPutSizeParam[HT_DIVP_MAX_CASE]=
{
    {640,480},
    {720,480},
    {1280,720},
    {1920,1088},
};

HT_size_t stOutPutSizeParam[HT_DIVP_MAX_CASE]=
{
    {640,480},
    {720,480},
    {1280,720},
    {1920,1088},
};

static HT_RESULT HT_InputPort_Config(const HT_VPE_ChannelInfo_t *pstVpeChannel)
{
    MHalVpeIspInputConfig_t stInputPortConfig;
    memset(&stInputPortConfig,0,sizeof(MHalVpeIspInputConfig_t));

    if(NULL == pstVpeChannel)
    {
       printk("HT_InputPort_Config Failure\r\n");
       return HT_FAILURE;
    }

	//input file format
    if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        stInputPortConfig.enInType = E_MHAL_ISP_INPUT_YUV420;
        stInputPortConfig.ePixelFormat = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    }
    else if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        stInputPortConfig.enInType = E_MHAL_ISP_INPUT_YUV422;
        stInputPortConfig.ePixelFormat = E_MHAL_PIXEL_FRAME_YUV422_YUYV;
    }

    stInputPortConfig.eCompressMode = E_MHAL_COMPRESS_MODE_NONE;
    stInputPortConfig.u32Width = pstVpeChannel->stInPutSize.Width;
    stInputPortConfig.u32Height = pstVpeChannel->stInPutSize.Height;

	/*input_config*/
   ExecFunc(MHalVpeIspInputConfig(pstVpeChannel->pIspCtx, &stInputPortConfig),true);

    return HT_SUCCESS;
}

static HT_RESULT HT_OutputPort_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,const MHalVpeSclOutputPort_e ePort)
{
    MHalVpeSclOutputSizeConfig_t stOutPortSizeConfig;
	MHalVpeSclOutputDmaConfig_t  stPortDmaConfig;
    memset(&stOutPortSizeConfig,0,sizeof(MHalVpeSclOutputSizeConfig_t));
	memset(&stPortDmaConfig,0,sizeof(MHalVpeSclOutputDmaConfig_t));

    if(NULL == pstVpeChannel)
    {
       printk("HT_OutputPort_Config Failure\r\n");
       return HT_FAILURE;
    }

    stOutPortSizeConfig.enOutPort=ePort;
    stOutPortSizeConfig.u16Width = pstVpeChannel->stOutPutSize[ePort].Width;
    stOutPortSizeConfig.u16Height = pstVpeChannel->stOutPutSize[ePort].Height;

   ExecFunc(MHalVpeSclOutputSizeConfig(pstVpeChannel->pSclCtx, &stOutPortSizeConfig),true);

    if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
	    stPortDmaConfig.enOutFormat = E_MHAL_SCL_OUTPUT_NV12420;
    else if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
        stPortDmaConfig.enOutFormat = E_MHAL_SCL_OUTPUT_YUYV422;

    stPortDmaConfig.enOutPort = ePort;
	stPortDmaConfig.enCompress = E_MHAL_COMPRESS_MODE_NONE;//NO compress

    ExecFunc(MHalVpeSclOutputDmaConfig(pstVpeChannel->pSclCtx, &stPortDmaConfig),true);

    return HT_SUCCESS;
}

/*
static HT_RESULT HT_ROI_Config(const HT_VPE_ChannelInfo_t *pstVpeChannel,MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    MHalVpeIqWdrRoiHist_t stCfg;
    MHalVpeIqWdrRoiConfig_t ROI_Cfg;
    memset(&stCfg, 0, sizeof(MHalVpeIqWdrRoiHist_t));
    memset(&ROI_Cfg, 0, sizeof(MHalVpeIqWdrRoiConfig_t));

    ROI_Cfg.bEnSkip=FALSE;
    ROI_Cfg.u16RoiAccX[0]=0;
    ROI_Cfg.u16RoiAccY[0]=0;
    ROI_Cfg.u16RoiAccX[1]=0;
    ROI_Cfg.u16RoiAccY[1]=320;
    ROI_Cfg.u16RoiAccX[2]=0;
    ROI_Cfg.u16RoiAccY[2]=240;
    ROI_Cfg.u16RoiAccX[3]=320;
    ROI_Cfg.u16RoiAccY[3]=240;//320*240

    stCfg.bEn = TRUE;
    stCfg.u8WinCount = 1;
    stCfg.enPipeSrc  = E_MHAL_IQ_ROISRC_AFTER_WDR;
    stCfg.stRoiCfg[0] = ROI_Cfg;

    ExecFunc(MHalVpeIqSetWdrRoiMask(pstVpeChannel->pIqCtx, TRUE, pstCmdQInfo),true);//turn on ROI
    ExecFunc(MHalVpeIqSetWdrRoiHist(pstVpeChannel->pIqCtx, &stCfg),true);

    return HT_SUCCESS;
}*/

static MS_S32 HT_VPE_Free(MS_PHYADDR u64PhyAddr)
{
    HT_Free(u64PhyAddr);

    return HT_SUCCESS;
}

static MS_S32 HT_VPE_Alloc(HT_U8 *pu8Name, HT_U32 size, MS_PHYADDR * phyAddr)
{
    HT_VB_Info_t stVbInfo;
    stVbInfo.u32Size=size;
   // pstVbInfo.u8VirtAddr=u8MMAHeapName;
    HT_Malloc(&stVbInfo);
    stVbInfo.phyAddr=*phyAddr;

    return HT_SUCCESS;
}

static HT_RESULT HT_CreateChannel(HT_VPE_ChannelInfo_t *pstVpeChannel)
{
     MHalAllocPhyMem_t  stAlloc;
     MHalVpeSclWinSize_t  stMaxWin;

     if(NULL == pstVpeChannel)
     {
        printk("creat channel fail\r\n");
        return HT_FAILURE;
     }

     stAlloc.alloc = HT_VPE_Alloc;
     stAlloc.free = HT_VPE_Free;

     /*Max win*/
	stMaxWin.u16Width = HT_VPE_INPUT_MAXWIDTH;
	stMaxWin.u16Height = HT_VPE_INPUT_MAXHEIGHT;

    ExecFunc(MHalVpeCreateIqInstance(&stAlloc, &pstVpeChannel->pIqCtx), true);
    ExecFunc(MHalVpeCreateIspInstance(&stAlloc, &pstVpeChannel->pIspCtx), true);
    ExecFunc(MHalVpeCreateSclInstance(&stAlloc, &stMaxWin, &pstVpeChannel->pSclCtx), true);

        return HT_SUCCESS;
}

static HT_RESULT HT_ChannelConfig(HT_VPE_ChannelInfo_t *pstVpeChannel)
{
     MHalVpeIspRotationConfig_t  stRoationConfig;

     if(NULL == pstVpeChannel)
     {
        printk("HT_ChannelConfig Failure\r\n");
        return HT_FAILURE;
     }

    ExecFunc(MHalVpeIqOnOff(pstVpeChannel->pIqCtx,&(pstVpeChannel->stIqOnOff)),true);
    ExecFunc(MHalVpeIqConfig(pstVpeChannel->pIqCtx,&(pstVpeChannel->stIqConfig)),true);

	stRoationConfig.enRotType = pstVpeChannel->eRoationConfig;
    ExecFunc(MHalVpeIspRotationConfig(pstVpeChannel->pIspCtx, &stRoationConfig),true);

    ExecFunc(MHalVpeSclCropConfig(pstVpeChannel->pSclCtx,&pstVpeChannel->stCropConfig),true);

    ExecFunc(HT_InputPort_Config(pstVpeChannel),HT_SUCCESS);

    ExecFunc(HT_OutputPort_Config(pstVpeChannel,E_MHAL_SCL_OUTPUT_PORT0),HT_SUCCESS);
    ExecFunc(HT_OutputPort_Config(pstVpeChannel,E_MHAL_SCL_OUTPUT_PORT1),HT_SUCCESS);
    ExecFunc(HT_OutputPort_Config(pstVpeChannel,E_MHAL_SCL_OUTPUT_PORT2),HT_SUCCESS);
    ExecFunc(HT_OutputPort_Config(pstVpeChannel,E_MHAL_SCL_OUTPUT_PORT3),HT_SUCCESS);

    return HT_SUCCESS;
}

static HT_RESULT HT_VPE_ProcessWork(HT_VPE_ChannelInfo_t *pstVpeChannel, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, HT_YUVInfo_t *pstVbInputInfo, HT_YUVInfo_t *pstVbOutputInfo)
{
    HT_U8 s8PortId=0;

    MHalVpeSclOutputBufferConfig_t stOutputBufferInfo;
    MHalVpeIspVideoInfo_t stInputBufferInfo;

    printk("VPE PROCESS WORK \r\n");

    memset(&stOutputBufferInfo, 0, sizeof(MHalVpeSclOutputBufferConfig_t));
    memset(&stInputBufferInfo, 0, sizeof(MHalVpeIspVideoInfo_t));

    if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        for(s8PortId=0; s8PortId<HT_VPE_MAXPORT; s8PortId++)
        {
            stOutputBufferInfo.stCfg[s8PortId].bEn=TRUE;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u32Stride[0]=(HT_U32)pstVpeChannel->stOutPutSize[s8PortId].Width;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u32Stride[1] = (HT_U32)pstVpeChannel->stOutPutSize[s8PortId].Width;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u64PhyAddr[0] = pstVbOutputInfo[s8PortId].stYUV420SP.stVbInfo_y.phyAddr;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u64PhyAddr[1] = pstVbOutputInfo[s8PortId].stYUV420SP.stVbInfo_uv.phyAddr;
        }

        stInputBufferInfo.u64PhyAddr[0] = (HT_PHY)pstVbInputInfo->stYUV420SP.stVbInfo_y.phyAddr;
        stInputBufferInfo.u64PhyAddr[1] = (HT_PHY)pstVbInputInfo->stYUV420SP.stVbInfo_uv.phyAddr;
        stInputBufferInfo.u32Stride[0] = pstVpeChannel->stInPutSize.Width;
        stInputBufferInfo.u32Stride[1] = pstVpeChannel->stInPutSize.Width;
     }
    else if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        for(s8PortId=0; s8PortId<HT_VPE_MAXPORT; s8PortId++)
        {
            stOutputBufferInfo.stCfg[s8PortId].bEn=TRUE;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u32Stride[0]=(HT_U32)pstVpeChannel->stOutPutSize[s8PortId].Width*2;
            stOutputBufferInfo.stCfg[s8PortId].stBufferInfo.u64PhyAddr[0] = pstVbOutputInfo[s8PortId].stYUV422YUYV.stVbInfo_yuv.phyAddr;
        }

        stInputBufferInfo.u64PhyAddr[0] = (HT_PHY)pstVbInputInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr;
        stInputBufferInfo.u32Stride[0] = pstVpeChannel->stInPutSize.Width*2;
    }

    ExecFunc(MHalVpeIqProcess(pstVpeChannel->pIqCtx,pstCmdQInfo),true); // PQ proces
    ExecFunc(MHalVpeSclProcess(pstVpeChannel->pSclCtx,pstCmdQInfo, &stOutputBufferInfo),true); //out
    ExecFunc(MHalVpeIspProcess(pstVpeChannel->pIspCtx, pstCmdQInfo, &stInputBufferInfo),true);

    return HT_SUCCESS;
    //PORT3 need MDWIN HalVpeSclSetWaitMdwinDone(pstChnnInfo->pSclCtx, pstDevInfo->pstCmdMloadInfo);
}

/*argv_data [0]channel ID; [1]Iq param; [2]roation param; [3]crop param //more group*/
static HT_RESULT HT_VPE_Run(HT_VPE_ChannelInfo_t* pstVpeChannel, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, HT_U8  u8FrameId)
{
        HT_YUVInfo_t stVbOutputInfo[HT_VPE_MAXPORT];

        HT_U8  u8PortId = 0;
        for(u8PortId = 0; u8PortId<HT_VPE_MAXPORT; u8PortId++)
        {
            HT_U32 u32Height =(pstVpeChannel->stOutPutSize[u8PortId].Height);
            HT_U32 u32Width = (pstVpeChannel->stOutPutSize[u8PortId].Width);
            stVbOutputInfo[u8PortId].ePixelFormat = pstVpeChannel->ePixelFormat;
            ExecFunc(HT_MallocYUVOneFrame(&stVbOutputInfo[u8PortId],u32Width,u32Height), HT_SUCCESS);
        }
        printk("[Frame id %d][Channel id %d]  Malloc output buffer done ......\r\n", u8FrameId, pstVpeChannel->VpeCh);

        if(pstVpeChannel->bCreate == FALSE)
        {
            ExecFunc(HT_CreateChannel(pstVpeChannel),HT_SUCCESS);
            pstVpeChannel->bCreate = TRUE;
        }

       ExecFunc(HT_ChannelConfig(pstVpeChannel),HT_SUCCESS);

       //ExecFunc(MHalVpeIqSetDnrTblMask(pstVpeChannel->pIqCtx, FALSE, pstCmdQInfo),true);//turn off 3DNR
       //ExecFunc(MHalVpeIqRead3DNRTbl(pstVpeChannel->pIqCtx), true);//read 3DNR register ,read save where.

        /*if(HT_ROI_FRAMID == u8FrameId)
        {
            ExecFunc(HT_ROI_Config(pstVpeChannel, pstCmdQInfo),HT_SUCCESS);
        }*/

        ExecFunc(HT_VPE_ProcessWork(pstVpeChannel, pstCmdQInfo, &pstVpeChannel->stInputBuffer.stBuffer, stVbOutputInfo),HT_SUCCESS);

        //ExecFunc(MHalVpeIqSetDnrTblMask(pstVpeChannel->pIqCtx, TRUE, pstCmdQInfo),true);//turn on 3DNR register

        /*if(HT_ROI_FRAMID == u8FrameId)//ROI
        {
            static MHalVpeIqWdrRoiReport_t stRoi;
            ExecFunc(MHalVpeIqSetWdrRoiMask(pstVpeChannel->pIqCtx, FALSE, pstCmdQInfo),true);
            ExecFunc(MHalVpeIqGetWdrRoiHist(pstVpeChannel->pIqCtx, &stRoi),true);

            printk("lumadata is %d\r\n",stRoi.u32Y[0]);
        }*/

        #ifndef HT_WRITE_DDR
        for(u8PortId=0; u8PortId < HT_VPE_MAXPORT; u8PortId++)
        {
            char pfilename[30]="";
            if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
             sprintf(pfilename, "HT_VPE420SP_chn%d_port%d.yuv", pstVpeChannel->VpeCh, u8PortId);
            else if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
             sprintf(pfilename, "HT_VPE_YUYV_chn%d_port%d.yuv", pstVpeChannel->VpeCh, u8PortId);
            ExecFunc(HT_WriteYUVOneFrameToFile(pfilename, &stVbOutputInfo[u8PortId], &(pstVpeChannel->s64WritePos[u8PortId])), HT_SUCCESS);
        }
        #endif

        #ifdef HT_WRITE_DDR
        for(u8PortId = 0; u8PortId < HT_VPE_MAXPORT; u8PortId ++)
        {
            if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
            {
                HT_U8 *dest=pstVpeChannel->stOutWriteDDR[u8PortId].pu8VirtAddr;
                HT_S64 *pWritePos=&pstVpeChannel->s64WritePos[u8PortId];
                memcpy(dest+(*pWritePos), stVbOutputInfo[u8PortId].stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, stVbOutputInfo[u8PortId].stYUV422YUYV.stVbInfo_yuv.u32Size);
                *pWritePos += stVbOutputInfo[u8PortId].stYUV422YUYV.stVbInfo_yuv.u32Size;
            }
            else if(pstVpeChannel->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            {
                HT_U8 *dest=pstVpeChannel->stOutWriteDDR[u8PortId].pu8VirtAddr;
                HT_S64 *pWritePos=&pstVpeChannel->s64WritePos[u8PortId];
                memcpy(dest+(*pWritePos), stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_y.pu8VirtAddr, stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_y.u32Size);
                *pWritePos += stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_y.u32Size;
                memcpy(dest+(*pWritePos), stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_uv.pu8VirtAddr, stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_uv.u32Size);
                *pWritePos += stVbOutputInfo[u8PortId].stYUV420SP.stVbInfo_uv.u32Size;
            }
        }
        #endif

        for(u8PortId = 0; u8PortId < HT_VPE_MAXPORT; u8PortId ++)
        {
            ExecFunc(HT_FreeYUVOneFrame(&stVbOutputInfo[u8PortId]),HT_SUCCESS);//free outputport buffer
        }

        ExecFunc(HT_FreeYUVOneFrame(&pstVpeChannel->stInputBuffer.stBuffer), HT_SUCCESS);//free outputport buffer
        return HT_SUCCESS;
}

static HT_RESULT HT_VPE_Quit(HT_VPE_ChannelInfo_t* pstVpeChannel)
{
    if(pstVpeChannel==NULL)
    {
        printk("HT_VPE_Quit is failure \r\n");
    }
    pstVpeChannel->bCreate=FALSE;
    ExecFunc(MHalVpeDestroyIqInstance(pstVpeChannel->pIqCtx),true);
    ExecFunc(MHalVpeDestroyIspInstance(pstVpeChannel->pIspCtx),true);
    ExecFunc(MHalVpeDestroySclInstance(pstVpeChannel->pSclCtx),true);

	return HT_SUCCESS;
}

static HT_RESULT HT_VPE_SetUserConfig(const HT_U16 *pu16param,const HT_U8 u8ChannelWork_Num,HT_VPE_ChannelInfo_t *pstVpeChannel)
{
     HT_U8 i=0,j=0;
     HT_U8 CaseId=0;
     HT_OutputBufList_t *pstInputlist;
     HT_U8 ListInit = FALSE;
    for(i=0;i<u8ChannelWork_Num;i++)
    {
        pstVpeChannel[i].VpeCh = pu16param[i*HT_PARAMNUM_CHN];
        for(j=i; j>0; j--)
        {
            if(pstVpeChannel[i].VpeCh == pstVpeChannel[j-1].VpeCh )
            {
                printk("%d and %d channel ID same\r\n",i,j);
                    return HT_FAILURE;
            }
        }

        CaseId = pu16param[i*HT_PARAMNUM_CHN+1];
        if(CaseId == 0)
        {
            pstVpeChannel[i].stIqOnOff = stIqOnOff[0];
            pstVpeChannel[i].stIqConfig = stIqConfig[CaseId];
        }
        else if(CaseId>0&&CaseId<4)
        {
            pstVpeChannel[i].stIqOnOff = stIqOnOff[1];
            pstVpeChannel[i].stIqConfig = stIqConfig[CaseId-1];
        }
        else
            return HT_FAILURE;

        pstVpeChannel[i].eRoationConfig = pu16param[i*HT_PARAMNUM_CHN+2];

        CaseId = pu16param[i*HT_PARAMNUM_CHN+3];
        if(CaseId == 0)
            pstVpeChannel[i].stCropConfig.bCropEn = FALSE;
        else if(0<CaseId && CaseId<5)
        {
            pstVpeChannel[i].stCropConfig.bCropEn = TRUE;
            pstVpeChannel[i].stCropConfig.stCropWin = stCropWin[CaseId-1];
        }
        else
        {
            printk("1<crop param<5 error\r\n");
            return HT_FAILURE;
        }

        CaseId = pu16param[i*HT_PARAMNUM_CHN+4];
        pstVpeChannel[i].stInPutSize = stInPutSizeParam[CaseId];
        pstVpeChannel[i].ePixelFormat = pu16param[i*HT_PARAMNUM_CHN+6];

        if(CaseId <4) //max 4 input from vif
        {
            if(ListInit == FALSE)
            {
                INIT_LIST_HEAD(&gstOutputBufHead);
                ListInit =TRUE;
            }

            if(E_HT_PIXEL_FORMAT_YUV422_YUYV == pstVpeChannel[i].ePixelFormat)
            {
                switch(CaseId)
                {
                    case SIZE_1920_1088:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV422_YUYV_1920x1088;
                        break;
                    case SIZE_1280_720:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV422_YUYV_1280x720;
                        break;
                    case SIZE_720_480:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV422_YUYV_720x480;
                        break;
                    case SIZE_640_480:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV422_YUYV_640x480;
                        break;
                    default:
                    {
                        printk("no file for your input size\r\n");
                        printk("input size 0 is 1920*1080\r\n");
                        printk("input size 1 is 1280*720\r\n");
                        printk("input size 2 is 640*480\r\n");
                        printk("input size 3 is 320*240\r\n");
                        return HT_FAILURE;
                    }
                }
            }
            else if(E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pstVpeChannel[i].ePixelFormat)
            {
                switch(CaseId)
               {
                    case SIZE_1920_1088:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV420_SP_1920x1088;
                        break;
                    case SIZE_1280_720:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV420_SP_1280x720;
                        break;
                    case SIZE_720_480:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV420_SP_720x480;
                        break;
                    case SIZE_640_480:
                        pstVpeChannel[i].pFileName = HT_VPE_YUV420_SP_640x480;
                        break;
                    default:
                    {
                        printk("no file for your input size\r\n");
                        printk("input size 0 is 1920*1080\r\n");
                        printk("input size 1 is 1280*720\r\n");
                        printk("input size 2 is 640*480\r\n");
                        printk("input size 3 is 320*240\r\n");
                        return HT_FAILURE;
                    }
               }
            }
            else
            {
                printk("pixel fomat set error pixel is %d\r\n",pstVpeChannel[i].ePixelFormat);
                return HT_FAILURE;
            }

        }

        CaseId = pu16param[i*HT_PARAMNUM_CHN+5];
        pstVpeChannel[i].stOutPutSize[0] = stOutPutSizeParam[CaseId];
        pstVpeChannel[i].stOutPutSize[1] = stOutPutSizeParam[CaseId];
        pstVpeChannel[i].stOutPutSize[2] = stOutPutSizeParam[CaseId];
        pstVpeChannel[i].stOutPutSize[3] = stOutPutSizeParam[CaseId];

    }

    if(ListInit == TRUE)
    {
        for(j=0; j<HT_INPUTFRAME_NUM ;j++)
        {
            for(i=0;i<u8ChannelWork_Num;i++)
            {
                pstInputlist=(HT_OutputBufList_t *)kmalloc(sizeof(HT_OutputBufList_t),GFP_KERNEL);
                pstInputlist->eModuleID = E_HT_STREAM_MODULEID_NULL;
                pstInputlist->stYuvVbInfo.ePixelFormat = pstVpeChannel[i].ePixelFormat;
                pstInputlist->Width = pstVpeChannel[i].stInPutSize.Width;
                pstInputlist->Height = pstVpeChannel[i].stInPutSize.Height;
                pstInputlist->u8Chnidx = pstVpeChannel[i].VpeCh;
                list_add_tail(&(pstInputlist->list),&(gstOutputBufHead));
            }
        }
    }

    return HT_SUCCESS;
}

static HT_VPE_ChannelInfo_t    *pstVpeChannel;//every channel infomation

HT_RESULT HT_VPE(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_U8   u8FrameId = 0;
    HT_U8   u8ChannelId = 0;
    HT_U8   u8ChannelWork_Num=0;
    HT_OutputBufList_t *pos,*posN;

    MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo;
    MHAL_CMDQ_BufDescript_t cmdq_buff;

    u8ChannelWork_Num = u8CmdCnt / HT_PARAMNUM_CHN;

    if(u8ChannelWork_Num > HT_VPE_MAXCHANNEL || 0 != u8CmdCnt%HT_PARAMNUM_CHN)
    {
        printk("channel work %d cmdcnt %d\r\n",u8ChannelWork_Num,u8CmdCnt);
        return HT_FAILURE;
    }

    if(pstVpeChannel != NULL)
    {
        printk("channel addr %p, port 0 phyaddr %llx\r\n", pstVpeChannel, pstVpeChannel[0].stOutWriteDDR[0].phyAddr);
        for(u8ChannelId =0; u8ChannelId < HT_VPE_MAXCHANNEL; u8ChannelId++)
        {
            HT_U8 u8PortId = 0;
            for(u8PortId = 0; u8PortId <HT_VPE_MAXPORT; u8PortId++)
            {
                    HT_Free(pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].phyAddr);
            }
        }
        kfree(pstVpeChannel);
    }


    pstVpeChannel = (HT_VPE_ChannelInfo_t *) kmalloc(sizeof(HT_VPE_ChannelInfo_t) * u8ChannelWork_Num, GFP_KERNEL);
    memset(pstVpeChannel, 0, sizeof(HT_VPE_ChannelInfo_t) * u8ChannelWork_Num);//init 0,every channel is false;
    ExecFunc(HT_VPE_SetUserConfig(pau16CmdValue,u8ChannelWork_Num,pstVpeChannel),HT_SUCCESS);


#ifdef HT_WRITE_DDR
    {
        HT_U32 u32OutDDRsize = 0;
        HT_U8   u8PortId = 0;

        for(u8ChannelId =0; u8ChannelId < u8ChannelWork_Num; u8ChannelId++)
        {
            for(u8PortId =0; u8PortId< HT_VPE_MAXPORT; u8PortId++)
            {
                if(pstVpeChannel[u8ChannelId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
                {
                    u32OutDDRsize = pstVpeChannel[u8ChannelId].stOutPutSize[u8PortId].Height * pstVpeChannel[u8ChannelId].stOutPutSize[u8PortId].Width *2 *HT_INPUTFRAME_NUM;
                }
                else if(pstVpeChannel[u8ChannelId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
                {
                    u32OutDDRsize = pstVpeChannel[u8ChannelId].stOutPutSize[u8PortId].Height * pstVpeChannel[u8ChannelId].stOutPutSize[u8PortId].Width *3/2 *HT_INPUTFRAME_NUM;
                }

                pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].u32Size = u32OutDDRsize;
                HT_Malloc(&pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId]);
            }
         }
    }
#endif

    cmdq_buff.u32CmdqBufSize=1024;
    cmdq_buff.u32CmdqBufSizeAlign=0;
    cmdq_buff.u32MloadBufSize=1024;
    cmdq_buff.u16MloadBufSizeAlign=0;
    pstCmdQInfo =MHAL_CMDQ_GetSysCmdqService(E_MHAL_CMDQ_ID_VPE, &cmdq_buff, TRUE);

    {
        u8ChannelId = 0;
        list_for_each_entry_safe(pos,posN,&gstOutputBufHead, list)
        {
            if(pos ==NULL)
                return HT_FAILURE;

            pstVpeChannel[u8ChannelId].VpeCh =(HT_U8) pos->u8Chnidx;
            pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID = pos->eModuleID;
            pstVpeChannel[u8ChannelId].stInPutSize.Width = pos->Width;
            pstVpeChannel[u8ChannelId].stInPutSize.Height = pos->Height;

            if(pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID == E_HT_STREAM_MODULEID_NULL)
            {
                HT_U32 u32Width = pstVpeChannel[u8ChannelId].stInPutSize.Width;
                HT_U32 u32Height = pstVpeChannel[u8ChannelId].stInPutSize.Height;
                pstVpeChannel[u8ChannelId].stInputBuffer.stBuffer.ePixelFormat = pos->stYuvVbInfo.ePixelFormat;
                ExecFunc(HT_MallocYUVOneFrame(&pstVpeChannel[u8ChannelId].stInputBuffer.stBuffer, u32Width, u32Height),HT_SUCCESS);  //malloc inputport one frame buffer
                ExecFunc(HT_ReadYUVOneFrameToBuffer(pstVpeChannel[u8ChannelId].pFileName, &pstVpeChannel[u8ChannelId].stInputBuffer.stBuffer, &(pstVpeChannel[u8ChannelId].s64ReadPos)), HT_SUCCESS);//read one frme file to inputbuffer
                printk("input buffer module id %d  \r\n",pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID);
            }
            else if(pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID == E_HT_STREAM_MODULEID_VIF)
            {
                pstVpeChannel[u8ChannelId].stInputBuffer.stBuffer = pos->stYuvVbInfo;
                printk("input buffer module id %d  \r\n",pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID);
            }
            else
            {
                printk("input buffer module id %d is error \r\n",pstVpeChannel[u8ChannelId].stInputBuffer.eModuleID);
                return HT_FAILURE;
            }

            printk("u8ChannelId %d VpeCh %d frameid %d begin ...........\r\n",u8ChannelId,pstVpeChannel[u8ChannelId].VpeCh,u8FrameId);
            ExecFunc(HT_VPE_Run(&pstVpeChannel[u8ChannelId],pstCmdQInfo,u8FrameId),HT_SUCCESS);
            list_del(&pos->list);
            kfree(pos);
            printk("u8ChannelId %d VpeCh %d frameid %d end...........\r\n",u8ChannelId,pstVpeChannel[u8ChannelId].VpeCh,u8FrameId);
            u8ChannelId ++;
            if(u8ChannelId >u8ChannelWork_Num -1)
            {
                u8ChannelId =0;
                u8FrameId ++;
            }
        }

    }

    for(u8ChannelId =0; u8ChannelId < u8ChannelWork_Num; u8ChannelId ++)
    {
        ExecFunc(HT_VPE_Quit(&pstVpeChannel[u8ChannelId]),HT_SUCCESS);
    }

    for(u8ChannelId = 0; u8ChannelId < u8ChannelWork_Num; u8ChannelId ++)
    {
        HT_U8  channelID = pstVpeChannel[u8ChannelId].VpeCh;
        HT_U16 cropx = pstVpeChannel[u8ChannelId].stCropConfig.stCropWin.u16X;
        HT_U16 cropy = pstVpeChannel[u8ChannelId].stCropConfig.stCropWin.u16Y;
        HT_U16 crop_width = pstVpeChannel[u8ChannelId].stCropConfig.stCropWin.u16Width;
        HT_U16 crop_height = pstVpeChannel[u8ChannelId].stCropConfig.stCropWin.u16Height;
        HT_U32 inputsize_width = pstVpeChannel[u8ChannelId].stInPutSize.Width;
        HT_U32 inputsize_height = pstVpeChannel[u8ChannelId].stInPutSize.Height;
        HT_U32 outputsize_width = pstVpeChannel[u8ChannelId].stOutPutSize[0].Width;
        HT_U32 outputsize_height = pstVpeChannel[u8ChannelId].stOutPutSize[0].Height;

        #ifdef HT_WRITE_DDR
        {
            HT_U8 u8PortId = 0;
            HT_U8 *viraddr;
            HT_U32 u32Size = 0;
            HT_PHY phyAddr = 0;
            for(u8PortId=0; u8PortId<HT_VPE_MAXPORT; u8PortId++)
            {
                viraddr = pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].pu8VirtAddr;
                u32Size = pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].u32Size;
                phyAddr = pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].phyAddr;

                #if 1
                {
                    char pfilename[50]="";
                    if(pstVpeChannel[u8ChannelId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
                        sprintf(pfilename, "HT_VPEChn%dPort%dYUYV.yuv", channelID, u8PortId);
                    else if(pstVpeChannel[u8ChannelId].ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
                        sprintf(pfilename, "HT_VPEChn%dPort%d_420SP.yuv", channelID, u8PortId);

                    HT_WriteFile(pfilename, pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].pu8VirtAddr, pstVpeChannel[u8ChannelId].stOutWriteDDR[u8PortId].u32Size, 0);
                }
                #endif

                printk("[CHANNEL %d port ID %d]viraddr %p, phyaddr %lld, size %d\r\n",channelID, u8PortId,viraddr,phyAddr,u32Size);
            }
        }
        #endif

        printk("channelID %d   Crop %d %d %d %d  input size %d*%d  outputsize %d*%d ,pxiel %d\r\n",  \
        channelID,cropx,cropy,crop_width,crop_height,inputsize_width,inputsize_height,outputsize_width,outputsize_height,pstVpeChannel[u8ChannelId].ePixelFormat);
    }

    #ifndef HT_WRITE_DDR
    kfree(pstVpeChannel);
    pstVpeChannel = NULL;
    #endif

    return HT_SUCCESS;
}

void HT_VPE_DisplayHelp(void)
{
    printk("input param list is channel ID..iq ..roation..crop...inputsize..outputsize..fileformat......\r\n");
    printk("0<=	channel ID 		<=16\r\n");
    printk("0<=	Pq param   		<=4\r\n");
    printk("0<= Roation param 	<=4\r\n");
    printk("Crop param [0]disable\r\n");
	printk("           [1](0,0,1920,1088)\r\n");
	printk("           [2](0,0,1280,720)\r\n");
	printk("           [3](0,0,720,480)\r\n");
	printk("           [4](0,0,640,480)\r\n");
	printk("input size [0](0,0,1920,1088)\r\n");
	printk("           [1](0,0,1280,720)\r\n");
	printk("           [2](0,0,720,480)\r\n");
	printk("           [3](0,0,640,480)\r\n");
	printk("output size[0](0,0,1920,1088)\r\n");
	printk("           [1](0,0,1280,720)\r\n");
	printk("           [2](0,0,720,480)\r\n");
	printk("           [3](0,0,640,480)\r\n");
	printk("pixel foramt [0]YUYV\r\n");
	printk("             [9]YUV420SP\r\n");
}

