#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <asm/io.h>

//#include "HW_Config.h"
#include "UFO.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "apiVDEC_EX.h"
#include "apiJPEG.h"    //for jpeg

#include "ht_common.h"
#include "ht_vdec.h"


//#define MODULE_LINKAGE //Module Linkage switch

#define HT_VDEC_CHN HT_U16

#ifdef HT_WRITE_DDR //save yuv to ddr
#define HT_VDEC_WRITE_FRAME_NUM 3
#else //save yuv to file
	#ifdef MODULE_LINKAGE
		#define HT_VDEC_WRITE_FRAME_NUM 10 //limit save 10 frame when in MODULE_LINKAGE mode
	#else
		#define HT_VDEC_WRITE_FRAME_NUM 40
	#endif
#endif

#define HT_VDEC_PARA_NUM_GRP 4  // num of parameter in a group
#define HT_VDEC_MAX_PARA_NUM 16 //input max num of parameter

#define HT_VDEC_MAX_CHN_NUM 16
#define HT_VDEC_FILENAME_LEN 64 //file path length

#define HT_VDEC_ES_MEM_SIZE    (1*1024*1024)

#define HT_VDEC_READ_STREAM_LEN  (1024 * 128)               //read stream length one time
#define HT_JPEG_READ_BUFFER_LEN (1024*1024)

#define HT_VDEC_PAGE_SIZE (0x1000)
#define HT_VDEC_IMAGE_PITCH_ALIGN_SIZE (32)

#define HT_U32VALUE(pu8Data, index) ((pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3]))
#define ALIGN_BYTES(value, base)       (((value) + ((base) - 1)) & ~((base) - 1))

#define DOWN(x) down(x)
#define UP(x) up(x)

extern struct list_head gstOutputBufHead;//outputbuf list

#define CHECK_NULL_POINTER_NORET(func, pointer) \
	do\
	{\
		if(NULL == pointer)\
		{\
			printk("%s %p is NULL", func, pointer);\
			return;\
		}\
	}while(0)


#define HT_VDEC_ChkChnCreate(VdecChn) \
		do\
		{\
			if (FALSE == _stResMgr.astChnInfo[VdecChn].bCreate)\
			{\
				return;\
			}\
		}while(0)


typedef void *HT_VDEC_DRV_HANDLE;


typedef enum
{
    E_HT_VDEC_CODEC_TYPE_H264 = 0x0,
    E_HT_VDEC_CODEC_TYPE_H265,
    E_HT_VDEC_CODEC_TYPE_JPEG,
    E_HT_VDEC_CODEC_TYPE_MAX
} HT_VDEC_CodecType_e;

typedef enum
{
    E_HT_VDEC_VIDEO_MODE_STREAM = 0x0,
    E_HT_VDEC_VIDEO_MODE_FRAME,
    E_HT_VDEC_VIDEO_MODE_MAX
} HT_VDEC_VideoMode_e;

typedef enum
{
    E_HT_VDEC_JPEG_FORMAT_YCBCR400 = 0x0,
    E_HT_VDEC_JPEG_FORMAT_YCBCR420,
    E_HT_VDEC_JPEG_FORMAT_YCBCR422,
    E_HT_VDEC_JPEG_FORMAT_YCBCR444,
    E_HT_VDEC_JPEG_FORMAT_MAX
} HT_VDEC_JpegFormat_e;

typedef struct HT_VDEC_JpegAttr_s
{
    HT_VDEC_JpegFormat_e    eJpegFormat;
}HT_VDEC_JpegAttr_t;

typedef struct HT_VDEC_VideoAttr_s
{
    HT_U32  u32RefFrameNum;
}HT_VDEC_VideoAttr_t;

typedef struct HT_VDEC_MemBufInfo_s
{
    HT_PHY phyAddr;
    void   *pVirAddr;
    HT_U32 u32BufSize;
} HT_VDEC_MemBufInfo_t;

typedef enum
{
    E_HT_VDEC_DECODE_MODE_ALL = 0x0,
    E_HT_VDEC_DECODE_MODE_I,
    E_HT_VDEC_DECODE_MODE_IP,
    E_HT_VDEC_DECODE_MODE_MAX
} HT_VDEC_DecodeMode_e;

typedef enum
{
    E_HT_VDEC_OUTPUT_ORDER_DISPLAY = 0x0,
    E_HT_VDEC_OUTPUT_ORDER_DECODE,
    E_HT_VDEC_OUTPUT_ORDER_MAX,
} HT_VDEC_OutputOrder_e;

typedef enum
{
    E_HT_VDEC_VIDEO_FORMAT_TILE = 0x0,
    E_HT_VDEC_VIDEO_FORMAT_REDUCE,
    E_HT_VDEC_VIDEO_FORMAT_MAX
} HT_VDEC_VideoFormat_e;

typedef struct HT_VDEC_ChnParam_s
{
    HT_U32 u32DecFramebufferNum;
    HT_VDEC_DecodeMode_e eDecMode;
    HT_VDEC_OutputOrder_e eOutputOrder;
    HT_VDEC_VideoFormat_e eVideoFormat;
} HT_VDEC_ChnParam_t;

typedef enum
{
    E_HT_VDEC_DISPLAY_MODE_PREVIEW = 0x0,
    E_HT_VDEC_DISPLAY_MODE_PLAYBACK,
    E_HT_VDEC_DISPLAY_MODE_MAX,
} HT_VDEC_DisplayMode_e;


typedef enum
{
    E_HT_VDEC_FRAME_TILE_MODE_NONE = 0,
    E_HT_VDEC_FRAME_TILE_MODE_16x16,      // tile mode 16x16
    E_HT_VDEC_FRAME_TILE_MODE_16x32,      // tile mode 16x32
    E_HT_VDEC_FRAME_TILE_MODE_32x16,      // tile mode 32x16
    E_HT_VDEC_FRAME_TILE_MODE_32x32,      // tile mode 32x32
    E_HT_VDEC_FRAME_TILE_MODE_MAX
} HT_VDEC_FrameTileMode_e;


typedef enum
{
    E_HT_VDEC_FRAME_SCAN_MODE_PROGRESSIVE = 0x0,  // progessive.
    E_HT_VDEC_FRAME_SCAN_MODE_INTERLACE   = 0x1,  // interlace.
    E_HT_VDEC_FRAME_SCAN_MODE_MAX,
} HT_VDEC_FrameScanMode_e;

typedef enum
{
    E_HT_VDEC_FIELDTYPE_NONE,        //< no field.
    E_HT_VDEC_FIELDTYPE_TOP,           //< Top field only.
    E_HT_VDEC_FIELDTYPE_BOTTOM,    //< Bottom field only.
    E_HT_VDEC_FIELDTYPE_BOTH,        //< Both fields.
    E_HT_VDEC_FIELDTYPE_NUM
} HT_VDEC_FieldType_e;


typedef enum
{
    E_HT_VDEC_FRAME_TYPE_I = 0,
    E_HT_VDEC_FRAME_TYPE_P,
    E_HT_VDEC_FRAME_TYPE_B,
    E_HT_VDEC_FRAME_TYPE_OTHER,
    E_HT_VDEC_FRAME_TYPE_NUM
} HT_VDEC_FrameType_e;


typedef struct
{
    HT_U32 u32LumaAddr;                           // frame buffer base + the start offset of current displayed luma data. Unit: byte.
    HT_U32 u32ChromaAddr;                       // frame buffer base + the start offset of current displayed chroma data. Unit: byte.
    HT_U32 u32TimeStamp;                         // Time stamp(DTS, PTS) of current displayed frame. Unit: ms (todo: 90khz)
    HT_U32 u32IdL;                                    // low part of ID number
    HT_U32 u32IdH;                                   // high part of ID number
    HT_U16 u16Pitch;                                   // pitch
    HT_U16 u16Width;                                  // width
    HT_U16 u16Height;                                 // hight
    HT_VDEC_FrameType_e eFrameType;    //< Frame type: I, P, B frame
    HT_VDEC_FieldType_e eFieldType;         //< Field type: Top, Bottom, Both
} HT_VDEC_FrameInfo_t;


typedef struct HT_VDEC_FrameInfoExt_s
{
    HT_PHY phyLumaAddr2bit;                   // physical address of Luma 2bit buffer
    HT_PHY phyChromaAddr2bit;                 // physical address of Chroma 2bit buffer
    HT_U8  u8LumaBitDepth;                    // Luma Frame bitdepth, support 8~10bits now
    HT_U8  u8ChromaBitDepth;                  // Chroma Frame bitdepth, support 8~10bits now
    HT_U16 u16Pitch2bit;                      // pitch of 2bits frame buffer
    HT_VDEC_FrameTileMode_e eFrameTileMode;  // Frame tile mode
    HT_VDEC_FrameScanMode_e eFrameScanMode;  // Frame scan mode
} HT_VDEC_FrameInfoExt_t;

typedef enum
{
    E_HT_VDEC_DB_MODE_H264_H265   = 0x00,
    E_HT_VDEC_DB_MODE_VP9         = 0x01,
    E_HT_VDEC_DB_MODE_MAX
} HT_VDEC_DbMode_e; // Decoder Buffer Mode

typedef struct HT_VDEC_DbInfo_s
{
    HT_BOOL bDbEnable;           // Decoder Buffer Enable
    HT_U8   u8DbSelect;          // Decoder Buffer select
    HT_BOOL bHMirror;
    HT_BOOL bVMirror;
    HT_BOOL bUncompressMode;
    HT_BOOL bBypassCodecMode;
    HT_VDEC_DbMode_e eDbMode;        // Decoder Buffer mode
    HT_U16 u16StartX;
    HT_U16 u16StartY;
    HT_U16 u16HSize;
    HT_U16 u16VSize;
    HT_PHY phyDbBase;          // Decoder Buffer base addr
    HT_U16 u16DbPitch;         // Decoder Buffer pitch
    HT_U8  u8DbMiuSel;         // Decoder Buffer Miu select
    HT_PHY phyLbAddr;          // Lookaside buffer addr
    HT_U8  u8LbSize;           // Lookaside buffer size
    HT_U8  u8LbTableId;        // Lookaside buffer table Id
}HT_VDEC_DbInfo_t; // Decoder Buffer Info

typedef enum
{

    E_HT_VDEC_PIXEL_FRAME_YUV422_YUYV = 0,
    E_HT_VDEC_PIXEL_FRAME_ARGB8888,
    E_HT_VDEC_PIXEL_FRAME_ABGR8888,

    E_HT_VDEC_PIXEL_FRAME_RGB565,
    E_HT_VDEC_PIXEL_FRAME_ARGB1555,
    E_HT_VDEC_PIXEL_FRAME_I2,
    E_HT_VDEC_PIXEL_FRAME_I4,
    E_HT_VDEC_PIXEL_FRAME_I8,

    E_HT_VDEC_PIXEL_FRAME_YUV_SEMIPLANAR_422,
    E_HT_VDEC_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    E_HT_VDEC_PIXEL_FRAME_YUV_MST_420,


    //vdec mstar private video format
    E_HT_VDEC_PIXEL_FRAME_YC420_MSTTILE1_H264,
    E_HT_VDEC_PIXEL_FRAME_YC420_MSTTILE2_H265,
    E_HT_VDEC_PIXEL_FRAME_YC420_MSTTILE3_H265,
    E_HT_VDEC_PIXEL_FRAME_FORMAT_MAX,
} HT_VDEC_PixelFormat_e;


typedef struct HT_VDEC_DispFrame_s
{
    HT_VDEC_FrameInfo_t stFrmInfo;    //< frame information
    HT_U32 u32PriData;                         //< firmware private data
    HT_U32 u32Idx;                               //< index used by apiVDEC to manage VDEC_DispQ[][]
    HT_VDEC_FrameInfoExt_t stFrmInfoExt;        // Frame Info Extend
    HT_VDEC_DbInfo_t stDbInfo;
    HT_VDEC_PixelFormat_e ePixelFrm;
	HT_U64 u64FastChnId;
} HT_VDEC_DispFrame_t;


typedef struct HT_VDEC_VdecStream_s
{
	HT_U8	*pu8Addr;
    HT_U32  u32Len;
    HT_U64  u64PTS;
    HT_BOOL bEndOfFrame;
    HT_BOOL bEndOfStream;

	HT_BOOL bReadSuccess;
	HT_BOOL bQuerySuccess;
	HT_BOOL bWriteSuccess;
} HT_VDEC_Stream_t;


typedef struct HT_VDEC_BufInfo_s
{
    HT_U8   *pu8Addr;
    HT_PHY  phyAddr;
    HT_U32  u32BufSize;
    HT_U64  u64Pts;
    HT_BOOL bEndOfStream;

    HT_BOOL bPictureStart;
    HT_BOOL bBrokenByUs;
} HT_VDEC_BufInfo_t;

typedef struct HT_VDEC_ChnAttr_s
{
    HT_VDEC_CodecType_e eCodecType;
    HT_U32 u32BufSize;
    HT_U32 u32Priority;
    HT_U32 u32PicWidth;
    HT_U32 u32PicHeight;
    HT_VDEC_VideoMode_e eVideoMode;
    union
    {
        HT_VDEC_JpegAttr_t stVdecJpegAttr;
        HT_VDEC_VideoAttr_t stVdecVideoAttr;
    };
} HT_VDEC_ChnAttr_t;


typedef struct
{
    HT_VDEC_MemBufInfo_t stCpuMemInfo;
    HT_VDEC_MemBufInfo_t stEsMemInfo;
    HT_VDEC_MemBufInfo_t stFrameMemInfo;
    HT_BOOL bCreate;
    HT_BOOL bStart;
	HT_BOOL bChnConfig;

    HT_VDEC_ChnAttr_t stChnAttr;
    HT_VDEC_ChnParam_t stChnParam;
    HT_VDEC_DisplayMode_e eDisplayMode;
    VDEC_StreamId stVDECStreamId;
    HT_U8 u8EnableMfcodec;

	HT_U64 u64JpegDecErr;
	HT_U64 u64JpegDecSuc;
	HT_U64 u64JpegDecBytes;
} HT_VDEC_ChnInfo_t;


