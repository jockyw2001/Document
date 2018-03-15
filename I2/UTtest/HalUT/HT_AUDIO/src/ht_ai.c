#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>
#include <linux/delay.h>

#include "ht_common.h"
#include "mhal_audio.h"
#include "ht_ai.h"
#include "ht_aio_datatype.h"

#define HT_AI_WRITE_FILE 1

#define HT_AI_CHAN_NUM_MAX  16
#define HT_AI_DEV_NUM_MAX   2
#define HT_AI_START_WRTIE_THRESHOLD   3840
#define HT_AI_PCM_BUF_SIZE_BYTE      (HT_AUDIO_MAX_SAMPLES_PER_FRAME * HT_AUDIO_MAX_FRAME_NUM * HT_AI_CHAN_NUM_MAX * 4)
#define HT_AI_READ_PEROID_SAMPLES     128


typedef struct HT_AI_ChanInfo_s
{
    HT_BOOL                 bChanEnable;
    HT_BOOL                 bPortEnable;
    HT_S32                  s32ChnId;
    HT_S32                  s32OutputPortId;
    //HT_AI_ChnParam_t        stChnParam;
    //MI_BOOL                 bResampleEnable;
    //MI_BOOL                 bResampleInit;   // ???
    //MI_AUDIO_SampleRate_e   eOutResampleRate;
    //MI_BOOL                 bVqeEnable;
    //MI_BOOL                 bVqeAttrSet;
    //MI_S32                  s32VolumeDb;
    //MI_AI_VqeConfig_t       stAiVqeConfig;

}HT_AI_ChanInfo_t;

typedef struct HT_AI_DevInfo_s
{
    HT_BOOL                 bDevEnable;
    HT_AUDIO_DEV            AiDevId;
    HT_AUDIO_PubAttr_t      stDevAttr;
    HT_AI_ChanInfo_t        astChanInfo[HT_AI_CHAN_NUM_MAX];
    HT_U64                  u64PhyBufAddr;          // for DMA HW address
    HT_U8 *                 pu8TempBufAddr;         // for audio driver temp buffer
    struct task_struct *    pstAiReadDataThread;    // Kernel thread for _HT_AI_ReadDataThread
    struct list_head stOutPutBufQueue;
    struct semaphore stBuffQueueMutex;
}HT_AI_DevInfo_t;

typedef struct HT_AI_OutPutBufInfo_s
{
    HT_VB_Info_t stVbInfo;
    struct list_head stOutPutBufNode;

}HT_AI_OutPutBufInfo_t;

typedef struct HT_AI_OutPutFileInfo_s
{
    HT_U8 pu8OutputFilePath[30];
    HT_S64 s64OutputFilePos;

}HT_AI_OutPutFileInfo_t;

typedef struct HT_AI_TestValue_s
{
    HT_AUDIO_DEV AiDevId;
    HT_S32 s32AiChnNum;
    HT_AUDIO_SoundMode_e eSoundMode;
    HT_AUDIO_SampleRate_e eSampleRete;

}HT_AI_TestValue_t;

static DECLARE_WAIT_QUEUE_HEAD(Ai_WriteData_waitqueue);

