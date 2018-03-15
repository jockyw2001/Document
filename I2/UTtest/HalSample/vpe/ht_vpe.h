////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  @file HAL_VPE.h
 *  @brief HAL_VPE Driver interface
 */

/**
 * \defgroup HAL_VPE_group  HAL_VPE driver
 * @{
 */
#ifndef __HAL_VPE_H__
#define __HAL_VPE_H__

//=============================================================================
// Includs
//=============================================================================
#include "../inc/ht_common_datatype.h"

//-------------------------------------------------------------------------------------------------
//  Defines & enum
//-------------------------------------------------------------------------------------------------
// #define ROI_WINDOW_MAX 4
// #define WDR_HIST_BUFFER 4
// typedef enum
// {
    // E_HAL_ISP_ROTATION_Off,     //
    // E_HAL_ISP_ROTATION_90,      //
    // E_HAL_ISP_ROTATION_180,     //
    // E_HAL_ISP_ROTATION_270,     //
    // E_HAL_ISP_ROTATION_TYPE,    //
// }HalVpeIspRotationType_e;
// typedef enum
// {
    // E_HAL_ISP_INPUT_YUV420,      //
    // E_HAL_ISP_INPUT_YUV422,      //
    // E_HAL_ISP_INPUT_TYPE,        //
// }HalVpeIspInputFormat_e;
// typedef enum
// {
    // E_HAL_SCL_OUTPUT_DRAM,      //
    // E_HAL_SCL_OUTPUT_MDWIN,      //
    // E_HAL_SCL_OUTPUT_TYPE,        //
// }HalVpeSclOutputType_e;
// typedef enum
// {
    // E_HAL_IQ_ROISRC_BEFORE_WDR,      //
    // E_HAL_IQ_ROISRC_AFTER_WDR,      //
    // E_HAL_IQ_ROISRC_WDR,        //
    // E_HAL_IQ_ROISRC_TYPE,        //
// }HalVpeIqWdrRoiSrcType_e;
// typedef enum
// {
    // E_HAL_SCL_OUTPUT_NV12420,      //
    // E_HAL_SCL_OUTPUT_YUYV422,      //
    // E_HAL_SCL_OUTPUT_YCSEP422,      //
    // E_HAL_SCL_OUTPUT_YUVSEP422,      //
    // E_HAL_SCL_OUTPUT_YUVSEP420,      //
   // // E_HAL_SCL_OUTPUT_TYPE,        //
    // E_HAL_SCL_OUTPUT_FORMAT,        //
// }HalVpeSclOutputFormat_e;
// typedef enum
// {
    // E_HAL_SCL_OUTPUT_PORT0,      //
    // E_HAL_SCL_OUTPUT_PORT1,      //
    // E_HAL_SCL_OUTPUT_PORT2,      //
    // E_HAL_SCL_OUTPUT_PORT3,      //
    // E_HAL_SCL_OUTPUT_MAX,        //
// }HalVpeSclOutputPort_e;
// typedef enum
// {
    // E_HAL_SCL_OUTPUT_MDWin0,      //
    // E_HAL_SCL_OUTPUT_MDWin1,      //
    // E_HAL_SCL_OUTPUT_MDWin2,      //
    // E_HAL_SCL_OUTPUT_MDWin3,      //
    // E_HAL_SCL_OUTPUT_MDWin4,      //
    // E_HAL_SCL_OUTPUT_MDWin5,      //
    // E_HAL_SCL_OUTPUT_MDWin6,      //
    // E_HAL_SCL_OUTPUT_MDWin7,      //
    // E_HAL_SCL_OUTPUT_MDWin8,      //
    // E_HAL_SCL_OUTPUT_MDWin9,      //
    // E_HAL_SCL_OUTPUT_MDWin10,      //
    // E_HAL_SCL_OUTPUT_MDWin11,      //
    // E_HAL_SCL_OUTPUT_MDWin12,      //
    // E_HAL_SCL_OUTPUT_MDWin13,      //
    // E_HAL_SCL_OUTPUT_MDWin14,      //
    // E_HAL_SCL_OUTPUT_MDWin15,      //
// }HalVpeSclOutputMDWin_e;
// typedef enum
// {
    // E_HAL_SCL_OSD_NUM0,      //
    // E_HAL_SCL_OSD_NUM1,      //
    // E_HAL_SCL_OSD_NUM2,      //
    // E_HAL_SCL_OSD_NUM3,      //
    // E_HAL_SCL_COVER_NUM0,      //
    // E_HAL_SCL_COVER_NUM1,      //
    // E_HAL_SCL_OSD_TYPE,        //
