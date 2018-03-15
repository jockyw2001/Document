#ifndef __ht_aio_datatype_h__
#define __ht_aio_datatype_h__

#include "ht_common_datatype.h"

#define HT_AUDIO_MAX_SAMPLES_PER_FRAME     2048
#define HT_AUDIO_MAX_FRAME_NUM             6
#define HT_AUDIO_SAMPLE_PER_FRAME          128
#define HT_AUDIO_DMA_BUFFER_SIZE          128*100

#define HT_AUDIO_TRANS_EMODE_TO_CHAN(u32Chan, eSoundmode) \
    switch(eSoundmode)  \
    {   \
        case E_HT_AUDIO_SOUND_MODE_MONO:        \
            u32Chan = 1;    \
            break;      \
        case E_HT_AUDIO_SOUND_MODE_STEREO:   \
            u32Chan = 2;    \
            break;  \
        default:    \
            u32Chan = 0;    \
            printk("eSoundmode is illegal = %u.\n", u32Chan); \
    }

#define HT_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, eWidth)          \
    switch(eWidth) \
    {   \
        case E_HT_AUDIO_BIT_WIDTH_16:   \
            u32BitWidthByte = 2;    \
        break;  \
        case E_HT_AUDIO_BIT_WIDTH_24:   \
            u32BitWidthByte = 4;    \
            break;  \
        default:    \
            u32BitWidthByte = 0; \
            printk("BitWidth is illegal = %u.\n", eWidth); \
            break; \
    }

typedef enum
{
    E_HT_AUDIO_SOUND_MODE_MONO =0, /* mono */
    E_HT_AUDIO_SOUND_MODE_STEREO =1, /* stereo */
    E_HT_AUDIO_SOUND_MODE_MAX,
}HT_AUDIO_SoundMode_e;

typedef enum
{
    E_HT_AUDIO_BIT_WIDTH_16 =0, /* 16bit width */
    E_HT_AUDIO_BIT_WIDTH_24 =1, /* 24bit width */
    E_HT_AUDIO_BIT_WIDTH_MAX,
}HT_AUDIO_BitWidth_e;

typedef enum
{
    E_HT_AUDIO_SAMPLE_RATE_8000 =8000, /* 8kHz sampling rate */
    E_HT_AUDIO_SAMPLE_RATE_16000 =16000, /* 16kHz sampling rate */
    E_HT_AUDIO_SAMPLE_RATE_32000 =32000, /* 32kHz sampling rate */
    E_HT_AUDIO_SAMPLE_RATE_48000 =48000, /* 48kHz sampling rate */
    E_HT_AUDIO_SAMPLE_RATE_INVALID,
}HT_AUDIO_SampleRate_e;

typedef enum
{
    E_HT_AUDIO_MODE_I2S_MASTER, /* I2S master mode */
    E_HT_AUDIO_MODE_I2S_SLAVE, /* I2S slave mode */
    E_HT_AUDIO_MODE_TDM_MASTER, /* TDM master mode */
    E_HT_AUDIO_MODE_MAX,
}HT_AUDIO_Mode_e;

typedef enum
{
    E_HT_AUDIO_HPF_FREQ_80 = 80, /* 80Hz */
    E_HT_AUDIO_HPF_FREQ_120 = 120, /* 120Hz */
    E_HT_AUDIO_HPF_FREQ_150 = 150, /* 150Hz */
    E_HT_AUDIO_HPF_FREQ_INVALID,
}HT_AUDIO_HpfFreq_e;

typedef struct HT_AUDIO_HpfConfig_s
{
    HT_BOOL bUsrMode;
    HT_AUDIO_HpfFreq_e eHpfFreq; /*freq to be processed*/
}HT_AUDIO_HpfConfig_t;

typedef struct HT_AUDIO_AnrConfig_s
{
    HT_BOOL bUsrMode;
    HT_S16 s16NrIntensity;
    HT_S32 s32Reserved;
}HT_AUDIO_AnrConfig_t;

typedef struct HT_AUDIO_AgcConfig_s
{
    HT_BOOL bUsrMode;
    HT_S8 s8TargetLevel;
    HT_S8 s8NoiseFloor;
    HT_S8 s8MaxGain;
    HT_S32 s32Reserved;
}HT_AUDIO_AgcConfig_t;

typedef struct HT_AUDIO_EqConfig_s
{
    HT_S8   au8GaindB[256];
    HT_S32  s32Reserved;
}HT_AUDIO_EqConfig_t;

typedef struct HT_AUDIO_PubAttr_s
{
    HT_AUDIO_SampleRate_e eSamplerate; /*sample rate*/
    HT_AUDIO_BitWidth_e eBitwidth; /*bitwidth*/
    HT_AUDIO_Mode_e eWorkmode; /*master or slave mode*/
    HT_AUDIO_SoundMode_e eSoundmode; /*mono or stereo*/
    HT_U32 u32FrmNum; /*frame num in buffer*/
    HT_U32 u32PtNumPerFrm; /*number of samples*/
    HT_U32 u32CodecChnCnt; /*channel number on Codec */
    HT_U32 u32ChnCnt;

}HT_AUDIO_PubAttr_t;

typedef HT_S32 HT_AUDIO_CHN;
typedef HT_S32 HT_AUDIO_DEV;

#endif


