#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/list.h>
#include "ht_common_datatype.h"
#include "ht_common.h"
#include "ht_vb_type.h"
//#include "mhal_common.h"
#include "mhal_venc.h"
#include "ht_venc.h"
#include "mhal_my_venc.h"

// JPEG FormatType is not sure line 559



#define HT_VENC_PRT_ERR_INFO 1                     //print err information
#define HT_VENC_PRT_DBG_INFO 0                     //print debug information, if want print, set 1

//#define HT_WRITE_DDR                             //the MACRO in ht_common.h, define here just to test
												   //if define, will not write to file, and malloc a large
												   //buff to store encoder data, the buff will not free;
												   //if not define, write to file, and every malloc
                                                   //buff will be free


#define HT_VENC_MSLEEP_TIME 40                      //40ms

//Device handle select
#define HT_VENC_DEVICE_HANDLE_H265_0   0
#define HT_VENC_DEVICE_HANDLE_H265_1   1
#define HT_VENC_DEVICE_HANDLE_H264     2
#define HT_VENC_DEVICE_HANDLE_JPEG     3

#define HT_VENC_INPUT_FILE_FRAME_MAX   2    //input yuv file frame count for one file
#define HT_VENC_OUTPUT_FILE_FRAME_MAX  5     //output yuv file frame count(all encoder frame count)

//argument select
#define HT_VENC_ENCODE_TYPE  0
#define HT_VENC_RATE_CONTROL_TYPE 1
#define HT_VENC_ROI_TYPE          2
#define HT_VENC_MULTISLICE_TYPE   3
#define HT_VENC_RESOLUTION_TYPE   4

//Encode type
#define HT_VENC_H265 0
#define HT_VENC_H264 1
#define HT_VENC_JPEG_YUV420 2
#define HT_VENC_JPEG_YUV422 3

//Rate control type
#define HT_VENC_NONE  0
#define HT_VENC_CBR   1
#define HT_VENC_VBR   2
#define HT_VENC_FIXQP 3

//Resolution type
#define HT_VENC_3840_2160 0
#define HT_VENC_1920_1088 1
#define HT_VENC_1280_720  2
#define HT_VENC_720_480   3
#define HT_VENC_640_480   4


typedef unsigned char BOOL;

#if HT_VENC_PRT_DBG_INFO
    #define HT_VENC_PRT_DONE(string) printk("Line:%d,%s Done\n", __LINE__, string)
#else
    #define HT_VENC_PRT_DONE(string) ;
#endif



#define HT_VENC_ERR_INFO   do{                  \
    if (HT_VENC_PRT_DBG_INFO)                   \
    {                                           \
        printk("File:%s\n", __FILE__);          \
        printk("Func:%s\n", __FUNCTION__);      \
        printk("Line:%d\n", __LINE__);          \
    }                                           \
} while (0)

#define HT_VENC_PRT_ERR(string) do{             \
    if (HT_VENC_PRT_DBG_INFO)                   \
    {                                           \
        printk("Error:%s\n", string);           \
        HT_VENC_ERR_INFO;                       \
    }                                           \
}while(0)

#define HT_VENC_MEMSET_STRUCT(type, _struct)  (memset(&_struct, 0, sizeof(type)))

typedef struct HT_VENC_InterBuff_s{
    HT_VB_Info_t stIntrRef;// to malloc InterRefBuffSize
    HT_VB_Info_t stIntrAl; // to malloc InterAlBuffSize
}HT_VENC_InterBuff_t;

/*
    for yuv420, u32YUVBuffSize store Y data
    for yuv420, u32YUVBuffSize2 sotre UV data,but don't have to malloc
    for yuv420, u32YUBBuffSize3 is not use

    for yuv422, u32YUVBuffSize store Y,U,V data
    for yuv422, u32YUVBuffSize2 is not use
    for yuv422, u32YUBBuffSize3 is not use
 */

typedef struct HT_VENC_Res_s{  //resolution
    HT_U32 u32Width;
    HT_U32 u32Heigh;
}HT_VENC_Res_t;

typedef struct HT_VENC_Vbr_s{ //RateControl VBR
    HT_S32 u32Gop;
    HT_S32 u32MaxBitRate;
    HT_S32 u32MaxQp;
    HT_S32 u32MinQp;
    HT_S32 u32SrcFrmRat;
    HT_S32 u32StatTime ;
}HT_VENC_Vbr_t;

typedef struct HT_VENC_Cbr_s{//RateControl CBR
    HT_S32 u32BitRate;
    HT_S32 u32FluctuateLevel;
    HT_S32 u32Gop;
    HT_S32 u32SrcFrmRate;
    HT_S32 u32StatTime;
}HT_VENC_Cbr_t;

typedef struct HT_VENC_FixQp_s{//RateControl FixQp
    HT_U32	u32Gop;
    HT_U32	u32SrcFrmRate;
    HT_U32	u32IQp;
    HT_U32	u32PQp;
}HT_VENC_FixQp_t;



typedef struct HT_VENC_Roi_s{//ROI
    MS_U32 u32Index;
    MS_BOOL bEnable;
    MS_BOOL bAbsQp;
    MS_U32 u32X;
    MS_U32 u32Y;
    MS_U32 u32W;
    MS_U32 u32H;
    MS_S32 s32SrcFrmRate;
    MS_S32 s32DstFrmRate;;
    MS_U8 *pDaQpMap;
}HT_VENC_Roi_t;

typedef struct HT_VENC_Split_s //MulitSlice
{
    MS_BOOL bSplitEnable;
    MS_U32 u32SliceRowCount;
} HT_VENC_Split_t;

/*
typedef enum
{
    E_MHAL_VENC_FMT_NV12 = 0,   //!< pixel format NV12.
    E_MHAL_VENC_FMT_NV21,       //!< pixel format NV21.
    E_MHAL_VENC_FMT_YUYV,       //!< pixel format YUYV.
    E_MHAL_VENC_FMT_YVYU,       //!< pixel format YVYU.
} VEncInputFmt_e;
*/

typedef struct HT_VENC_TaskInfo_s{
    HT_U32 u32YUVBuffSize1;       //Input Y size
    HT_U32 u32YUVBuffSize2;      //Input UV size
    HT_U32 u32YUVBuffSize3;      //for yuv420, is not use
    HT_U32 u32ReserveOutBuffSize; //data size after encoder
    MHAL_VENC_INST_HANDLE hInstance;
    MHAL_VENC_DEV_HANDLE hDevice;
    struct file *pstOutputFile;
    HT_S8 *ps8InputFileName;
    HT_VENC_InterBuff_t stInterBuff;
    HT_S32 s32InputFileFrameCount;
    HT_S32 s32OutputFileFrameCount;
    HT_S32 s32OutputFileFrameMax;
    HT_S32 s32InputFileFrameMax;
    HT_S64 s64InputOffset;
    HT_S64 s64OutputOffset;
    HT_S32 EncodeType; //h265, 264, jpeg
    HT_S32 RateControlType; //Rate Control
    HT_S32 ROIType;
    HT_S32 MultType;
    HT_S32 ResolutionType;
    HT_S32 FormatType;// reference VEncInputFmt_e in mhal_venc.h
    HT_S32 ID;
    HT_VENC_Res_t stRes; //resolution height width
    struct list_head list;
}HT_VENC_TaskInfo_t;

#if 1

