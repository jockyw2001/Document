#include <linux/version.h>
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

#include "mhal_audio_datatype.h"
#include "mhal_audio.h"
#include "ht_ao.h"
#include "ht_aio_datatype.h"
#include "ht_hdmi.h"
#include "ht_common.h"

#define HT_AO_MONO_INPUT_FILE_PATH "/mnt/wavfile/SRC_8K_MONO.wav"
#define HT_AO_STEREO_INPUT_FILE_PATH "/mnt/wavfile/SRC_48K_STEREO.wav"

#define HT_AO_DEV_NUM_MAX   4
#define HT_AO_CHAN_NUM_MAX  1
#define HT_AO_START_WRTIE_THRESHOLD   3840
#define HT_AO_PCM_BUF_SIZE_BYTE      (HT_AUDIO_MAX_SAMPLES_PER_FRAME * HT_AUDIO_MAX_FRAME_NUM * 2 * 4)

typedef struct HT_AO_WavHeader_s
{
    HT_U8   riff[4];                // RIFF string
    HT_U32  ChunkSize;              // overall size of file in bytes
    HT_U8   wave[4];                // WAVE string
    HT_U8   fmt_chunk_marker[4];    // fmt string with trailing null char
    HT_U32  length_of_fmt;          // length of the format data
    HT_U16  format_type;            // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    HT_U16  channels;               // no.of channels
    HT_U32  sample_rate;            // sampling rate (blocks per second)
    HT_U32  byterate;               // SampleRate * NumChannels * BitsPerSample/8
    HT_U16  block_align;            // NumChannels * BitsPerSample/8
    HT_U16  bits_per_sample;        // bits per sample, 8- 8bits, 16- 16 bits etc
    HT_U8   data_chunk_header [4];  // DATA string or FLLR string
    HT_U32  data_size;              // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read

}HT_AO_WavHeader_t;

typedef struct HT_AO_DevInfo_s
{
    HT_BOOL                 bDevEnable;
    HT_BOOL                 bDevAttrSet;
    HT_U8                   AoDevId;
    HT_AUDIO_PubAttr_t      stDevAttr;
    HT_S32                  s32VolumeDb;
    HT_BOOL                 bMuteEnable;
    HT_U64                  u64PhyBufAddr;          // for DMA HW address
    HT_U8 *                 pu8VirtAddr;
    HT_S64 s64FilePos;
    struct task_struct *    pstAoWriteDataThread;    // Kernel thread for _MI_AO_WriteDataThread
    struct semaphore stBuffQueueMutex;
    struct list_head stBuffQueue;
}HT_AO_DevInfo_t;

typedef struct MI_AUDIO_Frame_s
{
    HT_AUDIO_BitWidth_e eBitwidth; /*audio frame bitwidth*/
    HT_AUDIO_SoundMode_e eSoundmode; /*audio frame momo or stereo mode*/
    void *apVirAddr;
    HT_PHY phyAddr;
    HT_U64 u64TimeStamp;/*audio frame timestamp*/
    HT_U32 u32Seq; /*audio frame seq*/
    HT_U32 u32Len; /*data lenth per channel in frame*/
    HT_U32 au32PoolId[2];
    struct list_head stBuffNode;
}HT_AUDIO_Frame_t;

typedef struct HT_AO_TestValue_s
{
    HT_BOOL bEn;
    HT_U8 u8DevId;
    HT_S32 s32VolumeDb;
    HT_U8 *pu8FilePath;
}HT_AO_TestValue_t;

static HT_AO_TestValue_t gastAoTestValue[HT_AO_DEV_NUM_MAX];
static HT_AO_DevInfo_t gastAoDevInfo[HT_AO_DEV_NUM_MAX];
static DECLARE_WAIT_QUEUE_HEAD(Ao_WriteData_waitqueue);