typedef struct HT_VDEC_ResMgr_s
{
    HT_BOOL bInitFlag;

	HT_BOOL bWrBufTaskRun;  //_pWrBufTask Flag
	HT_BOOL bPutFrmTaskRun; //_pPutFrmTask Flag
    HT_BOOL bPutFrmJpegTaskRun; //_pPutFrmJpegTask Flag

	struct task_struct *pWrBufTask;  //write buffer thread
	struct task_struct *pPutFrmTask; //put frame thread
    struct task_struct *pPutJpegFrmTask; //put Jpeg frame thread

	HT_U32 u32ChnNum;
	HT_U16 au16ChnId[HT_VDEC_MAX_CHN_NUM];
    HT_VDEC_ChnInfo_t astChnInfo[HT_VDEC_MAX_CHN_NUM];

    //wait_queue_head_t stInjWaitQueueHead;
    //wait_queue_head_t stPutWaitQueueHead;
    //HT_VDEC_DRV_HANDLE hVdecDev;
	HT_VDEC_MemBufInfo_t stCpuMem;
    HT_VDEC_MemBufInfo_t stDvXcShmMem;
    HT_VDEC_MemBufInfo_t stJpegInternalMem;

	HT_S8 as8EsFilename[HT_VDEC_MAX_CHN_NUM][HT_VDEC_FILENAME_LEN];
	HT_S8 as8YuvFilename[HT_VDEC_MAX_CHN_NUM][HT_VDEC_FILENAME_LEN];
	HT_BOOL bFileOpen[HT_VDEC_MAX_CHN_NUM];
	struct file *fpFrame[HT_VDEC_MAX_CHN_NUM];
    mm_segment_t fsFrame[HT_VDEC_MAX_CHN_NUM];

    HT_VB_Info_t stYuv2DdrMem[HT_VDEC_MAX_CHN_NUM];
	HT_U32	as32SaveYuvNum[HT_VDEC_MAX_CHN_NUM];
	HT_BOOL bAllocDdrMem[HT_VDEC_MAX_CHN_NUM];

    struct semaphore semChnLock[HT_VDEC_MAX_CHN_NUM];
} HT_VDEC_ResMgr_t;

typedef enum
{
    E_HT_VDEC_MFCODEC_UNSUPPORT = 0x00,
    E_HT_VDEC_MFCODEC_10 = 0x01,           // MFDEC version 1.0
    E_HT_VDEC_MFCODEC_20 = 0x05,           // MFDEC version 2.0
    E_HT_VDEC_MFCODEC_25 = 0x06,           // MFDEC version 2.5
    E_HT_VDEC_MFCODEC_30 = 0x07,           // MFDEC version 3.0
    E_HT_VDEC_MFCODEC_DISABLE   = 0xFF,
} HT_VDEC_MfCodecVersion_e;


/**************************************************************/
// for jpeg use
typedef enum
{
    E_HT_BUFDATA_RAW = 0,
    E_HT_BUFDATA_FRAME,
    E_HT_BUFDATA_META,
} HT_BufDataType_e;

typedef enum
{
    E_HT_COMPRESS_MODE_NONE, 	//no compress
    E_HT_COMPRESS_MODE_SEG,  	//compress unit is 256 bytes as a segment
    E_HT_COMPRESS_MODE_LINE, 	//compress unit is the whole line
    E_HT_COMPRESS_MODE_FRAME,	//compress unit is the whole frame
    E_HT_COMPRESS_MODE_BUTT, 	//number
} HT_CompressMode_e;

typedef enum
{
    E_HT_MODULE_ID_VDEC = 0,
    E_HT_MODULE_ID_VENC,
    E_HT_MODULE_ID_DISP,
    E_HT_MODULE_ID_VIF,
    E_HT_MODULE_ID_AI,
    E_HT_MODULE_ID_AO,
    E_HT_MODULE_ID_RGN,
    E_HT_MODULE_ID_VPE,
    E_HT_MODULE_ID_DIVP,
    E_HT_MODULE_ID_GFX,
    E_HT_MODULE_ID_IVE,
    E_HT_MODULE_ID_IAE,
    E_HT_MODULE_ID_MD,
    E_HT_MODULE_ID_OD,
    E_HT_MODULE_ID_VDF,
    E_HT_MODULE_ID_VDISP,
    E_HT_MODULE_ID_FB,
    E_HT_MODULE_ID_MAX,
} HT_ModuleId_e;

typedef struct HT_FrameData_s
{
    HT_VDEC_FrameTileMode_e eTileMode;
    HT_VDEC_PixelFormat_e   ePixelFormat;
    HT_CompressMode_e 		eCompressMode;
    HT_VDEC_FrameScanMode_e eFrameScanMode;
    HT_VDEC_FieldType_e 	eFieldType;

    HT_U16 u16Width;
    HT_U16 u16Height;

    void*  pVirAddr[3];
    HT_PHY phyAddr[3];	  //notice that this is miu bus addr,not cpu bus addr.
    HT_U32 u32Stride[3];

} HT_FrameData_t;

typedef struct HT_RawData_s
{
    void*   pVirAddr;
    HT_PHY  phyAddr;//notice that this is miu bus addr,not cpu bus addr.
    HT_U32  u32BufSize;

    HT_U32  u32ContentSize;
    HT_BOOL bEndOfFrame;
} HT_RawData_t;

typedef struct HT_MetaData_s
{
    void*  pVirAddr;
    HT_PHY phyAddr;//notice that this is miu bus addr,not cpu bus addr.

    HT_U32 u32Size;
    HT_U32 u32ExtraData;    /*driver special flag*/
    HT_ModuleId_e eDataFromModule;
} HT_MetaData_t;

typedef struct HT_DataBufInfo_s
{
    HT_U64  u64Pts;
    HT_U64  u64SidebandMsg;
    HT_BufDataType_e eBufType;
    HT_BOOL bEndOfStream;
    HT_BOOL bUsrBuf;
    union
    {
        HT_FrameData_t stFrameData;
        HT_RawData_t   stRawData;
        HT_MetaData_t  stMetaData;
    };
} HT_BufInfo_t;
/**************************************************************/


static HT_VDEC_ResMgr_t _stResMgr; //vdec info
static HT_BOOL _bChnTaskFinished[HT_VDEC_MAX_CHN_NUM] = {FALSE};


static HT_RESULT _HT_VDEC_ReadOneFrame(HT_VDEC_Stream_t *pstStreamBuf, struct file *fp)
{
	HT_U32 u32Len  = 0;
	HT_U64 u64PtsH = 0;
	HT_U64 u64PtsL = 0;
	HT_U32 u32Pos  = 0;
    HT_U8  au8Header[16] = {0};

	CHECK_NULL_POINTER(__FUNCTION__, pstStreamBuf);
	CHECK_NULL_POINTER(__FUNCTION__, fp);

	memset(au8Header, 0, 16);
	u32Pos = fp->f_op->llseek(fp, 0L, SEEK_CUR);
	u32Len = fp->f_op->read(fp, au8Header, 16, &(fp->f_pos));
	if(u32Len <= 0)
	{
		pstStreamBuf->bReadSuccess = FALSE;
		fp->f_op->llseek(fp, 0L, SEEK_SET);
		return HT_FAILURE;
	}

	pstStreamBuf->u32Len = HT_U32VALUE(au8Header, 4);
	u64PtsH = HT_U32VALUE(au8Header, 8);
	u64PtsL = HT_U32VALUE(au8Header, 12);
	pstStreamBuf->u64PTS = u64PtsH << 32 | u64PtsL;

	u32Len = fp->f_op->read(fp, pstStreamBuf->pu8Addr, pstStreamBuf->u32Len, &(fp->f_pos));
	if(u32Len <= 0)
	{
		pstStreamBuf->bReadSuccess = FALSE;
		fp->f_op->llseek(fp, 0L, SEEK_SET);
		return HT_FAILURE;
	}

	pstStreamBuf->bReadSuccess = TRUE;
	return HT_SUCCESS;
}


static HT_BOOL _HT_VDEC_CheckChnTaskStatus(HT_VDEC_CHN VdecChn)
{
	return _bChnTaskFinished[VdecChn];
}

static HT_BOOL _HT_VDEC_CheckAllChnTaskStatus(void)
{
	HT_S32 i = 0;
	HT_VDEC_CHN VdecChn = 0;

	for(i = 0; i < _stResMgr.u32ChnNum; i++)
	{
		VdecChn = _stResMgr.au16ChnId[i];
		if(FALSE == _HT_VDEC_CheckChnTaskStatus(VdecChn))
		{
			return FALSE;
		}
	}

	return TRUE;
}

static void _HT_VDEC_SetChnTaskStatus(HT_VDEC_CHN VdecChn, HT_BOOL bTaskFinished)
{
	_bChnTaskFinished[VdecChn] = bTaskFinished;
}

static void _HT_VDEC_SetParamDft(void)
{
	HT_VDEC_CHN VdecChn = 0;

	/*parameter init*/
	_stResMgr.bInitFlag 	 = FALSE;

	_stResMgr.bWrBufTaskRun  = FALSE;
	_stResMgr.bPutFrmTaskRun = FALSE;
	_stResMgr.bPutFrmJpegTaskRun = FALSE;

	_stResMgr.pWrBufTask	 	= NULL;
	_stResMgr.pPutFrmTask	 	= NULL;
	_stResMgr.pPutJpegFrmTask	= NULL;

	for(VdecChn = 0; VdecChn < HT_VDEC_MAX_CHN_NUM; VdecChn++)
	{
		_stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType 	 = E_HT_VDEC_CODEC_TYPE_H264;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth  = 1920;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight = 1080;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32BufSize   = 1920 * 1080 * 3/2;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.eVideoMode	 = E_HT_VDEC_VIDEO_MODE_FRAME;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32Priority  = 0;
		_stResMgr.astChnInfo[VdecChn].stChnAttr.stVdecVideoAttr.u32RefFrameNum = 6;

		_stResMgr.astChnInfo[VdecChn].bCreate    = FALSE;
		_stResMgr.astChnInfo[VdecChn].bStart     = FALSE;
		_stResMgr.astChnInfo[VdecChn].bChnConfig = FALSE;
		_stResMgr.bFileOpen[VdecChn] 			 = FALSE;

		// Chn Param
		_stResMgr.astChnInfo[VdecChn].stChnParam.eDecMode = E_HT_VDEC_DECODE_MODE_ALL;
		_stResMgr.astChnInfo[VdecChn].stChnParam.eOutputOrder = E_HT_VDEC_OUTPUT_ORDER_DECODE;
		//_stResMgr.astChnInfo[VdecChn].stChnParam.eVideoFormat = ;
		//_stResMgr.astChnInfo[VdecChn].stChnParam.u32DecFramebufferNum = ;

		_stResMgr.astChnInfo[VdecChn].eDisplayMode = E_HT_VDEC_DISPLAY_MODE_PLAYBACK; //这个参数与流程有关，无对应mapi接口

		//_stResMgr.astChnInfo[VdecChn].u8EnableMfcodec = ;

	}
}

static HT_RESULT \
_HT_VDEC_GetEsFilename(HT_VDEC_CHN VdecChn, HT_U32 u32Width, HT_U32 u32Height, HT_S8 *ps8Filename, HT_VDEC_CodecType_e eCodecType)
{
	CHECK_NULL_POINTER(__FUNCTION__, ps8Filename);

	// ES file name
	snprintf(ps8Filename, HT_VDEC_FILENAME_LEN, "/mnt/ESFILE/%u_%u_Chn%d", u32Width, u32Height, VdecChn);

	switch(eCodecType)
	{
		case E_HT_VDEC_CODEC_TYPE_H264:
			strncat(ps8Filename, ".h264", sizeof(".h264"));
			break;
		case E_HT_VDEC_CODEC_TYPE_H265:
			strncat(ps8Filename, ".h265", sizeof(".h265"));
			break;
		case E_HT_VDEC_CODEC_TYPE_JPEG:
			strncat(ps8Filename, ".jpeg", sizeof(".jpeg"));
			break;
		default:
			printk("eCodecType is error!!VdecChn: %d\n", VdecChn);
			return HT_FAILURE;
	}

	return HT_SUCCESS;
}

#ifndef HT_WRITE_DDR
static HT_RESULT \
_HT_VDEC_GetYuvFilename(HT_VDEC_CHN VdecChn, HT_U32 u32Width, HT_U32 u32Height, HT_S8 *ps8Filename)
{
	CHECK_NULL_POINTER(__FUNCTION__, ps8Filename);

	snprintf(ps8Filename, HT_VDEC_FILENAME_LEN, "/mnt/%u_%u_Chn%d.yuv", u32Width, u32Height, VdecChn);

	return HT_SUCCESS;
}
#endif

static HT_RESULT _HT_VDEC_GetParam(HT_U16 *pu16CmdValue, HT_U8 u8CmdCnt)
{
	HT_S32  i = 0;
	HT_S32  j = 0;
	HT_RESULT Ret = HT_FAILURE;
	HT_VDEC_CHN VdecChn = 0;

	CHECK_NULL_POINTER(__FUNCTION__, pu16CmdValue);

	// Defines an integer multiple of 4 times the number of arguments passed in each time
	if ((0 == u8CmdCnt) || (0 != u8CmdCnt % HT_VDEC_PARA_NUM_GRP))
	{
		printk("Input param num error!!\n");
		return HT_FAILURE;
	}

	_stResMgr.u32ChnNum = u8CmdCnt / HT_VDEC_PARA_NUM_GRP;

	for(j = 0; j < _stResMgr.u32ChnNum; j++)
	{
		/*ChnId*/
		_stResMgr.au16ChnId[j] = pu16CmdValue[i++];
		VdecChn = _stResMgr.au16ChnId[j];

		/*eCodecType*/
		_stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType 	 = (HT_VDEC_CodecType_e)pu16CmdValue[i++];

		/*u32PicWidth*/
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth  = (HT_U32)pu16CmdValue[i++];

		/*u32PicHeight*/
		_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight = (HT_U32)pu16CmdValue[i++];

		// set es file name
		Ret = _HT_VDEC_GetEsFilename(VdecChn, _stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, \
				_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, _stResMgr.as8EsFilename[VdecChn], _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType);
		if (Ret)
		{
			printk("VDEC Get Es Filename failed!!VdecChn: %d\n", VdecChn);
			return HT_FAILURE;
		}
		_stResMgr.astChnInfo[VdecChn].bChnConfig = TRUE;
    }

	return HT_SUCCESS;
}

