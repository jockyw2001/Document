#ifndef __HT_COMMON_H__
#define __HT_COMMON_H__
#ifdef __cplusplus
extern "C"
{
#endif


#include "ht_vb_type.h"
#include "ht_common_datatype.h"
#include "ht_vb_type.h"

#define CHECK_NULL_POINTER(func, pointer) \
    do\
	{\
	    if(pointer==NULL) \
	    {\
            printk("%s Parameter NULL!Line: %d\n", func, __LINE__);\
            return HT_FAILURE;\
	    }\
	}while(0);

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printk("[%d]exec function failed\n", __LINE__);\
        return HT_FAILURE;\
    }\
    else\
    {\
        printk("(%d)exec function pass\n", __LINE__);\
    }

typedef enum
{
    E_HT_PIXEL_FORMAT_NULL,//(0)
    E_HT_PIXEL_FORMAT_YUV422_YUYV,//(1)
    E_HT_PIXEL_FORMAT_ARGB8888,//(2)
    E_HT_PIXEL_FORMAT_ABGR8888,//(3)
    E_HT_PIXEL_FORMAT_RGB565,//(4)
    E_HT_PIXEL_FORMAT_ARGB1555,//(5)
    E_HT_PIXEL_FORMAT_I2,//(6)
    E_HT_PIXEL_FORMAT_I4,//(7)
    E_HT_PIXEL_FORMAT_I8,//(8)
    E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_422,//(9)
    E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420,//(10)
    E_HT_PIXEL_FORMAT_YUV_MST_420,//(11)

    //vdec mstar private video format
    E_HT_PIXEL_FORMAT_YC420_MSTTILE1_H264,//(12)
    E_HT_PIXEL_FORMAT_YC420_MSTTILE2_H265,//(13)
    E_HT_PIXEL_FORMAT_YC420_MSTTILE3_H265,//(14)
    E_HT_PIXEL_FORMAT_MAX,
}HT_PxlFmt_e;

typedef enum
{
    E_HT_STREAM_MODULEID_NULL,
    E_HT_STREAM_MODULEID_VIF,
    E_HT_STREAM_MODULEID_VPE,
    E_HT_STREAM_MODULEID_VDEC,
    E_HT_STREAM_MODULEID_DIVP,
}HT_Stream_ModuleID;

typedef struct HT_YUVInfo_s
{
    HT_PxlFmt_e ePixelFormat;
    union
    {
       HT_YUV420_SP_Info_t  stYUV420SP;
       HT_YUV422_YUYV_Info_t stYUV422YUYV;
    };
}HT_YUVInfo_t;
typedef struct HT_OutputBufList_s
{
    HT_U8 u8Chnidx;
    HT_U8 u8Portidx;
    HT_U32 Width;
    HT_U32 Height;
    HT_YUVInfo_t stYuvVbInfo;
    HT_Stream_ModuleID eModuleID;
    HT_BOOL bWriteFileFlag;
    struct list_head list;
}HT_OutputBufList_t;


typedef enum
{
    E_HT_HVD = 0,  // 16 * 32
    E_HT_MVD,      //  8 * 32
    E_HT_EVD,      // 32 * 16
} HT_TILE_MODE;

#define GET_FRAME_HEIGHT(height)  ((((height)+31)>>5)<<5)
#define GET_FRAME_HEIGHT_EVD(height)  ((((height)+15)>>4)<<4)

/*malloc buffer*/
HT_U32 HT_Malloc(HT_VB_Info_t *pstVbInfo);
HT_U32 HT_MallocAlign(HT_VB_Info_t *pstVbInfo, HT_U32 u32Alignment);
HT_RESULT HT_MmapParser(HT_U8 *name, HT_U32 *addr, HT_U32 *size, HT_U8 *miu);

/*free buffer*/
void HT_Free(HT_PHY Pa);

HT_U8* HT_FindVirAddr(HT_PHY phyAddr);

/*write file*/
HT_S32 HT_WriteFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 s32Length, HT_S64 s64Pos);

/*read file*/
HT_S32 HT_ReadFile(const HT_U8 *pszFileName, HT_U8 *pszReadBuf, HT_S32 s32Length, HT_S64 s64Pos);

/*Malloc yuv One Frame buffer*/
HT_RESULT HT_MallocYUVOneFrame(HT_YUVInfo_t *pstYUVInfo, HT_U32 u32Width, HT_U32 u32Height);

/*Free One Frame buffer*/
HT_RESULT HT_FreeYUVOneFrame(HT_YUVInfo_t           *pstYUVInfo);

/*Read One Frame to buffer*/
HT_RESULT HT_ReadYUVOneFrameToBuffer(const HT_U8 *pszFileName, HT_YUVInfo_t               *pstYUVInfo, HT_S64 *ps64Pos);

/*Write One Frame to File*/
HT_RESULT HT_WriteYUVOneFrameToFile(const HT_U8 *pszFileName, HT_YUVInfo_t               *pstYUVInfo, HT_S64 *ps64Pos);

/*Sort Yuv*/
HT_RESULT HT_Soft_DetileYUV(void *pDstBuf, void *pSrcYBuf, void *pSrcUVBuf, HT_U32 u32Width, HT_U32 u32Pitch, HT_U32 u32Height, HT_TILE_MODE eMode);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif // __HT_VB_H__