static HT_RESULT _HT_AO_Init(void)
{
    ExecFunc(MHAL_AUDIO_Init(),MHAL_SUCCESS);
    return HT_SUCCESS;
}
static HT_RESULT _HT_AO_Stop(HT_AO_DevInfo_t* pstDevInfo)
{
    kthread_stop(pstDevInfo->pstAoWriteDataThread);
    return HT_SUCCESS;
}
static HT_RESULT _HT_AO_DeInit(void)
{
    return HT_SUCCESS;
}

static HT_S32 _HT_AO_SetPubAttr(HT_AO_DevInfo_t* pstDevInfo, HT_AO_TestValue_t *pstTestValue)
{
    HT_S32 s32Ret = HT_SUCCESS;
    HT_AUDIO_PubAttr_t stPubAttr;
    HT_AO_WavHeader_t stWavHeaderInput;
    HT_U8 u8ReadSize = 0;
    HT_U8 *pu8FilePath;
    HT_VB_Info_t stVbInfo;

    pu8FilePath = pstTestValue->pu8FilePath;

    memset(&stWavHeaderInput, 0, sizeof(HT_AO_WavHeader_t));
    u8ReadSize=HT_ReadFile(pu8FilePath,(HT_U8*)&stWavHeaderInput,sizeof(HT_AO_WavHeader_t),0);
    if(u8ReadSize!=sizeof(HT_AO_WavHeader_t))
    {
        printk("Ao header not match!!!\n");
        return HT_FAILURE;
    }

    stPubAttr.eBitwidth = E_HT_AUDIO_BIT_WIDTH_16;
    stPubAttr.eWorkmode = E_HT_AUDIO_MODE_I2S_MASTER;
    stPubAttr.u32FrmNum = 6;
    stPubAttr.u32PtNumPerFrm = HT_AUDIO_SAMPLE_PER_FRAME;
    stPubAttr.u32ChnCnt = stWavHeaderInput.channels;
    stPubAttr.eSoundmode = (stPubAttr.u32ChnCnt == 2)?E_HT_AUDIO_SOUND_MODE_STEREO:(((stPubAttr.u32ChnCnt == 1))?E_HT_AUDIO_SOUND_MODE_MONO:E_HT_AUDIO_SOUND_MODE_MAX);
    stPubAttr.eSamplerate = (HT_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;

    pstDevInfo->bDevAttrSet = TRUE;
    pstDevInfo->AoDevId = pstTestValue->u8DevId;
    stVbInfo.u32Size = HT_AUDIO_DMA_BUFFER_SIZE;
    HT_Malloc(&stVbInfo);
    pstDevInfo->u64PhyBufAddr = stVbInfo.phyAddr;
    pstDevInfo->pu8VirtAddr = stVbInfo.pu8VirtAddr;
    INIT_LIST_HEAD(&pstDevInfo->stBuffQueue);
    sema_init(&(pstDevInfo->stBuffQueueMutex),1);
    memcpy(&pstDevInfo->stDevAttr, &stPubAttr, sizeof(HT_AUDIO_PubAttr_t));

    return s32Ret;

}

static HT_S32 _HT_AO_SetVolume(HT_AO_DevInfo_t* pstDevInfo)
{
    HT_S32 s32Ret = HT_SUCCESS;
    MHAL_AUDIO_DEV AoutDevId = (MHAL_AUDIO_DEV) pstDevInfo->AoDevId;

    if(TRUE != pstDevInfo->bDevEnable)
        return HT_FAILURE;
    s32Ret = MHAL_AUDIO_SetGainOut(AoutDevId, (MS_S16)pstDevInfo->s32VolumeDb);

    return s32Ret;
}
static HT_BOOL _HT_AO_ReadOneFrame(HT_AO_DevInfo_t* pstDevInfo, HT_AO_TestValue_t *pstTestValue)
{
    HT_U32 u32RdSize;
    HT_VB_Info_t stVbInfo;
    HT_AUDIO_Frame_t *pstBuffInfo;

    stVbInfo.u32Size = HT_AUDIO_SAMPLE_PER_FRAME*2*(pstDevInfo->stDevAttr.u32ChnCnt);
    HT_Malloc(&stVbInfo);
    u32RdSize = HT_ReadFile(pstTestValue->pu8FilePath, stVbInfo.pu8VirtAddr, HT_AUDIO_SAMPLE_PER_FRAME*2*(pstDevInfo->stDevAttr.u32ChnCnt), pstDevInfo->s64FilePos);
    pstBuffInfo = kmalloc(sizeof(HT_AUDIO_Frame_t), GFP_KERNEL);
    if(pstBuffInfo==NULL)
    {
        printk("[%d]kzalloc failed\r\n",__LINE__);
        return HT_FAILURE;
    }
    memset(pstBuffInfo,0,sizeof(HT_AUDIO_Frame_t));
    pstBuffInfo->eBitwidth = pstDevInfo->stDevAttr.eBitwidth;
    pstBuffInfo->eSoundmode = pstDevInfo->stDevAttr.eSoundmode;
    if(pstDevInfo->stDevAttr.eSoundmode == E_HT_AUDIO_SOUND_MODE_MONO)
    {
        pstBuffInfo->u32Len = u32RdSize;
    }
    else
    {
        pstBuffInfo->u32Len = u32RdSize/2;
    }
    pstBuffInfo->apVirAddr = stVbInfo.pu8VirtAddr;
    pstBuffInfo->phyAddr = stVbInfo.phyAddr;
    down(&pstDevInfo->stBuffQueueMutex);
    list_add_tail(&(pstBuffInfo->stBuffNode), &pstDevInfo->stBuffQueue);
    up(&pstDevInfo->stBuffQueueMutex);
    pstDevInfo->s64FilePos += u32RdSize;

    return (u32RdSize == HT_AUDIO_SAMPLE_PER_FRAME*2*(pstDevInfo->stDevAttr.u32ChnCnt));
}

static int _HT_AO_WriteDataThread(void* data)
{
    HT_S32 s32WriteActualSize = 0;
    HT_U16 u16ChanlNum=0;
    struct list_head *pstPos = NULL;
    struct list_head *n = NULL;
    HT_AUDIO_Frame_t *pstTmpBufInfo;
    HT_AO_DevInfo_t* pstAoDevInfo = (HT_AO_DevInfo_t*)data;

    if (!pstAoDevInfo)
    {
        printk("invalid dev info\n");
        return HT_FAILURE;
    }
    printk("start audio play\n");
    while(!kthread_should_stop())
    {
        //msleep(1);
        down(&pstAoDevInfo->stBuffQueueMutex);
        if(!list_empty(&pstAoDevInfo->stBuffQueue))
        {
            list_for_each_safe(pstPos,n,&pstAoDevInfo->stBuffQueue)
            {
                pstTmpBufInfo = list_entry(pstPos,HT_AUDIO_Frame_t,stBuffNode);
                HT_AUDIO_TRANS_EMODE_TO_CHAN(u16ChanlNum, pstTmpBufInfo->eSoundmode);
                s32WriteActualSize = MHAL_AUDIO_WriteDataOut((HT_AUDIO_DEV)pstAoDevInfo->AoDevId, pstTmpBufInfo->apVirAddr, pstTmpBufInfo->u32Len * u16ChanlNum, TRUE);
                list_del(pstPos);
                kfree(pstTmpBufInfo);
                HT_Free(pstTmpBufInfo->phyAddr);
            }
        }
        up(&pstAoDevInfo->stBuffQueueMutex);
    }
    return HT_SUCCESS;
}

static HT_S32 _HT_AO_Start(HT_AO_DevInfo_t* pstDevInfo, const HT_U8 * pu8Name)
{
    HT_S32 s32Ret = HT_SUCCESS;

    if(TRUE != pstDevInfo->bDevEnable)
        return HT_FAILURE;

#if 0
    s32Ret = MHAL_AUDIO_StartPcmOut(pstDevInfo->AoDevId);
    if(s32Ret!=MHAL_SUCCESS)
    {
        printk("start pcm out failed\n");
        return HT_FAILURE;
    }
#endif
    pstDevInfo->pstAoWriteDataThread = kthread_run(_HT_AO_WriteDataThread, pstDevInfo, pu8Name);

    return s32Ret;
}

static HT_S32 _HT_AO_EnableChn(HT_AO_DevInfo_t *pAoDevInfo)
{
    HT_S32 s32Ret = HT_SUCCESS;
    HT_U32 u32BitWidthByte;
    MHAL_AUDIO_PcmCfg_t stDmaConfig;
    MHAL_AUDIO_I2sCfg_t stI2sConfig;


    if(TRUE != pAoDevInfo->bDevAttrSet)
        return HT_FAILURE;

    if(TRUE == pAoDevInfo->bDevEnable)
        return HT_FAILURE;


    // 1.0 write AO device HW configure
    memset(&stDmaConfig, 0, sizeof(MHAL_AUDIO_PcmCfg_t));

    stDmaConfig.eWidth = (MHAL_AUDIO_BitWidth_e)pAoDevInfo->stDevAttr.eBitwidth;
    stDmaConfig.eRate = (MHAL_AUDIO_Rate_e)pAoDevInfo->stDevAttr.eSamplerate;
    stDmaConfig.u16Channels = pAoDevInfo->stDevAttr.u32ChnCnt;
    stDmaConfig.bInterleaved = TRUE;

    HT_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, stDmaConfig.eWidth);

    stDmaConfig.u32PeriodSize = pAoDevInfo->stDevAttr.u32PtNumPerFrm * u32BitWidthByte * stDmaConfig.u16Channels ;
    stDmaConfig.phyDmaAddr = pAoDevInfo->u64PhyBufAddr;
    stDmaConfig.pu8DmaArea = pAoDevInfo->pu8VirtAddr;
    stDmaConfig.u32BufferSize = HT_AUDIO_DMA_BUFFER_SIZE ;
    stDmaConfig.u32StartThres = HT_AO_START_WRTIE_THRESHOLD;
    stDmaConfig.eRate= pAoDevInfo->stDevAttr.eSamplerate;
    stDmaConfig.eWidth=(HT_AUDIO_BitWidth_e)pAoDevInfo->stDevAttr.eBitwidth;
    // virual address:
    // stDmaConfig.pu8DmaArea
    s32Ret = MHAL_AUDIO_ConfigPcmOut(pAoDevInfo->AoDevId, &stDmaConfig);
    // 1.1 write AO device I2S HW configure
    memset(&stI2sConfig, 0, sizeof(MHAL_AUDIO_I2sCfg_t));
    stI2sConfig.eMode = E_MHAL_AUDIO_MODE_I2S_MASTER;
    stI2sConfig.eWidth = (HT_AUDIO_BitWidth_e)pAoDevInfo->stDevAttr.eBitwidth;
    stI2sConfig.eFmt = E_MHAL_AUDIO_I2S_FMT_LEFT_JUSTIFY;
    stI2sConfig.u16Channels = pAoDevInfo->stDevAttr.u32ChnCnt;
    s32Ret = MHAL_AUDIO_ConfigI2sOut(pAoDevInfo->AoDevId, &stI2sConfig);
    s32Ret = MHAL_AUDIO_OpenPcmOut(pAoDevInfo->AoDevId);
    if(s32Ret!=MHAL_SUCCESS)
    {
        printk("open pcm out failed\n");
        return HT_FAILURE;
    }

    pAoDevInfo->bDevEnable = TRUE;

    return s32Ret;
}
static HT_RESULT _HT_AO_Config(HT_AO_DevInfo_t* pstDevInfo, HT_AO_TestValue_t *pstTestValue)
{
    _HT_AO_SetPubAttr(pstDevInfo, pstTestValue);
    _HT_AO_EnableChn(pstDevInfo);
    _HT_AO_SetVolume(pstDevInfo);

    return HT_SUCCESS;
}