static void _HT_VDEC_MMABufMapping(HT_VDEC_CHN VdecChn, HT_BOOL bMapping)
{
    if (bMapping)
    {
        ///ES buffer mmap
        MsOS_MPool_Add_PA2VARange(
            _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr,
            (HT_U32)_stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr,
            _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize,
            TRUE);

        _stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr =
            (void *)MsOS_MPool_PA2KSEG1(_stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr);

        ///Frame buffer mmap
        MsOS_MPool_Add_PA2VARange(
            _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr,
            (HT_U32)_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr,
            _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize,
            TRUE);

        _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr =
            (void *)MsOS_MPool_PA2KSEG1(_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr);
    }
    else
    {
        ///ES buffer unmmap
        MsOS_MPool_Remove_PA2VARange(
            _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr,
            (MS_U32)_stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr,
            _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize,
            TRUE);

        ///Frame buffer unmmap
        MsOS_MPool_Remove_PA2VARange(
            _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr,
            (MS_U32)_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr,
            _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize,
            TRUE);
    }
}

static HT_RESULT _HT_VDEC_CreateChn(HT_VDEC_CHN VdecChn)
{
	HT_VB_Info_t stEsVbInfo;
	HT_VB_Info_t stFrameVbInfo;
	HT_VDEC_ChnAttr_t *pstChnAttr = &(_stResMgr.astChnInfo[VdecChn].stChnAttr);

	memset(&stEsVbInfo, 	0x0, sizeof(HT_VB_Info_t));
	memset(&stFrameVbInfo, 	0x0, sizeof(HT_VB_Info_t));

    DOWN(&(_stResMgr.semChnLock[VdecChn]));

	// alloc cpu buffer
    _stResMgr.astChnInfo[VdecChn].stCpuMemInfo.phyAddr = _stResMgr.stCpuMem.phyAddr;
    _stResMgr.astChnInfo[VdecChn].stCpuMemInfo.u32BufSize =_stResMgr.stCpuMem.u32BufSize;
    printk("############# Os cpu phy addr %llx\n",_stResMgr.astChnInfo[VdecChn].stCpuMemInfo.phyAddr);
    printk("############# Size %x\n", _stResMgr.astChnInfo[VdecChn].stCpuMemInfo.u32BufSize);

	// alloc Es buffer
	pstChnAttr->u32BufSize = HT_VDEC_ES_MEM_SIZE * 2;
    if (E_HT_VDEC_CODEC_TYPE_JPEG == pstChnAttr->eCodecType)
    {
        pstChnAttr->u32BufSize = \
			ALIGN_BYTES(pstChnAttr->u32PicWidth, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE)\
			* ALIGN_BYTES(pstChnAttr->u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE)\
			* 4;
		if(JPEG_DEFAULT_EXIF_SIZE > pstChnAttr->u32BufSize)
		{
			pstChnAttr->u32BufSize = JPEG_DEFAULT_EXIF_SIZE;
		}
    }
	stEsVbInfo.u32Size = pstChnAttr->u32BufSize;
	stEsVbInfo.u32Size = ALIGN_BYTES(stEsVbInfo.u32Size, HT_VDEC_PAGE_SIZE);

	if (HT_SUCCESS != HT_MallocAlign(&stEsVbInfo, 64))
	{
		printk("alloc Es buffer failed!!\n");
		HT_Free(_stResMgr.astChnInfo[VdecChn].stCpuMemInfo.phyAddr);
		goto _UNLOCK;
	}
	memset(stEsVbInfo.pu8VirtAddr, 0x0, stEsVbInfo.u32Size);
	_stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize = stEsVbInfo.u32Size;
	_stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr 	 = stEsVbInfo.phyAddr;
    _stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr 	 = stEsVbInfo.pu8VirtAddr;

    printk("############# Os ES phy addr %llx\n", _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr);
    printk("############# Os ES virt addr %p\n", _stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr);
    printk("############# Size %x\n", _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize);

	// alloc frame buffer
    if (6 > pstChnAttr->stVdecVideoAttr.u32RefFrameNum)
    {
        pstChnAttr->stVdecVideoAttr.u32RefFrameNum = 6;
    }

	if(E_HT_VDEC_CODEC_TYPE_JPEG == pstChnAttr->eCodecType)
	{
        pstChnAttr->stVdecVideoAttr.u32RefFrameNum = 1;
		stFrameVbInfo.u32Size = \
			ALIGN_BYTES(pstChnAttr->u32PicWidth, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE)\
			* ALIGN_BYTES(pstChnAttr->u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE)\
			* 4\
			* pstChnAttr->stVdecVideoAttr.u32RefFrameNum;
	}
	else
	{
		stFrameVbInfo.u32Size = \
			(pstChnAttr->u32PicWidth * pstChnAttr->u32PicHeight * 3 / 2) * pstChnAttr->stVdecVideoAttr.u32RefFrameNum;
	}

	stFrameVbInfo.u32Size = ALIGN_BYTES(stFrameVbInfo.u32Size, HT_VDEC_PAGE_SIZE);

	if (HT_SUCCESS != HT_MallocAlign(&stFrameVbInfo, 64))
	{
		printk("alloc frame buffer failed!!\n");
		HT_Free(_stResMgr.astChnInfo[VdecChn].stCpuMemInfo.phyAddr);
		HT_Free(stEsVbInfo.phyAddr);
		goto _UNLOCK;
	}
	memset(stFrameVbInfo.pu8VirtAddr, 0x0, stFrameVbInfo.u32Size);
	_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize = stFrameVbInfo.u32Size;
	_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr 	= stFrameVbInfo.phyAddr;
    _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr 	= stFrameVbInfo.pu8VirtAddr;

    printk("############# Os frame phy addr %llx\n", _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr);
    printk("############# Os frame virt addr %p\n", _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr);
    printk("############# Size %x\n", _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize);

	// Jpeg decode, ES buffer and Frame buffer mmap
    if (pstChnAttr->eCodecType == E_HT_VDEC_CODEC_TYPE_JPEG)
    {
        _HT_VDEC_MMABufMapping(VdecChn, TRUE);
    }

	_stResMgr.astChnInfo[VdecChn].bCreate = TRUE;
	UP(&(_stResMgr.semChnLock[VdecChn]));

	return HT_SUCCESS;
_UNLOCK:
	UP(&(_stResMgr.semChnLock[VdecChn]));

	return HT_FAILURE;
}

