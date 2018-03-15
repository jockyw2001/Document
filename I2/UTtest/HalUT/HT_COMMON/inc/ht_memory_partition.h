////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HT_MEMORY_PARTITION_H__
#define __HT_MEMORY_PARTITION_H__


#define SCA_TOOL_VERSION            "SN SCA V3.0.2 "

////////////////////////////////////////////////////////////////////////////////
// DRAM memory map
//
// Every Module Memory Mapping need 4 define,
// and check code in "msAPI_Memory_DumpMemoryMap"
// 1. XXX_AVAILABLE : For get avaialble memory start address
// 2. XXX_ADR       : Real Address with Alignment
// 3. XXX_GAP_CHK   : For Check Memory Gap, for avoid memory waste
// 4. XXX_LEN       : For the Memory size of this Module usage
////////////////////////////////////////////////////////////////////////////////
#define ENABLE_MIU_1                0
#define ENABLE_MIU_2                0
#define MIU_DRAM_LEN                0x0010000000
#define MIU_DRAM_LEN0               0x0010000000
#define MIU_DRAM_LEN1               0x0000000000
#define MIU_DRAM_LEN2               0x0000000000
#define MIU_INTERVAL                0x0010000000
#define CPU_ALIGN                   0x0000001000

////////////////////////////////////////////////////////////////////////////////
//MIU SETTING
////////////////////////////////////////////////////////////////////////////////
#define MIU0_GROUP_SELMIU                        0000:0000:0000:0000:0000:0000
#define MIU0_GROUP_PRIORITY                        1:0:2:3
#define MIU1_GROUP_SELMIU                        5016:0201:1280:80B8:0004:F61F
#define MIU1_GROUP_PRIORITY                        1:0:2:3
#define MIU2_GROUP_SELMIU                        0000:0000:0000:0000:0000:0000
#define MIU2_GROUP_PRIORITY                        0:0:0:0
////////////////////////////////////////////////////////////////////////////////
//MEMORY TYPE
////////////////////////////////////////////////////////////////////////////////
#define MIU0                        (0x0000)
#define MIU1                        (0x0001)
#define MIU2                        (0x0002)

#define TYPE_NONE                   (0x0000 << 2)

#define UNCACHE                     (0x0001 << 2)
#define REMAPPING_TO_USER_SPACE     (0x0002 << 2)
#define CACHE                       (0x0004 << 2)
#define NONCACHE_BUFFERABLE         (0x0008 << 2)


#define CMA                         (0x0010 << 2)


/* E_MMAP_ID_DUMMY5   */
#define E_MMAP_ID_DUMMY5_LAYER                                 0
#define E_MMAP_ID_DUMMY5_AVAILABLE                             0x000D800000
#define E_MMAP_ID_DUMMY5_ADR                                   0x000D800000  //Alignment 0
#define E_MMAP_ID_DUMMY5_GAP_CHK                               0x0000000000
#define E_MMAP_ID_DUMMY5_LEN                                   0x0002800000
#define E_MMAP_ID_DUMMY5_MEMORY_TYPE                           (MIU0 | TYPE_NONE | UNCACHE | TYPE_NONE)
#define E_MMAP_ID_DUMMY5_CMA_HID                               0

#define MSTAR_MIU0_BUS_BASE                      0x20000000UL
/*there is no miu1 TLB at k6,so we can't access 0xC0000000(address of miu1 tlb,which is used by us to replace accessing 0x60000000) anymore
*Instead, we need to directly access address of miu1(0x60000000)
*/
#define MSTAR_MIU1_BUS_BASE                      0x60000000UL

#define ARM_MIU0_BUS_BASE                        MSTAR_MIU0_BUS_BASE
#define ARM_MIU1_BUS_BASE                        MSTAR_MIU1_BUS_BASE
#define ARM_MIU2_BUS_BASE                        0xFFFFFFFFUL
#define ARM_MIU3_BUS_BASE                        0xFFFFFFFFUL

#define ARM_MIU0_BASE_ADDR                       0x00000000UL
#define ARM_MIU1_BASE_ADDR                       0x80000000UL
#define ARM_MIU2_BASE_ADDR                       0xFFFFFFFFUL
#define ARM_MIU3_BASE_ADDR                       0xFFFFFFFFUL


#define MIU0_BUS_OFFSET 	ARM_MIU0_BUS_BASE
#define MIU1_BUS_OFFSET   ARM_MIU1_BUS_BASE
#define MIU2_BUS_OFFSET   ARM_MIU2_BUS_BASE
#define MIU1_INTERVAL 		(ARM_MIU1_BUS_BASE - ARM_MIU0_BUS_BASE)


#endif // #ifndef __HT_MEMORY_PARTITION_H__