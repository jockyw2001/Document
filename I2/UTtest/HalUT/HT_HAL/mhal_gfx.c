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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   mi_gfx_impl.c
/// @brief gfx module impl
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <linux/string.h>
#include "MsTypes.h"

#include "mhal_gfx.h"
#include "mhal_gfx_datatype.h"

#include "apiGFX.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Macros
//-------------------------------------------------------------------------------------------------

#define GFX_ISVALID_POINT(X)  \
    {   \
        if( X == NULL)  \
        {   \
            printk("MI_ERR_INVALID_PARAMETER!\n");  \
            return MI_ERR_GFX_INVALID_PARAM;   \
        }   \
    }   \

//-------------------------------------------------------------------------------------------------
//  Local Enum
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_MI_GFX_YUV_RGB2YUV_PC = 0,  //Y£º16~235 UV:16~240
    E_MI_GFX_YUV_RGB2YUV_255,     //Y£º0-255  UV:0~255
    E_MI_GFX_YUV_RGB2YUV_MAX
} MI_GFX_Rgb2Yuv_e;

typedef enum
{
    E_MI_GFX_YUV_OUT_255 = 0,    // 0~255
    E_MI_GFX_YUV_OUT_PC,         // Y: 16~235, UV:16~240
    E_MI_GFX_YUV_OUT_MAX
}MI_GFX_YuvOutRange_e;

typedef enum
{
    E_MI_GFX_UV_IN_255 = 0,    // 0~255
    E_MI_GFX_UV_IN_127,        // -128~127
    E_MI_GFX_UV_IN_MAX
} MI_GFX_UvInRange_e;

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------
typedef struct MI_GFX_ResMgr_s
{
    MS_BOOL bInitFlag;
    MS_U8 u8Thresholdvalue;
    MS_U8 u8InitCnt;
} MI_GFX_ResMgr_t;

//-------------------------------------------------------------------------------------------------
// Local l Variables
//-------------------------------------------------------------------------------------------------
static MI_GFX_ResMgr_t _stGfxMgr = {0};

//-------------------------------------------------------------------------------------------------
//  local function  prototypes
//-------------------------------------------------------------------------------------------------
static MS_ColorFormat _MI_GFX_MappingColorFmt(MS_GFX_ColorFmt_e eColorFmt)
{
    switch(eColorFmt)
    {
        case E_MS_GFX_FMT_I1:
            return E_MS_FMT_I1;
        case E_MS_GFX_FMT_I2:
            return E_MS_FMT_I2;
        case E_MS_GFX_FMT_I4:
            return E_MS_FMT_I4;
        case E_MS_GFX_FMT_I8:
            return E_MS_FMT_I8;
        case E_MS_GFX_FMT_FABAFGBG2266:
            return E_MS_FMT_FaBaFgBg2266;
        case E_MS_GFX_FMT_1ABFGBG12355:
            return E_MS_FMT_1ABFgBg12355;
        case E_MS_GFX_FMT_RGB565:
            return E_MS_FMT_RGB565;
        case E_MS_GFX_FMT_ARGB1555:
            return E_MS_FMT_ARGB1555;
        case E_MS_GFX_FMT_ARGB4444:
            return E_MS_FMT_ARGB4444;
        case E_MS_GFX_FMT_ARGB1555_DST:
            return E_MS_FMT_ARGB1555_DST;
        case E_MS_GFX_FMT_YUV422:
            return E_MS_FMT_YUV422;
        case E_MS_GFX_FMT_ARGB8888:
            return E_MS_FMT_ARGB8888;
        case E_MS_GFX_FMT_RGBA5551:
            return E_MS_FMT_RGBA5551;
        case E_MS_GFX_FMT_RGBA4444:
            return E_MS_FMT_RGBA4444;
        case E_MS_GFX_FMT_ABGR8888:
            return E_MS_FMT_ABGR8888;
        case E_MS_GFX_FMT_BGRA5551:
            return E_MS_FMT_BGRA5551;
        case E_MS_GFX_FMT_ABGR1555:
            return E_MS_FMT_ABGR1555;
        case E_MS_GFX_FMT_ABGR4444:
            return E_MS_FMT_ABGR4444;
        case E_MS_GFX_FMT_BGRA4444:
            return E_MS_FMT_BGRA4444;
        case E_MS_GFX_FMT_BGR565:
            return E_MS_FMT_BGR565;
        case E_MS_GFX_FMT_RGBA8888:
            return E_MS_FMT_RGBA8888;
        case E_MS_GFX_FMT_BGRA8888:
            return E_MS_FMT_BGRA8888;
        default:
            return E_MS_FMT_ARGB8888;
    }
}