static HT_RESULT _HT_VDEC_StartChn(HT_VDEC_CHN VdecChn)
{
    VDEC_EX_DecModCfg stDecModCfg;
    VDEC_EX_INPUT_TSP eInputTSP = E_VDEC_EX_INPUT_TSP_NONE;
    VDEC_EX_DynmcDispPath stDynmcDispPath;
    VDEC_EX_DISPLAY_MODE eDispMode = E_VDEC_EX_DISPLAY_MODE_MCU;
    VDEC_EX_MFCodec_mode eMFCodecMode = E_VDEC_EX_MFCODEC_FORCE_ENABLE;
    VDEC_EX_InitParam stInitParams;
    VDEC_EX_CodecType eVdecCodecType = E_VDEC_EX_CODEC_TYPE_NONE;
    VDEC_EX_BufferInfo stBitstreamBufInfo;
    VDEC_EX_BufferInfo stFrameBufInfo;
    VDEC_EX_BufferInfo stDvXcShmBufInfo;

    HT_U8 miu = 0;

    if (_stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType == E_HT_VDEC_CODEC_TYPE_H264)
    {
        eVdecCodecType = E_VDEC_EX_CODEC_TYPE_H264;
    }
    else if (_stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType == E_HT_VDEC_CODEC_TYPE_H265)
    {
        eVdecCodecType = E_VDEC_EX_CODEC_TYPE_HEVC;
    }
    else if (_stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType == E_HT_VDEC_CODEC_TYPE_JPEG)
    {
        eVdecCodecType = E_VDEC_EX_CODEC_TYPE_MJPEG;
    }
    else
    {
        printk("UnSupport Codec Type:%d\n", _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType);
        return HT_FAILURE;
    }

    if (E_VDEC_EX_OK != MApi_VDEC_EX_GetFreeStream(&_stResMgr.astChnInfo[VdecChn].stVDECStreamId,
        sizeof(VDEC_StreamId), E_VDEC_EX_N_STREAM, eVdecCodecType))
    {
        printk("Get FreeStream Fail:%d %d\n", _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType, eVdecCodecType);
        return HT_FAILURE;
    }

    memset(&stDecModCfg, 0, sizeof(VDEC_EX_DecModCfg));
    stDecModCfg.eDecMod = E_VDEC_EX_DEC_MODE_DUAL_INDIE;
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_DECODE_MODE, (MS_U32)&stDecModCfg);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_CONNECT_INPUT_TSP, eInputTSP);

    memset(&stDynmcDispPath, 0x0, sizeof(VDEC_EX_DynmcDispPath));
    stDynmcDispPath.bConnect  = TRUE;
    stDynmcDispPath.eMvopPath = E_VDEC_EX_DISPLAY_PATH_NONE;
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_CONNECT_DISPLAY_PATH, (MS_U32)&stDynmcDispPath);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_DISPLAY_MODE, (MS_U32)eDispMode);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_BITSTREAMBUFFER_MONOPOLY, TRUE);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_FRAMEBUFFER_MONOPOLY, TRUE);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_DYNAMIC_CMA_MODE, FALSE);
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_MFCODEC_MODE, (MS_U32)&eMFCodecMode);

    memset(&stInitParams, 0x0, sizeof(VDEC_EX_InitParam));
    if (E_HT_VDEC_CODEC_TYPE_H264 == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
    {
        stInitParams.eCodecType = E_VDEC_EX_CODEC_TYPE_H264;
    }
    else if (E_HT_VDEC_CODEC_TYPE_H265 == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
    {
        stInitParams.eCodecType = E_VDEC_EX_CODEC_TYPE_HEVC;
    }
    else if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
    {
        stInitParams.eCodecType = E_VDEC_EX_CODEC_TYPE_MJPEG;
        stInitParams.VideoInfo.u32FrameRate = 30;
        stInitParams.VideoInfo.u32FrameRateBase = 30;
    }
    else
    {
        MApi_VDEC_EX_Exit(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId));
        printk("Vdec Chn(%d) UnSupport Codec(%d) Failed\n", VdecChn, _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType);
        return HT_FAILURE;
    }

    stInitParams.VideoInfo.eSrcMode = E_VDEC_EX_SRC_MODE_FILE;
    stInitParams.VideoInfo.eTimeStampType = E_VDEC_EX_TIME_STAMP_PTS;

    stInitParams.EnableDynaScale = FALSE;
    stInitParams.bDisableDropErrFrame = TRUE;
    stInitParams.bDisableErrConceal = FALSE;
    stInitParams.bRepeatLastField = TRUE;

    ///cpu buffer
    stInitParams.SysConfig.u32CodeBufAddr = _stResMgr.astChnInfo[VdecChn].stCpuMemInfo.phyAddr;
    stInitParams.SysConfig.u32CodeBufSize = _stResMgr.astChnInfo[VdecChn].stCpuMemInfo.u32BufSize;
    stInitParams.SysConfig.u32FWBinaryAddr = stInitParams.SysConfig.u32CodeBufAddr;
    stInitParams.SysConfig.u32FWBinarySize = stInitParams.SysConfig.u32CodeBufSize;

    ///framebuffer
    stInitParams.SysConfig.u32FrameBufAddr = _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr;
    stInitParams.SysConfig.u32FrameBufSize = _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize;

    memset(&stFrameBufInfo, 0, sizeof(VDEC_EX_BufferInfo));
    HT_MmapParser("E_LX_MEM", (HT_U32 *)&stFrameBufInfo.phyAddr, (HT_U32 *)&stFrameBufInfo.szSize, &miu);
    stFrameBufInfo.eType     = E_VDEC_EX_BUFFERTYPE_FB_MAIN;
    stFrameBufInfo.u32Config = 0;
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_BUFFER_INFO, (MS_VIRT)&stFrameBufInfo);

    ///bitstream buffer
    stInitParams.SysConfig.u32BitstreamBufAddr = _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr;
    stInitParams.SysConfig.u32BitstreamBufSize = _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize;
    stInitParams.SysConfig.u32DrvProcBufAddr = stInitParams.SysConfig.u32BitstreamBufAddr;
    stInitParams.SysConfig.u32DrvProcBufSize = 0;

    memset(&stBitstreamBufInfo, 0, sizeof(VDEC_EX_BufferInfo));
    stBitstreamBufInfo.eType     = E_VDEC_EX_BUFFERTYPE_BS_MAIN;
    HT_MmapParser("E_LX_MEM", (HT_U32 *)&stBitstreamBufInfo.phyAddr, (HT_U32 *)&stBitstreamBufInfo.szSize, &miu);
    stBitstreamBufInfo.u32Config = 0;
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_BUFFER_INFO, (MS_VIRT)&stBitstreamBufInfo);

    printk("Chn(%d) CPU:(0x%llx, 0x%x) Framebuffer:(0x%llx, 0x%x) Bitstream:(0x%llx, 0x%x) MS_PHY:%d\n",
        VdecChn,
        stInitParams.SysConfig.u32CodeBufAddr,
        stInitParams.SysConfig.u32CodeBufSize,
        stInitParams.SysConfig.u32FrameBufAddr,
        stInitParams.SysConfig.u32FrameBufSize,
        stInitParams.SysConfig.u32BitstreamBufAddr,
        stInitParams.SysConfig.u32BitstreamBufSize, sizeof(MS_PHY));

    ///set dv xc shm info
    memset(&stDvXcShmBufInfo, 0, sizeof(VDEC_EX_BufferInfo));
    stDvXcShmBufInfo.eType     = E_VDEC_EX_BUFFERTYPE_HDRSHM_MAIN;
    stDvXcShmBufInfo.phyAddr = _stResMgr.stDvXcShmMem.phyAddr;
    stDvXcShmBufInfo.szSize = _stResMgr.stDvXcShmMem.u32BufSize;
    stDvXcShmBufInfo.u32Config = 0;
    MApi_VDEC_EX_PreSetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_BUFFER_INFO, (MS_VIRT)&stDvXcShmBufInfo);

    stInitParams.SysConfig.eDbgMsgLevel = E_VDEC_EX_DBG_LEVEL_ERR;
    if (E_VDEC_EX_OK != MApi_VDEC_EX_Init(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), &stInitParams))
    {
        MApi_VDEC_EX_Exit(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId));
        printk("Vdec Chn(%d) Init Failed\n", VdecChn);
        return HT_FAILURE;
    }

    MApi_VDEC_EX_SetBlockDisplay(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), TRUE);
    MApi_VDEC_EX_EnableESBuffMalloc(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), TRUE);
    MApi_VDEC_EX_SetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_DISP_OUTSIDE_CTRL_MODE, TRUE);
    MApi_VDEC_EX_SetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SET_DISP_FINISH_MODE, TRUE);
    MApi_VDEC_EX_DisableDeblocking(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), FALSE);
    MApi_VDEC_EX_SetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_AVC_SUPPORT_REF_NUM_OVER_MAX_DPB_SIZE, TRUE);
    ///set decoder order output
    MApi_VDEC_EX_SetControl(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), E_VDEC_EX_USER_CMD_SHOW_DECODE_ORDER, TRUE);

    MApi_VDEC_EX_AVSyncOn(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId), FALSE, 180, 40);
    if (E_VDEC_EX_OK != MApi_VDEC_EX_Play(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId)))
    {
        MApi_VDEC_EX_Exit(&(_stResMgr.astChnInfo[VdecChn].stVDECStreamId));
        printk("Vdec Chn(%d) Play Failed\n", VdecChn);
        return HT_FAILURE;
    }

    _stResMgr.astChnInfo[VdecChn].u8EnableMfcodec = TRUE;

    _stResMgr.astChnInfo[VdecChn].bStart = TRUE;

    printk("Start Chn(%d) u32Id(%d) Done\n", VdecChn, _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id);
    return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_SetChnParam(HT_VDEC_CHN VdecChn, HT_VDEC_ChnParam_t *pstChnParam)
{
	HT_BOOL bDecOrder = TRUE;
	VDEC_StreamId stStreamId;
	VDEC_EX_TrickDec eTrickDec = E_VDEC_EX_TRICK_DEC_ALL;

	CHECK_NULL_POINTER(__FUNCTION__, pstChnParam);

	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));

	stStreamId.u32Id      = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

	switch(pstChnParam->eDecMode)
	{
		case E_HT_VDEC_DECODE_MODE_ALL:
			eTrickDec = E_VDEC_EX_TRICK_DEC_ALL;
			break;
		case E_HT_VDEC_DECODE_MODE_I:
			eTrickDec = E_VDEC_EX_TRICK_DEC_I;
			break;
		case E_HT_VDEC_DECODE_MODE_IP:
			eTrickDec = E_VDEC_EX_TRICK_DEC_IP;
			break;
		default:
			printk("VdecChn(%d) eDecMode error!!\n", VdecChn);
			return HT_FAILURE;
	}

 	MApi_VDEC_EX_SetTrickMode(&stStreamId, eTrickDec);

    ///TODO:set frame buffer number
 	//pstChnParam->u32DecFramebufferNum

	if (E_HT_VDEC_OUTPUT_ORDER_DISPLAY == pstChnParam->eOutputOrder)
	{
		bDecOrder = FALSE;
	}
	else
	{
		bDecOrder = TRUE;
	}
	MApi_VDEC_EX_SetControl(&stStreamId, E_VDEC_EX_USER_CMD_SHOW_DECODE_ORDER, bDecOrder);

    return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_Start(void)
{
	HT_S32 i = 0;
	HT_RESULT Ret = HT_FAILURE;
	HT_VDEC_CHN VdecChn = 0;

	for(i = 0; i < _stResMgr.u32ChnNum; i++)
	{
		VdecChn = _stResMgr.au16ChnId[i];

		/*Create Chn*/
		Ret = _HT_VDEC_CreateChn(VdecChn);
		if(HT_SUCCESS != Ret)
		{
			printk("Vdec Create Chn %d failed!!\n", VdecChn);
			return HT_FAILURE;
		}

		/*Start Chn*/
		if(E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			Ret = HT_SUCCESS;

			_stResMgr.astChnInfo[VdecChn].bStart = TRUE;
			printk("Start Jpeg Chn(%d) Done\n", VdecChn);
		}
		else
		{
			Ret = _HT_VDEC_StartChn(VdecChn);
		}
		if(HT_SUCCESS != Ret)
		{
			printk("Vdec Start Chn %d failed!!\n", VdecChn);
			return HT_FAILURE;
		}

		/*Set chn param*/
		if(E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			Ret = _HT_VDEC_SetChnParam(VdecChn, &_stResMgr.astChnInfo[VdecChn].stChnParam);
			if(HT_SUCCESS != Ret)
			{
				printk("Vdec Set Chn %d Param failed!!\n", VdecChn);
				return HT_FAILURE;
			}
		}
	}

	printk("Vdec start success\n");
	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_QueryFreeBuffer(HT_VDEC_CHN VdecChn, HT_VDEC_BufInfo_t *pstBufInfo)
{
    HT_U32 u32AvailSize = 0;
    HT_PHY phyAddr = 0;
    HT_U32 u32Vacany = 0;
	VDEC_StreamId stStreamId;

	CHECK_NULL_POINTER(__FUNCTION__, pstBufInfo);
	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));
	stStreamId.u32Id      = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

    u32Vacany = MApi_VDEC_EX_GetESBuffVacancy(&stStreamId, NULL);
	if (u32Vacany < pstBufInfo->u32BufSize)
    {
        return HT_FAILURE;
    }

    if (E_VDEC_EX_OK == MApi_VDEC_EX_GetESBuff(&stStreamId, (MS_U32)pstBufInfo->u32BufSize, (MS_U32 *)&u32AvailSize, (MS_PHY *)&phyAddr))
    {
        pstBufInfo->phyAddr = phyAddr;
    }
	else
	{
        return HT_FAILURE;
	}

	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_WrBufComplete(HT_VDEC_CHN VdecChn, HT_VDEC_BufInfo_t *pstBufInfo)
{
    VDEC_EX_DecCmd stDecCmd;
	VDEC_StreamId stStreamId;

	CHECK_NULL_POINTER(__FUNCTION__, pstBufInfo);

    memset(&stDecCmd,   0x0, sizeof(VDEC_EX_DecCmd));
	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));

	stStreamId.u32Id      	= _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version 	= _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

    stDecCmd.u32StAddr 		= pstBufInfo->phyAddr;
    stDecCmd.u32Size 		= pstBufInfo->u32BufSize;
    stDecCmd.u32ID_H 		= ((pstBufInfo->u64Pts >> 32)& 0xFFFFFFFF);
    stDecCmd.u32ID_L 		= (pstBufInfo->u64Pts & 0xFFFFFFFF);
    stDecCmd.u32Timestamp 	= (MS_U32)(pstBufInfo->u64Pts & 0xFFFFFFFF);

    if (E_VDEC_EX_OK != MApi_VDEC_EX_PushDecQ(&stStreamId, (VDEC_EX_DecCmd*)(void*)&stDecCmd))
    {
        printk("PushDecQ Error\n");
		return HT_FAILURE;
    }

    if (E_VDEC_EX_OK != MApi_VDEC_EX_FireDecCmd(&stStreamId))
    {
        printk("FireDecCmd Error\n");
		return HT_FAILURE;
    }

	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_WrBuf(HT_VDEC_CHN VdecChn, HT_VDEC_Stream_t *pstStreamBuf)
{
	HT_VDEC_BufInfo_t stBufInfo;
    HT_U32 u32Offset = 0;
    HT_U8 *pu8VirAddr = NULL;

	CHECK_NULL_POINTER(__FUNCTION__, pstStreamBuf);

	if (FALSE == pstStreamBuf->bQuerySuccess)
	{
	    memset(&stBufInfo, 0x0, sizeof(HT_VDEC_BufInfo_t));
	    stBufInfo.u32BufSize = pstStreamBuf->u32Len;
	    if (stBufInfo.u32BufSize >= _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize)
	    {
			printk("Out Of Es Buffer Limited, Drop Es. Push Data Size:%u, Es Buffer Size:%u\n",
	            stBufInfo.u32BufSize, _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize);
	        return HT_SUCCESS;
	    }

		/*check ES buffer，if used up, then reset the pointer to the start*/
	    if (HT_SUCCESS != _HT_VDEC_QueryFreeBuffer(VdecChn, &stBufInfo))
	    {
			pstStreamBuf->bQuerySuccess = FALSE;
	        return HT_FAILURE;
	    }
		pstStreamBuf->bQuerySuccess = TRUE;
        //printk("Query free buff %llx\n", stBufInfo.phyAddr);
        if (stBufInfo.phyAddr < _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr)
        {
            printk("Get es buufer error!\n");
	        return HT_FAILURE;
        }
        u32Offset = stBufInfo.phyAddr - _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr;
        pu8VirAddr = (HT_U8 *)(_stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr) + u32Offset;

        //printk("From %p to %p\n", pstStreamBuf->pu8Addr, pu8VirAddr);
		//Copy Frame to ES buffer, then release frame buffer
	    stBufInfo.u32BufSize = pstStreamBuf->u32Len;
	    memcpy(pu8VirAddr, pstStreamBuf->pu8Addr, stBufInfo.u32BufSize);
		stBufInfo.u64Pts = pstStreamBuf->u64PTS;
	    stBufInfo.bEndOfStream = TRUE;

		pstStreamBuf->bReadSuccess = FALSE; //mem cp finished
	}

    if (HT_SUCCESS != _HT_VDEC_WrBufComplete(VdecChn, &stBufInfo))
    {
    	printk("Push Buffer Faild\n");
        return HT_FAILURE;
    }

	pstStreamBuf->bQuerySuccess = FALSE; //write finished

	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_GetFrame(HT_VDEC_CHN VdecChn, HT_VDEC_DispFrame_t *pstGetDispFrm)
{
	VDEC_EX_DispFrame* pstNextDispFrm;
    HT_BOOL b10Bit;
    VDEC_EX_FrameInfoExt_v6 stFrmInfoExt_v6;
    VDEC_EX_DispInfo stDispInfo;
	VDEC_EX_FrameInfoEX stFrmInfo;
	HT_U32 u32MFCodecInfo = 0;
	HT_VDEC_MfCodecVersion_e eMfDecVersion = E_HT_VDEC_MFCODEC_DISABLE;
	VDEC_StreamId stStreamId;

	CHECK_NULL_POINTER(__FUNCTION__, pstGetDispFrm);

	memset(&stFrmInfo, 0, sizeof(stFrmInfo));
	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));

	stStreamId.u32Id      = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

    if(MApi_VDEC_EX_GetNextDispFrame(&stStreamId, &pstNextDispFrm) == E_VDEC_EX_OK)
    {
        pstGetDispFrm->stFrmInfo.u32LumaAddr   = pstNextDispFrm->stFrmInfo.u32LumaAddr;
        pstGetDispFrm->stFrmInfo.u32ChromaAddr = pstNextDispFrm->stFrmInfo.u32ChromaAddr;
        pstGetDispFrm->stFrmInfo.u32TimeStamp  = pstNextDispFrm->stFrmInfo.u32TimeStamp;
        pstGetDispFrm->stFrmInfo.u32IdL        = pstNextDispFrm->stFrmInfo.u32ID_L;
        pstGetDispFrm->stFrmInfo.u32IdH        = pstNextDispFrm->stFrmInfo.u32ID_H;
        pstGetDispFrm->stFrmInfo.u16Pitch      = pstNextDispFrm->stFrmInfo.u16Pitch;
        pstGetDispFrm->stFrmInfo.u16Width      = pstNextDispFrm->stFrmInfo.u16Width;
        pstGetDispFrm->stFrmInfo.u16Height     = pstNextDispFrm->stFrmInfo.u16Height;
        pstGetDispFrm->stFrmInfo.eFrameType    = (HT_VDEC_FrameType_e)(pstNextDispFrm->stFrmInfo.eFrameType);
        pstGetDispFrm->stFrmInfo.eFieldType    = (HT_VDEC_FieldType_e)(pstNextDispFrm->stFrmInfo.eFieldType);

        pstGetDispFrm->u32PriData              = pstNextDispFrm->u32PriData;
        pstGetDispFrm->u32Idx                  = pstNextDispFrm->u32Idx;
        pstGetDispFrm->stFrmInfoExt.eFrameScanMode  = (HT_VDEC_FrameScanMode_e)((pstNextDispFrm->stFrmInfo.u32ID_L >> 19) & 0x03);
        b10Bit = (pstNextDispFrm->stFrmInfo.u32ID_L >> 21) & 0x01;
    }
    else
    {
        return HT_FAILURE;
    }

    if(b10Bit == TRUE)
    {
        if (pstGetDispFrm->ePixelFrm == E_HT_VDEC_PIXEL_FRAME_YC420_MSTTILE2_H265)
        {
            pstGetDispFrm->ePixelFrm = E_HT_VDEC_PIXEL_FRAME_YC420_MSTTILE3_H265;
        }

        if (E_VDEC_EX_OK == MApi_VDEC_EX_GetControl(&stStreamId, E_VDEC_EX_USER_CMD_GET_FRAME_INFO_EX, (MS_U32 *)&stFrmInfo))
        {
            pstGetDispFrm->stFrmInfoExt.phyLumaAddr2bit 	= stFrmInfo.u32LumaAddr_2bit;
            pstGetDispFrm->stFrmInfoExt.phyChromaAddr2bit 	= stFrmInfo.u32ChromaAddr_2bit;
            pstGetDispFrm->stFrmInfoExt.u8LumaBitDepth 		= stFrmInfo.u8LumaBitdepth;
            pstGetDispFrm->stFrmInfoExt.u8ChromaBitDepth 	= stFrmInfo.u8ChromaBitdepth;
            pstGetDispFrm->stFrmInfoExt.u16Pitch2bit 		= stFrmInfo.u16Pitch_2bit;
        }
        else
        {
            return HT_FAILURE;
        }
    }

    // check support MFDEC or not
    if(sizeof(VDEC_EX_FrameInfoExt_v6) <= 1)
    {
        return HT_SUCCESS;
    }

    memset(&stFrmInfoExt_v6, 0, sizeof(stFrmInfoExt_v6));
    stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.stVerCtl.u32version = 6;
    stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.stVerCtl.u32size 	 = sizeof(VDEC_EX_FrameInfoExt_v6);
    if (E_VDEC_EX_OK == MApi_VDEC_EX_GetControl(&stStreamId, E_VDEC_EX_USER_CMD_GET_NEXT_DISP_FRAME_INFO_EXT, (MS_U32*)&stFrmInfoExt_v6))
    {
        u32MFCodecInfo = stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.u32MFCodecInfo;
        eMfDecVersion = (HT_VDEC_MfCodecVersion_e)(u32MFCodecInfo & 0xff);
        if((eMfDecVersion != E_HT_VDEC_MFCODEC_UNSUPPORT) && (eMfDecVersion != E_HT_VDEC_MFCODEC_DISABLE))
        {
            pstGetDispFrm->stDbInfo.bDbEnable 			= TRUE;
            pstGetDispFrm->stDbInfo.bBypassCodecMode 	= FALSE;
            pstGetDispFrm->stDbInfo.bUncompressMode 	= (u32MFCodecInfo >> 28) & 0x1;
            pstGetDispFrm->stDbInfo.u8DbSelect 			= (HT_U8)((u32MFCodecInfo >> 8) & 0x1);
            pstGetDispFrm->stDbInfo.eDbMode 			= (HT_VDEC_DbMode_e)((u32MFCodecInfo >> 29) & 0x1);
            pstGetDispFrm->stDbInfo.u16HSize 			= \
				stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.sFrameInfo.u16Width;
            pstGetDispFrm->stDbInfo.u16VSize 			= \
				stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.sFrameInfo.u16Height;
            pstGetDispFrm->stDbInfo.phyDbBase 			= \
				(HT_PHY)stFrmInfoExt_v6.sFrameInfoExt_v5.sFrameInfoExt_v4.sFrameInfoExt_v3.sFrameInfoExt.u32LumaMFCbitlen;
            pstGetDispFrm->stDbInfo.u16DbPitch 			= (HT_U16)((u32MFCodecInfo >> 16) & 0xFF);
            pstGetDispFrm->stDbInfo.u8DbMiuSel 			= (HT_U8)((u32MFCodecInfo >> 24) & 0x0F);
            pstGetDispFrm->stDbInfo.phyLbAddr 			= (HT_PHY)stFrmInfoExt_v6.u32HTLBEntriesAddr;
            pstGetDispFrm->stDbInfo.u8LbSize 			= stFrmInfoExt_v6.u8HTLBEntriesSize;
            pstGetDispFrm->stDbInfo.u8LbTableId 		= stFrmInfoExt_v6.u8HTLBTableId;
        }
        else
        {
            pstGetDispFrm->stDbInfo.bDbEnable 		 = FALSE;
            pstGetDispFrm->stDbInfo.bBypassCodecMode = TRUE;
        }
        pstGetDispFrm->stFrmInfoExt.eFrameTileMode 		= (HT_VDEC_FrameTileMode_e)(stFrmInfoExt_v6.eTileMode);
        _stResMgr.astChnInfo[VdecChn].u8EnableMfcodec 	= pstGetDispFrm->stDbInfo.bUncompressMode;
    }
    else
    {
        return HT_FAILURE;
    }

    memset(&stDispInfo, 0, sizeof(stDispInfo));
    if(E_VDEC_EX_OK == MApi_VDEC_EX_GetDispInfo(&stStreamId, &stDispInfo))
    {
        pstGetDispFrm->stDbInfo.u16StartX = stDispInfo.u16CropLeft;
        pstGetDispFrm->stDbInfo.u16StartY = stDispInfo.u16CropTop;
    }
    else
    {
        return HT_FAILURE;
    }

	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_RlsFrame(HT_VDEC_CHN VdecChn, HT_VDEC_DispFrame_t *pstRlsDispFrm)
{
    VDEC_EX_DispFrame stDispFrm;
	VDEC_StreamId stStreamId;

	CHECK_NULL_POINTER(__FUNCTION__, pstRlsDispFrm);

    memset(&stDispFrm,  0x0, sizeof(VDEC_EX_DispFrame));
	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));

	stStreamId.u32Id      = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

    stDispFrm.stFrmInfo.u32LumaAddr     = (HT_PHY)pstRlsDispFrm->stFrmInfo.u32LumaAddr;
    stDispFrm.stFrmInfo.u32ChromaAddr   = (HT_PHY)pstRlsDispFrm->stFrmInfo.u32ChromaAddr;
    stDispFrm.stFrmInfo.u32TimeStamp    = (HT_U32)pstRlsDispFrm->stFrmInfo.u32TimeStamp;
    stDispFrm.stFrmInfo.u32ID_L         = (HT_U32)pstRlsDispFrm->stFrmInfo.u32IdL;
    stDispFrm.stFrmInfo.u32ID_H         = (HT_U32)pstRlsDispFrm->stFrmInfo.u32IdH;
    stDispFrm.stFrmInfo.u16Pitch        = (HT_U16)pstRlsDispFrm->stFrmInfo.u16Pitch;
    stDispFrm.stFrmInfo.u16Width        = (HT_U16)pstRlsDispFrm->stFrmInfo.u16Width;
    stDispFrm.stFrmInfo.u16Height       = (HT_U16)pstRlsDispFrm->stFrmInfo.u16Height;
    stDispFrm.stFrmInfo.eFrameType      = (VDEC_EX_FrameType)pstRlsDispFrm->stFrmInfo.eFrameType;
    stDispFrm.stFrmInfo.eFieldType      = (VDEC_EX_FieldType)pstRlsDispFrm->stFrmInfo.eFieldType;
    stDispFrm.u32PriData                = (HT_U32)pstRlsDispFrm->u32PriData;
    stDispFrm.u32Idx                    = (HT_U32)pstRlsDispFrm->u32Idx;
    if(E_VDEC_EX_OK == MApi_VDEC_EX_ReleaseFrame(&stStreamId, &stDispFrm))
    {
        return HT_SUCCESS;
    }

    return HT_FAILURE;
}

