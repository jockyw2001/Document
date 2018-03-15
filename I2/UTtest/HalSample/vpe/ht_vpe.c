//#include <stdlib.h>
#include <linux/module.h>      
#include <linux/init.h>         
//#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include "ht_vpe.h"
//#include "cmdq_service.h"
//#include "../inc/ht_common_datatype.h"
//#include "stringTOcmd.h"
#include "../inc/ht_common.h"
#include "../inc/ht_vb_type.h"
//#include "file.h"

// #define HT_VPE_MAXCHANNEL 16
// #define HT_VPE_MAXPORT    4

// typedef enum {
    // off,
    // Little,
	// Middle,
	// Strong,
// }HT_IqParamLevel_E;

// typedef enum {
    // off,
    // Little,
	// Middle,
	// Big,
// }HT_CropParamLevel_E;


// typedef struct HT_VPE_ChannelInfo_t{
    // void *pIspCtx;                                            // HAL layer: ISP context pointer
    // void *pIqCtx;                                             // HAL layer: IQ context pointer
    // void *pSclCtx;                                            // HAL layer: SCL context pointer
    // HT_U8 u8IqParam;                                          // Channel Iq param
    // HT_U8 u8RoationParam;                                     // Channel roation parameter
    // HT_U8 u8CropParam;                                       // Channel crop param

    // HalVpeSclOutputSizeConfig_t  stPortSizeConfig[HT_VPE_MAXPORT];
    
    // HT_VB_Info_t pstVbInputInfo_y;
    // HT_VB_Info_t pstVbInputInfo_u;
    // HT_VB_Info_t pstVbInputInfo_v;
    
    // HT_VB_Info_t pstVbOutputInfo_y[HT_VPE_MAXPORT];
    // HT_VB_Info_t pstVbOutputInfo_u[HT_VPE_MAXPORT];
    // HT_VB_Info_t pstVbOutputInfo_v[HT_VPE_MAXPORT];
  
    // MI_VPE_CHANNEL         VpeCh;
// } HT_VPE_ChannelInfo_t;


// /* 
// PQ Parameter
// u8NRC_SF_STR =0~255;
// u8NRC_TF_STR=0~255;
// u8NRY_SF_STR=0~255;
// u8NRY_TF_STR=0~255;
// u8NRY_BLEND_MOTION_TH=0~15;
// u8NRY_BLEND_STILL_TH=0~15;
// u8NRY_BLEND_MOTION_WEI=0~31;
// u8NRY_BLEND_OTHER_WEI=0~31;
// u8NRY_BLEND_STILL_WEI=0~31;
// u8EdgeGain[6]=0~255;
// u8Contrast=0~255;  */

// const HalVpeIqConfig_t stIqStrongPara = {200, 200,200,200,14,14,28,28,28,200,{200,200,200,200,200,200},200};
// const HalVpeIqConfig_t stIqMiddlePara = {100, 100,100,100,10,10,15,15,15,100,{100,100,100,100,100,100},100};
// const HalVpeIqConfig_t stIqLittlePara = {50, 50,50,50,5,5,10,10,10,50,{50,50,50,50,50,50},50};
// HT_RESULT HT_Iq_ParamConfig(HT_IqParamLevel_E eIqlevel,HalVpeIqOnOff_t *pstIqOnOffParam,HalVpeIqConfig_t *pstIqParam)//Iq param
// {
// #if 0
    // if(eIqlevel==off)
    // {
		// pstIqOnOffParam.bNREn= FALSE;
		// pstIqOnOffParam.bEdgeEn=FALSE;
		// pstIqOnOffParam.bESEn=FALSE;
		// pstIqOnOffParam.bContrastEn=FALSE;
		// pstIqOnOffParam.bUVInvert=FALSE;
	// }
    // else
    // {
        // pstIqOnOffParam.bNREn=TRUE;
		// pstIqOnOffParam.bEdgeEn= TRUE;
		// pstIqOnOffParam.bESEn=TRUE;
		// pstIqOnOffParam.bContrastEn=TRUE;
		// pstIqOnOffParam.bUVInvert=TRUE;
        
    	// if(eIqlevel==Strong)
    		// pstIqParam = stIqStrongPara;
    	// else if(eIqlevel==Middle)
    		// pstIqParam = stIqMiddlePara;
    	// else if(eIqlevel==Little)
    		// pstIqParam = stIqLittlePara;
        // else
           // {
             // printf("Iq param error\r\n");
             // return HT_FAILURE;
            // }
    // }
