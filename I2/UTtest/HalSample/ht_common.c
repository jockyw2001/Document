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

#include "inc/ht_memory_partition.h"
#include "inc/ht_common_datatype.h"
#include "inc/ht_common.h"
#include "inc/ht_vb_type.h"
//#include "inc/ht_util_list.h"
HT_VB_List_t g_HtVblist;
static HT_U32 g_HtSeqno;
/*malloc buffer*/
HT_U32 HT_Malloc(HT_VB_Info_t* pstVbInfo)
{
    // HT_VB_t *stVbInfo;
    // HT_U32 u32Pos = 0;
    // struct list_head *pos, *q;
    // //if(g_HtVblist.bInit == FALSE)
	// if(false == g_HtVblist.bInit)
    // {
		// struct list_head *pAddr = &(g_HtVblist.list);
        // LIST_HEAD_INIT(pAddr);
        // pstVbInfo->u8VirtAddr = ioremap(E_MMAP_ID_DUMMY5_AVAILABLE + MIU0_BUS_OFFSET,pstVbInfo->u32Size);
        // pstVbInfo->u32PhyAddr = E_MMAP_ID_DUMMY5_AVAILABLE;
        // stVbInfo.u32Num = g_HtSeqno;
        // stVbInfo.u32PhyAddr = E_MMAP_ID_DUMMY5_AVAILABLE;
        // stVbInfo.u32Size = pstVbInfo->u32Size;
        // stVbInfo->u8VirtAddr = pstVbInfo->u8VirtAddr;
        // list_add(&stVbInfo.list,&g_HtVblist.list);
        // g_HtSeqno++;
        // g_HtVblist.bInit = TRUE;
    // }
    // else
    // {
        // u32Pos = E_MMAP_ID_DUMMY5_AVAILABLE;
        // list_for_each_safe(pos,q,&g_HtVblist.list)
        // {
            // stVbInfo = list_entry(pos, HT_VB_t, list);
            // u32Pos += stVbInfo->u32Size;//calc PHY addr
        // }

        // stVbInfo->u32Num = g_HtSeqno;
        // stVbInfo->u32PhyAddr = u32Pos;
        // stVbInfo->u32Size = pstVbInfo->u32Size;
        // pstVbInfo->u8VirtAddr = ioremap(stVbInfo->u32PhyAddr + MIU0_BUS_OFFSET,pstVbInfo->u32Size);
        // pstVbInfo->u32PhyAddr = u32Pos;
        // stVbInfo->u8VirtAddr = pstVbInfo->u8VirtAddr;
        // list_add_tail(&stVbInfo->list,&g_HtVblist.list);
        // g_HtSeqno++;
    // }
    return 0;
}

/*free buffer*/
void HT_Free(void* Va)
{
    HT_VB_t *stVbInfo;
    struct list_head *pos, *q;

    list_for_each_safe(pos,q,&g_HtVblist.list)
    {
        stVbInfo = list_entry(pos, HT_VB_t, list);
        if(stVbInfo->u8VirtAddr == Va)
        {
            iounmap(Va);
            list_del(&g_HtVblist.list);
            g_HtSeqno--;
        }
    }

    if(g_HtSeqno == 0)
    {
        //g_HtVblist.bInit = FALSE;
		g_HtVblist.bInit = false;
    }
}

/*write file*/
HT_S32 HT_WriteFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 nLength, HT_S64 nPos)
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
    pos = nPos; 
    writeBytes = vfs_write(pFile, (char*)pszBuffer, nLength, &pos);
	filp_close(pFile, NULL);
	set_fs(tFs);
    printk("Write %d bytes to %s.\n", writeBytes, pszFileName);
	return (HT_S32)writeBytes;
}

/*read file*/
HT_S32 HT_ReadFile(const HT_U8 *pszFileName, HT_U8 *pszReadBuf, HT_S32 nLength, HT_S64 nPos)
{
	struct file *pFile = NULL;
	mm_segment_t tFs;
    loff_t pos = 0;
	ssize_t readBytes = 0;
	memset(pszReadBuf, 0, nLength);
	pFile = filp_open(pszFileName, O_RDONLY, 0);
	
    if (IS_ERR(pFile))
	{ 
        printk("Open file:%s error.\n", pszFileName);
        return -1;
    } 
	
	tFs = get_fs();
    set_fs(KERNEL_DS);
    pos = nPos; 
    readBytes = vfs_read(pFile, (char*)pszReadBuf, nLength, &pos);
	filp_close(pFile, NULL);
	set_fs(tFs);
    printk("Read %d bytes from %s.\n", readBytes, pszFileName);
	return (HT_S32)readBytes;
} 
