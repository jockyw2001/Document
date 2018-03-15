#ifndef __HT_VB_TYPE_H__
#define __HT_VB_TYPE_H__
#include "ht_common_datatype.h"
#include <linux/list.h>

typedef struct HT_VB_s
{
    struct list_head list;
    HT_U32 u32Num;
    HT_PHY phyAddr;
    HT_U32 u32Size;
    HT_U8* pu8VirtAddr;
}HT_VB_t;


typedef struct HT_VB_Info_s
{
    HT_U32 u32Size;
    HT_PHY phyAddr;
    HT_U8* pu8VirtAddr;
}HT_VB_Info_t;


typedef struct HT_VB_List_s
{
    struct list_head list;
    HT_BOOL bInit;
}HT_VB_List_t;

typedef struct HT_YUV420_SP_Info_s{
    HT_VB_Info_t stVbInfo_y;
    HT_VB_Info_t stVbInfo_uv;
}HT_YUV420_SP_Info_t;

typedef struct HT_YUV422_YUYV_Info_s{
    HT_VB_Info_t stVbInfo_yuv;
}HT_YUV422_YUYV_Info_t;


#endif // #define __HT_VB_TYPE_H__