MHalVencDrv_t stVencDriver = {
.CreateDevice =    MHAL_VENC_MyCreateDevice,
.DestroyDevice =   MHAL_VENC_MyDestroyDevice,
.GetDevConfig =    MHAL_VENC_MyGetDevConfig,
.CreateInstance =  MHAL_VENC_MyCreateInstance,
.DestroyInstance = MHAL_VENC_MyDestroyInstance,
.SetParam =        MHAL_VENC_MySetParam,
.GetParam =        MHAL_VENC_MyGetParam,
.EncodeOneFrame =  MHAL_VENC_MyEncodeOneFrame,
.EncodeDone =      MHAL_VENC_MyEncDone,
.QueryBufSize =    MHAL_VENC_MyQueryBufSize
};

#else

MHalVencDrv_t stVencDriver = {
.CreateDevice =    MHAL_MFE_CreateDevice,
.DestroyDevice =   MHAL_MFE_DestroyDevice,
.GetDevConfig =    MHAL_MFE_GetDevConfig,
.CreateInstance =  MHAL_MFE_CreateInstance,
.DestroyInstance = MHAL_MFE_DestroyInstance,
.SetParam =        MHAL_MFE_SetParam,
.GetParam =        MHAL_MFE_GetParam,
.EncodeOneFrame =  MHAL_MFE_EncodeOneFrame,
.EncodeDone =      MHAL_MFE_EncodeFrameDone,
.QueryBufSize =    MHAL_MFE_QueryBufSize
};

#endif




                                  //The number range of arguments
                                  //Encode Type,  RateControl,    ROI,  MultiSlice,  Resolution
static HT_U16 _s16arCheckTab[5][2] = { {0, 3},      {0, 3},     {0, 3},   {0, 1},     {0, 4} };

/***********************************/
//Resolution Start
static HT_VENC_Res_t _stResTab[5] =  {
// width heigh
{3840,   2160},
{1920,   1088},  //byte alignment
{1280,   720},
{720,    640},
{640,    480}
};
//Resolution End
/***********************************/


/**************************************/
//RateControl Start
static HT_VENC_Vbr_t _stVbrTab[5] = { // VBR
//Gop    MaxBitRate   MaxQp   MinQp      SrcFrmRate     StartTime
{  50,     4096,       20,      5,           25,              3}, //3840 2160
{  50,     4096,       20,      5,           25,              3}, //1920 1088
{  50,     4096,       20,      5,           25,              3}, //1280 720
{  50,     4096,       20,      5,           25,              3}, //720 480
{  50,     4096,       20,      5,           25,              3}  //640 480
};

static HT_VENC_Cbr_t _stCbrTab[5] = {  // CBR
// BitRate  FluctuateLevel  Gop  SrcFrmRate  StatTime
{   4096,        1 ,         50 ,    25 ,       1},  //3840 2160
{   4096,        1 ,         50 ,    25 ,       1},  //1920 1088
{   4096,        1 ,         50 ,    25 ,       1},  //1280 720
{   4096,        1 ,         50 ,    25 ,       1},  //720 480
{   4096,        1 ,         50 ,    25 ,       1}   //640 480
};

static HT_VENC_FixQp_t _stFixQpTab[5] = {
 // FixQp
// Gop     SrcFrmRate  IQp      PQp
{  30,        30,       25,      25},
{  30,        30,       25,      25},
{  30,        30,       25,      25},
{  30,        30,       25,      25},
{  30,        30,       25,      25}
};

//RateControl End
/**********************************************/


static HT_VENC_Roi_t _stRoiBGTab[4] = {  //ROI_BG
{0, 0, 0, 0, 0, 0, 0, 0, 0, NULL},  //0-Disable
 //Index   EnableAbsQp   AbsQp  X       Y       W         H     SrcFrmRate  DstFrmRate   DaQpMap
{   1,      TRUE,    	  5,  16*10,  16*10, 16*200,    16*200,   25,         25,         NULL}, //1-AbsQp+ROI_BG
{   1,      FALSE,    	  5,  16*10,  16*10, 16*200,    16*200,   25,         25,         NULL}, //2-ROI_BG+Qp_Map
{   1,      TRUE,         5,  16*0,   16*0,  16*0,      16*0,     25,         25,         NULL}  //3-AbsQp+Qp_Map
};


static HT_VENC_Split_t _stSplitTab[2] = {
// Enable RowCount
{   FALSE,   0},    //1-Disable
{   TRUE,    2048}  //2-Enable
};


static HT_S8 *_ps8InputYUV420FileName[5] = {
    "/mnt/YUVFILE/YUV420SP_3840_2160.yuv",  //not have the file now
    "/mnt/YUVFILE/YUV420SP_1920_1088.yuv",
    "/mnt/YUVFILE/YUV420SP_1280_720.yuv",
    "/mnt/YUVFILE/YUV420SP_720_480.yuv",
    "/mnt/YUVFILE/YUV420SP_640_480.yuv"
};

static HT_S8 *_ps8InputYUV422FileName[5] = {
    "/mnt/YUVFILE/YUV422_YUYV3840_2160.yuv",  //not have the file now
    "/mnt/YUVFILE/YUV422_YUYV1920_1088.yuv",
    "/mnt/YUVFILE/YUV422_YUYV1920_1088.yuv",
    "/mnt/YUVFILE/YUV422_YUYV720_480.yuv",
    "/mnt/YUVFILE/YUV422_YUYV640_480.yuv "
};

#ifndef HT_WRITE_DDR
/************************************************************/
//use to create output file name
static HT_S8 *_ps8EncodeString[4] = { //Encoder Type
"H265_",
"H264_",
"JPEG420_",
"JPEG422_"
};

static HT_S8 *_ps8RcString[4] = { //RateControl
"", //Disable
"CBR_",
"VBR_",
"FixQp_"
};

static HT_S8 *_ps8RoiString[4] = { //ROI
"",  //Disable
"AbsQp+RoiBG_",
"RoiBG+QpMap_",
"AbsQp+QpMap_"
};

static HT_S8 *_ps8MultString[2] = { //MultiSlice
"", //Disable
"MultiSlice_"
};

static HT_S8 *_ps8ResString[5] = {
"3480x2160",
"1920x1088",
"1280x720",
"720x480",
"640x480"
};
//End
/************************************************************/
#endif


/************************************************************/
//use to set encode buff size
//reference resolution

//for yuv 420, BuffSize1 Size: width * height * 1.5
//for yuv 420, BuffSize2 Size: 0
//for yuv 420, BuffSize3 Size: 0

static HT_U32 _u32YUVH26X420BuffSize1[5] =  {
    12441600,  //3840 * 2160 * 1.5
    3133440,   //1920 * 1088 * 1.5
    1382400,   //1280 * 720  * 1.5
    518400,    //720  * 480  * 1.5
    460800     //640  * 480  * 1.5
};

static HT_U32 _u32YUVH26X420BuffSize2[5] = {0, 0, 0, 0, 0};
static HT_U32 _u32YUVH26X420BuffSize3[5] = {0, 0, 0, 0, 0};


static HT_U32 _u32YUVJPEG420BuffSize1[5] =  {
    12441600,  //3840 * 2160 * 1.5
    3133440,   //1920 * 1088 * 1.5
    1382400,   //1280 * 720  * 1.5
    518400,    //720  * 480  * 1.5
    460800     //640  * 480  * 1.5
};

static HT_U32 _u32YUVJPEG420BuffSize2[5] = {0, 0, 0, 0, 0};
static HT_U32 _u32YUVJPEG420BuffSize3[5] = {0 ,0, 0, 0, 0};


//for yuv 422, BuffSize1 Size: width * height * 2
//for yuv 422, BuffSize2 Size: 0
//for yuv 422, BuffSize3 Size: 0
static HT_U32 _u32YUVJPEG422BuffSize1[5] =  {
    16588800,  //3840 * 2160 * 2
    4177920,   //1920 * 1088 * 2
    1843200,   //1280 * 720  * 2
    691200,    //720  * 480  * 2
    614400     //640  * 480  * 2
};

