#include <linux/module.h>      
#include <linux/init.h>         
//#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/fs.h> 

#include "ht_test.h"
#include "inc/ht_common.h"
#include "vpe/ht_vpe.h"
  
#define HT_HALUT_MODULE_DIR				"hal"
#define HT_HALUT_MODULE_ENTRY			"uttest"
#define HT_HALUT_NODE_INFO_LEN 			128
#define HT_HALUT_VALUE_CNT_MAX			5

static HT_HALUT_UtData_t _gtUtNodes[HT_HALUT_NODE_COUNT_MAX] = 
{	
	{"vif", "-", 0},
	{"vpe", "-", 0},
	{"venc", "-", 0},
	{"vdec", "-", 0},
	{"dvip", "-", 0},
	{"dsip", "-", 0},
	{"ai", "-", 0},
	{"ao", "-", 0}
};

const static HT_U8 *pszSaveFileName = "/tmp/kernel_file";
struct proc_dir_entry *g_pHalDir = NULL;
struct proc_dir_entry *g_pHalFile = NULL;
HT_U8 *g_pszHalutInfo = NULL;
HT_BOOL g_bSaveFile = true;

//extern HT_S32 HT_WriteFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 nLength);
//extern HT_S32 HT_ReadFile(const HT_U8 *pszFileName, const HT_U8 *pszReadBuf, HT_S32 nLength);
//extern HT_S32 UT_VPE(HT_U8 *pszCmd);
static HT_S32 _HalutOpen(struct inode *pInode, struct file *pFile);
static ssize_t _HalutWrite(struct file *pFile, const char *pszBuffer, size_t nLength, loff_t *pOffset);

static const struct file_operations _gtHalFops = 
{
	.owner = THIS_MODULE,
	.open = _HalutOpen,
	.write = _HalutWrite,
	.read = seq_read,	
	.llseek  = seq_lseek,
	.release = single_release,
};

static void _HalutSendCmdToNode(HT_S32 nIndex)
{
	switch((E_HT_HALUT_NODE_e)nIndex)
	{
		case E_HT_HALUT_VIF:
			break;
		case E_HT_HALUT_VPE:
			UT_VPE(_gtUtNodes[nIndex].szValue);
			break;
		case E_HT_HALUT_VENC:
			break;
		case E_HT_HALUT_VDEC:
			break;
		case E_HT_HALUT_DVIP:
			break;
		case E_HT_HALUT_DSIP:
			break;
		case E_HT_HALUT_AI:
			break;
		case E_HT_HALUT_AO:
			break;
		default:
			break;
	}
}

static HT_S32 _HalutSaveFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 nLength)
{
	HT_U8 *pszReadBuf = kmalloc((nLength + 1) * sizeof(HT_U8), GFP_KERNEL);
	HT_S32 writeBytes = 0;
	HT_S32 readBytes = 0;
	memset(pszReadBuf, 0, nLength + 1);
	writeBytes = HT_WriteFile(pszFileName, pszBuffer, nLength);
	readBytes = HT_ReadFile(pszFileName, pszReadBuf, writeBytes);
	
	if ((readBytes != writeBytes) || memcmp(pszBuffer, pszReadBuf, nLength))
	{
		printk(KERN_INFO"Save file:%s failed!\n", pszFileName);
		kfree(pszReadBuf);
		return -1;
	}
	
    printk(KERN_INFO"Save file:%s successfully!\n", pszFileName);
	kfree(pszReadBuf);
	return 0;
}

// check format
static HT_S32 _HalutCheckFormat(HT_S32 nodeIndex, HT_S32 varIndex, HT_S8 *value)
{
//	if (match format rules)
		return 0;
//	else
//		return -1;
}

// help info
static void _HalutDisplayHelp(HT_S32 index)
{
	printk(KERN_INFO"Usage: %s [param1] [param2] [param3] [param4] [param5]\n", _gtUtNodes[index].szName);
}

// initial halut version info
static HT_S32 _HalutInitModuleInfo(void)
{
	g_pszHalutInfo = kmalloc((HT_HALUT_NODE_INFO_LEN) * sizeof(char), GFP_KERNEL);
	
	if (!g_pszHalutInfo)
	{
		return -ENOMEM;
	}
	
	memset(g_pszHalutInfo, 0, HT_HALUT_NODE_INFO_LEN);
	sprintf(g_pszHalutInfo, "hal uttest module - Version: %s\n%-8s%-24s%-4s\n", HT_HALUT_VERSION, "UtNode", "Values", "VarCnt");
	return 0;
}

// initial halut nodes info
static HT_S32 _HalutInitNdoes(void)
{
	return 0;
}

// create halut entry
static HT_S32 _HalutCreateEntry(void)
{
	g_pHalDir = proc_mkdir(HT_HALUT_MODULE_DIR, NULL);  
    if(g_pHalDir == NULL) 
	{ 
		printk(KERN_ALERT"proc_mkdir error!\n");
        return -ENOMEM;    
    } 
	
	g_pHalFile = proc_create(HT_HALUT_MODULE_ENTRY, 0644, g_pHalDir, &_gtHalFops);    
    if(g_pHalFile == NULL) 
	{    
		remove_proc_entry(HT_HALUT_MODULE_DIR, NULL); 
		printk(KERN_ALERT"proc_create error!\n");
        return -ENOMEM;      
    }    
	
	printk(KERN_INFO"%s %s initialised!\n", HT_HALUT_MODULE_DIR, HT_HALUT_MODULE_ENTRY);
	return 0;
}

