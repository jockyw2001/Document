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

#include "ht_module.h"
#include "ht_common.h"
#ifdef MOD_VPE
#include "ht_vpe.h"
#endif
#ifdef MOD_DIVP
#include "ht_divp.h"
#endif
#ifdef MOD_VIF
#include "ht_vif.h"
#endif
#ifdef MOD_VDEC
#include "ht_vdec.h"
#endif
#ifdef MOD_DISP
#include "ht_disp.h"
#endif
#ifdef MOD_AO
#include "ht_ao.h"
#endif
#ifdef MOD_AI
#include "ht_ai.h"
#endif
#ifdef MOD_HDMI
#include "ht_hdmi.h"
#endif


#define HT_HALUT_MODULE_DIR				"hal"
#define HT_HALUT_MODULE_ENTRY			"uttest"
#define HT_MODULE_INFO_MAX_LENGTH 		256

typedef HT_RESULT (*HT_MODULE_UnitTest)(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt);
typedef void (*HT_MODULE_DisplayHelp)(void);

static HT_UtData_t _gtUtModules[E_HT_MODULE_MAX] =
{
	{"vif", 0, NULL},
	{"vpe", 0, NULL},
	{"venc", 0, NULL},
	{"vdec", 0, NULL},
	{"divp", 0, NULL},
	{"disp", 0, NULL},
	{"ai", 0, NULL},
	{"ao", 0, NULL},
	{"hdmi", 0, NULL}
};

const static HT_U8 *pszSaveFileName = "/tmp/kernel_file";
struct proc_dir_entry *g_pHalDir = NULL;
struct proc_dir_entry *g_pHalFile = NULL;
HT_U8 *g_pszHalutInfo = NULL;
HT_BOOL g_bSaveFile = false;

static HT_S32 _HalutOpen(struct inode *pInode, struct file *pFile);
static ssize_t _HalutWrite(struct file *pFile, const char *pszBuffer, size_t nLength, loff_t *pOffset);
static void _HalutDisplayHelp(HT_S32 s32Index);

