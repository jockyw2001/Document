#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/pfn.h>
#include <linux/delay.h>
#include <linux/compat.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
#include <../../mstar2/drv/miu/mdrv_miu.h>
#include "../../mstar2/drv/mma_heap/mdrv_mma_heap.h"
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
#include <../drivers/mstar/miu/mdrv_miu.h>
#include <../drivers/mstar/mma_heap/mdrv_mma_heap.h>
#endif
//#include "mstar_chip.h"
//#include "SN_MMAP_DTV_MM_K6LITE_256.h"
//#include "HW_Config.h"
#include "ht_memory_partition.h"
#include "ht_common.h"
#include "ht_vb_type.h"
//#include "inc/ht_util_list.h"
struct list_head g_HtVblist;
HT_BOOL g_HtVblistInit;

struct list_head gstOutputBufHead = LIST_HEAD_INIT(gstOutputBufHead);

typedef struct HT_VB_BufListData_s
{
    struct list_head stBufListHead;
    HT_VB_Info_t stHtVbInfo;
}HT_VB_BufListData_t;

static LIST_HEAD(_gstBufListHead);
static DEFINE_SEMAPHORE(HT_PhyBuf_Sem);

/*malloc buffer*/
HT_U8* HT_FindVirAddr(HT_PHY phyAddr)
{
    HT_VB_BufListData_t *pos;

    down(&HT_PhyBuf_Sem);
    list_for_each_entry(pos, &_gstBufListHead, stBufListHead)
    {
        if (phyAddr == pos->stHtVbInfo.phyAddr && pos->stHtVbInfo.u32Size != 0)
        {
            printk("[BUF_DEBUG] Ioremap Pa is  %llx ,size is %x, va is %x \r\n", phyAddr, pos->stHtVbInfo.u32Size, *pos->stHtVbInfo.pu8VirtAddr);
            up(&HT_PhyBuf_Sem);

            return pos->stHtVbInfo.pu8VirtAddr;
        }
    }
    up(&HT_PhyBuf_Sem);
    return NULL;
}

static char * _HT_ReadLine(HT_U8 *buf, HT_U32 len, struct file *fp)
{
    int ret = 0;
    int i = 0;

    ret = fp->f_op->read(fp, buf, len, &(fp->f_pos));
    if (ret <= 0)
    {
        return NULL;
    }

    while(i < ret)
    {
        if (buf[i++] == '\n')
        {
            break;
        }
    }

    if (i < ret)
    {
        fp->f_op->llseek(fp, (i - ret), SEEK_CUR);
    }

    if(i < len)
    {
        buf[i] = 0;
    }

    return buf;
}

static int _HT_GetMiu(HT_U8 *dst, HT_U8 *value, struct file *fp)
{
    char buf[512];
    char *pos = NULL;
    while ( (pos = _HT_ReadLine(buf, sizeof(buf), fp)) )
    {
        char *p = strstr(pos, dst);
        if (!p)
        {
            continue;
        }

        pos = p;
        p = strstr(pos, "MIU0");
        if (p)
        {
            *value = 0;
            return 1;
        }

        p = strstr(pos, "MIU1");
        if (p)
        {
            *value = 1;
            return 1;
        }

        p = strstr(pos, "MIU2");
        if (p)
        {
            *value = 2;
            return 1;
        }
    }

    return 0;
}

static int _HT_GetValue(HT_U8 *dst, HT_U32 *value, struct file *fp)
{
    char buf[512];
    char *pos = NULL;
    while ( (pos = _HT_ReadLine(buf, sizeof(buf), fp)) )
    {
        char *p = NULL;
        if ( (p = strstr(pos, dst)) )
        {
            char *start = NULL, *end = NULL;
            start = strstr(p, "0x");
            if (!start)
            {
                pos = NULL;
                break;
            }

            end = strchr(start, ' ');
            if (!end)
            {
                end = strchr(start, '\n');
            }

            if (!end)
            {
                end = strchr(start, '/');
            }

            if (!end)
            {
                pos = NULL;
                break;
            }

            *end = 0;
            *value = simple_strtol(start, NULL, 16);
            return 1;
        }
        else
        {
            continue;
        }
    }

    return 0;
}

