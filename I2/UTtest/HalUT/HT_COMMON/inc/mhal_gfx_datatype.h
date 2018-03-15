//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#ifndef _MS_GFX_DATATYPE_H_
#define _MS_GFX_DATATYPE_H_
#ifdef __cplusplus
extern "C" {
#endif

#define  E_MS_MODULE_ID_GFX   13
#define  E_MS_ERR_LEVEL_ERROR 2

#define MHAL_ERR_ID  (0x80000000L + 0x20000000L)

#define MHAL_DEF_ERR( module, level, errid) \
    ((MS_S32)( (MHAL_ERR_ID) | ((module) << 16 ) | ((level)<<13) | (errid) ))

typedef enum
{
    E_MS_ERR_INVALID_DEVID = 1, /* invlalid device ID                           */
    E_MS_ERR_INVALID_CHNID = 2, /* invlalid channel ID                          */
    E_MS_ERR_ILLEGAL_PARAM = 3, /* at lease one parameter is illagal
                               * eg, an illegal enumeration value             */
    E_MS_ERR_EXIST         = 4, /* resource exists                              */
    E_MS_ERR_UNEXIST       = 5, /* resource unexists                            */
    E_MS_ERR_NULL_PTR      = 6, /* using a NULL point                           */
    E_MS_ERR_NOT_CONFIG    = 7, /* try to enable or initialize system, device
                              ** or channel, before configing attribute       */
    E_MS_ERR_NOT_SUPPORT   = 8, /* operation or type is not supported by NOW    */
    E_MS_ERR_NOT_PERM      = 9, /* operation is not permitted
                              ** eg, try to change static attribute           */
    E_MS_ERR_NOMEM         = 12,/* failure caused by malloc memory              */
    E_MS_ERR_NOBUF         = 13,/* failure caused by malloc buffer              */
    E_MS_ERR_BUF_EMPTY     = 14,/* no data in buffer                            */
    E_MS_ERR_BUF_FULL      = 15,/* no buffer for new data                       */
    E_MS_ERR_SYS_NOTREADY  = 16,/* System is not ready,maybe not initialed or
                              ** loaded. Returning the error code when opening
                              ** a device file failed.                        */
    E_MS_ERR_BADADDR       = 17,/* bad address,
                              ** eg. used for copy_from_user & copy_to_user   */
    E_MS_ERR_BUSY          = 18,/* resource is busy,
                              ** eg. destroy a venc chn without unregister it */
    E_MS_ERR_CHN_NOT_STARTED          = 19,/* channel not start*/
    E_MS_ERR_CHN_NOT_STOPED          = 20,/* channel not stop*/
    E_MS_ERR_NOT_INIT          = 21,/* module not init before use it*/
    E_MS_ERR_NOT_ENABLE          = 22,/* device or channel  not  enable*/
    E_MS_ERR_SYS_TIMEOUT          = 23,/* sys timeout*/
    E_MS_ERR_FAILED          ,/* unexpected error */

    E_MS_ERR_MAX          = 127,/* maxium code, private error code of all modules
                              ** must be greater than it                      */
}MS_ErrCode_e;


typedef enum
{
    E_MS_GFX_FMT_I1 = 0, /* MS_ColorFormat */
    E_MS_GFX_FMT_I2,
    E_MS_GFX_FMT_I4,
    E_MS_GFX_FMT_I8,
    E_MS_GFX_FMT_FABAFGBG2266,
    E_MS_GFX_FMT_1ABFGBG12355,
    E_MS_GFX_FMT_RGB565,
    E_MS_GFX_FMT_ARGB1555,
    E_MS_GFX_FMT_ARGB4444,
    E_MS_GFX_FMT_ARGB1555_DST,
    E_MS_GFX_FMT_YUV422,
    E_MS_GFX_FMT_ARGB8888,
    E_MS_GFX_FMT_RGBA5551,
    E_MS_GFX_FMT_RGBA4444,
    E_MS_GFX_FMT_ABGR8888,
    E_MS_GFX_FMT_BGRA5551,
    E_MS_GFX_FMT_ABGR1555,
    E_MS_GFX_FMT_ABGR4444,
    E_MS_GFX_FMT_BGRA4444,
    E_MS_GFX_FMT_BGR565,
    E_MS_GFX_FMT_RGBA8888,
    E_MS_GFX_FMT_BGRA8888,
    E_MS_GFX_FMT_MAX
} MS_GFX_ColorFmt_e;


typedef enum
{
    E_MS_GFX_ROP_BLACK = 0,     /*Blackness*/
    E_MS_GFX_ROP_NOTMERGEPEN,   /*~(S2+S1)*/
    E_MS_GFX_ROP_MASKNOTPEN,    /*~S2&S1*/
    E_MS_GFX_ROP_NOTCOPYPEN,    /* ~S2*/
    E_MS_GFX_ROP_MASKPENNOT,    /* S2&~S1 */
    E_MS_GFX_ROP_NOT,           /* ~S1 */
    E_MS_GFX_ROP_XORPEN,        /* S2^S1 */
    E_MS_GFX_ROP_NOTMASKPEN,    /* ~(S2&S1) */
    E_MS_GFX_ROP_MASKPEN,       /* S2&S1 */
    E_MS_GFX_ROP_NOTXORPEN,     /* ~(S2^S1) */
    E_MS_GFX_ROP_NOP,           /* S1 */
    E_MS_GFX_ROP_MERGENOTPEN,   /* ~S2+S1 */
    E_MS_GFX_ROP_COPYPEN,       /* S2 */
    E_MS_GFX_ROP_MERGEPENNOT,   /* S2+~S1 */
    E_MS_GFX_ROP_MERGEPEN,      /* S2+S1 */
    E_MS_GFX_ROP_WHITE,         /* Whiteness */
    E_MS_GFX_ROP_NONE,          /* No Rop Op */
    E_MS_GFX_ROP_MAX,
} MS_GFX_RopCode_e;

typedef enum
{
    E_MS_GFX_RGB_OP_EQUAL = 0,
    E_MS_GFX_RGB_OP_NOT_EQUAL,
    E_MS_GFX_ALPHA_OP_EQUAL,
    E_MS_GFX_ALPHA_OP_NOT_EQUAL,
    E_MS_GFX_ARGB_OP_EQUAL,
    E_MS_GFX_ARGB_OP_NOT_EQUAL,
    E_MS_GFX_CKEY_OP_MAX,
} MS_GFX_ColorKeyOp_e;

typedef enum
{
    E_MS_GFX_DFB_BLD_ZERO = 0,
    E_MS_GFX_DFB_BLD_ONE,
    E_MS_GFX_DFB_BLD_SRCCOLOR,
    E_MS_GFX_DFB_BLD_INVSRCCOLOR,
    E_MS_GFX_DFB_BLD_SRCALPHA,
    E_MS_GFX_DFB_BLD_INVSRCALPHA,
    E_MS_GFX_DFB_BLD_DESTALPHA,
    E_MS_GFX_DFB_BLD_INVDESTALPHA,
    E_MS_GFX_DFB_BLD_DESTCOLOR,
    E_MS_GFX_DFB_BLD_INVDESTCOLOR,
    E_MS_GFX_DFB_BLD_SRCALPHASAT,
    E_MS_GFX_DFB_BLD_NONE,
    E_MS_GFX_DFB_BLD_MAX,
} MS_GFX_DfbBldOp_e;

typedef enum
{
    E_MS_GFX_MIRROR_NONE = 0,
    E_MS_GFX_MIRROR_HORIZONTAL,
    E_MS_GFX_MIRROR_VERTICAL,
    E_MS_GFX_MIRROR_BOTH,
    E_MS_GFX_MIRROR_MAX
} MS_GFX_Mirror_e;

typedef enum
{
    E_MS_GFX_ROTATE_0 = 0,
    E_MS_GFX_ROTATE_90,
    E_MS_GFX_ROTATE_180,
    E_MS_GFX_ROTATE_270,
    E_MS_GFX_ROTATE_MAX
} MS_GFX_Rotate_e;

typedef enum
{
    E_MS_GFX_YUV_YVYU = 0, // YUV422 format
    E_MS_GFX_YUV_YUYV,
    E_MS_GFX_YUV_VYUY,
    E_MS_GFX_YUV_UYVY,
    E_MS_GFX_YUV_MAX
} MS_GFX_Yuv422_e;

typedef enum
{
    E_MS_GFX_ERR_NOT_INIT = (0X22),
    E_MS_GFX_ERR_GFX_DRV_NOT_SUPPORT,
    E_MS_GFX_ERR_GFX_DRV_FAIL_FORMAT,
    E_MS_GFX_ERR_GFX_NON_ALIGN_ADDRESS,
    E_MS_GFX_ERR_GFX_NON_ALIGN_PITCH,
    E_MS_GFX_ERR_GFX_DRV_FAIL_OVERLAP,
    E_MS_GFX_ERR_GFX_DRV_FAIL_STRETCH,
    E_MS_GFX_ERR_GFX_DRV_FAIL_ITALIC,
    E_MS_GFX_ERR_GFX_DRV_FAIL_LOCKED,
    E_MS_GFX_ERR_GFX_DRV_FAIL_BLTADDR,
    E_MS_GFX_ERR_MAX
} MS_GFX_ErrCode_e;


/* GFX Module ErrorCode */
#define MI_ERR_GFX_INVALID_PARAM MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_ERR_ILLEGAL_PARAM)
#define MI_ERR_GFX_INVALID_DEVID MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_ERR_INVALID_DEVID)
#define MI_ERR_GFX_DEV_BUSY MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_ERR_BUSY)

#define MI_ERR_GFX_NOT_INIT MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_NOT_INIT)
#define MI_ERR_GFX_DRV_NOT_SUPPORT MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_NOT_SUPPORT)
#define MI_ERR_GFX_DRV_FAIL_FORMAT MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_FORMAT)
#define MI_ERR_GFX_NON_ALIGN_ADDRESS MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_NON_ALIGN_ADDRESS)
#define MI_ERR_GFX_NON_ALIGN_PITCH MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_NON_ALIGN_PITCH)
#define MI_ERR_GFX_DRV_FAIL_OVERLAP MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_OVERLAP)
#define MI_ERR_GFX_DRV_FAIL_STRETCH MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_STRETCH)
#define MI_ERR_GFX_DRV_FAIL_ITALIC MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_ITALIC)
#define MI_ERR_GFX_DRV_FAIL_LOCKED MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_LOCKED)
#define MI_ERR_GFX_DRV_FAIL_BLTADDR MHAL_DEF_ERR(E_MS_MODULE_ID_GFX, E_MS_ERR_LEVEL_ERROR, E_MS_GFX_ERR_GFX_DRV_FAIL_BLTADDR)

typedef struct MS_GFX_Rect_s
{
    MS_S32 s32Xpos;
    MS_S32 s32Ypos;
    MS_U32 u32Width;
    MS_U32 u32Height;
} MS_GFX_Rect_t;

typedef struct MS_GFX_ColorKey_s
{
    MS_U32 u32ColorStart;
    MS_U32 u32ColorEnd;
} MS_GFX_ColorKeyValue_t;

typedef struct MS_GFX_ColorKeyInfo_s
{
    MS_BOOL bEnColorKey;
    MS_GFX_ColorKeyOp_e eCKeyOp;
    MS_GFX_ColorFmt_e eCKeyFmt;
    MS_GFX_ColorKeyValue_t stCKeyVal;
} MS_GFX_ColorKeyInfo_t;


typedef struct MS_GFX_Surface_s
{
    MS_U64 phyAddr;
    MS_GFX_ColorFmt_e eColorFmt;
    MS_U32 u32Width;
    MS_U32 u32Height;
    MS_U32 u32Stride;
} MS_GFX_Surface_t;

typedef struct MS_GFX_Opt_s
{
    MS_BOOL bEnGfxRop;
    MS_GFX_RopCode_e eRopCode;
    MS_GFX_Rect_t stClipRect;
    MS_GFX_ColorKeyInfo_t stSrcColorKeyInfo;
    MS_GFX_ColorKeyInfo_t stDstColorKeyInfo;
    MS_GFX_DfbBldOp_e eSrcDfbBldOp;
    MS_GFX_DfbBldOp_e eDstDfbBldOp;
    MS_GFX_Mirror_e eMirror;
    MS_GFX_Yuv422_e eSrcYuvFmt;
    MS_GFX_Yuv422_e eDstYuvFmt;
    MS_GFX_Rotate_e eRotate;
} MS_GFX_Opt_t;

#ifdef __cplusplus
}
#endif

#endif///_MI_GFX_DATATYPE_H_