static void _HT_AO_InitVar(void)
{
    memset(gastAoDevInfo,0,sizeof(HT_AO_DevInfo_t) * HT_AO_DEV_NUM_MAX);
    memset(gastAoTestValue,0,sizeof(HT_AO_TestValue_t) * HT_AO_DEV_NUM_MAX);
}

static HT_RESULT _HT_AO_ParseStrings(HT_U16 *pau16CmdValue,HT_U8 u8CmdCnt)
{
    HT_U8 i = 0;

    if(u8CmdCnt % 3 != 0 && u8CmdCnt > 3 * HT_AO_DEV_NUM_MAX)
    {
        printk("Invalid CmdCnt:%d\n",u8CmdCnt);
        return HT_FAILURE;
    }

    for (i = 0; (i < HT_AO_DEV_NUM_MAX) && (i * 3 < u8CmdCnt); i++)
    {
        gastAoTestValue[i].u8DevId = pau16CmdValue[3 * i];
        gastAoTestValue[i].pu8FilePath = pau16CmdValue[3 * i + 1]?HT_AO_MONO_INPUT_FILE_PATH:HT_AO_STEREO_INPUT_FILE_PATH;
        gastAoTestValue[i].s32VolumeDb = pau16CmdValue[3 * i + 2];
        gastAoTestValue[i].bEn = TRUE;
        printk("Var get device id %d\n", gastAoTestValue[i].u8DevId);
        printk("Var get file:[%s]\n", (char *)gastAoTestValue[i].pu8FilePath);
        printk("Var get Volum %d\n", gastAoTestValue[i].s32VolumeDb);

    }

    return HT_SUCCESS;
}


