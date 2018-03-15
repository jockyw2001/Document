#ifndef __HT_VB_TYPE_H__
#define __HT_VB_TYPE_H__

typedef struct HT_VB_s
{
    struct list_head list;
    HT_U32 u32Num;
    HT_U32 u32PhyAddr;
    HT_U32 u32Size;
    HT_U8* u8VirtAddr;
}HT_VB_t;


typedef struct HT_VB_Info_s
{
    HT_U32 u32Size;
    HT_U32 u32PhyAddr;
    HT_U8* u8VirtAddr;
}HT_VB_Info_t;


typedef struct HT_VB_List_s
{
    struct list_head list;
    HT_BOOL bInit;
}HT_VB_List_t;

#endif // #define __HT_VB_TYPE_H__