static GFX_DFBBldOP _MI_GFX_GetDfbBlendMode(MS_GFX_DfbBldOp_e eDfbBlendMode)
{
    switch(eDfbBlendMode)
    {
        case E_MS_GFX_DFB_BLD_ZERO:
            return GFX_DFB_BLD_OP_ZERO;
        case E_MS_GFX_DFB_BLD_ONE:
            return GFX_DFB_BLD_OP_ONE;
        case E_MS_GFX_DFB_BLD_SRCCOLOR:
            return GFX_DFB_BLD_OP_SRCCOLOR;
        case E_MS_GFX_DFB_BLD_INVSRCCOLOR:
            return GFX_DFB_BLD_OP_INVSRCCOLOR;
        case E_MS_GFX_DFB_BLD_SRCALPHA:
            return GFX_DFB_BLD_OP_SRCALPHA;
        case E_MS_GFX_DFB_BLD_INVSRCALPHA:
            return GFX_DFB_BLD_OP_INVSRCALPHA;
        case E_MS_GFX_DFB_BLD_DESTALPHA:
            return GFX_DFB_BLD_OP_DESTALPHA;
        case E_MS_GFX_DFB_BLD_INVDESTALPHA:
            return GFX_DFB_BLD_OP_INVDESTALPHA;
        case E_MS_GFX_DFB_BLD_DESTCOLOR:
            return GFX_DFB_BLD_OP_DESTCOLOR;
        case E_MS_GFX_DFB_BLD_INVDESTCOLOR:
            return GFX_DFB_BLD_OP_INVDESTCOLOR;
        case E_MS_GFX_DFB_BLD_SRCALPHASAT:
            return GFX_DFB_BLD_OP_SRCALPHASAT;
        default:
            return GFX_DFB_BLD_OP_ZERO;
    }
}

static GFX_ROP2_Op _MI_GFX_GetROP(MS_GFX_RopCode_e eROP)
{
    switch(eROP)
    {
        case E_MS_GFX_ROP_BLACK:
            return ROP2_OP_ZERO;
        case E_MS_GFX_ROP_NOTMERGEPEN:
            return ROP2_OP_NOT_PS_OR_PD;
        case E_MS_GFX_ROP_MASKNOTPEN:
            return ROP2_OP_NS_AND_PD;
        case E_MS_GFX_ROP_NOTCOPYPEN:
            return ROP2_OP_NS;
        case E_MS_GFX_ROP_MASKPENNOT:
            return ROP2_OP_PS_AND_ND;
        case E_MS_GFX_ROP_NOT:
            return ROP2_OP_ND;
        case E_MS_GFX_ROP_XORPEN:
            return ROP2_OP_PS_XOR_PD;
        case E_MS_GFX_ROP_NOTMASKPEN:
            return ROP2_OP_NOT_PS_AND_PD;
        case E_MS_GFX_ROP_MASKPEN:
            return ROP2_OP_PS_AND_PD;
        case E_MS_GFX_ROP_NOTXORPEN:
            return ROP2_OP_PS_XOR_PD;
        case E_MS_GFX_ROP_NOP:
            return ROP2_OP_PD;
        case E_MS_GFX_ROP_MERGENOTPEN:
            return ROP2_OP_NS_OR_PD;
        case E_MS_GFX_ROP_COPYPEN:
            return ROP2_OP_PS;
        case E_MS_GFX_ROP_MERGEPENNOT:
            return ROP2_OP_PS_OR_ND;
        case E_MS_GFX_ROP_MERGEPEN:
            return ROP2_OP_PD_OR_PS;
        case E_MS_GFX_ROP_WHITE:
            return ROP2_OP_ONE;
        default:
            break;
    }
    return ROP2_OP_PS;
}

static MS_S32 _MI_GFX_GetDefaultBlitOpt(MS_GFX_Opt_t *pstBlitOpt)
{
    pstBlitOpt->bEnGfxRop = FALSE;
    pstBlitOpt->eRopCode = E_MS_GFX_ROP_NONE;
    pstBlitOpt->eSrcDfbBldOp = E_MS_GFX_DFB_BLD_NONE;
    pstBlitOpt->eDstDfbBldOp = E_MS_GFX_DFB_BLD_NONE;
    pstBlitOpt->eMirror = E_MS_GFX_MIRROR_NONE;
    pstBlitOpt->eRotate = E_MS_GFX_ROTATE_0;
    pstBlitOpt->eSrcYuvFmt = 0;
    pstBlitOpt->eDstYuvFmt = 0;
    pstBlitOpt->stClipRect.s32Xpos = 0;
    pstBlitOpt->stClipRect.s32Ypos = 0;
    pstBlitOpt->stClipRect.u32Width  = 0;
    pstBlitOpt->stClipRect.u32Height = 0;

    return true;
}