HT_RESULT HT_MmapParser(HT_U8 *name, HT_U32 *addr, HT_U32 *size, HT_U8 *miu)
{
    struct file *fp = NULL;
    mm_segment_t fs;
    HT_RESULT ret = HT_FAILURE;
    HT_U8 targe[64];

    if ((strlen(name) + strlen("_MEMORY_TYPE")) >= sizeof(targe))
    {
        printk("name over flow\n");
        return 0;
    }

    fp =filp_open("/config/mmap.ini",O_RDONLY,0644);
    if (IS_ERR(fp))
    {
        return 0;
    }

    fs =get_fs();
    set_fs(KERNEL_DS);
    ///N+1 m, all cpu address and size
    fp->f_op->llseek(fp, 0, SEEK_SET);
    sprintf(targe, "%s_ADR", name);
    if (!_HT_GetValue(targe, addr, fp))
    {
        printk("Can't Find ADDR\n");
        goto _end;
    }

    sprintf(targe, "%s_LEN", name);
    if (!_HT_GetValue(targe, size, fp))
    {
        printk("Can't Find LEN\n");
        goto _end;
    }

    sprintf(targe, "%s_MEMORY_TYPE", name);
    if (!_HT_GetMiu(targe, miu, fp))
    {
        printk("Can't Find MIU\n");
        goto _end;
    }

    ret = HT_SUCCESS;
_end:
    set_fs(fs);
    filp_close(fp,NULL);
    return ret;
}
///mmap parser end



#ifdef USE_MI_SYS
#include "mi_common.h"
#include "mi_sys_internal.h"
HT_U32 HT_MallocAlign(HT_VB_Info_t *pstVbInfo, HT_U32 u32Alignment)
{
    HT_U32 u32Size = 0;
    HT_VB_BufListData_t *pstBufData = NULL;

    if (pstVbInfo == NULL)
    {
        printk("[BUF_ERROR]Error!! pstVbInfo is NULL!\n");
        return HT_FAILURE;
    }
    u32Size = pstVbInfo->u32Size;
    u32Alignment = u32Alignment;

    pstBufData = (HT_VB_BufListData_t*)kmalloc(sizeof(HT_VB_BufListData_t),GFP_KERNEL);
    if (pstBufData == NULL)
    {
        printk("[BUF_ERROR]Error!! pstBufData is NULL!\n");
        return HT_FAILURE;
    }
    if (MI_SUCCESS != mi_sys_MMA_Alloc(NULL, u32Size, (MI_PHY *)&pstVbInfo->phyAddr))
    {
        printk("[BUF_ERROR]Error!! pstVbInfo is NULL!\n");
        kfree(pstBufData);
        return HT_FAILURE;
    }
    pstVbInfo->pu8VirtAddr = (HT_U8 *)mi_sys_Vmap((MI_PHY)pstVbInfo->phyAddr, u32Size, FALSE);
    if (NULL == pstVbInfo->pu8VirtAddr)
    {
        printk("[BUF_ERROR]Error!! map error!\n");
        mi_sys_MMA_Free((MI_PHY)pstVbInfo->phyAddr);
        kfree(pstBufData);
        return HT_FAILURE;
    }
    memcpy(&pstBufData->stHtVbInfo, pstVbInfo, sizeof(HT_VB_Info_t));
    down(&HT_PhyBuf_Sem);
    list_add(&pstBufData->stBufListHead, &_gstBufListHead);
    up(&HT_PhyBuf_Sem);
    //printk("[BUF_DEBUG]pstVbInfo.u32PhyAddr is %llx size is %x \r\n",pstVbInfo->phyAddr,u32Size);
    //printk("[BUF_DEBUG]pstVbInfo.u32VirtAddr is %p size is %x \r\n",pstVbInfo->pu8VirtAddr,u32Size);

    return HT_SUCCESS;
}