// }HalVpeSclOsdNum_e;

// /// data type 64bit physical address
// typedef unsigned long long                       PHY;        // 8 bytes
// typedef enum
// {
    // E_HAL_PIXEL_FRAME_YUV422_YUYV = 0,
    // E_HAL_PIXEL_FRAME_ARGB8888,
    // E_HAL_PIXEL_FRAME_ABGR8888,

    // E_HAL_PIXEL_FRAME_RGB565,
    // E_HAL_PIXEL_FRAME_ARGB1555,
    // E_HAL_PIXEL_FRAME_I2,
    // E_HAL_PIXEL_FRAME_I4,
    // E_HAL_PIXEL_FRAME_I8,

    // E_HAL_PIXEL_FRAME_YUV_SEMIPLANAR_422,
    // E_HAL_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    // // mstar mdwin/mgwin
    // E_HAL_PIXEL_FRAME_YUV_MST_420,

    // //vdec mstar private video format
    // E_HAL_PIXEL_FRAME_YC420_MSTTILE1_H264,
    // E_HAL_PIXEL_FRAME_YC420_MSTTILE2_H265,
    // E_HAL_PIXEL_FRAME_YC420_MSTTILE3_H265,
    // E_HAL_PIXEL_FRAME_FORMAT_MAX,
// } HalPixelFormat_e;

// typedef enum
// {
    // E_HAL_COMPRESS_MODE_NONE,//no compress
    // E_HAL_COMPRESS_MODE_SEG,//compress unit is 256 bytes as a segment
    // E_HAL_COMPRESS_MODE_LINE,//compress unit is the whole line
    // E_HAL_COMPRESS_MODE_FRAME,//compress unit is the whole frame
    // E_HAL_COMPRESS_MODE_BUTT, //number
// } HalPixelCompressMode_e;

// //-------------------------------------------------------------------------------------------------
// //  structure
// //-------------------------------------------------------------------------------------------------
// typedef struct
// {
    // HT_U8 u8Channel;
// } HalVpeSclExchangeCtx_t;

// typedef struct
// {
    // HalVpeIspRotationType_e enRotType;

// } HalVpeIspRotationConfig_t;

// typedef struct
// {
    // HalVpeIspInputFormat_e enInType;
    // HalPixelFormat_e  ePixelFormat;
    // HalPixelCompressMode_e eCompressMode;
    // HT_U32 u32Width;
    // HT_U32 u32Height;
// } HalVpeIspInputConfig_t;

// typedef struct
// {
    // HT_BOOL bNREn;
    // HT_BOOL bEdgeEn;
    // HT_BOOL bESEn;
    // HT_BOOL bContrastEn;
    // HT_BOOL bUVInvert;
// } HalVpeIqOnOff_t;
// typedef struct
// {
    // HT_U8 u8NRC_SF_STR; //0 ~ 255;
    // HT_U8 u8NRC_TF_STR; //0 ~ 255
    // HT_U8 u8NRY_SF_STR; //0 ~ 255
    // HT_U8 u8NRY_TF_STR; //0 ~ 255
    // HT_U8 u8NRY_BLEND_MOTION_TH; //0 ~ 15
    // HT_U8 u8NRY_BLEND_STILL_TH; //0 ~ 15
    // HT_U8 u8NRY_BLEND_MOTION_WEI; //0 ~ 31
    // HT_U8 u8NRY_BLEND_OTHER_WEI; //0 ~ 31
    // HT_U8 u8NRY_BLEND_STILL_WEI; //0 ~ 31
    // HT_U8 u8EdgeGain[6];//0~255
    // HT_U8 u8Contrast;//0~255
// } HalVpeIqConfig_t;

// //-------------------------------------------------
// // ROI Defination:
// //       AccX[0]AccY[0]   AccX[1]AccY[1]
// //       *---------------------*
// //       |                     |
// //       |                     |
// //       *---------------------*
// //       AccX[2]AccY[2]   AccX[3]AccY[3]
// //--------------------------------------------------
// typedef struct
// {
    // HT_BOOL bEnSkip;
    // HT_U16 u16RoiAccX[4];
    // HT_U16 u16RoiAccY[4];
// } HalVpeIqWdrRoiConfig_t;

// typedef struct
// {
    // HT_U32 u32Y[ROI_WINDOW_MAX];
// } HalVpeIqWdrRoiReport_t;