static struct file * _HT_VDEC_OpenFile(HT_S8 *ps8FileName, HT_S32 s32OpenMode, HT_S32 s32Mode, mm_segment_t *fs)
{
	struct file *fp;

	if(NULL == ps8FileName)
	{
		printk("%s ps8FileName is NULL!!\n", __FUNCTION__);
		return NULL;
	}

	fp = filp_open(ps8FileName, s32OpenMode, s32Mode);
	if (IS_ERR(fp))
	{
		printk("Open %s Faild!!\n", ps8FileName);
		return NULL;
	}

	*fs = get_fs();
	set_fs(KERNEL_DS);
	fp->f_op->llseek(fp, 0, SEEK_SET);

	return fp;
}

//JPEG DEC
static JPEG_Result _HT_VDEC_JpegWaitDone(JPEG_InitParam *pJpegInitParam)
{
    JPEG_Event enEvent;
    JPEG_Result eDecRet = E_JPEG_OKAY;

    //For H/W bug, Check Vidx.
    if (E_JPEG_FAILED == MApi_JPEG_HdlVidxChk())
    {
        enEvent = E_JPEG_EVENT_DEC_ERROR_MASK;
    }
    else
    {
        enEvent = MApi_JPEG_GetJPDEventFlag();
    }

    if (E_JPEG_EVENT_DEC_DONE & enEvent)
    {
        eDecRet = E_JPEG_DONE;
    }
    else if (E_JPEG_EVENT_DEC_ERROR_MASK & enEvent)
    {
        eDecRet = E_JPEG_FAILED;
    }

    return eDecRet;
}

void _MI_VDEC_IMPL_DebugDumpJpegDecImage(HT_U8 *pYuv422Data, HT_U32 width, HT_U32 height, HT_U32 pitch, int chn)
{
    struct file *fp = NULL;
    mm_segment_t fs;
    char name[128];
    int length = width * height * 2;
    static int frmcnt[HT_VDEC_MAX_CHN_NUM] = {0};

    memset(name, 0x0, sizeof(name));
    sprintf(name, "/mnt/chn_%d_jpeg_dump_vdec[%d_%d_%d]_%d.yuv", chn, width, height, pitch, frmcnt[chn]++);
    fp =filp_open(name, O_RDWR | O_CREAT, 0777);
    if (IS_ERR(fp))
    {
        printk("Open File Faild  PTR_ERR_fp=%ld\n",PTR_ERR(fp));
        return;
    }

    fs =get_fs();
    set_fs(KERNEL_DS);
    fp->f_op->llseek(fp, 0, SEEK_SET);

    if(fp->f_op->write(fp, pYuv422Data, length, &(fp->f_pos)) != length)
    {
        printk("fwrite %s failed\n", name);
        goto _END;
    }

    printk("dump file(%s) ok ..............[len:%d]\n", name, length);

_END:
    set_fs(fs);
    filp_close(fp,NULL);
}


static HT_BOOL _HT_VDEC_JpegDecoder(HT_VDEC_CHN VdecChn, HT_BufInfo_t *pstBufInfo, HT_BufInfo_t *pstPutBufInfo)
{
    JPEG_InitParam JpegInitParam;
    HT_BOOL 	bRet 	= FALSE;
    HT_U32 u32PicWidth 	= 0;
    HT_U32 u32PicHeight = 0;
    HT_U32 u32PicPitch 	= 0;
    HT_U32 u32PicSize 	= 0;
    //HT_U32 u32EsBufSize = 0;
    JPEG_Result eDecRet = E_JPEG_FAILED;
    HT_U8  u8TryDecCnt 	= 0;
    //HT_BOOL bFreeMMABuf = FALSE;
	HT_VB_Info_t stEsVbInfo;

	CHECK_NULL_POINTER(__FUNCTION__, pstPutBufInfo);
	CHECK_NULL_POINTER(__FUNCTION__, pstBufInfo);

	memset(&stEsVbInfo, 0x0, sizeof(HT_VB_Info_t));

	//copy es data to bitstream buffer
	memcpy(_stResMgr.astChnInfo[VdecChn].stEsMemInfo.pVirAddr, pstBufInfo->stRawData.pVirAddr, pstBufInfo->stRawData.u32ContentSize);

	memset(&JpegInitParam, 0x0, sizeof(JPEG_InitParam));
    //MApi_JPEG_SetDbgLevel(E_JPEG_DEBUG_ALL);
    MApi_JPEG_SetMaxDecodeResolution(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, 	 _stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth);
    MApi_JPEG_SetProMaxDecodeResolution(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, _stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth);

    JpegInitParam.u32MRCBufAddr = _stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr;
    JpegInitParam.u32MRCBufSize = _stResMgr.astChnInfo[VdecChn].stEsMemInfo.u32BufSize;
    JpegInitParam.u32MWCBufAddr = _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr;    ///pstPutBufInfo->stFrameData.phyAddr[0];
    JpegInitParam.u32MWCBufSize = _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.u32BufSize; ///pstPutBufInfo->stFrameData.u16Height * pstPutBufInfo->stFrameData.u16Width * 2;
	JpegInitParam.u32InternalBufAddr = _stResMgr.stJpegInternalMem.phyAddr;
    JpegInitParam.u32InternalBufSize = _stResMgr.stJpegInternalMem.u32BufSize;
    JpegInitParam.u32DecByteRead 	 = pstBufInfo->stRawData.u32ContentSize;
    JpegInitParam.bEOF = TRUE;

    JpegInitParam.u8DecodeType = E_JPEG_TYPE_MAIN;
    JpegInitParam.bInitMem 	   = TRUE;
    JpegInitParam.pFillHdrFunc = NULL;

    MApi_JPEG_Init(&JpegInitParam);
    if (E_JPEG_NO_ERROR != MApi_JPEG_GetErrorCode())
    {
        MApi_JPEG_Rst();
        MApi_JPEG_Exit();
        printk("Jpeg HW Init Faild\n");
        return FALSE;
    }

    ///decode jpeg file header
    MApi_JPEG_DecodeHdr();
    if (MApi_JPEG_GetAlignedPitch() > ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE))
    {
        printk("Dst Res Pitch(%d) > Max PicWidth(%d)\n",
            MApi_JPEG_GetAlignedPitch(),
            ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE));
        goto _END_DEC_JPEG;
    }

    if (MApi_JPEG_GetAlignedHeight() > ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE))
    {
        printk("Dst Res Height(%d) > Max Pic Height(%d)\n",
            MApi_JPEG_GetAlignedHeight(),
            ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE));
        goto _END_DEC_JPEG;
    }

    if (MApi_JPEG_GetAlignedPitch_H() > ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE))
    {
        printk("Dst Res Aligned Pitch Height(%d) > Max Pic Height(%d)\n",
            MApi_JPEG_GetAlignedPitch_H(),
            ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE));
        goto _END_DEC_JPEG;
    }

    eDecRet = MApi_JPEG_Decode();
    bRet = FALSE;
    while (u8TryDecCnt++ < 10)
    {
        if ((E_JPEG_DONE == eDecRet) || (E_JPEG_OKAY == eDecRet))
        {
            eDecRet = _HT_VDEC_JpegWaitDone(&JpegInitParam);
            if (E_JPEG_DONE == eDecRet)
            {
                u32PicWidth  = (HT_U32)MApi_JPEG_GetAlignedWidth();
                u32PicHeight = (HT_U32)MApi_JPEG_GetAlignedHeight();
                u32PicPitch  = (HT_U32)MApi_JPEG_GetAlignedPitch() * 2;
                u32PicSize 	 = u32PicPitch * u32PicHeight;
                //printk("u32PicWidth:%d, u32PicHeight:%d, u32PicPitch:%d, u32PicSize:%d\n", u32PicWidth, u32PicHeight, u32PicPitch, u32PicSize);
                bRet = TRUE;
                break;
            }
        }

        if (E_JPEG_FAILED == eDecRet)
        {
            printk("Chn(%d) Decoder Jpeg Faild, Error Code:%d\n", VdecChn, MApi_JPEG_GetErrorCode());
            bRet = FALSE;
            break;
        }

		mdelay(10);
    }

    if (bRet)
    {
        pstPutBufInfo->stFrameData.ePixelFormat = E_HT_VDEC_PIXEL_FRAME_YUV422_YUYV;
        pstPutBufInfo->stFrameData.u16Width 	= u32PicWidth;
        pstPutBufInfo->stFrameData.u16Height 	= u32PicHeight;
        pstPutBufInfo->stFrameData.u32Stride[0] = u32PicPitch;

        //_MI_VDEC_IMPL_DebugDumpJpegDecImage(_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr, u32PicWidth, u32PicHeight, u32PicPitch, VdecChn);
        memcpy(pstPutBufInfo->stFrameData.pVirAddr[0], _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr, pstPutBufInfo->stFrameData.u32Stride[0] * pstPutBufInfo->stFrameData.u16Height);
        _stResMgr.astChnInfo[VdecChn].u64JpegDecSuc++;
    }
    else
    {
        _stResMgr.astChnInfo[VdecChn].u64JpegDecErr++;
    }

    MApi_JPEG_Rst();
    MApi_JPEG_Exit();