static HT_S32 _HalutProcShow(struct seq_file *pSeq, void *pVoid)
{
	HT_S32 i = 0;
	seq_printf(pSeq, "%s", g_pszHalutInfo);
	
	for	(i =0; i < HT_HALUT_NODE_COUNT_MAX; i++)
	{
		if (_gtUtNodes[i].nValueCnt > 0)
		{
			seq_printf(pSeq, "%-8s%-24s%4d\n", _gtUtNodes[i].szName, _gtUtNodes[i].szValue, _gtUtNodes[i].nValueCnt); 
		}
		else
		{
			seq_printf(pSeq, "%-8s%-24s%-4s\n", _gtUtNodes[i].szName, "-", "   -");
		}
	}
	
	return 0;        
}

static HT_S32 _HalutOpen(struct inode *pInode, struct file *pFile)
{
//	printk(KERN_INFO"halut_open.\n");
	return single_open(pFile, _HalutProcShow, pInode->i_private);
}  

static ssize_t _HalutWrite(struct file *pFile, const char *pszBuffer, size_t nLength, loff_t *pOffset)
{
	HT_S8 *pszSaveCmd = kmalloc(HT_HALUT_NODE_VALUE_MAX * sizeof(HT_S8), GFP_KERNEL);
	HT_S8 *pszRcvBuf = kstrdup(pszBuffer, GFP_KERNEL);
	HT_S8 *pszRefBuf = NULL;	
	HT_S8 *pszRcvCmd = NULL;
	HT_S8 *pszRefCmd = NULL;	
	HT_S8 *pszTemp = NULL;
	HT_S32 i =0;
	HT_BOOL bExist = false;
	HT_BOOL bFirstCmd = false;
	
	if ('\n' == pszRcvBuf[nLength-1])
		pszRcvBuf[nLength-1] = '\0';
		
	pszRefBuf = pszRcvBuf;	
	pszTemp = strsep((char**)(&pszRefBuf), " ");
	
	for (i = 0; i < HT_HALUT_NODE_COUNT_MAX; i++)
	{
		if (!strcmp(pszTemp, _gtUtNodes[i].szName))
		{
			bExist = true;
			break;
		}
	}

	if (!bExist)
	{
		printk(KERN_INFO"Bad commands! %s is not exist.\n", pszTemp);
		goto RET;
	}
	memset(pszSaveCmd, 0, HT_HALUT_NODE_VALUE_MAX);
	
	if (!pszRefBuf)
	{
		printk(KERN_INFO"Bad commands! too few paramaters.\n");
		_HalutDisplayHelp(i);
		goto RET;
	}
	
	memcpy(pszSaveCmd, pszRefBuf, strlen(pszRefBuf));
	memset(_gtUtNodes[i].szValue, 0, HT_HALUT_NODE_VALUE_MAX);
	_gtUtNodes[i].nValueCnt = 0;
	pszRcvCmd = kstrdup(pszRefBuf, GFP_KERNEL);
	pszRefCmd = pszRcvCmd;
	bFirstCmd = true;
	
    while((pszTemp = strsep((char**)(&pszRefCmd), " ")) != NULL)      
    {
		//printk(KERN_INFO"var:%s......\n", pszTemp);
		if ( bFirstCmd && (!strcmp(pszTemp, "help")) )
		{
			_HalutDisplayHelp(i);		
			goto RET;
		}
		
		bFirstCmd = false;
		if (!_HalutCheckFormat(i, _gtUtNodes[i].nValueCnt, pszTemp))
		{
			_gtUtNodes[i].nValueCnt++;
		}
		else
		{
			_gtUtNodes[i].nValueCnt = 0;
			printk(KERN_INFO"Bad commands! %s is worng format.\n", pszTemp);
			_HalutDisplayHelp(i);
			goto RET;
		}	
    }

	// check nValueCnt
	if (_gtUtNodes[i].nValueCnt > HT_HALUT_VALUE_CNT_MAX)
	{
		_gtUtNodes[i].nValueCnt = 0;
		printk(KERN_INFO"Bad commands! too many parameters.\n");
		_HalutDisplayHelp(i);
		goto RET;
	}
	
	memcpy(_gtUtNodes[i].szValue, pszSaveCmd, (size_t)strlen((char*)pszSaveCmd));
	
	// send cmd to node
	_HalutSendCmdToNode(i);	
		
	if (g_bSaveFile)
	{
		_HalutSaveFile(pszSaveFileName, (HT_U8*)pszBuffer, nLength);
	}
	
RET:	
	kfree(pszSaveCmd);
	kfree(pszRcvCmd);
    kfree(pszRcvBuf);
	return nLength;		//return length: below 0, fault;0, repeat write;above 0, ok.
}
  
static HT_S32 __init _HalutInit(void)  
{   
	_HalutInitModuleInfo();
	_HalutCreateEntry();
	_HalutInitNdoes();
	return 0;
}  
							    
static void __exit _HalutExit(void)  
{   
	if (g_pszHalutInfo)
	{
		kfree(g_pszHalutInfo);
	}
	
	remove_proc_entry(HT_HALUT_MODULE_ENTRY, g_pHalDir);   
    remove_proc_entry(HT_HALUT_MODULE_DIR, NULL); 
    printk(KERN_INFO"%s %s removed\n", HT_HALUT_MODULE_DIR, HT_HALUT_MODULE_ENTRY);
}  

module_init(_HalutInit);  
module_exit(_HalutExit); 

MODULE_AUTHOR("MSTAR");  
MODULE_DESCRIPTION("HALUT TEST");
MODULE_LICENSE("GPL"); 