/*free buffer*/
void HT_Free(HT_PHY Pa)
{
    HT_VB_BufListData_t *pos, *posN;

    down(&HT_PhyBuf_Sem);
    list_for_each_entry_safe(pos, posN, &_gstBufListHead, stBufListHead)
    {
        if (Pa == pos->stHtVbInfo.phyAddr)
        {
            //printk("[BUF_DEBUG]FREE Pa is  %llx ,size is %x\r\n",Pa,pos->stHtVbInfo.u32Size);

            list_del(&pos->stBufListHead);
            mi_sys_UnVmap((void *)pos->stHtVbInfo.pu8VirtAddr);
            mi_sys_MMA_Free((MI_PHY)pos->stHtVbInfo.phyAddr);
            kfree(pos);
            break;
        }
    }
    if (&pos->stBufListHead == &_gstBufListHead)
    {
        printk("[BUF_DEBUG] Do not found any addr bad addr!\n");
    }
    up(&HT_PhyBuf_Sem);
}
#else
#define ALIGN_DOWN(val, alignment) val = (((val)/(alignment))*(alignment))
#define ALIGN_UP(val, alignment) val = ((( (val)+(alignment)-1)/(alignment))*(alignment))
extern struct MMA_BootArgs_Config mma_config[MAX_MMA_AREAS];
static HT_VB_BufListData_t stFirstBufListData;
HT_U32 HT_MallocAlign(HT_VB_Info_t *pstVbInfo, HT_U32 u32Alignment)
{
    HT_VB_BufListData_t *pos, *posN, *pstBufData;
    HT_U32 u32Size = 0;
    HT_U32 u32StartPhyAddr = mma_config[0].reserved_start - ARM_MIU0_BUS_BASE;
    HT_U32 u32EndPhyAddr = u32StartPhyAddr + mma_config[0].size;
    HT_U32 u32PhyAddr = 0;

    if (pstVbInfo == NULL)
    {
        printk("[BUF_ERROR]Error!! pstVbInfo is NULL!\n");
        return -0xFFFFFFFF;
    }
    u32Size = pstVbInfo->u32Size;

    if (0 == u32Size || u32Size > mma_config[0].size)
    {
        return 0xFFFFFFFF;
    }

    ALIGN_UP(u32Size, 4);

    down(&HT_PhyBuf_Sem);

    if (list_empty(&_gstBufListHead))
    {
        u32PhyAddr = u32StartPhyAddr;
        ALIGN_UP(u32PhyAddr, u32Alignment);
        if (u32EndPhyAddr < u32PhyAddr + u32Size)
        {
            printk("[BUF_ERROR]Error!! mmap buffer is full!\n");
            up(&HT_PhyBuf_Sem);
            return 0xFFFFFFFF;
        }
        memset(&stFirstBufListData, 0, sizeof(HT_VB_BufListData_t));
        stFirstBufListData.stHtVbInfo.phyAddr = u32EndPhyAddr;
        list_add_tail(&stFirstBufListData.stBufListHead, &_gstBufListHead);
        if (mma_config[0].reserved_start)
        {
            printk("mma base addr is %x without miu0 offset %llx size is %x endaddr is %x\n", (HT_U32)mma_config[0].reserved_start, stFirstBufListData.stHtVbInfo.phyAddr, (HT_U32)mma_config[0].size, u32EndPhyAddr);
        }
        u32PhyAddr = u32EndPhyAddr - u32Size;
        ALIGN_DOWN(u32PhyAddr, u32Alignment);

    }
    else
    {
        list_for_each_entry_safe(pos, posN, &_gstBufListHead, stBufListHead)
        {
            u32PhyAddr = (HT_U32)pos->stHtVbInfo.phyAddr - u32Size;
            ALIGN_DOWN(u32PhyAddr, u32Alignment);
            if(&posN->stBufListHead == &_gstBufListHead)
            {
                if (u32StartPhyAddr + u32Size > u32PhyAddr)
                {
                    printk("[BUF_ERROR]Error!! mmap buffer is full!\n");
                    up(&HT_PhyBuf_Sem);
                    return 0xFFFFFFFF;
                }
                break;
            }
            if (u32PhyAddr - posN->stHtVbInfo.phyAddr - posN->stHtVbInfo.u32Size >= u32Size)
            {
                break;
            }
        }
    }
    pstBufData = (HT_VB_BufListData_t*)kmalloc(sizeof(HT_VB_BufListData_t),GFP_KERNEL);
    if (pstBufData == NULL)
    {
        up(&HT_PhyBuf_Sem);
        return 0xFFFFFFFF;
    }
    memset(pstBufData, 0, sizeof(HT_VB_BufListData_t));
    pstBufData->stHtVbInfo.u32Size = u32Size; //4 byte align
    pstBufData->stHtVbInfo.phyAddr = pstVbInfo->phyAddr = u32PhyAddr;
    //printk("[BUF_DEBUG]Ioremap addr %llx\n", pstBufData->stHtVbInfo.phyAddr + ARM_MIU0_BUS_BASE);
    pstBufData->stHtVbInfo.pu8VirtAddr = pstVbInfo->pu8VirtAddr = ioremap(pstBufData->stHtVbInfo.phyAddr + ARM_MIU0_BUS_BASE, u32Size);
    list_add(&pstBufData->stBufListHead, &_gstBufListHead);
    up(&HT_PhyBuf_Sem);
    //printk("[BUF_DEBUG]pstVbInfo.u32PhyAddr is %llx size is %x \r\n",pstVbInfo->phyAddr, u32Size);
    //printk("[BUF_DEBUG]pstVbInfo.u32VirtAddr is %p size is %x \r\n",pstVbInfo->pu8VirtAddr,u32Size);
    return 0;
}
/*free buffer*/
void HT_Free(HT_PHY Pa)
{
    HT_VB_BufListData_t *pos, *posN;

    down(&HT_PhyBuf_Sem);
    list_for_each_entry_safe(pos, posN, &_gstBufListHead, stBufListHead)
    {
        if (Pa == pos->stHtVbInfo.phyAddr &&    \
            &pos->stBufListHead != &stFirstBufListData.stBufListHead)
        {
            //printk("[BUF_DEBUG]FREE Pa is  %llx ,size is %x\r\n",Pa,pos->stHtVbInfo.u32Size);

            list_del(&pos->stBufListHead);
            kfree(pos);
            //VirAddr=HT_FindVirAddr(Pa);
            iounmap(pos->stHtVbInfo.pu8VirtAddr);
            break;
        }
    }
    if (&pos->stBufListHead == &_gstBufListHead)
    {
        printk("[BUF_DEBUG] Do not found any addr bad addr!\n");
    }
    up(&HT_PhyBuf_Sem);
}
#endif