static HT_RESULT _HT_AI_ParseStrings(HT_U16 *pau16CmdValue,HT_AI_TestValue_t *pstAiTestValue)
{
    pstAiTestValue->AiDevId = pau16CmdValue[0];
    pstAiTestValue->s32AiChnNum = pau16CmdValue[1];
    if(pstAiTestValue->s32AiChnNum==0)
    {
        printk("Invalid Chn Num:%d\n",pstAiTestValue->s32AiChnNum);
        return HT_FAILURE;
    }
    pstAiTestValue->eSoundMode=(pau16CmdValue[2]==1)?E_HT_AUDIO_SOUND_MODE_MONO:((pau16CmdValue[2]==2)?E_HT_AUDIO_SOUND_MODE_STEREO:E_HT_AUDIO_SOUND_MODE_MAX);
    printk("sound mode:%d\n",pstAiTestValue->eSoundMode);
    switch(pau16CmdValue[3])
    {
        case 1:
            pstAiTestValue->eSampleRete=E_HT_AUDIO_SAMPLE_RATE_8000;
            break;
        case 2:
            pstAiTestValue->eSampleRete=E_HT_AUDIO_SAMPLE_RATE_16000;
            break;
        case 3:
            pstAiTestValue->eSampleRete=E_HT_AUDIO_SAMPLE_RATE_32000;
            break;
        case 4:
            pstAiTestValue->eSampleRete=E_HT_AUDIO_SAMPLE_RATE_48000;
            break;
        default:
            pstAiTestValue->eSampleRete=E_HT_AUDIO_SAMPLE_RATE_INVALID;
            break;
    }
    printk("AI devid:%d chnnum:%d\n",pstAiTestValue->AiDevId,pstAiTestValue->s32AiChnNum);
    return HT_SUCCESS;
}