static MS_S32 _MI_GFX_SetBitBlitOption(MS_GFX_Opt_t *pstBlitOpt, MS_GFX_ColorFmt_e eSrcFmt, MS_GFX_ColorFmt_e eDstFmt)
{
    //handle colorkey
    MS_GFX_ColorKeyOp_e eSrcCKeyOP = E_MS_GFX_RGB_OP_EQUAL;
    MS_GFX_ColorKeyOp_e eDstCKeyOP = E_MS_GFX_RGB_OP_EQUAL;
    MS_BOOL bEnSrcColorKey = FALSE;
    MS_BOOL bEnDstColorKey = FALSE;
    GFX_ColorKeyMode eColorKeyMode = CK_OP_EQUAL;
    GFX_Buffer_Format u8SrcFmt = E_MS_FMT_ARGB8888;
    GFX_Buffer_Format u8DstFmt = E_MS_FMT_ARGB8888;
    GFX_RgbColor stColorkeySrcStart = {0, 0, 0, 0};
    GFX_RgbColor stColorkeySrcEnd = {0, 0, 0, 0};
    GFX_RgbColor stColorkeyDstStart = {0, 0, 0, 0};
    GFX_RgbColor stColorkeyDstEnd = {0, 0, 0, 0};
    GFX_DFBBldOP eSrcDfbBldOp = _MI_GFX_GetDfbBlendMode(pstBlitOpt->eSrcDfbBldOp);
    GFX_DFBBldOP eDstDfbBldOp = _MI_GFX_GetDfbBlendMode(pstBlitOpt->eDstDfbBldOp);
    MS_BOOL bRopEnable = (E_MS_GFX_ROP_NONE == pstBlitOpt->bEnGfxRop) ? FALSE : TRUE;
    GFX_ROP2_Op eRopMode = ROP2_OP_ZERO;


    bEnSrcColorKey = pstBlitOpt->stSrcColorKeyInfo.bEnColorKey;
    if (TRUE == bEnSrcColorKey)
    {
        eSrcCKeyOP = pstBlitOpt->stSrcColorKeyInfo.eCKeyOp;
        u8SrcFmt = (GFX_Buffer_Format)_MI_GFX_MappingColorFmt(pstBlitOpt->stSrcColorKeyInfo.eCKeyFmt);

        stColorkeySrcStart.a = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorStart >> 24) & 0xFF;
        stColorkeySrcStart.r = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorStart >> 16) & 0xFF;
        stColorkeySrcStart.g = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorStart >> 8) & 0xFF;
        stColorkeySrcStart.b = pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorStart & 0xFF;

        stColorkeySrcEnd.a = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorEnd >> 24) & 0xFF;
        stColorkeySrcEnd.r = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorEnd >> 16) & 0xFF;
        stColorkeySrcEnd.g = (pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorEnd >> 8) & 0xFF;
        stColorkeySrcEnd.b = pstBlitOpt->stSrcColorKeyInfo.stCKeyVal.u32ColorEnd & 0xFF;

        if (E_MS_GFX_ARGB_OP_EQUAL == eSrcCKeyOP) // process rgb & alpha
        {
            eColorKeyMode = CK_OP_EQUAL;
            MApi_GFX_SetSrcColorKey(bEnSrcColorKey, eColorKeyMode, u8SrcFmt, &stColorkeySrcStart, &stColorkeySrcEnd);
            eColorKeyMode = AK_OP_EQUAL;
            MApi_GFX_SetSrcColorKey(bEnSrcColorKey, eColorKeyMode, u8SrcFmt, &stColorkeySrcStart, &stColorkeySrcEnd);
        }
        else if(E_MS_GFX_ARGB_OP_NOT_EQUAL == eSrcCKeyOP)
        {
            eColorKeyMode = CK_OP_NOT_EQUAL;
            MApi_GFX_SetSrcColorKey(bEnSrcColorKey, eColorKeyMode, u8SrcFmt, &stColorkeySrcStart, &stColorkeySrcEnd);
            eColorKeyMode = AK_OP_NOT_EQUAL;
            MApi_GFX_SetSrcColorKey(bEnSrcColorKey, eColorKeyMode, u8SrcFmt, &stColorkeySrcStart, &stColorkeySrcEnd);
        }
        else
        {
            eColorKeyMode = (GFX_ColorKeyMode)eSrcCKeyOP;
            MApi_GFX_SetSrcColorKey(bEnSrcColorKey, eColorKeyMode, u8SrcFmt, &stColorkeySrcStart, &stColorkeySrcEnd);
        }
    }
    else
    {
        memset(&stColorkeySrcStart, 0, sizeof(GFX_RgbColor));
        memset(&stColorkeySrcEnd, 0, sizeof(GFX_RgbColor));
        MApi_GFX_SetSrcColorKey(FALSE, CK_OP_NOT_EQUAL, GFX_FMT_ARGB1555, &stColorkeySrcStart, &stColorkeySrcEnd);
    }

    bEnDstColorKey = pstBlitOpt->stDstColorKeyInfo.bEnColorKey;
    if (TRUE == bEnDstColorKey)
    {
        eDstCKeyOP = pstBlitOpt->stDstColorKeyInfo.eCKeyOp;
        u8DstFmt = (GFX_Buffer_Format)_MI_GFX_MappingColorFmt(pstBlitOpt->stDstColorKeyInfo.eCKeyFmt);

        stColorkeyDstStart.a = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorStart >> 24) & 0xFF;
        stColorkeyDstStart.r = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorStart >> 16) & 0xFF;
        stColorkeyDstStart.g = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorStart >> 8) & 0xFF;
        stColorkeyDstStart.b = pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorStart & 0xFF;

        stColorkeyDstEnd.a = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorEnd >> 24) & 0xFF;
        stColorkeyDstEnd.r = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorEnd >> 16) & 0xFF;
        stColorkeyDstEnd.g = (pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorEnd >> 8) & 0xFF;
        stColorkeyDstEnd.b = pstBlitOpt->stDstColorKeyInfo.stCKeyVal.u32ColorEnd & 0xFF;

        if(E_MS_GFX_ARGB_OP_EQUAL == eSrcCKeyOP) // process rgb & alpha
        {
            eColorKeyMode = CK_OP_EQUAL;
            MApi_GFX_SetDstColorKey(bEnDstColorKey, eColorKeyMode, u8SrcFmt, &stColorkeyDstStart, &stColorkeyDstEnd);
            eColorKeyMode = AK_OP_EQUAL;
            MApi_GFX_SetDstColorKey(bEnDstColorKey, eColorKeyMode, u8SrcFmt, &stColorkeyDstStart, &stColorkeyDstEnd);
        }
        else if(E_MS_GFX_ARGB_OP_NOT_EQUAL == eSrcCKeyOP)
        {
            eColorKeyMode = CK_OP_NOT_EQUAL;
            MApi_GFX_SetDstColorKey(bEnDstColorKey, eColorKeyMode, u8SrcFmt, &stColorkeyDstStart, &stColorkeyDstEnd);
            eColorKeyMode = AK_OP_NOT_EQUAL;
            MApi_GFX_SetDstColorKey(bEnDstColorKey, eColorKeyMode, u8SrcFmt, &stColorkeyDstStart, &stColorkeyDstEnd);
        }
        else
        {
            eColorKeyMode = (GFX_ColorKeyMode)eSrcCKeyOP;
            MApi_GFX_SetDstColorKey(bEnDstColorKey, eColorKeyMode, u8SrcFmt, &stColorkeyDstStart, &stColorkeyDstEnd);
        }
    }
    else
    {
        memset(&stColorkeyDstStart, 0, sizeof(GFX_RgbColor));
        memset(&stColorkeyDstEnd, 0, sizeof(GFX_RgbColor));
        MApi_GFX_SetSrcColorKey(FALSE, CK_OP_NOT_EQUAL, GFX_FMT_ARGB1555, &stColorkeyDstStart, &stColorkeyDstEnd);
    }

    //handle alpha blend mode
    if ((E_MS_GFX_DFB_BLD_NONE != pstBlitOpt->eSrcDfbBldOp) || (E_MS_GFX_DFB_BLD_NONE != pstBlitOpt->eDstDfbBldOp))
    {
        MApi_GFX_EnableAlphaBlending(FALSE);
        MApi_GFX_SetDFBBldOP(eSrcDfbBldOp, eDstDfbBldOp);
        MApi_GFX_EnableDFBBlending(TRUE);
    }

    //handle rop mode
    if (TRUE == bRopEnable)
    {
        eRopMode = _MI_GFX_GetROP(pstBlitOpt->eRopCode);
        MApi_GFX_SetROP2(bRopEnable, eRopMode);
    }
    else
    {
        MApi_GFX_SetROP2(FALSE, ROP2_OP_ZERO);
    }

    //handle YUV csc fmt
    if ((E_MS_GFX_FMT_YUV422 == eSrcFmt) || (E_MS_GFX_FMT_YUV422 == eDstFmt))
    {
        MApi_GFX_SetDC_CSC_FMT(GFX_YUV_RGB2YUV_PC, GFX_YUV_OUT_PC, GFX_YUV_IN_255, pstBlitOpt->eSrcYuvFmt, pstBlitOpt->eDstYuvFmt);
    }

    //handle rotation
    MApi_GFX_SetRotate((GFX_RotateAngle)pstBlitOpt->eRotate);

    return true;
}