HT_U32 HT_Malloc(HT_VB_Info_t *pstVbInfo)
{
	return HT_MallocAlign(pstVbInfo, 1);
}


/*write file*/
HT_S32 HT_WriteFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 s32Length, HT_S64 s64Pos)
{
	struct file *pFile = NULL;
	mm_segment_t tFs;
    loff_t pos = 0;
	ssize_t writeBytes = 0;
	pFile = filp_open(pszFileName, O_RDWR | O_CREAT,0644);

    if (IS_ERR(pFile))
	{
        printk("Create file:%s error.\n", pszFileName);
        return -1;
    }

	tFs = get_fs();
    set_fs(KERNEL_DS);
    pos = s64Pos;
    writeBytes = vfs_write(pFile, (char*)pszBuffer, s32Length, &pos);
	filp_close(pFile, NULL);
	set_fs(tFs);
    //printk("Write %d bytes to %s.\n", writeBytes, pszFileName);

	return (HT_S32)writeBytes;
}

/*read file*/
HT_S32 HT_ReadFile(const HT_U8 *pszFileName, HT_U8 *pszReadBuf, HT_S32 s32Length, HT_S64 s64Pos)
{
	struct file *pFile = NULL;
	mm_segment_t tFs;
    loff_t pos = 0;
	ssize_t readBytes = 0;
	memset(pszReadBuf, 0, s32Length);
	pFile = filp_open(pszFileName, O_RDONLY, 0);

    if (IS_ERR(pFile))
	{
        printk("Open file:%s error.\n", pszFileName);
        return -1;
    }

	tFs = get_fs();
    set_fs(KERNEL_DS);
    pos = s64Pos;
    readBytes = vfs_read(pFile, (char*)pszReadBuf, s32Length, &pos);
	filp_close(pFile, NULL);
	set_fs(tFs);
    //printk("Read %d bytes from %s.\r\n", readBytes, pszFileName);

	return (HT_S32)readBytes;
}