static HT_RESULT _HT_AI_Config(HT_AI_DevInfo_t *pstAiDevInfo)
{
    HT_U32 u32BitWidthByte;
    MHAL_AUDIO_PcmCfg_t stDmaConfig;
    MHAL_AUDIO_I2sCfg_t stI2sConfig;

    memset(&stDmaConfig, 0, sizeof(MHAL_AUDIO_PcmCfg_t));
    stDmaConfig.eWidth = (MHAL_AUDIO_BitWidth_e)pstAiDevInfo->stDevAttr.eBitwidth;
    stDmaConfig.eRate = (MHAL_AUDIO_Rate_e)pstAiDevInfo->stDevAttr.eSamplerate;
    stDmaConfig.u16Channels = pstAiDevInfo->stDevAttr.u32ChnCnt;
    stDmaConfig.bInterleaved = FALSE;
    HT_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, stDmaConfig.eWidth);
    stDmaConfig.u32PeriodSize = pstAiDevInfo->stDevAttr.u32PtNumPerFrm * u32BitWidthByte * stDmaConfig.u16Channels ;
    stDmaConfig.phyDmaAddr = pstAiDevInfo->u64PhyBufAddr;
    stDmaConfig.u32BufferSize = HT_AI_PCM_BUF_SIZE_BYTE ;
    ExecFunc(MHAL_AUDIO_ConfigPcmIn((MHAL_AUDIO_DEV)pstAiDevInfo->AiDevId, &stDmaConfig), MHAL_SUCCESS);

    memset(&stI2sConfig, 0, sizeof(MHAL_AUDIO_I2sCfg_t));
    stI2sConfig.eMode = E_MHAL_AUDIO_MODE_I2S_MASTER;
    stI2sConfig.eWidth = (HT_AUDIO_BitWidth_e)pstAiDevInfo->stDevAttr.eBitwidth;
    stI2sConfig.eFmt = E_MHAL_AUDIO_I2S_FMT_I2S;
    stI2sConfig.u16Channels = pstAiDevInfo->stDevAttr.u32ChnCnt;
    ExecFunc(MHAL_AUDIO_ConfigI2sIn((MHAL_AUDIO_DEV)pstAiDevInfo->AiDevId, &stI2sConfig), MHAL_SUCCESS);
    ExecFunc(MHAL_AUDIO_OpenPcmIn((MHAL_AUDIO_DEV)pstAiDevInfo->AiDevId), MHAL_SUCCESS);
    ExecFunc(MHAL_AUDIO_StartPcmIn((MHAL_AUDIO_DEV)pstAiDevInfo->AiDevId), MHAL_SUCCESS);

    return HT_SUCCESS;
}
static HT_RESULT _HT_AI_ReadDataThread(void *data)
{
    HT_AI_DevInfo_t *pstAiDevInfo = NULL;
    HT_U32 u32BitWidthByte = 0;
    HT_AI_OutPutBufInfo_t *pstOutPutBufInfo = NULL;

    pstAiDevInfo = (HT_AI_DevInfo_t *)data;
    BUG_ON(!pstAiDevInfo);
    HT_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, pstAiDevInfo->stDevAttr.eBitwidth);

    while(!kthread_should_stop())
    {
        pstOutPutBufInfo=kmalloc(sizeof(HT_AI_OutPutBufInfo_t),GFP_KERNEL);
        BUG_ON(!pstOutPutBufInfo);
        pstOutPutBufInfo->stVbInfo.u32Size = pstAiDevInfo->stDevAttr.u32PtNumPerFrm * u32BitWidthByte * pstAiDevInfo->stDevAttr.u32ChnCnt * 100;
        BUG_ON(HT_Malloc(&pstOutPutBufInfo->stVbInfo) != HT_SUCCESS);
        while (MHAL_FAILURE == MHAL_AUDIO_ReadDataIn((MHAL_AUDIO_DEV)pstAiDevInfo->AiDevId, (VOID *)pstOutPutBufInfo->stVbInfo.pu8VirtAddr, pstOutPutBufInfo->stVbInfo.u32Size, FALSE))
        {
            msleep(1);
        }
#if(HT_AI_WRITE_FILE==1)
        down(&pstAiDevInfo->stBuffQueueMutex);
        list_add_tail(&pstOutPutBufInfo->stOutPutBufNode,&pstAiDevInfo->stOutPutBufQueue);
        up(&pstAiDevInfo->stBuffQueueMutex);
#endif
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_AI_Start(HT_AI_DevInfo_t *pstAiDevInfo)
{
    ExecFunc(MHAL_AUDIO_StartPcmIn(pstAiDevInfo->AiDevId), MHAL_SUCCESS);
    pstAiDevInfo->pstAiReadDataThread = kthread_run(_HT_AI_ReadDataThread, pstAiDevInfo, "HTAiReadDataThread");

    return HT_SUCCESS;
}
static HT_RESULT _HT_AI_Init(void)
{
    ExecFunc(MHAL_AUDIO_Init(), MHAL_SUCCESS);
    return HT_SUCCESS;
}
static HT_RESULT _HT_AI_DeInit(HT_AI_DevInfo_t *pstAiDevInfo)
{
    kthread_stop(pstAiDevInfo->pstAiReadDataThread);
    return HT_SUCCESS;
}
static void _HT_AI_InitVar(HT_AI_DevInfo_t *pstAiDevInfo, const HT_AI_TestValue_t *pstTestVal, HT_BOOL bEn)
{
    HT_U8 i = 0;

    BUG_ON(!pstAiDevInfo);
    BUG_ON(!pstTestVal);

    pstAiDevInfo->bDevEnable = bEn;
    pstAiDevInfo->AiDevId = pstTestVal->AiDevId;
    pstAiDevInfo->stDevAttr.eSamplerate = pstTestVal->eSampleRete;
    pstAiDevInfo->stDevAttr.eSoundmode = pstTestVal->eSoundMode;
    pstAiDevInfo->stDevAttr.u32ChnCnt = pstTestVal->s32AiChnNum;
    pstAiDevInfo->stDevAttr.eBitwidth = E_HT_AUDIO_BIT_WIDTH_16;
    pstAiDevInfo->stDevAttr.eWorkmode = E_HT_AUDIO_MODE_I2S_MASTER;
    pstAiDevInfo->stDevAttr.u32FrmNum = 16;
    pstAiDevInfo->stDevAttr.u32PtNumPerFrm = HT_AUDIO_SAMPLE_PER_FRAME;

    for (i = 0; i < pstAiDevInfo->stDevAttr.u32ChnCnt; i++)
    {
        pstAiDevInfo->astChanInfo[i].bChanEnable = TRUE;
        pstAiDevInfo->astChanInfo[i].bPortEnable = TRUE;
        pstAiDevInfo->astChanInfo[i].s32ChnId = i;
        pstAiDevInfo->astChanInfo[i].s32OutputPortId = 0;
    }

    INIT_LIST_HEAD(&pstAiDevInfo->stOutPutBufQueue);
    sema_init(&(pstAiDevInfo->stBuffQueueMutex),1);

}

void HT_AI_DisplayHelp(void)
{

}

static HT_AI_TestValue_t gstAiTestValue[HT_AI_DEV_NUM_MAX];
static HT_AI_DevInfo_t gstAiDevInfo[HT_AI_DEV_NUM_MAX];


HT_RESULT HT_AI(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_U8 i = 0;
    HT_U8 u8ChnIdx=0;
    HT_U8 *pVirBuff;
    HT_S32 s32StopFrameCnt=0;
    HT_S32 s32BuffSize,s32WriteBytes;
    HT_AI_OutPutFileInfo_t astOutputFileInfo[HT_AI_CHAN_NUM_MAX];
    HT_AI_OutPutBufInfo_t *pstTmpBuffInfo;
    struct list_head *pstPos;
    struct list_head *n;

    _HT_AI_Init();
    memset(&gstAiTestValue, 0, sizeof(HT_AI_TestValue_t) * HT_AI_DEV_NUM_MAX);
    for (i = 0; i < u8CmdCnt/4; i++)
    {
        _HT_AI_ParseStrings(pau16CmdValue + i * 4, &gstAiTestValue[i]);
        _HT_AI_InitVar(&gstAiDevInfo[i], &gstAiTestValue[i], TRUE);
        _HT_AI_Config(&gstAiDevInfo[i]);
        _HT_AI_Start(&gstAiDevInfo[i]);
    }

    i = 0;
    memset(astOutputFileInfo, 0, sizeof(HT_AI_OutPutFileInfo_t) * HT_AI_CHAN_NUM_MAX);
    for (s32StopFrameCnt = 0; s32StopFrameCnt < 1024; s32StopFrameCnt++)
    {
        msleep(1);
        if (!gstAiDevInfo[i].bDevEnable)
        {
            continue;
        }
        down(&gstAiDevInfo[i].stBuffQueueMutex);
        list_for_each_safe(pstPos,n,&gstAiDevInfo[i].stOutPutBufQueue)
        {
            pstTmpBuffInfo = list_entry(pstPos, HT_AI_OutPutBufInfo_t, stOutPutBufNode);
            pVirBuff = pstTmpBuffInfo->stVbInfo.pu8VirtAddr;
            s32BuffSize = (HT_S32)pstTmpBuffInfo->stVbInfo.u32Size / gstAiDevInfo[i].stDevAttr.u32ChnCnt;

            for(u8ChnIdx = 0; u8ChnIdx < HT_AI_CHAN_NUM_MAX; u8ChnIdx++)
            {
                if (!gstAiDevInfo[i].astChanInfo[u8ChnIdx].bChanEnable)
                {
                    continue;
                }

                sprintf(astOutputFileInfo[u8ChnIdx].pu8OutputFilePath, "/mnt/pcmfile/output_chn%d.pcm", u8ChnIdx);
                s32WriteBytes = HT_WriteFile(astOutputFileInfo[u8ChnIdx].pu8OutputFilePath, pVirBuff, s32BuffSize, astOutputFileInfo[u8ChnIdx].s64OutputFilePos);
                pVirBuff += s32WriteBytes;
                astOutputFileInfo[u8ChnIdx].s64OutputFilePos += s32WriteBytes;
            }

            HT_Free(pstTmpBuffInfo->stVbInfo.phyAddr);
            kfree(pstTmpBuffInfo);
            list_del(pstPos);
        }
        up(&gstAiDevInfo[i].stBuffQueueMutex);
        i = (i % (u8CmdCnt/4))?i+1:0;
    }
    s32StopFrameCnt = 0;
    memset(astOutputFileInfo,0,sizeof(HT_AI_OutPutFileInfo_t) * HT_AI_CHAN_NUM_MAX);

    for (i = 0; i < u8CmdCnt/4; i++)
    {
        printk("stop read data\n");
        _HT_AI_DeInit(&gstAiDevInfo[i]);
    }
    return HT_SUCCESS;
}




