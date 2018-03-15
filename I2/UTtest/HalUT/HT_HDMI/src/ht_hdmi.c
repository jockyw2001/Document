#include <linux/string.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include "ht_common.h"
#include "apiHDMITx.h"
#include "apiDAC.h"
#include "drvGPIO.h"

typedef enum
{
    E_HT_HDMI_DAC_TABTYPE_INIT,
    E_HT_HDMI_DAC_TABTYPE_INIT_GPIO,
    E_HT_HDMI_DAC_TABTYPE_INIT_SC,
    E_HT_HDMI_DAC_TABTYPE_INIT_MOD,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDGEN,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT_Divider,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT_Divider,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT_Divider,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT,
    E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT_Divider,
    E_HT_HDMI_DAC_TABTYPE_Gamma,
    E_HT_HDMI_DAC_TABTYPE_SC1_INIT,
    E_HT_HDMI_DAC_TABTYPE_SC1_INIT_SC,
    E_HT_HDMI_DAC_TABTYPE_NUMS,
    E_HT_HDMI_DAC_TABTYPE_MAX,
}HT_HDMI_DAC_TAB_TYPE; //Utopia enum

typedef enum
{
    E_HT_HDMI_TIMING_480_60I         = 0,
    E_HT_HDMI_TIMING_480_60P         = 1,
    E_HT_HDMI_TIMING_576_50I         = 2,
    E_HT_HDMI_TIMING_576_50P         = 3,
    E_HT_HDMI_TIMING_720_50P         = 4,
    E_HT_HDMI_TIMING_720_60P         = 5,
    E_HT_HDMI_TIMING_1080_50I        = 6,
    E_HT_HDMI_TIMING_1080_50P        = 7,
    E_HT_HDMI_TIMING_1080_60I        = 8,
    E_HT_HDMI_TIMING_1080_60P        = 9,
    E_HT_HDMI_TIMING_1080_30P        = 10,
    E_HT_HDMI_TIMING_1080_25P        = 11,
    E_HT_HDMI_TIMING_1080_24P        = 12,
    E_HT_HDMI_TIMING_4K2K_30P        = 13,
    E_HT_HDMI_TIMING_1440_50P        = 14,
    E_HT_HDMI_TIMING_1440_60P        = 15,
    E_HT_HDMI_TIMING_1440_24P        = 16,
    E_HT_HDMI_TIMING_1440_30P        = 17,
    E_HT_HDMI_TIMING_1470_50P        = 18,
    E_HT_HDMI_TIMING_1470_60P        = 19,
    E_HT_HDMI_TIMING_1470_24P        = 20,
    E_HT_HDMI_TIMING_1470_30P        = 21,
    E_HT_HDMI_TIMING_1920x2205_24P   = 22,
    E_HT_HDMI_TIMING_1920x2205_30P   = 23,
    E_HT_HDMI_TIMING_4K2K_25P        = 24,
    E_HT_HDMI_TIMING_4K1K_60P        = 25,
    E_HT_HDMI_TIMING_4K2K_60P        = 26,
    E_HT_HDMI_TIMING_4K2K_24P        = 27,
    E_HT_HDMI_TIMING_4K2K_50P        = 28,
    E_HT_HDMI_TIMING_2205_24P        = 29,
    E_HT_HDMI_TIMING_4K1K_120P       = 30,
    E_HT_HDMI_TIMING_4096x2160_24P   = 31,
    E_HT_HDMI_TIMING_4096x2160_25P   = 32,
    E_HT_HDMI_TIMING_4096x2160_30P   = 33,
    E_HT_HDMI_TIMING_4096x2160_50P   = 34,
    E_HT_HDMI_TIMING_4096x2160_60P   = 35,
    E_HT_HDMI_TIMING_1024x768_60P    = 36,
    E_HT_HDMI_TIMING_1280x1024_60P   = 37,
    E_HT_HDMI_TIMING_1440x900_60P    = 38,
    E_HT_HDMI_TIMING_1600x1200_60P   = 39,
    E_HT_HDMI_TIMING_MAX,
}HT_HDMI_TimingType_e;
typedef enum
{
    E_HT_HDMI_OUTPUT_MODE_HDMI = 0,
    E_HT_HDMI_OUTPUT_MODE_HDMI_HDCP,
    E_HT_HDMI_OUTPUT_MODE_DVI,
    E_HT_HDMI_OUTPUT_MODE_DVI_HDCP,
    E_HT_HDMI_OUTPUT_MODE_MAX,
}HT_HDMI_OutputMode_e;

typedef enum
{
    E_HT_HDMI_COLOR_TYPE_RGB444 = 0,
    E_HT_HDMI_COLOR_TYPE_YCBCR422,
    E_HT_HDMI_COLOR_TYPE_YCBCR444,
    E_HT_HDMI_COLOR_TYPE_YCBCR420,
    E_HT_HDMI_COLOR_TYPE_MAX
}HT_HDMI_ColorType_e;

typedef enum
{
    E_HT_HDMI_DEEP_COLOR_24BIT = 0x00,
    E_HT_HDMI_DEEP_COLOR_30BIT,
    E_HT_HDMI_DEEP_COLOR_36BIT,
    E_HT_HDMI_DEEP_COLOR_48BIT,
    E_HT_HDMI_DEEP_COLOR_MAX,
}HT_HDMI_DeepColor_e;