static HT_U32 _u32YUVJPEG422BuffSize2[5] = {0 ,0, 0, 0, 0};
static HT_U32 _u32YUVJPEG422BuffSize3[5] = {0 ,0, 0, 0, 0};



//the output size is not sure
static HT_U32 _u32ReserveOutBuffSize[5] = {
   1024 * 1024 * 2,    //2097152(2M)
   1024 * 1024 * 2,
   1024 * 1024 * 2,
   1024 * 1024 * 2,
   1024 * 1024 * 2
};
//End
/************************************************************/


static HT_RESULT _HT_VENC_CheckArgCount(HT_U8 s32ArgNumber);
static HT_RESULT _HT_VENC_CheckArg(HT_U16 *pu16Cmd,HT_S32 s32TaskCnt, HT_U16 puCheckTab[5][2]);
static HT_S32 _HT_VENC_GetTaskCount(HT_U8 s32ArgNumber);

static HT_RESULT _HT_VENC_SetRateControl(HT_VENC_TaskInfo_t stTaskInfo);
static HT_RESULT _HT_VENC_SetROI(HT_VENC_TaskInfo_t stTaskInfo);
static HT_RESULT _HT_VENC_SetMultiSlice(HT_VENC_TaskInfo_t stTaskInfo);
static HT_RESULT _HT_VENC_SetResolution(HT_VENC_TaskInfo_t *stTaskInfo);
static HT_RESULT _HT_VENC_SetInputFileName(HT_VENC_TaskInfo_t *pstTaskInfo);
static HT_RESULT _HT_VENC_SetInterBuff(HT_VENC_TaskInfo_t *pstTaskInfo);
static HT_RESULT _HT_VENC_SetEncodeBuff(HT_VENC_TaskInfo_t *pstTaskInfo);
#ifndef HT_WRITE_DDR
static HT_RESULT _HT_VENC_SetOutputFilePoiter(HT_VENC_TaskInfo_t *pstTaskInfo);
#endif
static HT_RESULT _HT_VENC_OpenFile(struct file **pstInputFile, HT_S8 *ps8FileName);
static HT_RESULT _HT_VENC_CloseFile(struct file **pstFile);
static HT_RESULT _HT_VENC_ReadBuff(HT_VB_Info_t *pstVB, struct file *pstFile,HT_U32 u32Size,  HT_S64 s64Offset);
#ifndef HT_WRITE_DDR
static HT_RESULT _HT_VENC_Write_FILEBuff(struct file *pstFile, HT_VB_Info_t *pstVB, HT_U32 u32Size,  HT_S64 s64Offset);
#endif

static HT_RESULT _HT_VENC_Malloc(HT_VB_Info_t *pstVB, HT_U32 u32Size);
static HT_RESULT _HT_VENC_Free(HT_VB_Info_t *pstVB);