MS_S32 MHalGFXOpen(void)
{
    GFX_Config stGFXcfg;

    _stGfxMgr.u8InitCnt++;
    if ((_stGfxMgr.bInitFlag))
    {
        //printk("%s: Module has been initialized!\n", __FUNCTION__);
        return true;
    }

#if 0 //wait mmap.ini
    MI_S8 sMMapName[30];
    memset(sMMapName, 0, sizeof(sMMapName));
    memset(&stGFXcfg,0,sizeof(GFX_Config));
    sprintf((char *)sMMapName, "%s", MI_GEVQ_MMAP_ITEM); /* use mmap.ini ??? */
    MMapInfo_t* pMMap = MMAPInfo::GetInstance((EN_MMAP_Type)0)->get_mmap(MMAPInfo::GetInstance((EN_MMAP_Type)MMAP_TYPE_ORIGINAL)->StrToMMAPID((char *)sMMapName));
    if(pMMap)
    {
        printk("Get MMAP Info [%s]\n", sMMapName);
        printk("u32Addr=0x%x, u32Size=0x%x miu=0x%x\n", pMMap->u32Addr, pMMap->u32Size, pMMap->u32MiuNo);
        stGFXcfg.u32VCmdQAddr = pMMap->u32Addr;
        stGFXcfg.u32VCmdQSize = pMMap->u32Size;
        stGFXcfg.u8Miu = pMMap->u32MiuNo;
    }
    else
    {
        printk("Get MMAP Info [%s] failed!!!\n", sMMapName);
        return MI_FAILURE;
    }
#endif
    stGFXcfg.bIsCompt = TRUE;
    stGFXcfg.bIsHK = TRUE;
    MApi_GFX_Init(&stGFXcfg);
#if 0
    if(pMMap->u32Size >= 4*1024)
    {
        GFX_VcmqBufSize vqSize;
        GFX_Result ret = GFX_FAIL;

        if(pMMap->u32Size >= 512*1024)
            vqSize = GFX_VCMD_512K;
        else if(pMMap->u32Size >= 256*1024)
            vqSize = GFX_VCMD_256K;
        else if(pMMap->u32Size >= 128*1024)
            vqSize = GFX_VCMD_128K;
        else if(pMMap->u32Size >= 64*1024)
            vqSize = GFX_VCMD_64K;
        else if(pMMap->u32Size >= 32*1024)
            vqSize = GFX_VCMD_32K;
        else if(pMMap->u32Size >= 16*1024)
            vqSize = GFX_VCMD_16K;
        else if(pMMap->u32Size >= 8*1024)
            vqSize = GFX_VCMD_8K;
        else
            vqSize = GFX_VCMD_4K;
        ret = MApi_GFX_SetVCmdBuffer(pMMap->u32Addr, vqSize);

        if(GFX_SUCCESS == ret)
            MApi_GFX_EnableVCmdQueue(TRUE);
        else
            MApi_GFX_EnableVCmdQueue(FALSE);
    }
    else
    {
        MApi_GFX_EnableVCmdQueue(FALSE);
    }
#endif
    _stGfxMgr.bInitFlag = TRUE;
    _stGfxMgr.u8Thresholdvalue = 1; //default

    return true;
}