typedef enum
{
    E_HT_HDMI_AUDIO_SAMPLERATE_UNKNOWN      = 0,
    E_HT_HDMI_AUDIO_SAMPLERATE_32K          = 1,
    E_HT_HDMI_AUDIO_SAMPLERATE_44K          = 2,
    E_HT_HDMI_AUDIO_SAMPLERATE_48K          = 3,
    E_HT_HDMI_AUDIO_SAMPLERATE_88K          = 4,
    E_HT_HDMI_AUDIO_SAMPLERATE_96K          = 5,
    E_HT_HDMI_AUDIO_SAMPLERATE_176K         = 6,
    E_HT_HDMI_AUDIO_SAMPLERATE_192K         = 7,
    E_HT_HDMI_AUDIO_SAMPLERATE_MAX,
}HT_HDMI_SampleRate_e;
typedef enum
{
    E_HT_HDMI_BIT_DEPTH_8   = 8,
    E_HT_HDMI_BIT_DEPTH_16  = 16,
    E_HT_HDMI_BIT_DEPTH_18  = 18,
    E_HT_HDMI_BIT_DEPTH_20  = 20,
    E_HT_HDMI_BIT_DEPTH_24  = 24,
    E_HT_HDMI_BIT_DEPTH_32  = 32,
    E_HT_HDMI_BIT_DEPTH_MAX
}HT_HDMI_BitDepth_e;
typedef enum
{
    E_HT_HDMI_ACODE_PCM = 0,
    E_HT_HDMI_ACODE_NON_PCM,
    E_HT_HDMI_ACODE_MAX
}HT_HDMI_AudioCodeType_e;
typedef enum
{
   E_HT_HDMI_BAR_INFO_NOT_VALID = 0,             /**< Bar Data not valid */
   E_HT_HDMI_BAR_INFO_V,                         /**< Vertical bar data valid */
   E_HT_HDMI_BAR_INFO_H,                         /**< Horizental bar data valid */
   E_HT_HDMI_BAR_INFO_VH,                        /**< Horizental and Vertical bar data valid */
   E_HT_HDMI_BAR_INFO_MAX
}HT_HDMI_BarInfo_e;
typedef enum
{
    E_HT_HDMI_SCAN_INFO_NO_DATA = 0,             /**< No Scan information*/
    E_HT_HDMI_SCAN_INFO_OVERSCANNED,             /**< Scan information, Overscanned (for television) */
    E_HT_HDMI_SCAN_INFO_UNDERSCANNED,            /**< Scan information, Underscanned (for computer) */
    E_HT_HDMI_SCAN_INFO_FUTURE,
    E_HT_HDMI_SCAN_INFO_MAX
}HT_HDMI_ScanInfo_e;
typedef enum
{
    E_HT_HDMI_COLORSPACE_NO_DATA = 0,
    E_HT_HDMI_COLORSPACE_ITU601,
    E_HT_HDMI_COLORSPACE_ITU709,
    E_HT_HDMI_COLORSPACE_EXTENDED,
    E_HT_HDMI_COLORSPACE_XVYCC_601,
    E_HT_HDMI_COLORSPACE_XVYCC_709,
    E_HT_HDMI_COLORSPACE_MAX
}HT_HDMI_ColorSpace_e;
typedef enum
{
    E_HT_HDMI_ASPECT_RATIO_INVALID = 0,       /**< unknown aspect ratio */
    E_HT_HDMI_ASPECT_RATIO_4TO3,              /**< 4:3 */
    E_HT_HDMI_ASPECT_RATIO_16TO9,             /**< 16:9 */
    E_HT_HDMI_ASPECT_RATIO_SQUARE,            /**< square */
    E_HT_HDMI_ASPECT_RATIO_14TO9,             /**< 14:9 */
    E_HT_HDMI_ASPECT_RATIO_221TO1,            /**< 221:100 */
    E_HT_HDMI_ASPECT_RATIO_ZOME,              /**< default not support, use source's aspect ratio to display */
    E_HT_HDMI_ASPECT_RATIO_FULL,              /**< default not support, full screen display */
    E_HT_HDMI_ASPECT_RATIO_MAX
}HT_HDMI_AspectRatio_e;
typedef enum
{
    E_HT_HDMI_RGB_QUANTIZATION_DEFAULT_RANGE = 0,    /**< Defaulr range, it depends on the video format */
    E_HT_HDMI_RGB_QUANTIZATION_LIMITED_RANGE,        /**< Limited quantization range of 220 levels when receiving a CE video format*/
    E_HT_HDMI_RGB_QUANTIZATION_FULL_RANGE,           /**< Full quantization range of 256 levels when receiving an IT video format*/
    E_HT_HDMI_RGB_QUANTIZATION_MAX
}HT_HDMI_RgbQuanRage_e;
typedef enum
{
    E_HT_HDMI_YCC_QUANTIZATION_LIMITED_RANGE = 0,    /**< Limited quantization range of 220 levels when receiving a CE video format*/
    E_HT_HDMI_YCC_QUANTIZATION_FULL_RANGE,           /**< Full quantization range of 256 levels when receiving an IT video format*/
    E_HT_HDMI_YCC_QUANTIZATION_MAX
}HT_HDMI_YccQuanRage_e;

typedef enum
{
    E_HT_HDMI_CONTNET_GRAPHIC = 0,               /**< Graphics type*/
    E_HT_HDMI_CONTNET_PHOTO,                     /**< Photo type*/
    E_HT_HDMI_CONTNET_CINEMA,                    /**< Cinema type*/
    E_HT_HDMI_CONTNET_GAME,                      /**< Game type*/
    E_HT_HDMI_CONTNET_MAX
}HT_HDMI_ContentType_e;
typedef enum
{
    E_HT_HDMI_AUDIO_CODING_REFER_STREAM_HEAD = 0,
    E_HT_HDMI_AUDIO_CODING_PCM,
    E_HT_HDMI_AUDIO_CODING_AC3,
    E_HT_HDMI_AUDIO_CODING_MPEG1,
    E_HT_HDMI_AUDIO_CODING_MP3,
    E_HT_HDMI_AUDIO_CODING_MPEG2,
    E_HT_HDMI_AUDIO_CODING_AAC,
    E_HT_HDMI_AUDIO_CODING_DTS,
    E_HT_HDMI_AUDIO_CODING_DDPLUS,
    E_HT_HDMI_AUDIO_CODING_MLP,
    E_HT_HDMI_AUDIO_CODING_WMA,
    E_HT_HDMI_AUDIO_CODING_MAX
}HT_HDMI_AudioCodingType_e;

typedef enum
{
    E_HT_HDMI_ID_0 = 0,
    E_HT_HDMI_ID_MAX
}HT_HDMI_DeviceId_e;

typedef struct HT_HDMI_VideoAttr_s
{
    HT_BOOL bEnableVideo;
    HT_HDMI_TimingType_e eTimingType;
    HT_HDMI_OutputMode_e eOutputMode;
    HT_HDMI_ColorType_e eColorType;
    HT_HDMI_DeepColor_e eDeepColorMode;
}HT_HDMI_VideoAttr_t;

typedef struct HT_HDMI_AudioAttr_s
{
    HT_BOOL bEnableAudio;
    HT_BOOL bIsMultiChannel;// 0->2channel 1->8channel
    HT_HDMI_SampleRate_e eSampleRate;
    HT_HDMI_BitDepth_e eBitDepth;
    HT_HDMI_AudioCodeType_e eCodeType;
}HT_HDMI_AudioAttr_t;

typedef struct HT_HDMI_EnInfoFrame_s
{
    HT_BOOL bEnableAviInfoFrame;
    HT_BOOL bEnableAudInfoFrame;
    HT_BOOL bEnableSpdInfoFrame;
}HT_HDMI_EnInfoFrame_t;

typedef struct HT_HDMI_Attr_s
{
    HT_BOOL bConnect;
    HT_HDMI_VideoAttr_t stVideoAttr;
    HT_HDMI_AudioAttr_t stAudioAttr;
    HT_HDMI_EnInfoFrame_t stEnInfoFrame;
}HT_HDMI_Attr_t;

typedef struct HT_HDMI_InerAttr_s
{
    HDMITX_OUTPUT_MODE eHdmiTxMode; //Utopia Enum
    HDMITX_VIDEO_COLOR_FORMAT eHdmiTxInColor;
    HDMITX_VIDEO_COLOR_FORMAT eHdmiTxOutColor;
    HDMITX_VIDEO_COLORDEPTH_VAL eHdmiTxColorDepth;
    HDMITX_AUDIO_FREQUENCY eAudioFreq;
    HDMITX_AUDIO_CHANNEL_COUNT eAudioChCnt;
    HDMITX_AUDIO_CODING_TYPE eAudioCodeType;
    HDMITX_AUDIO_SOURCE_FORMAT eAudioSrcFmt;
    HDMITX_VIDEO_TIMING eHdmiVideoTiming;
}HT_HDMI_InerAttr_t;