void HT_AO_DisplayHelp(void)
{

}

HT_RESULT HT_AO(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_S32 ret=HT_SUCCESS;
    HT_U16 au16HdmiTestValue[1];
    HT_U8 i = 0;
    HT_U8 name[30];
    HT_BOOL bReadEnd = FALSE;

    CHECK_NULL_POINTER(__FUNCTION__,pau16CmdValue);
    _HT_AO_InitVar();
    _HT_AO_ParseStrings(pau16CmdValue, u8CmdCnt);

    au16HdmiTestValue[0]=2;
    HT_HDMI(au16HdmiTestValue,1);

    ret=_HT_AO_Init();
    if(ret!=HT_SUCCESS)
    {
        printk("Audio Init Failed\r\n");
        return HT_FAILURE;
    }

    for (i = 0; i < HT_AO_DEV_NUM_MAX; i++)
    {
        if (gastAoTestValue[i].bEn)
        {
            _HT_AO_Config(&gastAoDevInfo[i], &gastAoTestValue[i]);
            sprintf(name, "pcm read process %d", i);
            _HT_AO_Start(&gastAoDevInfo[i], name);
        }
    }
REPLAY:
    bReadEnd = FALSE;
    while (!bReadEnd)
    {
        for (i = 0; i < HT_AO_DEV_NUM_MAX; i++)
        {
            if (gastAoTestValue[i].bEn)
            {
                if (!_HT_AO_ReadOneFrame(&gastAoDevInfo[i], &gastAoTestValue[i]))
                {
                    gastAoDevInfo[i].s64FilePos = 0;
                    bReadEnd = TRUE;
                    break;
                }
            }
        }
        //msleep(1);
    }
    goto REPLAY;

    for (i = 0; i < HT_AO_DEV_NUM_MAX; i++)
    {
        if (gastAoTestValue[i].bEn)
        {
            _HT_AO_Stop(&gastAoDevInfo[i]);
        }
    }

    ret=_HT_AO_DeInit();
    if(ret!=HT_SUCCESS)
    {
        printk("Audio DeInit Failed\r\n");
        return HT_FAILURE;
    }
    return ret;
}