_END_DEC_JPEG:
#if 0
	_HT_VDEC_MMABufMapping(VdecChn, FALSE);

	if (bFreeMMABuf)
	{
		HT_Free(_stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr);
	}
#endif
    return bRet;
}


static HT_RESULT _HT_VDEC_SaveTile2Yuv(HT_VDEC_FrameInfo_t *pstFrmInfo, HT_VDEC_CHN VdecChn)
{
    size_t length = 0;
    HT_S8  *ps8Tile_Buf_Y  = NULL;
    HT_S8  *ps8Tile_Buf_UV = NULL;
    HT_S8  *ps8Yuv         = NULL;
	HT_TILE_MODE eMode = E_HT_HVD;

	HT_PHY phyTileY  = (HT_U64)pstFrmInfo->u32LumaAddr;
	HT_PHY phyTileUV = (HT_U64)pstFrmInfo->u32ChromaAddr;
	HT_U32 u32Width  = (HT_U32)pstFrmInfo->u16Width;
	HT_U32 u32Height = (HT_U32)pstFrmInfo->u16Height;
	HT_U32 u32Pitch  = (HT_U32)pstFrmInfo->u16Pitch;

#ifdef HT_WRITE_DDR
	HT_VB_Info_t stDdrVbInfo;
	HT_U8 *pu8Addr = NULL;
	HT_S32 Index = 0;
#endif

	CHECK_NULL_POINTER(__FUNCTION__, pstFrmInfo);

    if (_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr)
    {
		ps8Tile_Buf_Y = phyTileY - _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr + _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr;
		ps8Tile_Buf_UV = phyTileUV - _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr + _stResMgr.astChnInfo[VdecChn].stFrameMemInfo.pVirAddr;
    }
    else
    {
        printk("VdecChn(%d) frame buffer unmap ....\n", VdecChn);
        return HT_FAILURE;
    }

    length = u32Width * u32Height * 3 / 2;
    ps8Yuv = (HT_S8 *)vmalloc(length);
    if (NULL == ps8Yuv)
    {
		printk("VdecChn(%d) vmalloc YUV buffer failed!!\n", VdecChn);
        return HT_FAILURE;
    }

	if(E_HT_VDEC_CODEC_TYPE_H265 == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
	{
		eMode = E_HT_EVD;
	}

	if(HT_SUCCESS != HT_Soft_DetileYUV((void *)ps8Yuv, (void *)ps8Tile_Buf_Y, (void *)ps8Tile_Buf_UV, u32Width, u32Pitch, u32Height, eMode))
	{
        printk("fwrite YUV detile failed!!VdecChn: %d\n", VdecChn);
		goto _VFREE;
	}

#ifdef MODULE_LINKAGE
    {
        HT_U32 length = 0;
        HT_OutputBufList_t *pstOutputBuf;

        pstOutputBuf=(HT_OutputBufList_t *)kmalloc(sizeof(HT_OutputBufList_t),GFP_KERNEL);
        pstOutputBuf->Height = u32Height;
        pstOutputBuf->Width = u32Width;

        length = pstOutputBuf->Height * pstOutputBuf->Width;
        pstOutputBuf->stYuvVbInfo.ePixelFormat = E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;

        HT_MallocYUVOneFrame(&pstOutputBuf->stYuvVbInfo,u32Width,u32Height);
        memcpy(pstOutputBuf->stYuvVbInfo.stYUV420SP.stVbInfo_y.pu8VirtAddr, ps8Yuv, length);
        memcpy(pstOutputBuf->stYuvVbInfo.stYUV420SP.stVbInfo_uv.pu8VirtAddr, ps8Yuv +length, length/2);

        pstOutputBuf->u8Chnidx = VdecChn;
        pstOutputBuf->u8Portidx = 0;
        pstOutputBuf->eModuleID = E_HT_STREAM_MODULEID_VDEC;
        list_add_tail(&(pstOutputBuf->list),&(gstOutputBufHead));
    }
#endif

#ifdef HT_WRITE_DDR
	memset(&stDdrVbInfo, 0x0, sizeof(HT_VB_Info_t));

	//TODO: alloc buffer for yuv, and not free
	if(FALSE == _stResMgr.bAllocDdrMem[VdecChn])
	{
		stDdrVbInfo.u32Size = u32Width * u32Height * 3 / 2 * HT_VDEC_WRITE_FRAME_NUM;
		if(HT_SUCCESS != HT_MallocAlign(&stDdrVbInfo, 64))
		{
			printk("alloc Ddr buffer failed!!VdecChn: %d\n", VdecChn);
			goto _VFREE;
		}
		memset(stDdrVbInfo.pu8VirtAddr, 0x0, sizeof(HT_VB_Info_t));
		_stResMgr.stYuv2DdrMem[VdecChn].phyAddr 	= stDdrVbInfo.phyAddr;
		_stResMgr.stYuv2DdrMem[VdecChn].u32Size 	= stDdrVbInfo.u32Size;
		_stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr = stDdrVbInfo.pu8VirtAddr;

		printk("############# Yuv Ddr phy addr %llx\n", _stResMgr.stYuv2DdrMem[VdecChn].phyAddr);
		printk("############# Yuv Ddr vir addr %p\n", _stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr);
		printk("############# Size %x, width %u, height %u\n", _stResMgr.stYuv2DdrMem[VdecChn].u32Size, u32Width, u32Height);

		_stResMgr.bAllocDdrMem[VdecChn] = TRUE;
	}

	pu8Addr = _stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr;
	Index	= _stResMgr.as32SaveYuvNum[VdecChn];

	memcpy(pu8Addr + Index * length, ps8Yuv, length);

	_stResMgr.as32SaveYuvNum[VdecChn] += 1;
#else
	if(FALSE == _stResMgr.bFileOpen[VdecChn])
	{
		// set yuv file name
		if (HT_SUCCESS != _HT_VDEC_GetYuvFilename(VdecChn, u32Width, u32Height, _stResMgr.as8YuvFilename[VdecChn]))
		{
			printk("VDEC Get Yuv Filename failed!!VdecChn: %d\n", VdecChn);
			goto _VFREE;
		}

		// openfile
		_stResMgr.fpFrame[VdecChn] = _HT_VDEC_OpenFile(_stResMgr.as8YuvFilename[VdecChn], O_RDWR | O_CREAT | O_TRUNC, 0777, &_stResMgr.fsFrame[VdecChn]);
		if(NULL == _stResMgr.fpFrame[VdecChn])
		{
			printk("VdecChn(%d) Open %s failed!!\n", VdecChn, _stResMgr.as8YuvFilename[VdecChn]);
			goto _END;
		}

		_stResMgr.bFileOpen[VdecChn] = TRUE;
	}

    if(_stResMgr.fpFrame[VdecChn]->f_op->write(_stResMgr.fpFrame[VdecChn], ps8Yuv, length, &(_stResMgr.fpFrame[VdecChn]->f_pos)) != length)
    {
        printk("fwrite YUV failed!!VdecChn: %d\n", VdecChn);
		goto _END;
    }
#endif

	vfree(ps8Yuv);
	ps8Yuv = NULL;

	return HT_SUCCESS;

#ifndef HT_WRITE_DDR
_END:
	set_fs(_stResMgr.fsFrame[VdecChn]);
	filp_close(_stResMgr.fpFrame[VdecChn], NULL);
#endif
_VFREE:
	vfree(ps8Yuv);
	ps8Yuv = NULL;

	return HT_FAILURE;
}

static HT_RESULT _HT_VDEC_SaveJpegDecYuv(HT_VDEC_CHN VdecChn, HT_BufInfo_t *pstPutBufInfo)
{
	HT_U32 u32Length = 0;

	HT_U8 *ps8Yuv 	 = pstPutBufInfo->stFrameData.pVirAddr[0];
	HT_U32 u32Width	 = pstPutBufInfo->stFrameData.u16Width;
	HT_U32 u32Height = pstPutBufInfo->stFrameData.u16Height;
	HT_U32 u32Pitch	 = pstPutBufInfo->stFrameData.u32Stride[0];

#ifdef HT_WRITE_DDR
	HT_VB_Info_t stDdrVbInfo;
	HT_U8 		 *pu8Addr = NULL;
	HT_S32 		 Index = 0;
#endif

	CHECK_NULL_POINTER(__FUNCTION__, pstPutBufInfo);

	u32Length = u32Pitch * u32Height; //存文件时，按对齐后的size存储

#ifdef HT_WRITE_DDR
	memset(&stDdrVbInfo, 0x0, sizeof(HT_VB_Info_t));

	//alloc buffer for yuv, and not free
	if(FALSE == _stResMgr.bAllocDdrMem[VdecChn])
	{
		stDdrVbInfo.u32Size = u32Length * HT_VDEC_WRITE_FRAME_NUM;
		if(HT_SUCCESS != HT_MallocAlign(&stDdrVbInfo, 64))
		{
			printk("alloc Ddr buffer failed!!VdecChn: %d\n", VdecChn);
			return HT_FAILURE;
		}
		memset(stDdrVbInfo.pu8VirtAddr, 0x0, sizeof(HT_VB_Info_t));
		_stResMgr.stYuv2DdrMem[VdecChn].phyAddr 	= stDdrVbInfo.phyAddr;
		_stResMgr.stYuv2DdrMem[VdecChn].u32Size 	= stDdrVbInfo.u32Size;
		_stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr = stDdrVbInfo.pu8VirtAddr;

		printk("############# Yuv Ddr phy addr %llx\n", _stResMgr.stYuv2DdrMem[VdecChn].phyAddr);
		printk("############# Yuv Ddr vir addr %p\n", _stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr);
		printk("############# Size %x\n", _stResMgr.stYuv2DdrMem[VdecChn].u32Size);
		printk("############# output  u32Width: %u\n", u32Width);
		printk("############# output u32Height: %u\n", u32Height);

		_stResMgr.bAllocDdrMem[VdecChn] = TRUE;
	}

	pu8Addr = _stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr;
	Index	= _stResMgr.as32SaveYuvNum[VdecChn];

	memcpy(pu8Addr + Index * u32Length, ps8Yuv, u32Length);

	_stResMgr.as32SaveYuvNum[VdecChn] += 1;
#else
	if(FALSE == _stResMgr.bFileOpen[VdecChn])
	{
		// set yuv file name
		if (HT_SUCCESS != _HT_VDEC_GetYuvFilename(VdecChn, u32Width, u32Height, _stResMgr.as8YuvFilename[VdecChn]))
		{
			printk("VDEC Get Yuv Filename failed!!VdecChn: %d\n", VdecChn);
			return HT_FAILURE;
		}

		// openfile
		_stResMgr.fpFrame[VdecChn] = _HT_VDEC_OpenFile(_stResMgr.as8YuvFilename[VdecChn], O_RDWR | O_CREAT | O_TRUNC, 0777, &_stResMgr.fsFrame[VdecChn]);
		if(NULL == _stResMgr.fpFrame[VdecChn])
		{
			printk("VdecChn(%d) Open %s failed!!\n", VdecChn, _stResMgr.as8YuvFilename[VdecChn]);
			goto _END;
		}

		_stResMgr.bFileOpen[VdecChn] = TRUE;
	}

    if(_stResMgr.fpFrame[VdecChn]->f_op->write(_stResMgr.fpFrame[VdecChn], ps8Yuv, u32Length, &(_stResMgr.fpFrame[VdecChn]->f_pos)) != u32Length)
    {
        printk("fwrite YUV failed!!VdecChn: %d\n", VdecChn);
		goto _END;
    }
#endif

	return HT_SUCCESS;
#ifndef HT_WRITE_DDR
_END:
	set_fs(_stResMgr.fsFrame[VdecChn]);
	filp_close(_stResMgr.fpFrame[VdecChn], NULL);
#endif

	return HT_FAILURE;
}

static HT_S32 _HT_VDEC_Thread_WrBuf(void *pData)
{
	HT_S32 i = 0;
	HT_RESULT Ret = HT_FAILURE;
	HT_VDEC_CHN VdecChn = 0;
	HT_VDEC_Stream_t stStreamBuf[HT_VDEC_MAX_CHN_NUM];
	struct file *fpStream[HT_VDEC_MAX_CHN_NUM] = {NULL};
    mm_segment_t fs[HT_VDEC_MAX_CHN_NUM];

	//CHECK_NULL_POINTER(__FUNCTION__, pData);

	for (i = 0; i < _stResMgr.u32ChnNum; i++)
	{
		VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		memset(&stStreamBuf[VdecChn], 0 , sizeof(HT_VDEC_Stream_t));
		stStreamBuf[VdecChn].pu8Addr = kmalloc(HT_VDEC_READ_STREAM_LEN, GFP_KERNEL);
		if (!stStreamBuf[VdecChn].pu8Addr)
		{
			printk("kmalloc for ES Stream buffer failed!!VdecChn: %d\n", VdecChn);
		}
		memset(stStreamBuf[VdecChn].pu8Addr, 0x0, HT_VDEC_READ_STREAM_LEN);

		fpStream[VdecChn] = _HT_VDEC_OpenFile(_stResMgr.as8EsFilename[VdecChn], O_RDONLY, 0644, &fs[VdecChn]);
		if(NULL == fpStream[VdecChn])
		{
			printk("VdecChn(%d) Open %s failed!!\n", VdecChn, _stResMgr.as8EsFilename[VdecChn]);
		}
	}

    while (_stResMgr.bWrBufTaskRun)
    {
		mdelay(10); //delay 10ms tentative
        for (i = 0; i < _stResMgr.u32ChnNum; i++)
        {
        	VdecChn = _stResMgr.au16ChnId[i];

            if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
            {
                continue;
            }

            DOWN(&_stResMgr.semChnLock[VdecChn]);
			if(TRUE == _HT_VDEC_CheckChnTaskStatus(VdecChn))
			{
                UP(&_stResMgr.semChnLock[VdecChn]);
				continue; //Chn task finished
			}

			if(FALSE == stStreamBuf[VdecChn].bReadSuccess)
			{
				Ret = _HT_VDEC_ReadOneFrame(&stStreamBuf[VdecChn], fpStream[VdecChn]);
				if (Ret)
				{
	                UP(&_stResMgr.semChnLock[VdecChn]);
					continue;
				}
			}

			Ret = _HT_VDEC_WrBuf(VdecChn, &stStreamBuf[VdecChn]);
			if(Ret)
			{
                UP(&_stResMgr.semChnLock[VdecChn]);
				continue;
			}

			UP(&_stResMgr.semChnLock[VdecChn]);
        }
    }

 	for (i = 0; i < _stResMgr.u32ChnNum; i++)
    {
    	VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		kfree(stStreamBuf[VdecChn].pu8Addr);
		stStreamBuf[VdecChn].pu8Addr = NULL;

		set_fs(fs[VdecChn]);
		filp_close(fpStream[VdecChn], NULL);
 	}

	_stResMgr.bWrBufTaskRun = FALSE;

    return HT_SUCCESS;
}

static HT_S32 _HT_VDEC_Thread_PutFrame(void *pData)
{
	HT_S32 i = 0;
	HT_VDEC_CHN VdecChn	= 0;
	HT_VDEC_DispFrame_t stDispFrm;
	HT_U32 au32WrFrmNum[HT_VDEC_MAX_CHN_NUM] = {0};

	//CHECK_NULL_POINTER(__FUNCTION__, pData);

    while (_stResMgr.bPutFrmTaskRun)
    {
        for (i = 0; i < _stResMgr.u32ChnNum; i++)
        {
        	VdecChn = _stResMgr.au16ChnId[i];

            if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
            {
                continue;
            }

			DOWN(&_stResMgr.semChnLock[VdecChn]);
			if(TRUE == _HT_VDEC_CheckChnTaskStatus(VdecChn))
			{
				UP(&_stResMgr.semChnLock[VdecChn]);
				continue; //Chn task finished
			}

			memset(&stDispFrm, 0x0, sizeof(HT_VDEC_DispFrame_t));
            if (HT_SUCCESS != _HT_VDEC_GetFrame(VdecChn, &stDispFrm))
            {
				UP(&_stResMgr.semChnLock[VdecChn]);
      			continue; // if GetFrame failed, continue
            }

			// record YUV or save yuv to ddr
			if(HT_SUCCESS == _HT_VDEC_SaveTile2Yuv(&stDispFrm.stFrmInfo, VdecChn))
			{
				au32WrFrmNum[VdecChn] += 1;
			}

			if(HT_SUCCESS != _HT_VDEC_RlsFrame(VdecChn, &stDispFrm))
			{
				printk("release frame buffer failed!!\n");
				UP(&_stResMgr.semChnLock[VdecChn]);
				return HT_FAILURE;
			}

			if(HT_VDEC_WRITE_FRAME_NUM == au32WrFrmNum[VdecChn])
			{
				_HT_VDEC_SetChnTaskStatus(VdecChn, TRUE);
			}
			UP(&_stResMgr.semChnLock[VdecChn]);
        }
    }

#ifndef HT_WRITE_DDR
 	for (i = 0; i < _stResMgr.u32ChnNum; i++)
    {
    	VdecChn = _stResMgr.au16ChnId[i];

		set_fs(_stResMgr.fsFrame[VdecChn]);
		filp_close(_stResMgr.fpFrame[VdecChn], NULL);

		_stResMgr.bFileOpen[VdecChn] = FALSE;
 	}
#endif

	_stResMgr.bPutFrmTaskRun = FALSE;

    return HT_SUCCESS;
}

static HT_S32 _HT_VDEC_Thread_PutJpegFrmTask(void *pData)
{
	HT_S32 i = 0;
	HT_RESULT Ret = HT_FAILURE;
	HT_VDEC_CHN VdecChn	= 0;
	//HT_VB_Info_t stEsVbInfo;
	HT_VB_Info_t stFrameVbInfo[HT_VDEC_MAX_CHN_NUM];
    HT_BufInfo_t stInjBufInfo;
    HT_BufInfo_t stPutBufInfo;

	HT_U32 au32WrFrmNum[HT_VDEC_MAX_CHN_NUM] = {0};
	HT_VDEC_Stream_t stStreamBuf[HT_VDEC_MAX_CHN_NUM];

	struct file *fpStream[HT_VDEC_MAX_CHN_NUM] = {NULL};
    mm_segment_t fs[HT_VDEC_MAX_CHN_NUM];

	//CHECK_NULL_POINTER(__FUNCTION__, pData);

	//memset(&stEsVbInfo,    0x0, sizeof(HT_VB_Info_t));
	memset(stFrameVbInfo,  0x0, sizeof(HT_VB_Info_t) * HT_VDEC_MAX_CHN_NUM);
	memset(&stInjBufInfo,  0x0, sizeof(HT_BufInfo_t));
	memset(&stPutBufInfo,  0x0, sizeof(HT_BufInfo_t));

	for (i = 0; i < _stResMgr.u32ChnNum; i++)
	{
		VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		memset(&stStreamBuf[VdecChn], 0 , sizeof(HT_VDEC_Stream_t));
		stStreamBuf[VdecChn].pu8Addr = (HT_U8 *)vmalloc(HT_JPEG_READ_BUFFER_LEN);
		if (!stStreamBuf[VdecChn].pu8Addr)
		{
			printk("vmalloc for Jpeg ES Stream buffer failed!!VdecChn: %d\n", VdecChn);
			goto _VFREE;
		}
		memset(stStreamBuf[VdecChn].pu8Addr, 0x0, HT_JPEG_READ_BUFFER_LEN);

		/// malloc buffer for stPutBufInfo
		stFrameVbInfo[VdecChn].u32Size = \
			ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicWidth, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE) \
			* ALIGN_BYTES(_stResMgr.astChnInfo[VdecChn].stChnAttr.u32PicHeight, HT_VDEC_IMAGE_PITCH_ALIGN_SIZE) \
			* 4;
		stFrameVbInfo[VdecChn].u32Size = ALIGN_BYTES(stFrameVbInfo[VdecChn].u32Size, HT_VDEC_PAGE_SIZE);
		if (HT_SUCCESS != HT_MallocAlign(&stFrameVbInfo[VdecChn], 64))
		{
			printk("alloc frame buffer failed!!\n");
			goto _FREE;
		}
		memset(stFrameVbInfo[VdecChn].pu8VirtAddr, 0x0, stFrameVbInfo[VdecChn].u32Size);

		fpStream[VdecChn] = _HT_VDEC_OpenFile(_stResMgr.as8EsFilename[VdecChn], O_RDONLY, 0644, &fs[VdecChn]);
		if(NULL == fpStream[VdecChn])
		{
			printk("VdecChn(%d) Open %s failed!!\n", VdecChn, _stResMgr.as8EsFilename[VdecChn]);
		}
	}

    while (_stResMgr.bPutFrmJpegTaskRun)
    {
        for (i = 0; i < _stResMgr.u32ChnNum; i++)
        {
        	VdecChn = _stResMgr.au16ChnId[i];

            if (E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
            {
                continue;
            }

			DOWN(&_stResMgr.semChnLock[VdecChn]);
			if(TRUE == _HT_VDEC_CheckChnTaskStatus(VdecChn))
			{
				UP(&_stResMgr.semChnLock[VdecChn]);
				continue; //Chn task finished
			}

			if(FALSE == stStreamBuf[VdecChn].bReadSuccess)
			{
				Ret = _HT_VDEC_ReadOneFrame(&stStreamBuf[VdecChn], fpStream[VdecChn]);
				if (Ret)
				{
	                UP(&_stResMgr.semChnLock[VdecChn]);
					continue;
				}
			}

			stPutBufInfo.stFrameData.phyAddr[0]  = stFrameVbInfo[VdecChn].phyAddr;
			stPutBufInfo.stFrameData.pVirAddr[0] = stFrameVbInfo[VdecChn].pu8VirtAddr;

			// 读文件的数据传递给JpegDecoder
			stInjBufInfo.u64Pts 				  = stStreamBuf[VdecChn].u64PTS;
			stInjBufInfo.stRawData.u32ContentSize = stStreamBuf[VdecChn].u32Len;
			stInjBufInfo.stRawData.pVirAddr	      = (void *)stStreamBuf[VdecChn].pu8Addr;

			stPutBufInfo.u64Pts = stInjBufInfo.u64Pts * 1000ULL;
			if (TRUE != _HT_VDEC_JpegDecoder(VdecChn, &stInjBufInfo, &stPutBufInfo))
			{
				UP(&_stResMgr.semChnLock[VdecChn]);
      			continue; // if GetFrame failed, continue
			}

			//解码1帧完成后，读文件标志位恢复
			stStreamBuf[VdecChn].bReadSuccess = FALSE;

			// record YUV or save YUV to ddr
			if(HT_SUCCESS == _HT_VDEC_SaveJpegDecYuv(VdecChn, &stPutBufInfo))
			{
				au32WrFrmNum[VdecChn] += 1;
			}

			// record decode frames
			if(HT_VDEC_WRITE_FRAME_NUM == au32WrFrmNum[VdecChn])
			{
				_HT_VDEC_SetChnTaskStatus(VdecChn, TRUE);
			}
			UP(&_stResMgr.semChnLock[VdecChn]);
        }
    }

 	for (i = 0; i < _stResMgr.u32ChnNum; i++)
    {
    	VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		vfree(stStreamBuf[VdecChn].pu8Addr);
		stStreamBuf[VdecChn].pu8Addr = NULL;

		if(NULL != stFrameVbInfo[VdecChn].pu8VirtAddr)
		{
			HT_Free(stFrameVbInfo[VdecChn].phyAddr);
		}

		// ESfile
		set_fs(fs[VdecChn]);
		filp_close(fpStream[VdecChn], NULL);

#ifndef HT_WRITE_DDR
		// YUVfile
		set_fs(_stResMgr.fsFrame[VdecChn]);
		filp_close(_stResMgr.fpFrame[VdecChn], NULL);

		_stResMgr.bFileOpen[VdecChn] = FALSE;
#endif
 	}

	_stResMgr.bPutFrmTaskRun = FALSE;

    return HT_SUCCESS;

_FREE:
 	for (i = 0; i < _stResMgr.u32ChnNum; i++)
    {
    	VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		if(NULL != stFrameVbInfo[VdecChn].pu8VirtAddr)
		{
			HT_Free(stFrameVbInfo[VdecChn].phyAddr);
		}
 	}
_VFREE:
 	for (i = 0; i < _stResMgr.u32ChnNum; i++)
    {
    	VdecChn = _stResMgr.au16ChnId[i];

		if (E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
			continue;
		}

		if(NULL != stStreamBuf[VdecChn].pu8Addr)
		{
			vfree(stStreamBuf[VdecChn].pu8Addr);
			stStreamBuf[VdecChn].pu8Addr = NULL;
		}
 	}

    return HT_FAILURE;
}

static HT_RESULT _HT_VDEC_GetCpuBuffer(HT_VDEC_MemBufInfo_t *pstCpuMemInfo)
{
	HT_VB_Info_t stCpuVbInfo;
    HT_U32 addr, size;
    HT_U8 miu;

	memset(&stCpuVbInfo, 	0x0, sizeof(HT_VB_Info_t));

    HT_MmapParser("E_MMAP_ID_VDEC_CPU", &addr, &size, &miu);

    pstCpuMemInfo->phyAddr = addr;
    pstCpuMemInfo->u32BufSize = size;
    printk("#################cpu phy addr %llx\n", pstCpuMemInfo->phyAddr);
    printk("#################cpu buffer size %x\n", pstCpuMemInfo->u32BufSize);


	return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_CreateThread(void)
{
    if (_stResMgr.bInitFlag)
    {
        printk("Already Init\n");
        return HT_SUCCESS;
    }

	_stResMgr.pWrBufTask = kthread_create(_HT_VDEC_Thread_WrBuf, NULL, "_HT_VDEC_Thread_WrBuf");
	if (IS_ERR(_stResMgr.pWrBufTask))
	{
		printk(KERN_ERR "print WrBuf: unable to Start kernel thread: %ld\n", PTR_ERR(_stResMgr.pWrBufTask));
		return HT_FAILURE;
	}

	/*get frame and record YUV*/
	_stResMgr.pPutFrmTask = kthread_create(_HT_VDEC_Thread_PutFrame, NULL, "_HT_VDEC_Thread_PutFrame");
	if (IS_ERR(_stResMgr.pPutFrmTask))
	{
		printk(KERN_ERR "print PutFrame: unable to Start kernel thread: %ld\n", PTR_ERR(_stResMgr.pPutFrmTask));
		return HT_FAILURE;
	}

    _stResMgr.pPutJpegFrmTask = kthread_create(_HT_VDEC_Thread_PutJpegFrmTask, NULL, "_HT_VDEC_Thread_PutJpegFrmTask");
	if (IS_ERR(_stResMgr.pPutJpegFrmTask))
	{
		printk(KERN_ERR "print PutJpegFrame: unable to Start kernel thread: %ld\n", PTR_ERR(_stResMgr.pPutJpegFrmTask));
		return HT_FAILURE;
	}

	_stResMgr.bWrBufTaskRun = TRUE;
	wake_up_process(_stResMgr.pWrBufTask);

	_stResMgr.bPutFrmTaskRun = TRUE;
	wake_up_process(_stResMgr.pPutFrmTask);

	_stResMgr.bPutFrmJpegTaskRun = TRUE;
	wake_up_process(_stResMgr.pPutJpegFrmTask);

	_stResMgr.bInitFlag = TRUE;

	printk("Vdec thread run\n");
	return HT_SUCCESS;
}

static void _HT_VDEC_StopChn(HT_VDEC_CHN VdecChn)
{
	VDEC_StreamId stStreamId;

	memset(&stStreamId, 0x0, sizeof(VDEC_StreamId));
	stStreamId.u32Id      = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Id;
	stStreamId.u32Version = _stResMgr.astChnInfo[VdecChn].stVDECStreamId.u32Version;

	DOWN(&(_stResMgr.semChnLock[VdecChn]));

    if (_stResMgr.astChnInfo[VdecChn].bStart)
    {
		if(E_HT_VDEC_CODEC_TYPE_JPEG != _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
		{
	        MApi_VDEC_EX_Exit(&stStreamId);
	        _stResMgr.astChnInfo[VdecChn].bStart = FALSE;
		}
    }

	UP(&(_stResMgr.semChnLock[VdecChn]));

}

static void _HT_VDEC_DestroyChn(HT_VDEC_CHN VdecChn)
{
	DOWN(&(_stResMgr.semChnLock[VdecChn]));

    HT_VDEC_ChkChnCreate(VdecChn);

    if (E_HT_VDEC_CODEC_TYPE_JPEG == _stResMgr.astChnInfo[VdecChn].stChnAttr.eCodecType)
    {
        _HT_VDEC_MMABufMapping(VdecChn, FALSE);
    }

    HT_Free(_stResMgr.astChnInfo[VdecChn].stEsMemInfo.phyAddr);
    HT_Free(_stResMgr.astChnInfo[VdecChn].stFrameMemInfo.phyAddr);
    memset(&_stResMgr.astChnInfo[VdecChn], 0x0, sizeof(HT_VDEC_ChnInfo_t));

	_stResMgr.astChnInfo[VdecChn].bCreate    = FALSE;

	UP(&(_stResMgr.semChnLock[VdecChn]));
}

static void _HT_VDEC_DestroyThread(void)
{
	HT_S32 s32Ret = -1;

	_stResMgr.bWrBufTaskRun = FALSE;
	if(!IS_ERR(_stResMgr.pWrBufTask))
	{
		s32Ret = kthread_stop(_stResMgr.pWrBufTask);
		printk(KERN_INFO "thread WrBuf end. s32Ret = %d\n", s32Ret);
	}

	_stResMgr.bPutFrmTaskRun = FALSE;
	if(!IS_ERR(_stResMgr.pPutFrmTask))
	{
		s32Ret = kthread_stop(_stResMgr.pPutFrmTask);
		printk(KERN_INFO "thread PutFrm end. s32Ret = %d\n", s32Ret);
	}

	_stResMgr.bPutFrmJpegTaskRun = FALSE;
	if(!IS_ERR(_stResMgr.pPutJpegFrmTask))
	{
		s32Ret = kthread_stop(_stResMgr.pPutJpegFrmTask);
		printk(KERN_INFO "thread Jpeg PutFrm end. s32Ret = %d\n", s32Ret);
	}
}

/*vdec utest exit*/
static void _HT_VDEC_Stop(void)
{
	HT_S32 i = 0;
	HT_VDEC_CHN VdecChn = 0;

	for(i = 0; i < HT_VDEC_MAX_CHN_NUM; i++)
	{
		VdecChn = _stResMgr.au16ChnId[i];
		if(FALSE == _stResMgr.astChnInfo[VdecChn].bChnConfig)
		{
			continue;
		}

		_HT_VDEC_StopChn(VdecChn);
		_HT_VDEC_DestroyChn(VdecChn);
	}

    printk("Task finished, exit.\n");
}
static void _HT_VDEC_IMPL_PoolMapping(void)
{
    HT_U32 u32Addr = 0, u32Size = 0;
    HT_U8 u8Miu = 0;
    if(!MsOS_Init())
    {
        printk("%s: MsOS_Init fail\n",__FUNCTION__);
        BUG();
    }

    if(!MsOS_MPool_Init())
    {
        printk("%s: MsOS_MPool_Init fail\n",__FUNCTION__);
        BUG();
    }

    HT_MmapParser("E_MMAP_ID_VDEC_CPU", &u32Addr, &u32Size, &u8Miu);

    if(!MsOS_MPool_Mapping_Dynamic(u8Miu, u32Addr, u32Size, TRUE))
    {
        printk("MsOS_MPool_Mapping_Dynamic E_MMAP_ID_VDEC_CPU fail!\n");
        if(MsOS_MPool_Mapping_Dynamic(u8Miu, u32Addr, u32Size, FALSE))
        {
            printk("MsOS_MPool_Mapping_Dynamic E_MMAP_ID_VDEC_CPU fail!\n");
        }
    }
}

static void _HT_VDEC_InitVar(void)
{
	 HT_VDEC_CHN VdecChn = 0;
     HT_OutputBufList_t *pos,*posN;

    if (!list_empty(&gstOutputBufHead))
     {
         list_for_each_entry_safe(pos,posN,&gstOutputBufHead, list)
         {
             HT_FreeYUVOneFrame(&pos->stYuvVbInfo);
             list_del(&pos->list);
             kfree(pos);
         }
     }

     INIT_LIST_HEAD(&gstOutputBufHead);
#ifdef HT_WRITE_DDR
	// free mem allocated last time
	for (VdecChn = 0; VdecChn < HT_VDEC_MAX_CHN_NUM; VdecChn++)
    {
		if(NULL != _stResMgr.stYuv2DdrMem[VdecChn].pu8VirtAddr)
		{
			HT_Free(_stResMgr.stYuv2DdrMem[VdecChn].phyAddr);
		}
    }
#endif

    memset(_bChnTaskFinished, 0, sizeof(HT_BOOL) * HT_VDEC_MAX_CHN_NUM);
	memset(&_stResMgr, 0, sizeof(HT_VDEC_ResMgr_t));

	for (VdecChn = 0; VdecChn < HT_VDEC_MAX_CHN_NUM; VdecChn++)
    {
        sema_init(&(_stResMgr.semChnLock[VdecChn]), 1);
    }
    _HT_VDEC_IMPL_PoolMapping();
}
static void _HT_VDEC_DeinitVar(void)
{

    MsOS_MPool_Close();

    memset(_bChnTaskFinished, 0, sizeof(HT_BOOL) * HT_VDEC_MAX_CHN_NUM);

//if save ddr, data cannot be cleared here
#ifndef HT_WRITE_DDR
	memset(&_stResMgr, 0, sizeof(HT_VDEC_ResMgr_t));
#endif
    printk("Deinit var!\n");

}

static HT_RESULT _HT_VDEC_InitXcShm(void)
{
	HT_VB_Info_t stDvXcShmMemInfo;

    stDvXcShmMemInfo.u32Size = _stResMgr.stDvXcShmMem.u32BufSize = 4*1024;
	if (HT_SUCCESS != HT_MallocAlign(&stDvXcShmMemInfo, 64))
	{
		printk("alloc XcShm buffer failed!!\n");
        return HT_FAILURE;
	}
    _stResMgr.stDvXcShmMem.phyAddr = stDvXcShmMemInfo.phyAddr;
    _stResMgr.stDvXcShmMem.pVirAddr = stDvXcShmMemInfo.pu8VirtAddr;

    return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_DeinitXcShm(void)
{
    HT_Free(_stResMgr.stDvXcShmMem.phyAddr);
    memset(&_stResMgr.stDvXcShmMem, 0, sizeof(HT_VDEC_MemBufInfo_t));

    return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_AllocJpegInterMem(void)
{
	HT_VB_Info_t stJpegInterMemInfo;

	memset(&stJpegInterMemInfo, 0, sizeof(HT_VB_Info_t));
    stJpegInterMemInfo.u32Size = 400*1024;
	if (HT_SUCCESS != HT_MallocAlign(&stJpegInterMemInfo, 64))
	{
		printk("alloc Jpeg Internal buffer failed!!\n");
        return HT_FAILURE;
	}
	_stResMgr.stJpegInternalMem.u32BufSize 	= stJpegInterMemInfo.u32Size;
    _stResMgr.stJpegInternalMem.phyAddr  	= stJpegInterMemInfo.phyAddr;
    _stResMgr.stJpegInternalMem.pVirAddr 	= stJpegInterMemInfo.pu8VirtAddr;

	printk("############# Jpeg Inter phy addr %llx\n", _stResMgr.stJpegInternalMem.phyAddr);
	printk("############# Jpeg Inter virt addr %p\n", _stResMgr.stJpegInternalMem.pVirAddr);
	printk("############# Size %x\n", _stResMgr.stJpegInternalMem.u32BufSize);

	MsOS_MPool_Add_PA2VARange(
		_stResMgr.stJpegInternalMem.phyAddr,
		(MS_U32)_stResMgr.stJpegInternalMem.pVirAddr,
		ALIGN_BYTES(_stResMgr.stJpegInternalMem.u32BufSize, HT_VDEC_PAGE_SIZE),
		TRUE);

	_stResMgr.stJpegInternalMem.pVirAddr =
		(void *)MsOS_MPool_PA2KSEG1(_stResMgr.stJpegInternalMem.phyAddr);

    return HT_SUCCESS;
}

static HT_RESULT _HT_VDEC_FreeJpegInterMem(void)
{
    MsOS_MPool_Remove_PA2VARange(
        _stResMgr.stJpegInternalMem.phyAddr,
        (MS_U32)_stResMgr.stJpegInternalMem.pVirAddr,
        ALIGN_BYTES(_stResMgr.stJpegInternalMem.u32BufSize, HT_VDEC_PAGE_SIZE),
        TRUE);

    HT_Free(_stResMgr.stJpegInternalMem.phyAddr);
    memset(&_stResMgr.stJpegInternalMem, 0, sizeof(HT_VDEC_MemBufInfo_t));

    return HT_SUCCESS;
}

/******************************************************************
	Input param declaration:
		[ChnId Protocol PicW PicH]
	use 4 data as a group, support multigroup
 ******************************************************************/
HT_RESULT HT_VDEC(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
	HT_RESULT Ret = HT_FAILURE;

	CHECK_NULL_POINTER(__FUNCTION__, pau16CmdValue);

	_HT_VDEC_InitVar();

	// set default parameter
	_HT_VDEC_SetParamDft();

	// get input parameter
	Ret = _HT_VDEC_GetParam(pau16CmdValue, u8CmdCnt);
	if(HT_SUCCESS != Ret)
	{
		printk("VDEC Get Param failed!!\n");
		goto GET_PARA2_ERROR;
	}

	//alloc cpu buffer
    Ret = _HT_VDEC_GetCpuBuffer(&_stResMgr.stCpuMem);
    if (HT_SUCCESS != Ret)
    {
        printk("Can't Get Cpu Buffer\n");
        goto GET_PARA2_ERROR;
    }

	//alloc XC Shm buffer
    Ret = _HT_VDEC_InitXcShm();
    if (HT_SUCCESS != Ret)
    {
        printk("Can't Get Xc Shm Buffer\n");
        goto GET_PARA2_ERROR;
    }

	// alloc Jpeg internal mem
    Ret = _HT_VDEC_AllocJpegInterMem();
    if (HT_SUCCESS != Ret)
    {
        printk("Can't Alloc Jpeg Internal Buffer\n");
        goto GET_PARA1_ERROR;
    }

	// vedio codec start
	Ret = _HT_VDEC_Start();
	if(HT_SUCCESS != Ret)
	{
		printk("VDEC Init failed!!\n");
		goto START_ERROR;
	}

	// Thread Init
	Ret = _HT_VDEC_CreateThread();
	if(HT_SUCCESS != Ret)
	{
		printk("VDEC CreateThread failed!!\n");
		goto START_ERROR;
	}

	// wait for writing file finished
	printk("Wait for task finished\n");
	while(TRUE)
	{
		if(_HT_VDEC_CheckAllChnTaskStatus())
		{
			break;
		}
		mdelay(100);
	}


    _HT_VDEC_DestroyThread();

START_ERROR:
    _HT_VDEC_Stop();
    _HT_VDEC_FreeJpegInterMem();
GET_PARA1_ERROR:
    _HT_VDEC_DeinitXcShm();
GET_PARA2_ERROR:
	_HT_VDEC_DeinitVar();

	return Ret;
}

extern void HT_VDEC_DisplayHelp(void)
{
	printk(" parameter declaration:  			\n");
	printk(" param 1: ChnId          			\n");
	printk(" param 2: eCodecType     			\n");
	printk(" param 3: u32PicWidth    			\n");
	printk(" param 4: u32PicHeight   			\n");
}