typedef struct HT_HDMIAviInforFrameVer_s
{
    HT_HDMI_TimingType_e eTimingType;
    HT_HDMI_OutputMode_e eOutputMode;
    HT_BOOL bActiveInfoPresent;
    HT_HDMI_BarInfo_e eBarInfo;
    HT_HDMI_ScanInfo_e eScanInfo;
    HT_HDMI_ColorSpace_e eColorimetry;
    HT_HDMI_AspectRatio_e eAspectRatio;
    HT_HDMI_AspectRatio_e eActiveAspectRatio;
    HT_HDMI_RgbQuanRage_e eRgbQuantization;
    HT_BOOL bIsItContent;
    HT_U32  u32PixelRepetition;
    HT_HDMI_ContentType_e eContentType;
    HT_HDMI_YccQuanRage_e eYccQuantization;
    HT_U32 u32LineNEndofTopBar;
    HT_U32 u32LineNStartofBotBar;
    HT_U32 u32PixelNEndofLeftBar;
    HT_U32 u32PixelNStartofRightBar;
} HT_HDMI_AviInfoFrameVer_t;

typedef struct HT_HDMI_AudInfoFrameVer_s
{
    HT_U32 u32ChannelCount;
    HT_HDMI_AudioCodingType_e eAudioCodingType;
    HT_U32 u32SampleSize;
    HT_U32 u32SamplingFrequency;
    HT_U32 u32ChannelAlloc;
    HT_U32 u32LevelShift;
    HT_BOOL bDownMixInhibit;
}HT_HDMI_AudInfoFrameVer_t;

typedef struct HT_HDMI_SpdInfoFrame_s
{
    HT_U8 au8VendorName[8];
    HT_U8 au8ProductDescription[16];
}HT_HDMI_SpdInfoFrame_t;

typedef struct HT_HDMI_ResMgr_s
{
    HT_BOOL bInitFlag;
    HT_BOOL bHdmiTxRuning;
    HT_BOOL bAvMute;
    HT_BOOL bPowerOn;
    HT_U32  u32swVerion;
    HT_HDMI_Attr_t stAttr;
    HT_HDMI_InerAttr_t stInerAttr;
    HT_HDMI_AviInfoFrameVer_t stAviInfoFrame;
    HT_HDMI_AudInfoFrameVer_t stAudInfoFrame;
    HT_HDMI_SpdInfoFrame_t stSpdInfoFrame;
} HT_HDMI_ResMgr_t;

#define HT_HDMI_HDMITX_DRIVER_DBG           0x01
#define HT_HDMI_HDMITX_DRIVER_DBG_HDCP      0x02

#define HT_HDMI_BLOCK0_INDEX 0
#define HT_HDMI_BLOCK1_INDEX 1
#define HT_HDMI_BLOCK_SIZE 128 /* Size per block */

#define HT_HDMI_EDID_CHECK_TIMEOUT 100
#define HT_HDMI_EDID_CHECK_DELAY   10

static HT_HDMI_ResMgr_t gstHdmiMgr = {0};
static HT_U8 gu8HdmiGpioPin = 26;

static HT_U32 _GetTime(void) {
    struct timeval stTimeVal;
    HT_U32 u32Ms;
    do_gettimeofday(&stTimeVal);
    u32Ms = (stTimeVal.tv_sec * 1000) + (stTimeVal.tv_usec / 1000);
    if(0 == u32Ms)
    {
        u32Ms = 1;
    }
    return u32Ms;
}