// typedef struct
// {
    // HT_BOOL bEn;
    // HT_U8 u8WinCount;
    // HalVpeIqWdrRoiSrcType_e enPipeSrc;
    // HalVpeIqWdrRoiConfig_t *pstRoiCfg[ROI_WINDOW_MAX];
// } HalVpeIqWdrRoiHist_t;

// typedef struct
// {
    // HT_U16 u16X;        ///< horizontal starting position
    // HT_U16 u16Y;        ///< vertical starting position
    // HT_U16 u16Width;    ///< horizontal size
    // HT_U16 u16Height;   ///< vertical size
// }HalSclCropWindowConfig_t;

// typedef struct
// {
    // HT_BOOL bCropEn;           ///< the control flag of Crop on/off
    // HalSclCropWindowConfig_t stCropWin;   ///< crop configuration
// } HalVpeSclCropConfig_t;

// typedef struct
// {
    // HalVpeSclOutputPort_e enOutPort;
    // HalPixelFormat_e      enOutFormat;
    // HalPixelCompressMode_e enCompress;
// } HalVpeSclOutputDmaConfig_t;

// typedef  struct  HalVideoBuffer_s
// {
    // HT_U64 u64PhyAddr[3];
    // HT_U32 u32Stride[3];
// } HalVideoBufferInfo_t;

// typedef HalVideoBufferInfo_t HalVpeIspVideoInfo_t;

// typedef struct
// {
    // HalVideoBufferInfo_t stBufferInfo;
    // HT_BOOL bEn;
// } HalVpeSclOutputPortBufferConfig_t;

// typedef struct
// {
    // HalVpeSclOutputPortBufferConfig_t stCfg[E_HAL_SCL_OUTPUT_MAX];
// } HalVpeSclOutputBufferConfig_t;
// typedef struct
// {
    // HalVpeSclOutputPort_e enOutPort;
    // HalVpeSclOutputType_e enOutType;
// } HalVpeSclOutputMDwinConfig_t;
// typedef struct
// {
    // HalVpeSclOutputPort_e enOutPort;
    // HT_U16 u16Width;
    // HT_U16 u16Height;
// } HalVpeSclOutputSizeConfig_t;

// typedef struct
// {
    // HT_U16 u16Width;
    // HT_U16 u16Height;
// } HalVpeSclWinSize_t;

// typedef struct {
    // HalPixelFormat_e  ePixelFormat;
    // HalPixelCompressMode_e eCompressMode;
    // HT_U32 u32Width;
    // HT_U32 u32Height;
// } HalVpeSclInputSizeConfig_t;

// #if 0
// typedef struct
// {
  // HT_U64 (*reserve_menuload_buf)(void *ctx, HT_U32 size);
  // s32 (*set_menuload_buf)(void *ctx, HT_U64 buf, HT_U32 size);
  // s32 (*write_reg_cmdq_mask)(void *ctx, HT_U32 reg_addr, HT_U16 value,  HT_U16 write_mask);
  // s32 (*write_reg_cmdq)(void *ctx, HT_U32 reg_addr, HT_U16 value,  HT_U16 write_mask);
  // s32 (*cmdq_poll_reg_bits)(void *ctx, HT_U32 reg_addr, HT_U16 value,  HT_U16 write_mask);
  // s32 (*cmdq_add_wait_event_cmd)(void *ctx, HT_U16 event);

  // s32 (*write_reg_menuload_mask)(void *ctx, HT_U32 reg_addr, HT_U16 value,  HT_U16 write_mask);
  // s32 (*write_reg_menuload)(void *ctx, HT_U32 reg_addr, HT_U16 value);
  // HT_U16 (*get_menuload_used_size)(void *ctx);
  // void *ctx;
// }HalVpeCmdMenuloadInterface_t;

// typedef struct
// {
	// HalVpeCmdMenuloadInterface_t *pstCmdOps;
	// HT_U32 CmdQId;
// } HalVpeCMDQInfo_t;
// // FOR HW IP DATA Buffer
// typedef struct {
    // s32 (*alloc)(const HT_U8* u8MMAHeapName, HT_U32 u32Size, PHY *phyAddr);
    // s32 (*free)(PHY u64PhyAddr);
// } HalAllocPhyMem_t;

// #endif
// //-------------------------------------------------------------------------------------------------
// //  Prototype
// //-------------------------------------------------------------------------------------------------

// //=============================================================================
// // API
// //=============================================================================
// #ifndef __HAL_VPE_C__
// #define INTERFACE extern
// #else
// #define INTERFACE
// #endif
// // Driver Physical memory: MI