// #endif
    // return HT_SUCCESS;
// }

// const HalSclCropWindowConfig_t stCropBigWin={0,0,1920,1080};
// const HalSclCropWindowConfig_t stCropMiddleWin={320,180,1280,720};
// const HalSclCropWindowConfig_t stCropLittleWin={640,360,640,360}; // confirm with richard.deng
// HT_RESULT HT_Crop_ParamConfig(HT_CropParamLevel_E elevel,HalSclCropWindowConfig_t *pstCropWin)//crop param
// {
	// if(elevel==Big)
		// pstCropWin = stCropBigWin;
	// else if(elevel==Middle)
		// pstCropWin = stCropMiddleWin;
	// else if(elevel==Little)
		// pstCropWin = stCropLittleWin;
	// else 
		// {
		  // printf("crop_win param error \r\n");
          // return HT_FAILURE;
       // }
    // return HT_SUCCESS;
// }

// HT_RESULT HT_Iq_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,HT_U8 u8IqParam,HalVpeIqOnOff_t  *pstIqOnOffConfig,HalVpeIqConfig_t *pstIqConfig)
// {
// #if 0
    // /**param set**/
	// HT_Iq_ParamConfig(u8IqParam,pstIqOnOffConfig,pstIqConfig);

     // /*config set*/
	// if(HT_SUCCESS!=(HalVpeIqConfig(pstVpeChannel->pIqCtx,pstIqConfig)))
    // {
       // printf("Iqconfig error\r\n");
       // return HT_FAILURE;
    // }   
	// if(HT_SUCCESS!=(HalVpeIqOnOff(pstVpeChannel->pIqCtx,pstIqOnOffConfig)))_//NR,edge...onoff
	// {
       // printf("IqOnOffConfig error\r\n");
       // return HT_FAILURE;
    // }   
// #endif
    // return HT_SUCCESS;
// }

// HT_RESULT HT_Roation_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,HT_U8 u8RoationParam,HalVpeIspRotationType_e eRoationConfig)
// {

    // return HT_SUCCESS;
// }

// HT_RESULT HT_Crop_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,HT_U8 u8CropParam,HalVpeSclCropConfig_t *pstCropConfig)
// {

    // return HT_SUCCESS;
// }

// HT_RESULT HT_InputPort_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,HalVpeIspInputConfig_t *pstInputPortConfig)
// {

    // return HT_SUCCESS;
// }

// HT_RESULT HT_OutputPort_Config(HT_VPE_ChannelInfo_t *pstVpeChannel,HalVpeSclOutputPort_e ePort)
// {

    // return HT_SUCCESS;

// }

// HT_RESULT HT_Process_Work(HT_VPE_ChannelInfo_t *pstVpeChannel,_cmd_mload_interface_s *pstCmdQInfo)
// {
    // // user read/write register : shuyuan.li
	// //-----------------------------------------------------------
	// // Trigger HW process input buffer 
	// // output_buffer: API input
	// // Caller: 1. Address 2. Port ON/OFF
	// // 3. YUV420: Y/UV  2 Address, need check with driver owner ??? u64PhyAddr[3]
	// // 4. Port3: MDWIN YUV 1 Address
	
	// return HT_SUCCESS;
			
// }

// HT_RESULT UT_ChannelConfig(HT_VPE_ChannelInfo_t *stVpeChannel,HT_U8 u8ChannelId)
// {
    
	// return HT_SUCCESS;
// }

// // HT_RESULT HT_WriteFile(HT_VPE_ChannelInfo_t *pstVpeChannel,HalVpeSclOutputPort_e eOutPort)
// // {
	// // return HT_SUCCESS;
   
// // }

// /*argv_data [0]channel ID; [1]Iq param; [2]roation param; [3]crop param*/
// // HT_RESULT UT_VPE(char *pstr)//channel more,
// // {
	// // printk("get string from virual module %s",pstr);
	// // return HT_SUCCESS;
// // }

HT_S32 UT_VPE(HT_U8 *pszCmd)
{
	printk("get cmd:[%s] from virual module.\n", pszCmd);
	return 0;
}