static HT_U32 _DiffFromNow(HT_U32 u32Time) {
    HT_U32 u32Now;
    struct timeval stTimeVal;
    do_gettimeofday(&stTimeVal);
    u32Now = (stTimeVal.tv_sec * 1000) + (stTimeVal.tv_usec / 1000);
    if(u32Now >= u32Time)
    {
        return u32Now - u32Time;
    }
    return (0xFFFFFFFF - u32Time) + u32Now;
}
static HT_BOOL _HT_HDMI_IsReceiverSupportYPbPr(void)
{
    HT_U8 au8BlockData[HT_HDMI_BLOCK_SIZE] = {0};

    if(MApi_HDMITx_GetEDIDData(au8BlockData, HT_HDMI_BLOCK1_INDEX) == FALSE)
    {
        printk("HDMITX %s[%d] Get EDID fail \n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if(0x00 == (au8BlockData[0x03] & 0x30))
    {
        printk("HDMITX %s[%d] Rx Not Support YCbCr \n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    else
    {
        printk("HDMITX %s[%d] Rx Support YUV444 or YUV422 \n", __FUNCTION__, __LINE__);
        return TRUE;
    }
    return FALSE;
}
static E_OUTPUT_VIDEO_TIMING_TYPE _HT_HDMI_TransTimingFmt(HT_HDMI_TimingType_e eTimingType)
{
    switch (eTimingType)
    {
        case E_HT_HDMI_TIMING_720_50P:
            return E_RES_1280x720P_50Hz;
        case E_HT_HDMI_TIMING_720_60P:
            return E_RES_1280x720P_60Hz;
        case E_HT_HDMI_TIMING_1080_50P:
            return E_RES_1920x1080P_50Hz;
        case E_HT_HDMI_TIMING_1080_60P:
            return E_RES_1920x1080P_60Hz;
        case E_HT_HDMI_TIMING_4K2K_30P:
            return E_RES_3840x2160P_30Hz;
        case E_HT_HDMI_TIMING_4K2K_60P:
            return E_RES_3840x2160P_30Hz;
        case E_HT_HDMI_TIMING_1024x768_60P:
            return E_RES_1024x768P_60Hz;
        case E_HT_HDMI_TIMING_1280x1024_60P:
            return E_RES_1280x1024P_60Hz;
        case E_HT_HDMI_TIMING_1440x900_60P:
            return E_RES_1440x900P_60Hz;
        case E_HT_HDMI_TIMING_1600x1200_60P:
            return E_RES_1600x1200P_60Hz;
        default:
            printk("Unsupported Timing.\n");
            return HT_FAILURE;
    }
}
static void _HT_HDMI_ChkColorFormatForSpecTiming(HT_HDMI_TimingType_e eTimingType)
{
    HT_U8 au8BlockData[128] = {0};

    HDMITX_VIDEO_COLOR_FORMAT eHdmiTxInColor = HDMITX_VIDEO_COLOR_RGB444;
    HDMITX_VIDEO_COLOR_FORMAT eHdmiTxOutColor = HDMITX_VIDEO_COLOR_RGB444;

    HT_U32 u32StartTime = _GetTime();

    while (TRUE != MApi_HDMITx_GetEDIDData(au8BlockData, HT_HDMI_BLOCK1_INDEX))
    {
        if (_DiffFromNow(u32StartTime) > HT_HDMI_EDID_CHECK_TIMEOUT)
        {
            if (MApi_HDMITx_EdidChecking())
            {
                MApi_HDMITx_GetEDIDData(au8BlockData, HT_HDMI_BLOCK1_INDEX);
                break;
            }
            else
            {
                printk("HDMITX %s[%d] EDID checking failed!!\n", __FUNCTION__, __LINE__);
                return;
            }
        }
        msleep(10);
    }
    switch (eTimingType)
    {
        case E_HT_HDMI_TIMING_4K2K_60P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_3840x2160p_60Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4k2k@60Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4k2k@60Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        case E_HT_HDMI_TIMING_4K2K_50P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_3840x2160p_50Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4k2k@50Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4k2k@50Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        case E_HT_HDMI_TIMING_4K2K_30P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_3840x2160p_30Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4k2k@30Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4k2k@30Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        case E_HT_HDMI_TIMING_4096x2160_60P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_4096x2160p_60Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4096x2160@60Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4096x2160@60Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        case E_HT_HDMI_TIMING_4096x2160_50P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_4096x2160p_50Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4096x2160@50Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4096x2160@50Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        case E_HT_HDMI_TIMING_4096x2160_30P:
        {
            HDMITX_EDID_COLOR_FORMAT ColorFmt = HDMITX_EDID_Color_YCbCr_444;
            MApi_HDMITx_GetColorFormatFromEDID(HDMITX_RES_4096x2160p_30Hz, &ColorFmt);
            if(ColorFmt != HDMITX_EDID_Color_YCbCr_420)
            {
                // support 4096x2160@30Hz YUV444
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                // only support 4096x2160@30Hz YUV420
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV420;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV420;
            }
            break;
        }
        default:
        {
            if(TRUE == _HT_HDMI_IsReceiverSupportYPbPr())
            {
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
            }
            else
            {
                eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
                eHdmiTxOutColor = HDMITX_VIDEO_COLOR_RGB444;
            }
            break;
        }
    }
    gstHdmiMgr.stInerAttr.eHdmiTxInColor = eHdmiTxInColor;
    gstHdmiMgr.stInerAttr.eHdmiTxOutColor = eHdmiTxOutColor;
    return;
}

static HT_RESULT _HT_HDMI_Init(void)
{
    if (gstHdmiMgr.bInitFlag)
    {
        printk("%s: Module has been initialized!\n",__FUNCTION__);
        return HT_SUCCESS;
    }
    gstHdmiMgr.stInerAttr.eAudioChCnt = HDMITX_AUDIO_CH_2;
    gstHdmiMgr.stInerAttr.eAudioCodeType = HDMITX_AUDIO_PCM;
    gstHdmiMgr.stInerAttr.eAudioFreq = HDMITX_AUDIO_48K;
    gstHdmiMgr.stInerAttr.eAudioSrcFmt = HDMITX_AUDIO_FORMAT_PCM;
    gstHdmiMgr.stInerAttr.eHdmiTxColorDepth = HDMITX_VIDEO_CD_NoID;
    gstHdmiMgr.stInerAttr.eHdmiTxInColor = HDMITX_VIDEO_COLOR_YUV444;
    gstHdmiMgr.stInerAttr.eHdmiTxOutColor = HDMITX_VIDEO_COLOR_YUV444;
    gstHdmiMgr.stInerAttr.eHdmiTxMode = HDMITX_HDMI;
    gstHdmiMgr.stInerAttr.eHdmiVideoTiming = HDMITX_RES_1920x1080p_60Hz;

    gstHdmiMgr.stAttr.stVideoAttr.bEnableVideo = TRUE;
    gstHdmiMgr.stAttr.stVideoAttr.eTimingType = E_HT_HDMI_TIMING_1080_60P;
    gstHdmiMgr.stAttr.stVideoAttr.eColorType = E_HT_HDMI_COLOR_TYPE_YCBCR444;
    gstHdmiMgr.stAttr.stVideoAttr.eOutputMode = E_HT_HDMI_OUTPUT_MODE_HDMI;
    gstHdmiMgr.stAttr.stVideoAttr.eDeepColorMode = E_HT_HDMI_DEEP_COLOR_MAX;
    gstHdmiMgr.stAttr.stAudioAttr.bEnableAudio = TRUE;
    gstHdmiMgr.stAttr.stAudioAttr.bIsMultiChannel = 0;
    gstHdmiMgr.stAttr.stAudioAttr.eBitDepth = E_HT_HDMI_BIT_DEPTH_16;
    gstHdmiMgr.stAttr.stAudioAttr.eCodeType = E_HT_HDMI_ACODE_PCM;
    gstHdmiMgr.stAttr.stAudioAttr.eSampleRate = E_HT_HDMI_AUDIO_SAMPLERATE_48K;

    gstHdmiMgr.stAttr.stEnInfoFrame.bEnableAudInfoFrame = FALSE;
    gstHdmiMgr.stAttr.stEnInfoFrame.bEnableAviInfoFrame = FALSE;
    gstHdmiMgr.stAttr.stEnInfoFrame.bEnableSpdInfoFrame = FALSE;

    gstHdmiMgr.bHdmiTxRuning = FALSE;
    gstHdmiMgr.bPowerOn = FALSE;
    gstHdmiMgr.bAvMute  = FALSE;

    mdrv_gpio_init();
    MApi_HDMITx_SetHPDGpioPin(gu8HdmiGpioPin);
    MApi_HDMITx_SetDbgLevel(HT_HDMI_HDMITX_DRIVER_DBG | HT_HDMI_HDMITX_DRIVER_DBG_HDCP);
    if (FALSE == MApi_DAC_Init())
    {
        printk("DEVICE_HDMITX %s[%d] MApi_DAC_Init Fail \n", __FUNCTION__, __LINE__);
        return HT_FAILURE;
    }
    MApi_DAC_SetClkInv(TRUE, TRUE);
    if (FALSE == MApi_HDMITx_Init())
    {
        printk("DEVICE_HDMITX %s[%d] MApi_HDMITx_Init Fail \n", __FUNCTION__, __LINE__);
        return HT_FAILURE;
    }
    gstHdmiMgr.bInitFlag = TRUE;
    return HT_SUCCESS;
}

static HT_RESULT _HT_HDMI_Open(HT_HDMI_DeviceId_e eHdmi)
{
    if (!gstHdmiMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return HT_FAILURE;
    }
    //MApi_HDMITx_UnHDCPRxControl(E_UNHDCPRX_BLUE_SCREEN); /* Unused HDCP */
    //MApi_HDMITx_HDCPRxFailControl(E_HDCPRXFail_BLUE_SCREEN);
    MApi_HDMITx_TurnOnOff(TRUE);
    MApi_HDMITx_SetHDMITxMode(gstHdmiMgr.stInerAttr.eHdmiTxMode);
    MApi_HDMITx_SetHDMITxMode_CD(gstHdmiMgr.stInerAttr.eHdmiTxMode, gstHdmiMgr.stInerAttr.eHdmiTxColorDepth);
    MApi_HDMITx_SetColorFormat(gstHdmiMgr.stInerAttr.eHdmiTxInColor, gstHdmiMgr.stInerAttr.eHdmiTxOutColor);
    MApi_HDMITx_SetVideoOutputTiming(gstHdmiMgr.stInerAttr.eHdmiVideoTiming);
    MApi_HDMITx_SetAudioConfiguration(gstHdmiMgr.stInerAttr.eAudioFreq, gstHdmiMgr.stInerAttr.eAudioChCnt,gstHdmiMgr.stInerAttr.eAudioCodeType);
    MApi_DAC_SetYPbPrOutputTiming(_HT_HDMI_TransTimingFmt(gstHdmiMgr.stAttr.stVideoAttr.eTimingType));
    MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT_Divider);
    MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT);

    MApi_HDMITx_SetVideoOnOff(TRUE);
    MApi_HDMITx_SetAudioOnOff(TRUE);
    MApi_HDMITx_Exhibit();
    //msleep(1000);
    gstHdmiMgr.bHdmiTxRuning = TRUE;

    return HT_SUCCESS;
}
static HT_S32 _HT_HDMI_Close(HT_HDMI_DeviceId_e eHdmi)
{
    if (!gstHdmiMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return HT_FAILURE;
    }
    if (FALSE == gstHdmiMgr.bHdmiTxRuning)
    {
        printk("HDMITX %s[%d] Already DisConnect \n", __FUNCTION__, __LINE__);
    }
    else
    {
        printk("HDMITX %s[%d] DisConnect \n", __FUNCTION__, __LINE__);
        MApi_HDMITx_TurnOnOff(FALSE);
        MApi_HDMITx_SetTMDSOnOff(FALSE);
        //MApi_HDMITx_SetVideoOnOff(FALSE);
        MApi_HDMITx_SetAudioOnOff(FALSE);
        gstHdmiMgr.bHdmiTxRuning = FALSE;
    }
    return HT_SUCCESS;
}
static HT_S32 _HT_HDMI_Start(HT_HDMI_DeviceId_e eHdmi)
{
    if (!gstHdmiMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return HT_FAILURE;
    }
    if ((FALSE == gstHdmiMgr.bPowerOn) && (eHdmi < E_HT_HDMI_ID_MAX))
    {
        MApi_HDMITx_DisableTMDSCtrl(FALSE);
        MApi_HDMITx_SetTMDSOnOff(TRUE);
        MApi_HDMITx_Exhibit();
        gstHdmiMgr.bPowerOn = TRUE;
    }
    return HT_SUCCESS;
}
static HT_S32 _HT_HDMI_SetAudioConfiguration(HT_HDMI_AudioAttr_t *stAudioAttr)
{
    HDMITX_AUDIO_FREQUENCY eAudioFreq = HDMITX_AUDIO_FREQ_NO_SIG;
    HDMITX_AUDIO_CHANNEL_COUNT eAudioChCnt = 2;
    HDMITX_AUDIO_CODING_TYPE eAudioCodeType = HDMITX_AUDIO_PCM;

    printk("HDMITX %s[%d] Frequency:%d, ChannelCount:%d, CodingType:%d \n", __FUNCTION__,__LINE__,
        stAudioAttr->eSampleRate, stAudioAttr->bIsMultiChannel, stAudioAttr->eCodeType);

    switch (stAudioAttr->eSampleRate)
    {
        case E_HT_HDMI_AUDIO_SAMPLERATE_UNKNOWN:
            eAudioFreq = HDMITX_AUDIO_FREQ_NO_SIG;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_32K:
            eAudioFreq = HDMITX_AUDIO_32K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_44K:
            eAudioFreq = HDMITX_AUDIO_44K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_48K:
            eAudioFreq = HDMITX_AUDIO_48K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_88K:
            eAudioFreq = HDMITX_AUDIO_88K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_96K:
            eAudioFreq = HDMITX_AUDIO_96K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_176K:
            eAudioFreq = HDMITX_AUDIO_176K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_192K:
            eAudioFreq = HDMITX_AUDIO_192K;
            break;
        case E_HT_HDMI_AUDIO_SAMPLERATE_MAX:
        default:
            printk("HDMITX %s[%d] Wrong Mode of HT_HDMI_SampleRate_e \n", __FUNCTION__,__LINE__);
            return HT_FAILURE;
    }
    switch (stAudioAttr->bIsMultiChannel)
    {
        case 1:
            eAudioChCnt = HDMITX_AUDIO_CH_8;
            break;
        case 0:
        default:
            eAudioChCnt = HDMITX_AUDIO_CH_2;
            break;
    }
    switch (stAudioAttr->eCodeType)
    {
        case E_HT_HDMI_ACODE_PCM:
            eAudioCodeType = HDMITX_AUDIO_PCM;
            break;
        case E_HT_HDMI_ACODE_NON_PCM:
            eAudioCodeType = HDMITX_AUDIO_NONPCM;
            break;
        default:
            printk("HDMITX %s[%d] Wrong Mode of HT_HDMI_AudioCodeType_e \n", __FUNCTION__,__LINE__);
            return HT_FAILURE;
    }

    if(gstHdmiMgr.bHdmiTxRuning)
    {
        MApi_HDMITx_SetAudioConfiguration(eAudioFreq, eAudioChCnt, eAudioCodeType);
        gstHdmiMgr.stInerAttr.eAudioChCnt = eAudioChCnt;
        gstHdmiMgr.stInerAttr.eAudioCodeType = eAudioCodeType;
        gstHdmiMgr.stInerAttr.eAudioFreq = eAudioFreq;
    }
    return HT_SUCCESS;
}

static HT_RESULT _HT_HDMI_SetColorType(HT_HDMI_ColorType_e eColorType)
{
    HDMITX_VIDEO_COLOR_FORMAT eOutColor = HDMITX_VIDEO_COLOR_YUV422;

    switch(eColorType)
    {
        case E_HT_HDMI_COLOR_TYPE_RGB444:
            eOutColor = HDMITX_VIDEO_COLOR_RGB444;
            break;
        case E_HT_HDMI_COLOR_TYPE_YCBCR422:
            eOutColor = HDMITX_VIDEO_COLOR_YUV422;
            break;
        case E_HT_HDMI_COLOR_TYPE_YCBCR444:
            eOutColor = HDMITX_VIDEO_COLOR_YUV444;
            break;
        case E_HT_HDMI_COLOR_TYPE_YCBCR420:
            eOutColor = HDMITX_VIDEO_COLOR_YUV420;
            break;
        default:
            printk("HDMITX %s[%d] Wrong Mode of MI_HDMI_ColorType_e (%d) \n", __FUNCTION__, __LINE__, eColorType);
            return HT_FAILURE;
    }
    if (gstHdmiMgr.stInerAttr.eHdmiTxOutColor == eOutColor)
    {
        printk("HDMITX %s[%d] Set the same color format setting, skip it!! \n", __FUNCTION__, __LINE__);
        return HT_SUCCESS;
    }
    gstHdmiMgr.stInerAttr.eHdmiTxOutColor = eOutColor;
    if ((HDMITX_DVI == gstHdmiMgr.stInerAttr.eHdmiTxMode) || (HDMITX_DVI_HDCP == gstHdmiMgr.stInerAttr.eHdmiTxMode))
    {
        gstHdmiMgr.stInerAttr.eHdmiTxOutColor = HDMITX_VIDEO_COLOR_RGB444;
    }
    if (MApi_HDMITx_ForceHDMIOutputColorFormat(1, gstHdmiMgr.stInerAttr.eHdmiTxOutColor) == FALSE)
    {
        printk("HDMITX %s[%d] MApi_HDMITx_ForceHDMIOutputColorFormat!! \n", __FUNCTION__, __LINE__);
        return HT_FAILURE;
    }
    else
    {
        MApi_HDMITx_SetAVMUTE(TRUE);
        //msleep(100);
        MApi_HDMITx_SetColorFormat(gstHdmiMgr.stInerAttr.eHdmiTxInColor, gstHdmiMgr.stInerAttr.eHdmiTxOutColor);
        //msleep(100);
        MApi_HDMITx_SetAVMUTE(FALSE);
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_HDMI_SetColorDepth(HT_HDMI_DeepColor_e eColorDepth)
{
    HDMITX_VIDEO_COLORDEPTH_VAL eColorDepthVal = HDMITX_VIDEO_CD_NoID;
    switch(eColorDepth)
    {
        case E_HT_HDMI_DEEP_COLOR_24BIT:
            eColorDepthVal = HDMITX_VIDEO_CD_24Bits;
            break;
        case E_HT_HDMI_DEEP_COLOR_30BIT:
            eColorDepthVal = HDMITX_VIDEO_CD_30Bits;
            break;
        case E_HT_HDMI_DEEP_COLOR_36BIT:
            eColorDepthVal = HDMITX_VIDEO_CD_36Bits;
            break;
        case E_HT_HDMI_DEEP_COLOR_48BIT:
            eColorDepthVal = HDMITX_VIDEO_CD_48Bits;
            break;
        case E_HT_HDMI_DEEP_COLOR_MAX:
            eColorDepthVal = HDMITX_VIDEO_CD_NoID;
            break;
        default:
            printk("HDMITX %s[%d] Wrong Mode of MI_HDMI_DeepColor_e \n", __FUNCTION__,__LINE__);
            return HT_FAILURE;
    }
    if(gstHdmiMgr.stInerAttr.eHdmiTxColorDepth == eColorDepthVal)
    {
        printk("HDMITX %s[%d] Set the same color depth setting, skip it!! \n", __FUNCTION__,__LINE__);
        return HT_SUCCESS;
    }
    gstHdmiMgr.stInerAttr.eHdmiTxColorDepth = eColorDepthVal;
    if(gstHdmiMgr.bHdmiTxRuning)
    {
        //_HT_HDMI_SetTiming(_stHdmiMgr.stAttr.stVideoAttr.eTimingType);
        MApi_DAC_SetYPbPrOutputTiming(_HT_HDMI_TransTimingFmt(gstHdmiMgr.stAttr.stVideoAttr.eTimingType));
        switch(eColorDepth)
        {
            case E_HT_HDMI_DEEP_COLOR_24BIT:
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT_Divider);
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT);
                break;
            case E_HT_HDMI_DEEP_COLOR_30BIT:
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT_Divider);
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT);
                break;
            case E_HT_HDMI_DEEP_COLOR_36BIT:
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT_Divider);
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT);
                break;
            case E_HT_HDMI_DEEP_COLOR_48BIT:
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT_Divider);
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT);
                break;
            case E_HT_HDMI_DEEP_COLOR_MAX:
                eColorDepthVal = HDMITX_VIDEO_CD_NoID;
                break;
            default:
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT_Divider);
                MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT);
                break;
        }
        MApi_HDMITx_SetHDMITxMode_CD(gstHdmiMgr.stInerAttr.eHdmiTxMode, gstHdmiMgr.stInerAttr.eHdmiTxColorDepth);
        MApi_HDMITx_Exhibit();
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_HDMI_SetOutputMode(HT_HDMI_OutputMode_e eOutputMode)
{
    HT_BOOL bIsSupportHDMIMode = FALSE;
    HT_U32 u32StartTime = _GetTime();
    HDMITX_OUTPUT_MODE HDMITX_Mode = HDMITX_HDMI;
    HDMITX_VIDEO_COLOR_FORMAT HDMITx_OutColor = HDMITX_VIDEO_COLOR_YUV422;

    if ((E_HT_HDMI_OUTPUT_MODE_DVI == eOutputMode) || (E_HT_HDMI_OUTPUT_MODE_HDMI == eOutputMode))
    {
        MApi_HDMITx_HDCP_StartAuth(FALSE);
    }
    switch (eOutputMode)
    {
        case E_HT_HDMI_OUTPUT_MODE_HDMI:
            HDMITX_Mode = HDMITX_HDMI;
            MApi_HDMITx_SetHDCPOnOff(FALSE);
            break;
        case E_HT_HDMI_OUTPUT_MODE_HDMI_HDCP:
            HDMITX_Mode = HDMITX_HDMI_HDCP;
            MApi_HDMITx_SetHDCPOnOff(TRUE);
            break;
        case E_HT_HDMI_OUTPUT_MODE_DVI:
            HDMITX_Mode = HDMITX_DVI;
            MApi_HDMITx_SetHDCPOnOff(FALSE);
            HDMITx_OutColor = HDMITX_VIDEO_COLOR_RGB444;
            break;
        case E_HT_HDMI_OUTPUT_MODE_DVI_HDCP:
            HDMITX_Mode = HDMITX_DVI_HDCP;
            MApi_HDMITx_SetHDCPOnOff(TRUE);
            HDMITx_OutColor = HDMITX_VIDEO_COLOR_RGB444;
            break;
        default:
            printk("HDMITX %s[%d] Wrong Mode of MI_HDMI_OutputMode_e \n", __FUNCTION__,__LINE__);
            return HT_FAILURE;
    }
    while(TRUE != MApi_HDMITx_EDID_HDMISupport(&bIsSupportHDMIMode))
    {
        if(_DiffFromNow(u32StartTime) > 100) // timeout: 100ms
        {
            if(MApi_HDMITx_EdidChecking())
            {
                MApi_HDMITx_EDID_HDMISupport(&bIsSupportHDMIMode);
            }
            else
                printk("HDMITX %s[%d] EDID checking failed!!\n", __FUNCTION__, __LINE__);
            break;
        }
        msleep(10);
    }
    if(bIsSupportHDMIMode)
    {
        if (HDMITX_DVI == HDMITX_Mode)
        {
            HDMITX_Mode = HDMITX_HDMI;
        }
        else if(HDMITX_DVI_HDCP == HDMITX_Mode)
        {
            HDMITX_Mode = HDMITX_HDMI_HDCP;
        }
        if (TRUE == _HT_HDMI_IsReceiverSupportYPbPr())
        {
            HDMITx_OutColor = HDMITX_VIDEO_COLOR_YUV444;//422 or 444
        }
        else
        {
            HDMITx_OutColor = HDMITX_VIDEO_COLOR_RGB444;
        }
    }
    else
    {
        if (HDMITX_HDMI == HDMITX_Mode)
        {
            HDMITX_Mode = HDMITX_DVI;
        }
        else if (HDMITX_HDMI_HDCP == HDMITX_Mode)
        {
            HDMITX_Mode = HDMITX_DVI_HDCP;
        }
        HDMITx_OutColor = HDMITX_VIDEO_COLOR_RGB444;
    }
    gstHdmiMgr.stInerAttr.eHdmiTxMode = HDMITX_Mode;
    gstHdmiMgr.stInerAttr.eHdmiTxOutColor = HDMITx_OutColor;
    if(FALSE == gstHdmiMgr.bHdmiTxRuning)
    {
        MApi_HDMITx_SetHDMITxMode(gstHdmiMgr.stInerAttr.eHdmiTxMode);

        if((HDMITX_DVI_HDCP == HDMITX_Mode)||(HDMITX_HDMI_HDCP == HDMITX_Mode))
        {
            MApi_HDMITx_HDCP_StartAuth(TRUE);
        }
        MApi_HDMITx_SetColorFormat(gstHdmiMgr.stInerAttr.eHdmiTxInColor, gstHdmiMgr.stInerAttr.eHdmiTxOutColor);
    }
    else
    {
        _HT_HDMI_Close(E_HT_HDMI_ID_0);
        msleep(200);//delay for Rocket reset
        _HT_HDMI_Open(E_HT_HDMI_ID_0);
        MApi_HDMITx_SetAudioOnOff(TRUE);
    }
    return HT_SUCCESS;
}
static HT_RESULT _HT_HDMI_SetTiming(HT_HDMI_TimingType_e eTimingType)
{
    HDMITX_VIDEO_TIMING eHdmiVideoTiming = HDMITX_RES_1920x1080p_60Hz;
    HDMITX_OUTPUT_MODE eHDMITxMode = gstHdmiMgr.stInerAttr.eHdmiTxMode;
    HDMITX_VIDEO_COLORDEPTH_VAL eHDMITxColorDepth = gstHdmiMgr.stInerAttr.eHdmiTxColorDepth;
    HDMITX_ANALOG_TUNING stHDMITxTun;

    _HT_HDMI_ChkColorFormatForSpecTiming(eTimingType); //IMPL COLOR FORMAT

    switch (eTimingType)
    {
        case E_HT_HDMI_TIMING_480_60I:
            eHdmiVideoTiming = HDMITX_RES_720x480i;
            break;
        case E_HT_HDMI_TIMING_480_60P:
            eHdmiVideoTiming = HDMITX_RES_720x480p;
            break;
        case E_HT_HDMI_TIMING_576_50I:
            eHdmiVideoTiming = HDMITX_RES_720x576i;
            break;
        case E_HT_HDMI_TIMING_576_50P:
            eHdmiVideoTiming = HDMITX_RES_720x576p;
            break;
        case E_HT_HDMI_TIMING_720_50P:
            eHdmiVideoTiming = HDMITX_RES_1280x720p_50Hz;
            break;
        case E_HT_HDMI_TIMING_720_60P:
            eHdmiVideoTiming = HDMITX_RES_1280x720p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1080_50I:
            eHdmiVideoTiming = HDMITX_RES_1920x1080i_50Hz;
            break;
        case E_HT_HDMI_TIMING_1080_50P:
            eHdmiVideoTiming = HDMITX_RES_1920x1080p_50Hz;
            break;
        case E_HT_HDMI_TIMING_1080_60I:
            eHdmiVideoTiming = HDMITX_RES_1920x1080i_60Hz;
            break;
        case E_HT_HDMI_TIMING_1080_60P:
            eHdmiVideoTiming = HDMITX_RES_1920x1080p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1080_30P:
            eHdmiVideoTiming = HDMITX_RES_1920x1080p_30Hz;
            break;
        case E_HT_HDMI_TIMING_1080_25P:
            eHdmiVideoTiming = HDMITX_RES_1920x1080p_25Hz;
            break;
        case E_HT_HDMI_TIMING_1080_24P:
            eHdmiVideoTiming = HDMITX_RES_1920x1080p_24Hz;
            break;
        case E_HT_HDMI_TIMING_4K2K_24P:
            eHdmiVideoTiming = HDMITX_RES_3840x2160p_24Hz;
            break;
        case E_HT_HDMI_TIMING_4K2K_25P:
            eHdmiVideoTiming = HDMITX_RES_3840x2160p_25Hz;
            break;
        case E_HT_HDMI_TIMING_4K2K_30P:
            eHdmiVideoTiming = HDMITX_RES_3840x2160p_30Hz;
            break;
        case E_HT_HDMI_TIMING_4K2K_50P:
            eHdmiVideoTiming = HDMITX_RES_3840x2160p_50Hz;
            break;
        case E_HT_HDMI_TIMING_4K2K_60P:
            eHdmiVideoTiming = HDMITX_RES_3840x2160p_60Hz;
            break;
        case E_HT_HDMI_TIMING_4096x2160_24P:
            eHdmiVideoTiming = HDMITX_RES_4096x2160p_24Hz;
            break;
        case E_HT_HDMI_TIMING_4096x2160_25P:
            eHdmiVideoTiming = HDMITX_RES_4096x2160p_25Hz;
            break;
        case E_HT_HDMI_TIMING_4096x2160_30P:
            eHdmiVideoTiming = HDMITX_RES_4096x2160p_30Hz;
            break;
        case E_HT_HDMI_TIMING_4096x2160_50P:
            eHdmiVideoTiming = HDMITX_RES_4096x2160p_50Hz;
            break;
        case E_HT_HDMI_TIMING_4096x2160_60P:
            eHdmiVideoTiming = HDMITX_RES_4096x2160p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1024x768_60P:
            eHdmiVideoTiming = HDMITX_RES_1024x768p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1280x1024_60P:
            eHdmiVideoTiming = HDMITX_RES_1280x1024p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1440x900_60P:
            eHdmiVideoTiming = HDMITX_RES_1440x900p_60Hz;
            break;
        case E_HT_HDMI_TIMING_1600x1200_60P:
            eHdmiVideoTiming = HDMITX_RES_1600x1200p_60Hz;
            break;
        default:
            printk("HDMITX %s[%d] Wrong Mode of MI_HDMI_TimingType_e (0x%x)\n", __FUNCTION__, __LINE__, eTimingType);
            return HT_FAILURE;
    }
    MApi_HDMITx_SetHDMITxMode_CD(eHDMITxMode, eHDMITxColorDepth);
    MApi_HDMITx_SetColorFormat(gstHdmiMgr.stInerAttr.eHdmiTxInColor, gstHdmiMgr.stInerAttr.eHdmiTxOutColor);

    if (gstHdmiMgr.stInerAttr.eHdmiTxColorDepth <= HDMITX_VIDEO_CD_30Bits)
    {
        stHDMITxTun.tm_txcurrent = 0x01;//HDMITx Info From broad.h
        stHDMITxTun.tm_pren2 = 0x0;
        stHDMITxTun.tm_precon = 0x0;
        stHDMITxTun.tm_pren = 0x0;
        stHDMITxTun.tm_tenpre = 0x0;
        stHDMITxTun.tm_ten = 0x0;
    }
    else
    {
        stHDMITxTun.tm_txcurrent = 0x00;
        stHDMITxTun.tm_pren2 = 0x0;
        stHDMITxTun.tm_precon = 0x0;
        stHDMITxTun.tm_pren = 0x0;
        stHDMITxTun.tm_tenpre = 0x0;
        stHDMITxTun.tm_ten = 0x7;
    }
    MApi_HDMITx_AnalogTuning(&stHDMITxTun);
    MApi_DAC_SetYPbPrOutputTiming(_HT_HDMI_TransTimingFmt(gstHdmiMgr.stAttr.stVideoAttr.eTimingType));
    switch (gstHdmiMgr.stInerAttr.eHdmiTxColorDepth)
    {
        case HDMITX_VIDEO_CD_30Bits:
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT_Divider);
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_10BIT);
            break;
        case HDMITX_VIDEO_CD_36Bits:
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT_Divider);
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_12BIT);
            break;
        case HDMITX_VIDEO_CD_48Bits:
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT_Divider);
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_16BIT);
            break;
        default: //24bit
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT_Divider);
            MApi_DAC_DumpTable(NULL, E_HT_HDMI_DAC_TABTYPE_INIT_HDMITX_8BIT);
            break;
    }
    printk("HDMITX %s[%d] eHdmiVideoTiming = %d\n", __FUNCTION__, __LINE__, eHdmiVideoTiming);
    MApi_HDMITx_SetVideoOutputTiming(eHdmiVideoTiming);
    MApi_HDMITx_Exhibit();
    // VS infoframe
    if ((E_HT_HDMI_TIMING_4K2K_24P == eTimingType) || (E_HT_HDMI_TIMING_4K2K_25P == eTimingType)
        || (E_HT_HDMI_TIMING_4K2K_30P == eTimingType) || (E_HT_HDMI_TIMING_4K2K_60P == eTimingType))
    {
        MApi_HDMITx_PKT_User_Define(HDMITX_VS_INFOFRAME, FALSE, HDMITX_CYCLIC_PACKET, 0x0);
        // VS_FCNT=0x0, send one vender packet per VS_FCNT+1 frames
        switch(eTimingType)
        {
            case E_HT_HDMI_TIMING_4K2K_24P:
                MApi_HDMITx_Set_VS_InfoFrame(HDMITX_VIDEO_VS_4k_2k, HDMITx_VIDEO_3D_Not_in_Use, HDMITX_VIDEO_4k2k_24Hz);
                break;
            case E_HT_HDMI_TIMING_4K2K_25P:
                MApi_HDMITx_Set_VS_InfoFrame(HDMITX_VIDEO_VS_4k_2k, HDMITx_VIDEO_3D_Not_in_Use, HDMITX_VIDEO_4k2k_25Hz);
                break;
            case E_HT_HDMI_TIMING_4K2K_30P:
                MApi_HDMITx_Set_VS_InfoFrame(HDMITX_VIDEO_VS_4k_2k, HDMITx_VIDEO_3D_Not_in_Use, HDMITX_VIDEO_4k2k_30Hz);
                break;
            case E_HT_HDMI_TIMING_4096x2160_24P:
                MApi_HDMITx_Set_VS_InfoFrame(HDMITX_VIDEO_VS_4k_2k, HDMITx_VIDEO_3D_Not_in_Use, HDMITx_VIDEO_4k2k_24Hz_SMPTE);
                break;
            default:
                printk("HDMITX %s[%d] Invalid 4K2K output mode\n", __FUNCTION__, __LINE__);
                MApi_HDMITx_Set_VS_InfoFrame(HDMITX_VIDEO_VS_No_Addition, HDMITx_VIDEO_3D_Not_in_Use, HDMITx_VIDEO_4k2k_Reserved);
                break;
        }
    }
    gstHdmiMgr.bHdmiTxRuning = TRUE;
    return HT_SUCCESS;
}