MS_S32 MHalGFXClose(void)
{
//    MS_U8 u8GfxExit = 0;
    _stGfxMgr.u8InitCnt--;
    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }
    if (0 == _stGfxMgr.u8InitCnt)
    {
        //MApi_GFX_Close(&u8GfxExit);
        _stGfxMgr.bInitFlag = FALSE;
    }

    return true;
}

MS_S32 MHalGFXPendingDone(MS_U16 u16TargetFence)
{
    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }
    if (GFX_SUCCESS != MApi_GFX_PollingTAGID(u16TargetFence))
    {
        return MI_ERR_GFX_DEV_BUSY;
    }

    return true;
}

MS_S32 MHalGFXWaitAllDone(MS_BOOL bWaitAllDone, MS_U16 u16TargetFence)
{
    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }

    if (TRUE == bWaitAllDone)
    {
        MS_U16 u16TagId = MApi_GFX_SetNextTAGID();
        MApi_GFX_WaitForTAGID(u16TagId);
    }
    else
    {
        MApi_GFX_WaitForTAGID(u16TargetFence);
    }

    return true;
}

MS_S32 MHalGFXQuickFill(MS_GFX_Surface_t *pstDst, MS_GFX_Rect_t *pstDstRect,
    MS_U32 u32ColorVal, MS_U16 *pu16Fence)
{
    MS_GFX_Opt_t stBlitOpt;
    GFX_RgbColor stColor = {0,0,0,0};
    GFX_RectFillInfo stFillBlk;
    GFX_Point stPiont0, stPiont1;
    GFX_BufferInfo stDstBuff;
    GFX_Result eRet;

    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }

    GFX_ISVALID_POINT(pstDst);
    GFX_ISVALID_POINT(pstDstRect);
    GFX_ISVALID_POINT(pu16Fence);
    memset(&stBlitOpt, 0x0, sizeof(MS_GFX_Opt_t));
    //INIT_ST(stBlitOpt);
    _MI_GFX_GetDefaultBlitOpt(&stBlitOpt);

    MApi_GFX_BeginDraw();
    //set destination area color
    memset(&stColor, 0x0,sizeof(GFX_RgbColor));
    stColor.a = (u32ColorVal >> 24) & 0xFF;
    stColor.r = (u32ColorVal >> 16) & 0xFF;
    stColor.g = (u32ColorVal >> 8) & 0xFF;
    stColor.b = (u32ColorVal) & 0xFF;

    // set destination clip area
    stPiont0.x = 0;
    stPiont0.y = 0;
    stPiont1.x = pstDst->u32Width;
    stPiont1.y = pstDst->u32Height;
    MApi_GFX_SetClip(&stPiont0, &stPiont1);

    // set destination buffer info
    stDstBuff.u32Addr = pstDst->phyAddr;
    stDstBuff.u32Width = pstDst->u32Width;
    stDstBuff.u32Height = pstDst->u32Height;
    stDstBuff.u32Pitch = pstDst->u32Stride;
    stDstBuff.u32ColorFmt = (GFX_Buffer_Format)_MI_GFX_MappingColorFmt(pstDst->eColorFmt);
    if (E_MS_GFX_FMT_ARGB1555 == pstDst->eColorFmt)
    {
        stDstBuff.u32ColorFmt = GFX_FMT_ARGB1555_DST;
    }
    MApi_GFX_SetDstBufferInfo(&stDstBuff,0);

    //Config the fill rect params to vars
    stFillBlk.dstBlock.x = pstDstRect->s32Xpos;
    stFillBlk.dstBlock.y = pstDstRect->s32Ypos;
    stFillBlk.dstBlock.width = pstDstRect->u32Width;
    stFillBlk.dstBlock.height = pstDstRect->u32Height;
    stFillBlk.colorRange.color_s = stColor;
    stFillBlk.colorRange.color_e = stColor;
    stFillBlk.fmt = pstDst->eColorFmt;
    stFillBlk.flag = GFXRECT_FLAG_COLOR_CONSTANT;
    eRet = MApi_GFX_RectFill(&stFillBlk);
    if (GFX_SUCCESS != eRet)
    {
        printk("MApi_GFX_BitBlt Fail rRet :%d \n",eRet);
        MApi_GFX_EndDraw();
        switch (eRet)
        {
            case GFX_INVALID_PARAMETERS:
                return MI_ERR_GFX_INVALID_PARAM;
            case GFX_DRV_NOT_SUPPORT:
                return MI_ERR_GFX_DRV_NOT_SUPPORT;
            case GFX_DRV_FAIL_FORMAT:
                return MI_ERR_GFX_DRV_FAIL_FORMAT;
            case GFX_NON_ALIGN_ADDRESS:
                return MI_ERR_GFX_NON_ALIGN_ADDRESS;
            case GFX_NON_ALIGN_PITCH:
                return MI_ERR_GFX_NON_ALIGN_PITCH;
            case GFX_DRV_FAIL_BLTADDR:
                return MI_ERR_GFX_DRV_FAIL_BLTADDR;
            case GFX_DRV_FAIL_OVERLAP:
                return MI_ERR_GFX_DRV_FAIL_OVERLAP;
            case GFX_DRV_FAIL_STRETCH:
                return MI_ERR_GFX_DRV_FAIL_STRETCH;
            case GFX_DRV_FAIL_ITALIC:
                return MI_ERR_GFX_DRV_FAIL_ITALIC;
            case GFX_DRV_FAIL_LOCKED:
                return MI_ERR_GFX_DRV_FAIL_LOCKED;
            default:
                return MI_ERR_GFX_INVALID_PARAM;
        }
    }
    *pu16Fence = MApi_GFX_SetNextTAGID();
    MApi_GFX_EndDraw();

    return true;
}