static const struct file_operations _gtHalFops =
{
	.owner = THIS_MODULE,
	.open = _HalutOpen,
	.write = _HalutWrite,
	.read = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static HT_RESULT _HalutSendCmdToModule(HT_S32 s32Index)
{
	HT_RESULT eRet = HT_SUCCESS;
	HT_MODULE_UnitTest pfUnitTest = NULL;

	switch((E_HT_MODULE_e)s32Index)
	{
		case E_HT_MODULE_VIF:
#ifdef MOD_VIF
            pfUnitTest = HT_VIF;		// to be done
#endif
			break;
		case E_HT_MODULE_VPE:
#ifdef MOD_VPE
            pfUnitTest = HT_VPE;
            //pfUnitTest = HT_TEST;
#endif
			break;
		case E_HT_MODULE_VENC:
			//pfUnitTest = UT_VENC;		// to be done
			break;
		case E_HT_MODULE_VDEC:
#ifdef MOD_VDEC
            pfUnitTest = HT_VDEC;		// to be done
#endif
			break;
		case E_HT_MODULE_DIVP:
#ifdef MOD_DIVP
			pfUnitTest = HT_DIVP;		// to be done
#endif
            break;
		case E_HT_MODULE_DISP:
#ifdef MOD_DISP
			pfUnitTest = HT_DISP;		// to be done
#endif
			break;
		case E_HT_MODULE_AI:
#ifdef MOD_AI
			pfUnitTest = HT_AI;		// to be done
#endif
			break;
		case E_HT_MODULE_AO:
#ifdef MOD_AO
			pfUnitTest = HT_AO;		// to be done
#endif
			break;
        case E_HT_MODULE_HDMI:
#ifdef MOD_HDMI
			pfUnitTest = HT_HDMI;		// to be done
#endif
			break;
		default:
			break;
	}

	if(pfUnitTest)
	{
		eRet = pfUnitTest(_gtUtModules[s32Index].pau16CmdValue, _gtUtModules[s32Index].u8CmdCnt);
		if(HT_FAILURE == eRet)
		{
			_HalutDisplayHelp(s32Index);
		}
	}

	return eRet;
}

static HT_S32 _HalutSaveFile(const HT_U8 *pszFileName, const HT_U8 *pszBuffer, HT_S32 s32Length)
{
	HT_U8 *pszReadBuf = kmalloc((s32Length + 1) * sizeof(HT_U8), GFP_KERNEL);
	HT_S32 s32WriteBytes = 0;
	HT_S32 s32ReadBytes = 0;
	memset(pszReadBuf, 0, s32Length + 1);
	s32WriteBytes = HT_WriteFile(pszFileName, pszBuffer, s32Length, 0);
	s32ReadBytes = HT_ReadFile(pszFileName, pszReadBuf, s32WriteBytes, 0);

	if ((s32ReadBytes != s32WriteBytes) || memcmp(pszBuffer, pszReadBuf, s32Length))
	{
		printk(KERN_INFO"Save file:%s failed!\n", pszFileName);
		kfree(pszReadBuf);
		return -1;
	}

    printk(KERN_INFO"Save file:%s successfully!\n", pszFileName);
	kfree(pszReadBuf);
	return 0;
}

// check format if value is a num
static HT_RESULT _HalutCheckFormat(HT_S8 *pu8Value, HT_U16 *pu16Value)
{
	char *pu8After = NULL;
	HT_U32 u32Len = strlen(pu8Value);

	if (1 == u32Len)
	{
		if (('0' <= *pu8Value) && ('9' >= *pu8Value))
		{
			*pu16Value = (HT_U16)(*pu8Value - '0');
			//printk("Len[%d], pSrc[%s], value[%d].\n", u32Len, pu8Value, *pu16Value);
		}
		else
		{
			//printk("Len[%d], pSrc[%s].\n", u32Len, pu8Value);
			return HT_FAILURE;
		}
	}
	else
	{
		unsigned long state = simple_strtoul(pu8Value, &pu8After, 10);
		//printk("Len[%d], pSrc[%s], pAfter[%s], value[%d].\n", u32Len, pu8Value, pu8After, (HT_U16)state);

		if ((0 == state) || (pu8After != (char*)pu8Value + strlen(pu8Value)))
		{
			return HT_FAILURE;
		}
		else
		{
			*pu16Value = (HT_U16)state;
		}
	}

	return HT_SUCCESS;
}

// help info
static void _HalutDisplayHelp(HT_S32 s32Index)
{
	//printk(KERN_INFO"Usage: %s [param1] [param2] [param3] [param4] [param5]\n", _gtUtModules[s32Index].szName);
	HT_MODULE_DisplayHelp pfDisplayHelp = NULL;
	switch((E_HT_MODULE_e)s32Index)
	{
		case E_HT_MODULE_VIF:
#ifdef MOD_VIF
			pfDisplayHelp = HT_VIF_DisplayHelp;
#endif
            break;
		case E_HT_MODULE_VPE:
#ifdef MOD_VPE
            pfDisplayHelp = HT_VPE_DisplayHelp;
			//pfDisplayHelp = HT_TEST_DisplayHelp;
#endif
            break;
		case E_HT_MODULE_VENC:
			//pfDisplayHelp = HT_VENC_DisplayHelp;
			break;
		case E_HT_MODULE_VDEC:
#ifdef MOD_VDEC
            pfDisplayHelp = HT_VDEC_DisplayHelp;
#endif
            break;
		case E_HT_MODULE_DIVP:
#ifdef MOD_DIVP
            pfDisplayHelp = HT_DIVP_DisplayHelp;
#endif
            break;
		case E_HT_MODULE_DISP:
#ifdef MOD_DISP
			pfDisplayHelp = HT_DISP_DisplayHelp;
#endif
			break;
		case E_HT_MODULE_AI:
#ifdef MOD_AI
			pfDisplayHelp = HT_AI_DisplayHelp;
#endif
			break;
		case E_HT_MODULE_AO:
#ifdef MOD_AO
			pfDisplayHelp = HT_AO_DisplayHelp;
#endif
			break;
        case E_HT_MODULE_HDMI:
#ifdef MOD_HDMI
			pfDisplayHelp = HT_HDMI_DisplayHelp;
#endif
			break;
		default:
			break;
	}

	if(pfDisplayHelp)
	{
		pfDisplayHelp();
	}
}

// initial halut version info
static HT_S32 _HalutInitModuleInfo(void)
{
	g_pszHalutInfo = kmalloc((HT_MODULE_INFO_MAX_LENGTH) * sizeof(char), GFP_KERNEL);

	if (!g_pszHalutInfo)
	{
		return -ENOMEM;
	}

	memset(g_pszHalutInfo, 0, HT_MODULE_INFO_MAX_LENGTH);
	sprintf(g_pszHalutInfo, "hal uttest module - Version: %s\n%-8s%-24s%-4s\n", HT_HALUT_VERSION, "UtNode", "Values", "VarCnt");
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
	HT_S32 j = 0;
	HT_S8 *pu8Cmd = kmalloc(HT_MODULE_INFO_MAX_LENGTH + 1, GFP_KERNEL);

	seq_printf(pSeq, "%s", g_pszHalutInfo);

	for	(i =0; i < E_HT_MODULE_MAX; i++)
	{
		memset(pu8Cmd, 0, HT_MODULE_INFO_MAX_LENGTH + 1);

		if (_gtUtModules[i].u8CmdCnt > 0)
		{
		    printk("%s: CmdCnt[%d].\n", _gtUtModules[i].szName, _gtUtModules[i].u8CmdCnt);
			for (j = 0; j < _gtUtModules[i].u8CmdCnt; j++)
			{
			    printk("cmd%d: %d.\n", j, _gtUtModules[i].pau16CmdValue[j]);
				sprintf(pu8Cmd, "%s %d", pu8Cmd, _gtUtModules[i].pau16CmdValue[j]);
			}
			seq_printf(pSeq, "%-8s%-24s%4d\n", _gtUtModules[i].szName, pu8Cmd, _gtUtModules[i].u8CmdCnt);
		}
		else
		{
			seq_printf(pSeq, "%-8s%-24s%-4s\n", _gtUtModules[i].szName, "-", "   -");
		}
	}

	kfree(pu8Cmd);
	return 0;
}

static HT_S32 _HalutOpen(struct inode *pInode, struct file *pFile)
{
//	printk(KERN_INFO"halut_open.\n");
	return single_open(pFile, _HalutProcShow, pInode->i_private);
}

static ssize_t _HalutWrite(struct file *pFile, const char *pszBuffer, size_t nLength, loff_t *pOffset)
{
	HT_S8 *pszSaveCmd = kmalloc(nLength + 1, GFP_KERNEL);
	HT_S8 *pszRcvBuf = kstrdup(pszBuffer, GFP_KERNEL);
	HT_S8 *pszRefBuf = NULL;
	HT_S8 *pszRcvCmd = NULL;
	HT_S8 *pszRefCmd = NULL;
	HT_S8 *pszTemp = NULL;
	HT_U16 *pu16Cmd = NULL;
	HT_S32 i =0;
	HT_BOOL bExist = false;
	HT_BOOL bFirstCmd = false;

	if ('\n' == pszRcvBuf[nLength-1])
		pszRcvBuf[nLength-1] = '\0';

	pszRefBuf = pszRcvBuf;
	pszTemp = strsep((char**)(&pszRefBuf), " ");

	for (i = 0; i < E_HT_MODULE_MAX; i++)
	{
		if (!strcmp(pszTemp, _gtUtModules[i].szName))
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
	memset(pszSaveCmd, 0, nLength + 1);

	if (!pszRefBuf)
	{
		printk(KERN_INFO"Bad commands! too few paramaters.\n");
		_HalutDisplayHelp(i);
		goto RET;
	}

	memcpy(pszSaveCmd, pszRefBuf, strlen(pszRefBuf));
	if (_gtUtModules[i].pau16CmdValue)
	{
		kfree(_gtUtModules[i].pau16CmdValue);
	}

	_gtUtModules[i].pau16CmdValue = (HT_U16*)kmalloc(HT_HALUT_VALUE_CNT_MAX * sizeof(HT_U16), GFP_KERNEL);
	memset(_gtUtModules[i].pau16CmdValue, 0, HT_HALUT_VALUE_CNT_MAX * sizeof(HT_U16));
	pu16Cmd = _gtUtModules[i].pau16CmdValue;

	_gtUtModules[i].u8CmdCnt = 0;
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
		if (HT_SUCCESS == _HalutCheckFormat(pszTemp, pu16Cmd))
		{
			_gtUtModules[i].u8CmdCnt++;
			printk("pu16Cmd point to [%d].\n", *pu16Cmd);
			pu16Cmd++;
		}
		else
		{
			_gtUtModules[i].u8CmdCnt = 0;
			printk(KERN_INFO"Bad commands! %s is worng format.\n", pszTemp);
			_HalutDisplayHelp(i);
			goto RET;
		}
    }

	// check u8CmdCnt
	if (_gtUtModules[i].u8CmdCnt > HT_HALUT_VALUE_CNT_MAX)
	{
		_gtUtModules[i].u8CmdCnt = 0;
		printk(KERN_INFO"Bad commands! too many parameters.\n");
		_HalutDisplayHelp(i);
		goto RET;
	}

	//memcpy(_gtUtModules[i].pau16CmdValue, pszSaveCmd, (size_t)strlen((char*)pszSaveCmd));



	// send cmd to module
	if(_HalutSendCmdToModule(i))
	{
		_gtUtModules[i].u8CmdCnt = 0;
		printk(KERN_INFO"Bad commands! bad format.\n");
		_HalutDisplayHelp(i);
		goto RET;
	}

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
	return 0;
}

static void __exit _HalutExit(void)
{
	HT_U8 i = 0;
	if (g_pszHalutInfo)
	{
		kfree(g_pszHalutInfo);
	}

	for (i = 0; i < E_HT_MODULE_MAX; i++)
	{
		if (_gtUtModules[i].pau16CmdValue)
		{
			kfree(_gtUtModules[i].pau16CmdValue);
		}
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