HT_RESULT HT_MallocYUVOneFrame(HT_YUVInfo_t           *pstYUVInfo, HT_U32 u32Width, HT_U32 u32Height)
{
    HT_U32 u32Ret;
    HT_U32 u32Size = u32Width * u32Height;
    if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size = u32Size;
        pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size = u32Size/2;

        u32Ret=HT_Malloc(&(pstYUVInfo->stYUV420SP.stVbInfo_y));
        u32Ret= u32Ret||HT_Malloc(&(pstYUVInfo->stYUV420SP.stVbInfo_uv));
        if(u32Ret !=0)
            return HT_FAILURE;

        memset(pstYUVInfo->stYUV420SP.stVbInfo_y.pu8VirtAddr, 0, pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size);
        memset(pstYUVInfo->stYUV420SP.stVbInfo_uv.pu8VirtAddr, 0, pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size);
    }
    else if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size = u32Size*2;
        u32Ret = HT_Malloc(&(pstYUVInfo->stYUV422YUYV.stVbInfo_yuv));
        if(u32Ret !=0)
            return HT_FAILURE;

        memset(pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, 0, pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size);
    }
    else
        return HT_FAILURE;

    return HT_SUCCESS;
}


HT_RESULT HT_FreeYUVOneFrame(HT_YUVInfo_t           *pstYUVInfo)
{
    if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {
        HT_Free(pstYUVInfo->stYUV420SP.stVbInfo_y.phyAddr);
        HT_Free(pstYUVInfo->stYUV420SP.stVbInfo_uv.phyAddr);
    }
    else if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {
        HT_Free(pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr);
    }
    else
        return HT_FAILURE;

   return HT_SUCCESS;
}


/*Read One Frame to buffer*/
HT_RESULT HT_ReadYUVOneFrameToBuffer(const HT_U8 *pszFileName, HT_YUVInfo_t               *pstYUVInfo, HT_S64 *ps64Pos)
{
    HT_S32 s32Readbyes = 0;
    if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {

        s32Readbyes = HT_ReadFile(pszFileName, pstYUVInfo->stYUV420SP.stVbInfo_y.pu8VirtAddr, pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size, *ps64Pos);
        *ps64Pos+= pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size;

        if(s32Readbyes < 0)
            return HT_FAILURE;

        s32Readbyes = HT_ReadFile(pszFileName, pstYUVInfo->stYUV420SP.stVbInfo_uv.pu8VirtAddr, pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size, *ps64Pos);
        *ps64Pos += pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size;

        if(s32Readbyes<0)
            return HT_FAILURE;
    }
    else if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {

        s32Readbyes=HT_ReadFile(pszFileName, pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size, *ps64Pos);
        *ps64Pos += pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size;

        if(s32Readbyes<0)
            return HT_FAILURE;
    }
    else
        return HT_FAILURE;

    return HT_SUCCESS;
}

/*Write One Frame to File*/
HT_RESULT HT_WriteYUVOneFrameToFile(const HT_U8 *pszFileName, HT_YUVInfo_t               *pstYUVInfo, HT_S64 *ps64Pos)
{
    HT_S32 s32WriteByes=0;
    if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    {

        s32WriteByes=HT_WriteFile(pszFileName, pstYUVInfo->stYUV420SP.stVbInfo_y.pu8VirtAddr, pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size, *ps64Pos);
        *ps64Pos += pstYUVInfo->stYUV420SP.stVbInfo_y.u32Size;

        if(s32WriteByes<0)
              return HT_FAILURE;

        s32WriteByes = HT_WriteFile(pszFileName, pstYUVInfo->stYUV420SP.stVbInfo_uv.pu8VirtAddr, pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size, *ps64Pos);
        *ps64Pos += pstYUVInfo->stYUV420SP.stVbInfo_uv.u32Size;
        if(s32WriteByes<0)
              return HT_FAILURE;
    }
    else if(pstYUVInfo->ePixelFormat == E_HT_PIXEL_FORMAT_YUV422_YUYV)
    {

        s32WriteByes = HT_WriteFile(pszFileName, pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.pu8VirtAddr, pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size, *ps64Pos);
        *ps64Pos += pstYUVInfo->stYUV422YUYV.stVbInfo_yuv.u32Size;

         if(s32WriteByes<0)
              return HT_FAILURE;
    }
    else
        return HT_FAILURE;

  return HT_SUCCESS;
}