MS_S32 MHalGFXGetAlphaThresholdValue(MS_U8 *pu8ThresholdValue)
{
    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }

    GFX_ISVALID_POINT(pu8ThresholdValue);
    *pu8ThresholdValue = _stGfxMgr.u8Thresholdvalue;

    return true;
}

MS_S32 MHalGFXSetAlphaThresholdValue(MS_U8 u8ThresholdValue)
{
    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }
    //if (GFX_SUCCESS != MApi_GFX_SetAlpha_ARGB1555(u8ThresholdValue))
    //{
    //   printk("%s: Module is NOT initialized!\n", __FUNCTION__);
    //    return MI_ERR_GFX_INVALID_PARAM;
    //}
    _stGfxMgr.u8Thresholdvalue = u8ThresholdValue;

    return true;
}

MS_S32 MHalGFXBitBlit(MS_GFX_Surface_t *pstSrc, MS_GFX_Rect_t *pstSrcRect,
    MS_GFX_Surface_t *pstDst,  MS_GFX_Rect_t *pstDstRect, MS_GFX_Opt_t *pstOpt, MS_U16 *pu16Fence)
{
    GFX_DrawRect stBitBltInfo;
    GFX_BufferInfo stGfxSrcBuf, stGfxDstBuf;
    GFX_Point stPiont0, stPiont1;
    MS_S32 s32Ret = -1;
    MS_U32 u32DrawFlag = 0;
    GFX_Result eRet;

    if (!_stGfxMgr.bInitFlag)
    {
        printk("%s: Module is NOT initialized!\n", __FUNCTION__);
        return MI_ERR_GFX_NOT_INIT;
    }

    GFX_ISVALID_POINT(pstSrc);
    GFX_ISVALID_POINT(pstSrcRect);
    GFX_ISVALID_POINT(pstDst);
    GFX_ISVALID_POINT(pstDstRect);
    GFX_ISVALID_POINT(pstOpt);
    GFX_ISVALID_POINT(pu16Fence);

    MApi_GFX_BeginDraw();

    s32Ret = _MI_GFX_SetBitBlitOption(pstOpt, pstSrc->eColorFmt, pstDst->eColorFmt);
    if (true == s32Ret)
    {
        printk("_MI_GFX_SetBitBlitOption success\n");
    }
    else
    {
        printk("_MI_GFX_SetBitBlitOption Fail!!!\n");
        MApi_GFX_EndDraw();
        return s32Ret;
    }
    switch (pstOpt->eMirror)
    {
        case E_MS_GFX_MIRROR_VERTICAL://error
            pstSrcRect->s32Xpos += pstSrcRect->u32Width - 1;
            MApi_GFX_SetMirror(TRUE, FALSE);
            break;
        case E_MS_GFX_MIRROR_HORIZONTAL:
            pstSrcRect->s32Ypos += pstSrcRect->u32Height - 1;
            MApi_GFX_SetMirror(FALSE, TRUE);
            break;
        case E_MS_GFX_MIRROR_BOTH:
            pstSrcRect->s32Xpos += pstSrcRect->u32Width - 1;
            pstSrcRect->s32Ypos += pstSrcRect->u32Height - 1;
            MApi_GFX_SetMirror(TRUE, TRUE);
            break;
        case E_MS_GFX_MIRROR_NONE:
        default:
            MApi_GFX_SetMirror(FALSE, FALSE);
            break;
    }

    stGfxSrcBuf.u32ColorFmt = (GFX_Buffer_Format)_MI_GFX_MappingColorFmt(pstSrc->eColorFmt);
    stGfxSrcBuf.u32Addr = pstSrc->phyAddr;
    stGfxSrcBuf.u32Pitch = pstSrc->u32Stride;
    stGfxSrcBuf.u32Width = pstSrc->u32Width;
    stGfxSrcBuf.u32Height = pstSrc->u32Height;
    MApi_GFX_SetSrcBufferInfo(&stGfxSrcBuf, 0);

    stGfxDstBuf.u32ColorFmt = (GFX_Buffer_Format)_MI_GFX_MappingColorFmt(pstDst->eColorFmt);
    stGfxDstBuf.u32Addr = pstDst->phyAddr;
    stGfxDstBuf.u32Pitch = pstDst->u32Stride;
    stGfxDstBuf.u32Width = pstDst->u32Width;
    stGfxDstBuf.u32Height = pstDst->u32Height;
    if(E_MS_GFX_FMT_ARGB1555 == pstDst->eColorFmt)
        stGfxDstBuf.u32ColorFmt = GFX_FMT_ARGB1555_DST;
    MApi_GFX_SetDstBufferInfo(&stGfxDstBuf, 0);

    stPiont0.x = 0;
    stPiont0.y = 0;

    if (pstSrc->u32Width >= pstDst->u32Width)
    {
        stPiont1.x = pstSrc->u32Width;
    }
    else
    {
        stPiont1.x = pstDst->u32Width;
    }
    if (pstSrc->u32Height >= pstDst->u32Height)
    {
        stPiont1.y = pstSrc->u32Height;
    }
    else
    {
        stPiont1.y = pstDst->u32Height;
    }
    MApi_GFX_SetClip(&stPiont0, &stPiont1);

    stBitBltInfo.srcblk.height = pstSrcRect->u32Height;
    stBitBltInfo.srcblk.width = pstSrcRect->u32Width;
    stBitBltInfo.srcblk.x = pstSrcRect->s32Xpos;
    stBitBltInfo.srcblk.y = pstSrcRect->s32Ypos;

    stBitBltInfo.dstblk.height = pstDstRect->u32Height;
    stBitBltInfo.dstblk.width = pstDstRect->u32Width;
    stBitBltInfo.dstblk.x = pstDstRect->s32Xpos;
    stBitBltInfo.dstblk.y = pstDstRect->s32Ypos;

    if ((pstSrcRect->u32Width == pstDstRect->u32Width) && (pstSrcRect->u32Height == pstDstRect->u32Height))
    {
        u32DrawFlag = GFXDRAW_FLAG_DEFAULT;
    }
    else // stretch bitblt, driver will switch  to 1P mode automatically
    {
        u32DrawFlag = GFXDRAW_FLAG_SCALE; // scale
    }
    eRet = MApi_GFX_BitBlt(&stBitBltInfo, u32DrawFlag);
    if(GFX_SUCCESS != eRet)
    {
        printk("MApi_GFX_BitBlt Fail rRet :%d \n",eRet);
        MApi_GFX_EndDraw();
        switch (eRet)
        {
            case GFX_INVALID_PARAMETERS:
                return MI_ERR_GFX_INVALID_PARAM;
            case GFX_DRV_NOT_SUPPORT:
                return MI_ERR_GFX_DRV_NOT_SUPPORT;
            case GFX_DRV_FAIL_FORMAT:
                return MI_ERR_GFX_DRV_FAIL_FORMAT;
            case GFX_NON_ALIGN_ADDRESS:
                return MI_ERR_GFX_NON_ALIGN_ADDRESS;
            case GFX_NON_ALIGN_PITCH:
                return MI_ERR_GFX_NON_ALIGN_PITCH;
            case GFX_DRV_FAIL_BLTADDR:
                return MI_ERR_GFX_DRV_FAIL_BLTADDR;
            case GFX_DRV_FAIL_OVERLAP:
                return MI_ERR_GFX_DRV_FAIL_OVERLAP;
            case GFX_DRV_FAIL_STRETCH:
                return MI_ERR_GFX_DRV_FAIL_STRETCH;
            case GFX_DRV_FAIL_ITALIC:
                return MI_ERR_GFX_DRV_FAIL_ITALIC;
            case GFX_DRV_FAIL_LOCKED:
                return MI_ERR_GFX_DRV_FAIL_LOCKED;
            default:
                return MI_ERR_GFX_INVALID_PARAM;
        }
    }
    *pu16Fence = MApi_GFX_SetNextTAGID();
    MApi_GFX_EndDraw();

    return true;
}