static HT_RESULT _HT_VENC_SetParam(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_Deinit(MHAL_VENC_DEV_HANDLE phDevs[], HT_S32 s32hDevMax, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_Init(MHAL_VENC_DEV_HANDLE *phDev, HT_S32 s32phCnt, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_CreateDevice(VOID *pOsDev, VOID** ppBase, HT_S32 *pSize, MHAL_VENC_DEV_HANDLE *phDev, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_DestroyDevice(MHAL_VENC_DEV_HANDLE *hDevice, HT_S32 s32hDevCnt, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_CreateInstance(MHAL_VENC_DEV_HANDLE hDevice, MHAL_VENC_INST_HANDLE *phInstance, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_DestroyInstance(MHAL_VENC_INST_HANDLE hInstance, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_EncodeOneFrame(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_InOutBuf_t* pInOutBuf, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_EncDone(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_EncResult_t* pEncRet, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_QueryBufSize(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_InternalBuf_t *pstSize, MHalVencDrv_t stVencOpt);
static HT_RESULT _HT_VENC_ReleaseInstHandle(MHAL_VENC_INST_HANDLE phIns, MHalVencDrv_t stVencOpt);


static void HT_VENC_PrtTaskInof(HT_VENC_TaskInfo_t stTaskInfo);




static LIST_HEAD(HT_VENC_list); //list head

static HT_RESULT _HT_VENC_NumberRange(HT_S32 s32Min, HT_S32 s32Max, HT_S32 s32Number)
{
    if ((s32Number < s32Min) || (s32Number > s32Max))
    {
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}


HT_RESULT HT_VENC(HT_U16 *pu16Cmd,HT_U8 u8CmdCnt)
{
    HT_S32 s32Ret = 0;
    HT_S32 s32TaskCnt = 0;
    HT_S32 i = 0;
    struct list_head *pos = NULL;
    struct list_head *tmp = NULL;
    MHAL_VENC_DEV_HANDLE phDevs[4];
    #ifdef HT_WRITE_DDR
    HT_VB_Info_t stWriteDDRBuff;
    HT_U32 s32WriteOffset = 0;
    #endif

    s32TaskCnt = _HT_VENC_GetTaskCount(u8CmdCnt);
    if (HT_FAILURE == s32TaskCnt)
    {
        return HT_FAILURE;
    }

    s32Ret = _HT_VENC_CheckArg(pu16Cmd, s32TaskCnt, _s16arCheckTab);
    if (s32Ret != HT_SUCCESS)
    {
        return HT_FAILURE;
    }

    for (i = 0; i < u8CmdCnt; i ++)
    {
        printk("%d:%d\n", i , pu16Cmd[i]);
    }

    //Create 4 DevHandles
    //phDevs[0]  H265_0
    //phDevs[1]  H265_1
    //phDevs[2]  H264
    //phDevs[3]  JPEG

    _HT_VENC_Init(phDevs, 4, stVencDriver);

    #ifdef HT_WRITE_DDR
    stWriteDDRBuff.u32Size = 1024 * 1024 * 30; //30M
    s32Ret = HT_Malloc(&stWriteDDRBuff);
    if (s32Ret != 0)

    {
        HT_VENC_PRT_ERR("malloc failed");
        return HT_FAILURE;
    }
    #endif

    //set all arguments
    for (i = 0; i < s32TaskCnt; i++)
    {
        HT_VENC_TaskInfo_t *pstTaskInfo = NULL;
        pstTaskInfo = (HT_VENC_TaskInfo_t *)kmalloc(sizeof(HT_VENC_TaskInfo_t), GFP_KERNEL);
        memset(pstTaskInfo, 0, sizeof(HT_VENC_TaskInfo_t));

        //pu16Cmd[0] EncodeType   0~2
        //pu16Cmd[1] RateControl  0~3
        //pu16Cmd[2] ROI          0~3
        //pu16Cmd[3] MultiSlice   0~1
        //pu16Cmd[4] Resolution   0~4

        pstTaskInfo->EncodeType =       pu16Cmd[HT_VENC_ENCODE_TYPE + i * 5];
        pstTaskInfo->RateControlType =  pu16Cmd[HT_VENC_RATE_CONTROL_TYPE  + i * 5];
        pstTaskInfo->ROIType =          pu16Cmd[HT_VENC_ROI_TYPE + i * 5];
        pstTaskInfo->MultType =         pu16Cmd[HT_VENC_MULTISLICE_TYPE + i * 5];
        pstTaskInfo->ResolutionType =   pu16Cmd[HT_VENC_RESOLUTION_TYPE + i * 5];
        pstTaskInfo->ID = i;
        pstTaskInfo->s64InputOffset = 0;
        pstTaskInfo->s64OutputOffset = 0;
        pstTaskInfo->s32InputFileFrameCount = 0;
        pstTaskInfo->s32OutputFileFrameCount = 0;
        pstTaskInfo->s32InputFileFrameMax = HT_VENC_INPUT_FILE_FRAME_MAX;
        pstTaskInfo->s32OutputFileFrameMax = HT_VENC_OUTPUT_FILE_FRAME_MAX;
        pstTaskInfo->FormatType = E_MHAL_VENC_FMT_NV12;// for h264, h265

        //if encode type is jpeg, the FormatType should change
        if (HT_VENC_JPEG_YUV420 == pu16Cmd[HT_VENC_ENCODE_TYPE])
        {
            pstTaskInfo->FormatType = 0;// the type is not sure, fill 0 to pass makefile
        }

        if (HT_VENC_JPEG_YUV422 == pu16Cmd[HT_VENC_ENCODE_TYPE])
        {
            pstTaskInfo->FormatType = E_MHAL_VENC_FMT_YUYV;
        }

        HT_VENC_PRT_DONE("pstTaskInfo set value");

        _HT_VENC_SetEncodeBuff(pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetEncodeBuff");

         //phDevs[0]->h265_0  phDevs[1]->h265_1, phDevs[2]->h264, phDevs[3]->jpg
        _HT_VENC_CreateInstance(phDevs[pstTaskInfo->EncodeType], &pstTaskInfo->hInstance, stVencDriver);
        HT_VENC_PRT_DONE("_HT_VENC_CreateInstance");

        //pstTaskInfo->stRes = _u32arResTab[pu16Cmd[HT_VENC_RESOLUTION] -1];
        _HT_VENC_SetRateControl(*pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetRateControl(*pstTaskInfo)");

        _HT_VENC_SetResolution(pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetResolution(*pstTaskInfo)");

        _HT_VENC_SetROI(*pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetROI(*pstTaskInfo)");

        _HT_VENC_SetMultiSlice(*pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetMultiSlice(*pstTaskInfo)");

        _HT_VENC_SetInterBuff(pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetInterBuff(pstTaskInfo)");

        _HT_VENC_SetInputFileName(pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetInputFileName(pstTaskInfo)");

        #ifndef HT_WRITE_DDR
        _HT_VENC_SetOutputFilePoiter(pstTaskInfo);
        HT_VENC_PRT_DONE("_HT_VENC_SetOutputFilePoiter(pstTaskInfo)");
        #endif

        list_add_tail(&pstTaskInfo->list, &HT_VENC_list);
        HT_VENC_PRT_DONE("list_add_tail");

    }

    printk("s32TaskCnt = %d\n", s32TaskCnt);
    while(s32TaskCnt)
    {
        list_for_each_safe(pos, tmp, &HT_VENC_list)
        {
            struct file *pstInputFile = NULL;
            HT_VENC_TaskInfo_t *pstTaskTmp = NULL;
            HT_VB_Info_t stInputYUVBuff1;
            HT_VB_Info_t stInputYUVBuff2;
            HT_VB_Info_t stInputYUVBuff3;
            HT_VB_Info_t stOutputBuff;
            MHAL_VENC_InOutBuf_t stEncBuff;
            MHAL_VENC_EncResult_t stVencResult;
            HT_S32 s32Ret = 0;

            HT_VENC_MEMSET_STRUCT(HT_VB_Info_t, stInputYUVBuff1);
            HT_VENC_MEMSET_STRUCT(HT_VB_Info_t, stInputYUVBuff2);
            HT_VENC_MEMSET_STRUCT(HT_VB_Info_t, stInputYUVBuff3);
            HT_VENC_MEMSET_STRUCT(HT_VB_Info_t, stOutputBuff);
            HT_VENC_MEMSET_STRUCT(MHAL_VENC_InOutBuf_t, stEncBuff);
            HT_VENC_MEMSET_STRUCT(MHAL_VENC_EncResult_t, stVencResult);

            HT_VENC_PRT_DONE("struct memset");

            pstTaskTmp = list_entry(pos, HT_VENC_TaskInfo_t, list);

            _HT_VENC_OpenFile(&pstInputFile, pstTaskTmp->ps8InputFileName);

            _HT_VENC_Malloc(&stInputYUVBuff1, pstTaskTmp->u32YUVBuffSize1);
            _HT_VENC_Malloc(&stInputYUVBuff2, pstTaskTmp->u32YUVBuffSize2);
            _HT_VENC_Malloc(&stInputYUVBuff3, pstTaskTmp->u32YUVBuffSize3);
            _HT_VENC_Malloc(&stOutputBuff, pstTaskTmp->u32ReserveOutBuffSize); //there have a problem

            if (NULL == pstInputFile)
            {
                printk("pstInputFile is NULL\n");
                return HT_FAILURE;
            }
            HT_VENC_PrtTaskInof(*pstTaskTmp);

            //for yuv 420 read Y data, for yuv 422 read Y U V data
            _HT_VENC_ReadBuff(&stInputYUVBuff1, pstInputFile, pstTaskTmp->u32YUVBuffSize1, pstTaskTmp->s64InputOffset);
            pstTaskTmp->s64InputOffset += pstTaskTmp->u32YUVBuffSize1;
            HT_VENC_PRT_DONE("_HT_VENC_ReadBuff(&stInputYUVBuff, pstInputFile, pstTaskTmp->u32YUVBuffSize, pstTaskTmp->s64InputOffset)");

            //for yuv 420 read UV data, for yuv 422 is not use
            _HT_VENC_ReadBuff(&stInputYUVBuff2, pstInputFile, pstTaskTmp->u32YUVBuffSize2, pstTaskTmp->s64InputOffset);
            pstTaskTmp->s64InputOffset += pstTaskTmp->u32YUVBuffSize2;
            HT_VENC_PRT_DONE("_HT_VENC_ReadBuff(&stInputYUVBuff2, pstInputFile, pstTaskTmp->u32YUVBuffSize2, pstTaskTmp->s64InputOffset)");

            //for yuv 420 and 422, the data is not use
            _HT_VENC_ReadBuff(&stInputYUVBuff3, pstInputFile, pstTaskTmp->u32YUVBuffSize3, pstTaskTmp->s64InputOffset);
            pstTaskTmp->s64InputOffset += pstTaskTmp->u32YUVBuffSize3;


            stEncBuff.phyInputYUVBuf1 = stInputYUVBuff1.phyAddr;
            stEncBuff.u32InputYUVBuf1Size = pstTaskTmp->u32YUVBuffSize1;

            stEncBuff.phyInputYUVBuf2 = stInputYUVBuff2.phyAddr;
            stEncBuff.u32InputYUVBuf2Size = pstTaskTmp->u32YUVBuffSize2;

            stEncBuff.phyInputYUVBuf3 = stInputYUVBuff3.phyAddr;
            stEncBuff.u32InputYUVBuf3Size = pstTaskTmp->u32YUVBuffSize3;

            stEncBuff.phyOutputBuf = stOutputBuff.phyAddr;
            stEncBuff.u32OutputBufSize = pstTaskTmp->u32ReserveOutBuffSize;
            stEncBuff.pCmdQ = NULL;
            HT_VENC_PRT_DONE("Set stEncBuff");

            s32Ret = _HT_VENC_EncodeOneFrame(pstTaskTmp->hInstance, &stEncBuff, stVencDriver);
            if (s32Ret != HT_SUCCESS)
            {
                HT_VENC_ERR_INFO;
            }
            HT_VENC_PRT_DONE("_HT_VENC_EncodeOneFrame(pstTaskTmp->hInstance, &stEncBuff)");

            /*************
            must wait Encode Done!!!
            just sleeping for some time
            **************/
            msleep(HT_VENC_MSLEEP_TIME);

            // get Encode buff size
            s32Ret = _HT_VENC_EncDone(pstTaskTmp->hInstance, &stVencResult, stVencDriver);
            if (s32Ret != HT_SUCCESS)
            {
                HT_VENC_ERR_INFO;
            }

            //if ReserveSize is too small, must make it larger
            if (stVencResult.u32OutputBufUsed > pstTaskTmp->u32ReserveOutBuffSize)
            {
                HT_VENC_PRT_ERR("ReserveOutBuffSize is too small");
            }

            #ifndef HT_WRITE_DDR
            _HT_VENC_Write_FILEBuff(pstTaskTmp->pstOutputFile, &stOutputBuff, stVencResult.u32OutputBufUsed, pstTaskTmp->s64OutputOffset);
            pstTaskTmp->s64OutputOffset += stVencResult.u32OutputBufUsed;
            HT_VENC_PRT_DONE(" _HT_VENC_Write_FILEBuff(pstTaskTmp->pstOutputFile, &stOutputBuff, stVencResult.OutputBufUsed, pstTaskTmp->s64OutputOffset)");
            #endif

            printk("pstTaskTmp->s32OutputFileFrameCount = %d\n", pstTaskTmp->s32OutputFileFrameCount);
            //printk("pstTaskTmp->s32InputFileFrameCount = %d\n", pstTaskTmp->s32InputFileFrameCount);

            pstTaskTmp->s32OutputFileFrameCount++;
            pstTaskTmp->s32InputFileFrameCount++;

            HT_VENC_PrtTaskInof(*pstTaskTmp);

             //free buff

            _HT_VENC_Free(&stInputYUVBuff1);
            _HT_VENC_Free(&stInputYUVBuff2);
            _HT_VENC_Free(&stInputYUVBuff3);

            #ifdef HT_WRITE_DDR
            memcpy(stWriteDDRBuff.pu8VirtAddr + s32WriteOffset, stOutputBuff.pu8VirtAddr, stVencResult.u32OutputBufUsed);
            s32WriteOffset += stVencResult.u32OutputBufUsed;
            printk("stWriteDDRBuff VirAddr = %p\n", stWriteDDRBuff.pu8VirtAddr);
            #endif

            _HT_VENC_Free(&stOutputBuff);
            HT_VENC_PRT_DONE("_HT_VENC_Free");

            _HT_VENC_CloseFile(&pstInputFile);
            HT_VENC_PRT_DONE(" _HT_VENC_CloseFile(&pstInputFile)");

            if (pstTaskTmp->s32InputFileFrameCount >= pstTaskTmp->s32InputFileFrameMax)
            {
                pstTaskTmp->s64InputOffset = 0;
                pstTaskTmp->s32InputFileFrameCount = 0;
            }

            if (pstTaskTmp->s32OutputFileFrameCount >= pstTaskTmp->s32OutputFileFrameMax)
            {
                list_del(pos);
                #ifndef HT_WRITE_DDR
                _HT_VENC_CloseFile(&pstTaskTmp->pstOutputFile);
                #endif
                _HT_VENC_ReleaseInstHandle(pstTaskTmp->hInstance, stVencDriver);
                _HT_VENC_DestroyInstance(pstTaskTmp->hInstance, stVencDriver);
                _HT_VENC_Free(&pstTaskTmp->stInterBuff.stIntrAl);
                _HT_VENC_Free(&pstTaskTmp->stInterBuff.stIntrRef);
                HT_VENC_PRT_DONE("_HT_VENC_Free");
                kfree(pstTaskTmp);
                s32TaskCnt--;
            }
        }
    }

    _HT_VENC_Deinit(phDevs, 4, stVencDriver);

    return HT_SUCCESS;
}

//Create 4 DeviceHandles
static HT_RESULT _HT_VENC_Init(MHAL_VENC_DEV_HANDLE *phDev, HT_S32 s32phCnt, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    //The input data is not sure, fill NULL to pass makefile
    s32Ret = _HT_VENC_CreateDevice(NULL, NULL, NULL, &phDev[HT_VENC_DEVICE_HANDLE_H265_0], stVencOpt);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    s32Ret = _HT_VENC_CreateDevice(NULL, NULL, NULL, &phDev[HT_VENC_DEVICE_HANDLE_H265_1], stVencOpt);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    s32Ret = _HT_VENC_CreateDevice(NULL, NULL, NULL, &phDev[HT_VENC_DEVICE_HANDLE_H264], stVencOpt);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    s32Ret = _HT_VENC_CreateDevice(NULL, NULL, NULL, &phDev[HT_VENC_DEVICE_HANDLE_JPEG], stVencOpt);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_Deinit(MHAL_VENC_DEV_HANDLE phDevs[], HT_S32 s32hDevMax, MHalVencDrv_t stVencOpt)
{
    _HT_VENC_DestroyDevice(phDevs, s32hDevMax, stVencOpt);

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_CreateDevice(VOID *pOsDev, VOID** ppBase, HT_S32 *pSize, MHAL_VENC_DEV_HANDLE *phDev, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.CreateDevice(pOsDev,  ppBase, pSize, phDev);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_DestroyDevice(MHAL_VENC_DEV_HANDLE *phDev, HT_S32 s32hDevCnt, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;
    int i = 0;

    for (i = 0; i < s32hDevCnt; i ++)
    {
        s32Ret = stVencOpt.DestroyDevice(phDev[i]);
        if (s32Ret != HT_SUCCESS)
        {
            HT_VENC_ERR_INFO;
            return HT_FAILURE;
        }
    }
    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_CreateInstance(MHAL_VENC_DEV_HANDLE hDevice, MHAL_VENC_INST_HANDLE *phInstance, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.CreateInstance(hDevice, phInstance);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_DestroyInstance(MHAL_VENC_INST_HANDLE hInstance, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.DestroyInstance(hInstance);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_SetParam(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_IDX type, MHAL_VENC_Param_t* param,  MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret =  stVencOpt.SetParam(hInstance, type, param);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_EncodeOneFrame(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_InOutBuf_t* pInOutBuf, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.EncodeOneFrame(hInstance, pInOutBuf);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_EncDone(MHAL_VENC_INST_HANDLE hInstance,MHAL_VENC_EncResult_t* pEncRet, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.EncodeDone(hInstance, pEncRet);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}


void HT_VENC_DisplayHelp(void)
{
    printk("argc:[0], Encoder Type: 0-H265, 1-H264, 2-JPG(420), 3-JPG(422)\n");

    printk("argc:[1], RateControl: 0-Disable, 1-CBR, 2-VBR, 3-FixQp(not support)\n");

    printk("argc:[2], ROI: 0-Disable, 1-AbsQp+ROI_BG, 2-ROI_BG+Qp_Map, 3-AbsQp+Qp_Map\n");

    printk("argc:[3], MultiSlice: 0-Disable, 1-Enable\n");

    printk("argc:[4], Resolution: 0-3840x2160, 1-1920x1088, 2-1280x1088, 3-720x480, 4-640x480\n");

    printk("if Encoder Type is 2-JPG, only Resolution work\n");
    printk("eg: echo venc 1 1 1 1 1 > /proc/hal/uttest\n");
}


static HT_RESULT _HT_VENC_OpenFile(struct file **pstInputFile, HT_S8 *ps8FileName)
{
    *pstInputFile = filp_open(ps8FileName, O_RDWR, 0644);
    if(IS_ERR(*pstInputFile))
    {
        printk("open %s failed\n", ps8FileName);
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_CloseFile(struct file **pstFile)
{
    if (NULL != *pstFile)
    {
        filp_close(*pstFile, NULL);
        *pstFile = NULL;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_ReadBuff(HT_VB_Info_t *pstVB, struct file *pstFile,HT_U32 u32Size,  HT_S64 s64Offset)
{
    HT_S32 s32Ret = 0;

    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    s32Ret = pstFile->f_op->read(pstFile, pstVB->pu8VirtAddr, u32Size, &s64Offset);
    if (s32Ret < 0)
    {
        HT_VENC_PRT_ERR("read failed");
        set_fs(old_fs);
        return HT_FAILURE;
    }

    set_fs(old_fs);

    return HT_SUCCESS;
}

#ifndef HT_WRITE_DDR
static HT_RESULT _HT_VENC_Write_FILEBuff(struct file *pstFile, HT_VB_Info_t *pstVB, HT_U32 u32Size,  HT_S64 s64Offset)
{
    HT_S32 s32Ret = 0;

    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    //s32Ret = pstFile->f_op->write(pstFile, pstVB->pu8VirtAddr, u32Size, &s64Offset);
    //printk("buff = %s\n", pstVB->pu8VirtAddr);
    //printk("u32Size = %d\n", u32Size);
    vfs_write(pstFile, pstVB->pu8VirtAddr, u32Size, &s64Offset);
    if (s32Ret < 0)
    {
        HT_VENC_PRT_ERR("write failed");
        set_fs(old_fs);
        return HT_FAILURE;
    }

    set_fs(old_fs);

    return HT_SUCCESS;
}
#endif

static HT_RESULT _HT_VENC_CheckArgCount(HT_U8 s32ArgNumber)
{
    if (0 != (s32ArgNumber % 5))
    {
        HT_VENC_PRT_ERR("Arg Count error");
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_S32 _HT_VENC_GetTaskCount(HT_U8 s32ArgNumber)
{
    HT_S32 s32TaskCnt = 0;
    HT_S32 s32Ret  = 0;

    s32Ret = _HT_VENC_CheckArgCount(s32ArgNumber);
    if (s32Ret != HT_SUCCESS)
    {
        return HT_FAILURE;
    }

    s32TaskCnt = s32ArgNumber / 5;

    return s32TaskCnt;
}


static HT_RESULT _HT_VENC_CheckArg(HT_U16 *pu16Cmd,HT_S32 s32TaskCnt, HT_U16 puCheckTab[5][2])
{
    HT_S32 s32Ret = 0;
    HT_S32 i = 0;
    HT_S32 k = 0;

     while(i < s32TaskCnt)
     {
        s32Ret = _HT_VENC_NumberRange(puCheckTab[k][0], puCheckTab[k][1], pu16Cmd[i]);
        if (s32Ret != HT_SUCCESS)
        {
            HT_VENC_PRT_ERR("out range");
            return HT_FAILURE;
        }
        k++;
        i++;

        if(5 == k)
        {
            k = 0;
        }
     }

    //_HT_VENC_NumberRange(puCheckTab[0][0], puCheckTab[0][1], pu16Cmd[0])
    //_HT_VENC_NumberRange(puCheckTab[1][0], puCheckTab[1][1], pu16Cmd[1])
    //_HT_VENC_NumberRange(puCheckTab[2][0], puCheckTab[2][1], pu16Cmd[2])

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_SetResolution(HT_VENC_TaskInfo_t *stTaskInfo)
{
    MHAL_VENC_IDX eIdxRes;
    MHAL_VENC_Param_t *pstParamRes = NULL;
    MHAL_VENC_Resoluton_t stVencResolution;

    switch(stTaskInfo->EncodeType)
    {
        case HT_VENC_H265:
            eIdxRes = E_MHAL_VENC_265_RESOLUTION;
            break;

        case HT_VENC_H264:
            break;
            eIdxRes = E_MHAL_VENC_264_RESOLUTION;

        case HT_VENC_JPEG_YUV420:
        case HT_VENC_JPEG_YUV422:
            eIdxRes = E_MHAL_VENC_JPEG_RESOLUTION;
            break;

        default:
            break;
    }

    pstParamRes = (MHAL_VENC_Param_t *)&(stVencResolution);
    MHAL_VENC_INIT_PARAM(MHAL_VENC_Resoluton_t, stVencResolution);

    if ( (HT_VENC_JPEG_YUV420 == stTaskInfo->EncodeType) || (HT_VENC_JPEG_YUV422 == stTaskInfo->EncodeType) )
    {
        stVencResolution.eFmt = stTaskInfo->FormatType;
    }
    else  //h264 h265
    {
        stVencResolution.eFmt = E_MHAL_VENC_FMT_NV12;
    }

    stVencResolution.u32Height = _stResTab[stTaskInfo->ResolutionType].u32Heigh;
    stVencResolution.u32Width =  _stResTab[stTaskInfo->ResolutionType].u32Width;

    stTaskInfo->stRes.u32Heigh = _stResTab[stTaskInfo->ResolutionType].u32Heigh;
    stTaskInfo->stRes.u32Width = _stResTab[stTaskInfo->ResolutionType].u32Width;

    _HT_VENC_SetParam(stTaskInfo->hInstance, eIdxRes, pstParamRes, stVencDriver);

    return HT_SUCCESS;
}

//jpeg not use rate control
static HT_RESULT _HT_VENC_SetRateControl(HT_VENC_TaskInfo_t stTaskInfo)
{
    MHAL_VENC_IDX eIdxRc;
    MHAL_VENC_Param_t *pstParamRc = NULL;
    MHAL_VENC_RcInfo_t stVENCRcInfo;

    switch(stTaskInfo.EncodeType)
    {
          case HT_VENC_H265:
            eIdxRc = E_MHAL_VENC_265_RC;
            break;

          case HT_VENC_H264:
            eIdxRc = E_MHAL_VENC_264_RC;
            break;

        default:
            break;
    }

    if (HT_VENC_H265 == stTaskInfo.EncodeType)
    {
        switch(stTaskInfo.RateControlType)
        {
            case HT_VENC_VBR:
                stVENCRcInfo.eRcMode = E_MHAL_VENC_RC_MODE_H265CBR;
                stVENCRcInfo.stAttrH265Vbr.u32Gop =          _stVbrTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH265Vbr.u32MaxBitRate =   _stVbrTab[stTaskInfo.ResolutionType].u32MaxBitRate;
                stVENCRcInfo.stAttrH265Vbr.u32MaxQp =        _stVbrTab[stTaskInfo.ResolutionType].u32MaxQp;
                stVENCRcInfo.stAttrH265Vbr.u32MinQp =        _stVbrTab[stTaskInfo.ResolutionType].u32MinQp;
                stVENCRcInfo.stAttrH265Vbr.u32SrcFrmRate =   _stVbrTab[stTaskInfo.ResolutionType].u32SrcFrmRat;
                stVENCRcInfo.stAttrH265Vbr.u32StatTime =     _stVbrTab[stTaskInfo.ResolutionType].u32StatTime;
                break;

            case HT_VENC_CBR:
                stVENCRcInfo.eRcMode = E_MHAL_VENC_RC_MODE_H265VBR;
                stVENCRcInfo.stAttrH265Cbr.u32BitRate =        _stCbrTab[stTaskInfo.ResolutionType].u32BitRate;
                stVENCRcInfo.stAttrH265Cbr.u32FluctuateLevel = _stCbrTab[stTaskInfo.ResolutionType].u32FluctuateLevel;
                stVENCRcInfo.stAttrH265Cbr.u32Gop =            _stCbrTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH265Cbr.u32SrcFrmRate =     _stCbrTab[stTaskInfo.ResolutionType].u32SrcFrmRate;
                stVENCRcInfo.stAttrH265Cbr.u32StatTime =       _stCbrTab[stTaskInfo.ResolutionType].u32StatTime ;
                break;
            case HT_VENC_FIXQP:
                stVENCRcInfo.stAttrH265FixQp.u32Gop =           _stFixQpTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH265FixQp.u32IQp =           _stFixQpTab[stTaskInfo.ResolutionType].u32IQp;
                stVENCRcInfo.stAttrH265FixQp.u32PQp =           _stFixQpTab[stTaskInfo.ResolutionType].u32PQp;
                stVENCRcInfo.stAttrH265FixQp.u32SrcFrmRate =    _stFixQpTab[stTaskInfo.ResolutionType].u32SrcFrmRate;

            default:
                break;
        }
    }


    if (HT_VENC_H264 == stTaskInfo.EncodeType)
    {
        switch(stTaskInfo.RateControlType)
        {
            case HT_VENC_VBR:
                stVENCRcInfo.eRcMode = E_MHAL_VENC_RC_MODE_H264CBR;
                stVENCRcInfo.stAttrH264Vbr.u32Gop =        _stVbrTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH264Vbr.u32MaxBitRate = _stVbrTab[stTaskInfo.ResolutionType].u32MaxBitRate;
                stVENCRcInfo.stAttrH264Vbr.u32MaxQp =      _stVbrTab[stTaskInfo.ResolutionType].u32MaxQp;
                stVENCRcInfo.stAttrH264Vbr.u32MinQp =      _stVbrTab[stTaskInfo.ResolutionType].u32MinQp;
                stVENCRcInfo.stAttrH264Vbr.u32SrcFrmRate = _stVbrTab[stTaskInfo.ResolutionType].u32SrcFrmRat;
                stVENCRcInfo.stAttrH264Vbr.u32StatTime =   _stVbrTab[stTaskInfo.ResolutionType].u32StatTime;
                break;

            case HT_VENC_CBR:
                stVENCRcInfo.eRcMode = E_MHAL_VENC_RC_MODE_H264VBR;
                stVENCRcInfo.stAttrH264Cbr.u32BitRate =        _stCbrTab[stTaskInfo.ResolutionType].u32BitRate;
                stVENCRcInfo.stAttrH264Cbr.u32FluctuateLevel = _stCbrTab[stTaskInfo.ResolutionType].u32FluctuateLevel;
                stVENCRcInfo.stAttrH264Cbr.u32Gop =            _stCbrTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH264Cbr.u32SrcFrmRate =     _stCbrTab[stTaskInfo.ResolutionType].u32SrcFrmRate;
                stVENCRcInfo.stAttrH264Cbr.u32StatTime =       _stCbrTab[stTaskInfo.ResolutionType].u32StatTime ;
                break;

             case HT_VENC_FIXQP:
                stVENCRcInfo.stAttrH264FixQp.u32Gop =           _stFixQpTab[stTaskInfo.ResolutionType].u32Gop;
                stVENCRcInfo.stAttrH264FixQp.u32IQp =           _stFixQpTab[stTaskInfo.ResolutionType].u32IQp;
                stVENCRcInfo.stAttrH264FixQp.u32PQp =           _stFixQpTab[stTaskInfo.ResolutionType].u32PQp;
                stVENCRcInfo.stAttrH264FixQp.u32SrcFrmRate =    _stFixQpTab[stTaskInfo.ResolutionType].u32SrcFrmRate;

            default:
                break;
        }
    }


    pstParamRc = (MHAL_VENC_Param_t*)&(stVENCRcInfo);
    MHAL_VENC_INIT_PARAM( MHAL_VENC_RcInfo_t, stVENCRcInfo);

    _HT_VENC_SetParam(stTaskInfo.hInstance, eIdxRc, pstParamRc, stVencDriver);

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_SetROI(HT_VENC_TaskInfo_t stTaskInfo)
{
    MHAL_VENC_IDX eIdxROI;
    MHAL_VENC_Param_t *pstParamROI = NULL;
    MHAL_VENC_RoiCfg_t stVENCRoiInfo;

    if (DISABLE == stTaskInfo.ROIType)
    {
        return HT_SUCCESS;
    }

    switch(stTaskInfo.EncodeType)
    {
    case HT_VENC_H265:
        eIdxROI = E_MHAL_VENC_265_ROI;
        break;

    case HT_VENC_H264:
        eIdxROI = E_MHAL_VENC_264_ROI;
        break;

    default:
        break;
    }

    pstParamROI = (MHAL_VENC_Param_t*)&(stVENCRoiInfo);
    MHAL_VENC_INIT_PARAM(MHAL_VENC_RoiCfg_t, stVENCRoiInfo);
    //the data is not sure, fill it to pass makefile
    stVENCRoiInfo.bAbsQp =   _stRoiBGTab[stTaskInfo.ROIType].bAbsQp;
    stVENCRoiInfo.bEnable =  _stRoiBGTab[stTaskInfo.ROIType].bEnable;
    stVENCRoiInfo.pDaQpMap = _stRoiBGTab[stTaskInfo.ROIType].pDaQpMap;
    stVENCRoiInfo.RoiBgCtl.s32DstFrmRate = _stRoiBGTab[stTaskInfo.ROIType].s32DstFrmRate;
    stVENCRoiInfo.RoiBgCtl.s32SrcFrmRate = _stRoiBGTab[stTaskInfo.ROIType].s32SrcFrmRate;
    stVENCRoiInfo.stRect.u32H = _stRoiBGTab[stTaskInfo.ROIType].u32H;
    stVENCRoiInfo.stRect.u32W = _stRoiBGTab[stTaskInfo.ROIType].u32W;
    stVENCRoiInfo.stRect.u32X = _stRoiBGTab[stTaskInfo.ROIType].u32X;
    stVENCRoiInfo.stRect.u32Y = _stRoiBGTab[stTaskInfo.ROIType].u32Y;
    stVENCRoiInfo.u32Index =    _stRoiBGTab[stTaskInfo.ROIType].u32Index;
    _HT_VENC_SetParam(stTaskInfo.hInstance, eIdxROI, pstParamROI, stVencDriver);

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_SetMultiSlice(HT_VENC_TaskInfo_t stTaskInfo)
{
    MHAL_VENC_IDX eIdxSpl;
    MHAL_VENC_Param_t *pstParamSpl = NULL;
    MHAL_VENC_ParamSplit_t stVencSplitInfo;

    switch(stTaskInfo.EncodeType)
    {
        case HT_VENC_H265:
            eIdxSpl = E_MHAL_VENC_265_I_SPLIT_CTL;
            break;

        case HT_VENC_H264:
            eIdxSpl = E_MHAL_VENC_264_I_SPLIT_CTL;;
            break;

        default:
            break;
    }

    pstParamSpl = (MHAL_VENC_Param_t *)&(stVencSplitInfo);
    MHAL_VENC_INIT_PARAM(MHAL_VENC_ParamSplit_t, stVencSplitInfo);

    stVencSplitInfo.bSplitEnable =     _stSplitTab[stTaskInfo.MultType].bSplitEnable;
    stVencSplitInfo.u32SliceRowCount = _stSplitTab[stTaskInfo.MultType].u32SliceRowCount;

    _HT_VENC_SetParam(stTaskInfo.hInstance, eIdxSpl, pstParamSpl, stVencDriver);

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_SetInputFileName(HT_VENC_TaskInfo_t *pstTaskInfo)
{
    if (HT_VENC_JPEG_YUV422 == pstTaskInfo->EncodeType)
    {
        pstTaskInfo->ps8InputFileName = _ps8InputYUV422FileName[pstTaskInfo->ResolutionType];
    }
    else //h254, h265, jpeg420
    {
        pstTaskInfo->ps8InputFileName = _ps8InputYUV420FileName[pstTaskInfo->ResolutionType];
    }

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_SetInterBuff(HT_VENC_TaskInfo_t *pstTaskInfo)
{
    HT_S32 s32Ret = 0;
    MHAL_VENC_InternalBuf_t stVencInternalBuf;

    MHAL_VENC_INIT_PARAM(MHAL_VENC_InternalBuf_t, stVencInternalBuf);

    //It will return AlBuffSize and RefBuffSize
	s32Ret = _HT_VENC_QueryBufSize(pstTaskInfo->hInstance, &stVencInternalBuf, stVencDriver);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    _HT_VENC_Malloc(&pstTaskInfo->stInterBuff.stIntrAl, stVencInternalBuf.u32IntrAlBufSize);
    _HT_VENC_Malloc(&pstTaskInfo->stInterBuff.stIntrRef, stVencInternalBuf.u32IntrRefBufSize);

    //give address to stVencInternalBuf's address
    stVencInternalBuf.phyIntrAlPhyBuf =  pstTaskInfo->stInterBuff.stIntrAl.phyAddr;
    stVencInternalBuf.pu8IntrAlVirBuf =  pstTaskInfo->stInterBuff.stIntrAl.pu8VirtAddr;
    stVencInternalBuf.phyIntrRefPhyBuf = pstTaskInfo->stInterBuff.stIntrRef.phyAddr;

    _HT_VENC_SetParam(pstTaskInfo->hInstance, E_MHAL_VENC_IDX_STREAM_ON, (MHAL_VENC_Param_t*)&stVencInternalBuf, stVencDriver);

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_Malloc(HT_VB_Info_t *pstVB, HT_U32 u32Size)
{
    HT_S32 s32Ret = 0;

    pstVB->u32Size = u32Size;

    s32Ret= HT_Malloc(pstVB);
    if (s32Ret != 0)
    {
        HT_VENC_PRT_ERR("malloc failed");
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_QueryBufSize(MHAL_VENC_INST_HANDLE hInstance, MHAL_VENC_InternalBuf_t *pstSize, MHalVencDrv_t stVencOpt)
{
    HT_S32 s32Ret = 0;

    s32Ret = stVencOpt.QueryBufSize(hInstance, pstSize);
    if (s32Ret != HT_SUCCESS)
    {
        HT_VENC_ERR_INFO;
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

#ifndef HT_WRITE_DDR
static HT_RESULT _HT_VENC_SetOutputFilePoiter(HT_VENC_TaskInfo_t *pstTaskInfo)
{
    //waring: maybe error
    HT_S8 ps8FileName[45] = {0};

     //ID, Encode_type, RateControl, ROI, Multsplice, resolution
    snprintf(ps8FileName, 45, "%d_%s%s%s%s%s",
    pstTaskInfo->ID,
    _ps8EncodeString[pstTaskInfo->EncodeType],
    _ps8RcString[pstTaskInfo->RateControlType],
    _ps8RoiString[pstTaskInfo->ROIType],
    _ps8MultString[pstTaskInfo->MultType],
    _ps8ResString[pstTaskInfo->ResolutionType]
    );


    pstTaskInfo->pstOutputFile = filp_open(ps8FileName, O_CREAT | O_RDWR, 0644);
    if (IS_ERR(pstTaskInfo->pstOutputFile))
    {
        HT_VENC_PRT_ERR("open fialed");
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}
#endif


static void HT_VENC_PrtTaskInof(HT_VENC_TaskInfo_t stTaskInfo)
{
#if HT_VENC_PRT_DBG_INFO
    printk("u32YUVBuffSize = %d\n", stTaskInfo.u32YUVBuffSize1);
    printk("u32YUVBuffSize2 = %d\n", stTaskInfo.u32YUVBuffSize2);
    printk("u32YUVBuffSize3 = %d\n",  stTaskInfo.u32YUVBuffSize3);
    printk("u32ReserveOutBuffSize = %d\n", stTaskInfo.u32ReserveOutBuffSize);

    printk("ps8InputFileName = %s\n", stTaskInfo.ps8InputFileName);

    printk("stInterBuff.stIntrAl.u32Size = %d\n", stTaskInfo.stInterBuff.stIntrAl.u32Size);
    printk("stInterBuff.stIntrRef.u32Size = %d\n", stTaskInfo.stInterBuff.stIntrAl.u32Size);

    printk("s32InputFileFrameCount = %d\n", stTaskInfo.s32InputFileFrameCount);
    printk("s32OutputFileFrameCount = %d\n", stTaskInfo.s32OutputFileFrameCount);
    printk("s32OutputFileFrameMax = %d\n", stTaskInfo.s32OutputFileFrameMax);
    printk("s32InputFileFrameMax = %d\n", stTaskInfo.s32InputFileFrameMax);

    printk("s64InputOffset = %lld\n", stTaskInfo.s64InputOffset);
    printk("s64OutputOffset = %lld\n", stTaskInfo.s64OutputOffset);

    printk("EncodeType = %d\n", stTaskInfo.EncodeType);
    printk("RateControlType = %d\n", stTaskInfo.RateControlType);
    printk("ROIType = %d\n", stTaskInfo.ROIType);
    printk("MultType = %d\n", stTaskInfo.MultType);
    printk("ResolutionType = %d\n", stTaskInfo.ResolutionType);
    printk("ID = %d\n", stTaskInfo.ID);

    printk("stRes.u32Widht = %d\n", stTaskInfo.stRes.u32Width);
    printk("stRes.u32Heihg = %d\n", stTaskInfo.stRes.u32Heigh);
#endif

}


static HT_RESULT _HT_VENC_SetEncodeBuff(HT_VENC_TaskInfo_t *pstTaskInfo)
{
    if (HT_VENC_JPEG_YUV422 == pstTaskInfo->EncodeType)
    {
        pstTaskInfo->u32YUVBuffSize1 = _u32YUVJPEG422BuffSize1[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize2 = _u32YUVJPEG422BuffSize2[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize3 = _u32YUVJPEG422BuffSize3[pstTaskInfo->ResolutionType];
    }
    else if (HT_VENC_JPEG_YUV420 == pstTaskInfo->EncodeType)
    {
        pstTaskInfo->u32YUVBuffSize1 = _u32YUVJPEG420BuffSize1[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize2 = _u32YUVJPEG420BuffSize2[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize3 = _u32YUVJPEG420BuffSize3[pstTaskInfo->ResolutionType];
    }
    else  //h264 h265
    {
        pstTaskInfo->u32YUVBuffSize1 = _u32YUVH26X420BuffSize1[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize2 = _u32YUVH26X420BuffSize2[pstTaskInfo->ResolutionType];
        pstTaskInfo->u32YUVBuffSize3 = _u32YUVH26X420BuffSize3[pstTaskInfo->ResolutionType];
    }

    pstTaskInfo->u32ReserveOutBuffSize = _u32ReserveOutBuffSize[pstTaskInfo->ResolutionType];

    return HT_SUCCESS;
}

static HT_RESULT _HT_VENC_ReleaseInstHandle(MHAL_VENC_INST_HANDLE phIns, MHalVencDrv_t stVencOpt)
{
    _HT_VENC_SetParam(phIns, E_MHAL_VENC_IDX_STREAM_OFF, NULL, stVencOpt);

    return HT_SUCCESS;
}


static HT_RESULT _HT_VENC_Free(HT_VB_Info_t *pstVB)
{
    HT_Free(pstVB->phyAddr);
    pstVB = NULL;

    return HT_SUCCESS;
}