// sort YUV
HT_RESULT HT_Soft_DetileYUV(void *pDstBuf, void *pSrcYBuf, void *pSrcUVBuf,
		HT_U32 u32Width, HT_U32 u32Pitch, HT_U32 u32Height, HT_TILE_MODE eMode)
{
    HT_U32 i = 0;
	HT_U32 j = 0;
    HT_U32 mfb_height;
	HT_U32 mfb_uv_height;
	HT_U32 mfetch_num_per_line;
    HT_U32 page_cnt = 0;
	HT_U32 row_st   = 0;
	HT_U32 col_st   = 0;
    HT_U32 owidth   = u32Width;
    HT_U8* pbuf     = (HT_U8 *)pSrcYBuf;
    HT_U8* pUDstBuf;
	HT_U8* pVDstBuf;
    HT_U32 shift = 0;
	HT_U32 endcol;
	HT_U32 fetch_size;
	HT_U32 DETILE_PAGE_SIZE  = 32;
    HT_U32 DETILE_FETCH_SIZE = 16;

	CHECK_NULL_POINTER(__FUNCTION__, pDstBuf);
	CHECK_NULL_POINTER(__FUNCTION__, pSrcYBuf);
	CHECK_NULL_POINTER(__FUNCTION__, pSrcUVBuf);

    if (E_HT_MVD == eMode)
	{
        DETILE_FETCH_SIZE = 8;
        DETILE_PAGE_SIZE  = 32;
    }
	else if (E_HT_EVD == eMode)
	{
        DETILE_FETCH_SIZE = 32;
        DETILE_PAGE_SIZE  = 16;
    }

    if (E_HT_EVD == eMode) {
        mfb_height    = GET_FRAME_HEIGHT_EVD(u32Height);
        mfb_uv_height = GET_FRAME_HEIGHT_EVD(u32Height / 2);
    }
	else
	{
        mfb_height    = GET_FRAME_HEIGHT(u32Height);
        mfb_uv_height = GET_FRAME_HEIGHT(u32Height / 2);
    }
    mfetch_num_per_line = u32Pitch / DETILE_FETCH_SIZE;

    if (u32Width != u32Pitch)
	{
        shift = u32Width % DETILE_FETCH_SIZE;
    }
	else
	{
        shift = DETILE_FETCH_SIZE;
    }
    endcol = owidth / DETILE_FETCH_SIZE * DETILE_FETCH_SIZE;

    for (i = 0; i < mfb_height * mfetch_num_per_line; i++)
	{
        page_cnt = i / (DETILE_PAGE_SIZE * mfetch_num_per_line);
        row_st   = i % DETILE_PAGE_SIZE + page_cnt * DETILE_PAGE_SIZE;
        col_st   = (i / DETILE_PAGE_SIZE) * DETILE_FETCH_SIZE % u32Pitch;

        if (row_st < u32Height && col_st < u32Width)
		{
            if (col_st == endcol)
			{
                fetch_size = shift;
            }
			else
			{
                fetch_size = DETILE_FETCH_SIZE;
            }
            memcpy((HT_U8*)pDstBuf + (row_st * owidth + col_st), pbuf, fetch_size);
        }

        pbuf += DETILE_FETCH_SIZE;
    }

    pbuf     = (HT_U8 *)pSrcUVBuf;
    pUDstBuf = (HT_U8 *)pDstBuf + (owidth * u32Height);
    pVDstBuf = (HT_U8 *)pUDstBuf + ((owidth * u32Height) / 4);
    u32Height /= 2;
    owidth    /= 2;

    // UV
    for (i = 0; i < mfb_uv_height * mfetch_num_per_line; i++)
	{
        page_cnt = i / (DETILE_PAGE_SIZE * mfetch_num_per_line);
        row_st   = (i % DETILE_PAGE_SIZE + page_cnt * DETILE_PAGE_SIZE);
        col_st   = (i / DETILE_PAGE_SIZE) * DETILE_FETCH_SIZE % u32Pitch;

        if (row_st < u32Height && col_st < u32Width)
		{
            if (col_st == endcol)
			{
                fetch_size = shift;
            }
			else
			{
                fetch_size = DETILE_FETCH_SIZE;
            }

            for (j = 0; j < fetch_size; j++)
			{
                if (j % 2 == 0)
                    *(pUDstBuf + (row_st * owidth + (col_st + j) / 2)) = pbuf[j];
                else
                    *(pVDstBuf + (row_st * owidth + (col_st + j) / 2)) = pbuf[j];
            }
        }

        pbuf += DETILE_FETCH_SIZE;
    }

	return HT_SUCCESS;
}