// //IQ
// #if 0
// INTERFACE HT_BOOL HalVpeCreateIqInstance(const HalAllocPhyMem_t *pstAlloc ,void **pCtx);
// INTERFACE HT_BOOL HalVpeDestroyIqInstance(void *pCtx);
// INTERFACE HT_BOOL HalVpeIqProcess(void *pCtx, const HalVpeCMDQInfo_t *pstCmdQInfo);
// INTERFACE HT_BOOL HalVpeIqDbgLevel(void *p);

// INTERFACE HT_BOOL HalVpeIqConfig(void *pCtx, const HalVpeIqConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeIqOnOff(void *pCtx, const HalVpeIqOnOff_t *pCfg);
// INTERFACE HT_BOOL HalVpeIqGetWdrRoiHist(void *pCtx, HalVpeIqWdrRoiReport_t * pstRoiReport);
// INTERFACE HT_BOOL HalVpeIqSetWdrRoiHist(void *pCtx, const HalVpeIqWdrRoiHist_t *pCfg);

// // Register write via cmdQ
// INTERFACE HT_BOOL HalVpeIqSetWdrRoiMask(void *pCtx,const HT_BOOL bEnMask, HalVpeCMDQInfo_t *pstCmdQInfo);
// // Register write via cmdQ
// INTERFACE HT_BOOL HalVpeIqSetDnrTblMask(void *pCtx,const HT_BOOL bEnMask, HalVpeCMDQInfo_t *pstCmdQInfo);

// //ROI Buffer

// //ISP
// INTERFACE HT_BOOL HalVpeCreateIspInstance(const HalAllocPhyMem_t *pstAlloc ,void **pCtx);
// INTERFACE HT_BOOL HalVpeDestroyIspInstance(void *pCtx);
// INTERFACE HT_BOOL HalVpeIspProcess(void *pCtx, HalVpeCMDQInfo_t *pstCmdQInfo, const HalVpeIspVideoInfo_t *pstVidInfo);
// INTERFACE HT_BOOL HalVpeIspDbgLevel(void *p);


// INTERFACE HT_BOOL HalVpeIspRotationConfig(void *pCtx, const HalVpeIspRotationConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeIspInputConfig(void *pCtx, const HalVpeIspInputConfig_t *pCfg);


// // SCL
// INTERFACE HT_BOOL HalVpeCreateSclInstance(const HalAllocPhyMem_t *pstAlloc, const HalVpeSclWinSize_t stMaxWin, void **pCtx);
// INTERFACE HT_BOOL HalVpeDestroySclInstance(void *pCtx);
// INTERFACE HT_BOOL HalVpeSclProcess(void *pCtx, HalVpeCMDQInfo_t *pstCmdQInfo, const HalVpeSclOutputBufferConfig_t *pBuffer);
// INTERFACE HT_BOOL HalVpeSclDbgLevel(void *p);
// INTERFACE HT_BOOL HalVpeSclCropConfig(void *pCtx, const HalVpeSclCropConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeSclOutputDmaConfig(void *pCtx, const HalVpeSclOutputDmaConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeSclInputConfig(void *pCtx, const HalVpeSclInputSizeConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeSclOutputSizeConfig(void *pCtx, const HalVpeSclOutputSizeConfig_t *pCfg);
// INTERFACE HT_BOOL HalVpeSclOutputMDWinConfig(void *pCtx, const HalVpeSclOutputMDwinConfig_t *pCfg);

// // SCL vpe irq.
// INTERFACE HT_BOOL HalVpeSclGetIrqNum(unsigned int *pIrqNum);
// // RIU write register
// INTERFACE HT_BOOL HalVpeSclEnableIrq(HT_BOOL bOn);
// // RIU write register
// INTERFACE HT_BOOL HalVpeSclClearIrq(void);
// INTERFACE HT_BOOL HalVpeSclCheckIrq(void);


// // SCL sw trigger irq by CMDQ or Sc
// INTERFACE HT_BOOL HalVpeSclSetSwTriggerIrq(HalVpeCMDQInfo_t *pstCmdQInfo);
// // SCL in irq bottom: Read 3DNR register
// INTERFACE HT_BOOL HalVpeSclRead3DNRTbl(void *pSclCtx);
// // SCL polling MdwinDone
// INTERFACE HT_BOOL HalVpeSclSetWaitMdwinDone(void *pSclCtx, HalVpeCMDQInfo_t *pstCmdQInfo);

// #endif


HT_S32 UT_VPE(HT_U8 *pszCmd);
#endif //
/** @} */ // end of HAL_VPE_group