static HT_HDMI_TimingType_e _HT_HDMI_ParseStrings(HT_U16 *pau16CmdValue,HT_U8 u8CmdCnt)
{
    HT_HDMI_TimingType_e eTimingType;
    switch(pau16CmdValue[0])
    {
        case 0:
            eTimingType=E_HT_HDMI_TIMING_720_60P;
            break;
        case 1:
            eTimingType=E_HT_HDMI_TIMING_1024x768_60P;
            break;
        case 2:
            eTimingType=E_HT_HDMI_TIMING_1080_60P;
            break;
        default:
            eTimingType=E_HT_HDMI_TIMING_1080_60P;
    }
    return eTimingType;
}

void HT_HDMI_DisplayHelp(void)
{

}
HT_RESULT HT_HDMI(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_S32 s32retVal;
    HT_HDMI_Attr_t stAttr;
    HT_HDMI_DeviceId_e eHdmi = E_HT_HDMI_ID_0;
    HT_HDMI_TimingType_e eTimingType;

    eTimingType=_HT_HDMI_ParseStrings(pau16CmdValue,u8CmdCnt);

    _HT_HDMI_Init();
    _HT_HDMI_Open(eHdmi);


    memset(&stAttr, 0, sizeof(HT_HDMI_Attr_t));
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stAttr.stAudioAttr.bEnableAudio = TRUE;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_HT_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_HT_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_HT_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = TRUE;
    stAttr.stVideoAttr.eColorType = E_HT_HDMI_COLOR_TYPE_RGB444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_HT_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = eTimingType;
    stAttr.stVideoAttr.eOutputMode = E_HT_HDMI_OUTPUT_MODE_HDMI;

    memcpy(&gstHdmiMgr.stAttr, &stAttr, sizeof(HT_HDMI_Attr_t));
    if (eHdmi < E_HT_HDMI_ID_MAX)
    {
        if (TRUE == stAttr.stVideoAttr.bEnableVideo) // process video attr
        {
            if(stAttr.stVideoAttr.eColorType < E_HT_HDMI_COLOR_TYPE_MAX)
            {
                s32retVal = _HT_HDMI_SetColorType(stAttr.stVideoAttr.eColorType);
                if (HT_SUCCESS != s32retVal)
                {
                    printk("%s: _HT_HDMI_SetColorType fail (0x%x)!\n", __FUNCTION__, s32retVal);
                    return HT_FAILURE;
                }
                gstHdmiMgr.stAttr.stVideoAttr.eColorType = stAttr.stVideoAttr.eColorType;
            }
            if (stAttr.stVideoAttr.eDeepColorMode < E_HT_HDMI_DEEP_COLOR_MAX)
            {
                s32retVal = _HT_HDMI_SetColorDepth(stAttr.stVideoAttr.eDeepColorMode);
                if (HT_SUCCESS != s32retVal)
                {
                    printk("%s: _MI_HDMI_SetColorDepth fail (0x%x)!\n", __FUNCTION__, s32retVal);
                    return HT_FAILURE;
                }
                gstHdmiMgr.stAttr.stVideoAttr.eDeepColorMode = stAttr.stVideoAttr.eDeepColorMode;
            }
            if (stAttr.stVideoAttr.eOutputMode < E_HT_HDMI_OUTPUT_MODE_MAX)
            {
                s32retVal = _HT_HDMI_SetOutputMode(stAttr.stVideoAttr.eOutputMode);
                if (HT_SUCCESS != s32retVal)
                {
                    printk("%s: _MI_HDMI_SetOutputMode fail (0x%x)!\n", __FUNCTION__, s32retVal);
                    return HT_FAILURE;
                }
                gstHdmiMgr.stAttr.stVideoAttr.eOutputMode = stAttr.stVideoAttr.eOutputMode;
            }
            if (stAttr.stVideoAttr.eTimingType < E_HT_HDMI_TIMING_MAX)
            {
                s32retVal = _HT_HDMI_SetTiming(stAttr.stVideoAttr.eTimingType);
                if (HT_SUCCESS != s32retVal)
                {
                    printk("%s: _MI_HDMI_SetTiming fail (0x%x)!\n", __FUNCTION__, s32retVal);
                    return HT_FAILURE;
                }
                gstHdmiMgr.stAttr.stVideoAttr.eTimingType = stAttr.stVideoAttr.eTimingType;
            }
            memcpy(&gstHdmiMgr.stAttr.stEnInfoFrame, &stAttr.stEnInfoFrame, sizeof(HT_HDMI_EnInfoFrame_t));
        }

        if (TRUE == stAttr.stAudioAttr.bEnableAudio) // process audio attr
        {
            s32retVal = _HT_HDMI_SetAudioConfiguration(&stAttr.stAudioAttr);
            if (HT_SUCCESS != s32retVal)
            {
                printk("%s: _HT_HDMI_SetAudioConfiguration fail (0x%x)!\n", __FUNCTION__, s32retVal);
                return HT_FAILURE;
            }
            memcpy(&gstHdmiMgr.stAttr.stAudioAttr, &stAttr.stAudioAttr, sizeof(HT_HDMI_AudioAttr_t));
        }
    }

    _HT_HDMI_Start(eHdmi);

    return HT_SUCCESS;
}



