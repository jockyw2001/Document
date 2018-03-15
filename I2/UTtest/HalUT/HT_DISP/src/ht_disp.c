#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>
#include <linux/delay.h>

#include "ht_common.h"
#include "mhal_common.h"
#include "mhal_disp.h"
#include "mhal_gfx.h"

#define HT_DISP_1920x1080_FILEPATH "/mnt/YUVFILE/YUV422_YUYV1920_1080_40.yuv"
#define HT_DISP_1280x720_FILEPATH "/mnt/YUVFILE/YUV422_YUYV1280_720_40.yuv"

#define HT_DISP_MAX_BUFFCNT 39
#define HT_DISP_INIT_BUFFCNT 10
#define HT_DISP_DEV_MAX 2
#define HT_DISP_LAYER_MAX 2
#define HT_DISP_INPUTPORT_MAX 16
#define HT_DISP_VIDEO_LAYER_INPUT_PORT_MAX 1
#define HT_DISP_VIDEO_LAYER_MAX 1
#define HT_DISP_FORCE_ENABLE_PTS_CHK 1

typedef HT_S32 HT_DISP_DEV;
typedef HT_S32 HT_DISP_LAYER;
typedef HT_S32 HT_DISP_CHN;
typedef HT_U32 HT_DISP_INPUTPORT;

typedef enum
{
    DISPLAYTIMING_MIN =0,
    DISPLAYTIMING_DACOUT_DEFAULT =0,
    DISPLAYTIMING_DACOUT_FULL_HD,
    DISPLAYTIMING_DACOUT_480I,
    DISPLAYTIMING_DACOUT_480P,
    DISPLAYTIMING_DACOUT_576I,
    DISPLAYTIMING_DACOUT_576P,//5
    DISPLAYTIMING_DACOUT_720P_50,
    DISPLAYTIMING_DACOUT_720P_60,
    DISPLAYTIMING_DACOUT_1080P_24,
    DISPLAYTIMING_DACOUT_1080P_25,
    DISPLAYTIMING_DACOUT_1080P_30,//10
    DISPLAYTIMING_DACOUT_1080I_50,
    DISPLAYTIMING_DACOUT_1080P_50,
    DISPLAYTIMING_DACOUT_1080I_60,
    DISPLAYTIMING_DACOUT_1080P_60,
    DISPLAYTIMING_DACOUT_1440P_50,
    DISPLAYTIMING_DACOUT_1470P_24,
    DISPLAYTIMING_DACOUT_1470P_30,
    DISPLAYTIMING_DACOUT_1470P_60,
    DISPLAYTIMING_DACOUT_2205P_24,
    //For 2k2k
    DISPLAYTIMING_DACOUT_2K2KP_24,//20
    DISPLAYTIMING_DACOUT_2K2KP_25,
    DISPLAYTIMING_DACOUT_2K2KP_30,
    DISPLAYTIMING_DACOUT_2K2KP_60,
    //For 4k0.5k
    DISPLAYTIMING_DACOUT_4K540P_240,
    //For 4k1k
    DISPLAYTIMING_DACOUT_4K1KP_30,
    DISPLAYTIMING_DACOUT_4K1KP_60,
    DISPLAYTIMING_DACOUT_4K1KP_120,
    //For 4k2k
    DISPLAYTIMING_DACOUT_4K2KP_24,
    DISPLAYTIMING_DACOUT_4K2KP_25,
    DISPLAYTIMING_DACOUT_4K2KP_30,//30
    DISPLAYTIMING_DACOUT_4K2KP_50,
    DISPLAYTIMING_DACOUT_4K2KP_60,
    DISPLAYTIMING_DACOUT_4096P_24,
    DISPLAYTIMING_DACOUT_4096P_25,
    DISPLAYTIMING_DACOUT_4096P_30,
    DISPLAYTIMING_DACOUT_4096P_50,
    DISPLAYTIMING_DACOUT_4096P_60,
    //For VGA OUTPUT
    DISPLAYTIMING_VGAOUT_640x480P_60,
    DISPLAYTIMING_VGAOUT_1280x720P_60,
    DISPLAYTIMING_VGAOUT_1024x768P_60,
    DISPLAYTIMING_VGAOUT_1280x1024P_60,
    DISPLAYTIMING_VGAOUT_1440x900P_60,
    DISPLAYTIMING_VGAOUT_1600x1200P_60,
    DISPLAYTIMING_VGAOUT_1920x1080P_60,//40
    //For TTL output
    DISPLAYTIMING_TTLOUT_480X272_60,
    DISPLAYTIMING_DACOUT_4K2KP_120,

    DISPLAYTIMING_MAX_NUM,
} HT_EN_DISPLAYTIMING_RES_TYPE;
typedef enum
{
    E_MAPI_LINK_TTL,                              ///< TTL  type
    E_MAPI_LINK_LVDS,                             ///< LVDS type
    E_MAPI_LINK_RSDS,                             ///< RSDS type
    E_MAPI_LINK_MINILVDS,                         ///< TCON
    E_MAPI_LINK_ANALOG_MINILVDS,                  ///< Analog TCON
    E_MAPI_LINK_DIGITAL_MINILVDS,                 ///< Digital TCON
    E_MAPI_LINK_MFC,                              ///< Ursa (TTL output to Ursa)
    E_MAPI_LINK_DAC_I,                            ///< DAC output
    E_MAPI_LINK_DAC_P,                            ///< DAC output
    E_MAPI_LINK_PDPLVDS,                          ///< For PDP(Vsync use Manually MODE)
    E_MAPI_LINK_EXT,                              /// EXT LPLL TYPE
}HT_MAPI_APIPNL_LINK_TYPE;
typedef enum
{
    E_MAPI_PNL_ASPECT_RATIO_4_3    = 0,         ///< set aspect ratio to 4 : 3
    E_MAPI_PNL_ASPECT_RATIO_WIDE,               ///< set aspect ratio to 16 : 9
    E_MAPI_PNL_ASPECT_RATIO_OTHER,              ///< resvered for other aspect ratio other than 4:3/ 16:9
}HT_MAPI_PNL_ASPECT_RATIO;
typedef enum
{
    E_MAPI_TI_10BIT_MODE = 0,
    E_MAPI_TI_8BIT_MODE = 2,
    E_MAPI_TI_6BIT_MODE = 3,
}HT_MAPI_APIPNL_TIBITMODE;
typedef enum
{
    E_MAPI_OUTPUT_10BIT_MODE = 0,//default is 10bit, becasue 8bit panel can use 10bit config and 8bit config.
    E_MAPI_OUTPUT_6BIT_MODE = 1, //but 10bit panel(like PDP panel) can only use 10bit config.
    E_MAPI_OUTPUT_8BIT_MODE = 2, //and some PDA panel is 6bit.
}HT_MAPI_APIPNL_OUTPUTFORMAT_BITMODE;
typedef enum
{
    E_MAPI_PNL_CHG_DCLK   = 0,      ///<change output DClk to change Vfreq.
    E_MAPI_PNL_CHG_HTOTAL = 1,      ///<change H total to change Vfreq.
    E_MAPI_PNL_CHG_VTOTAL = 2,      ///<change V total to change Vfreq.
} HT_MAPI_APIPNL_OUT_TIMING_MODE;

typedef enum
{
    E_HT_DISP_PIXEL_FRAME_YUV422_YUYV = 0,
    E_HT_DISP_PIXEL_FRAME_ARGB8888,
    E_HT_DISP_PIXEL_FRAME_ABGR8888,

    E_HT_DISP_PIXEL_FRAME_RGB565,
    E_HT_DISP_PIXEL_FRAME_ARGB1555,
    E_HT_DISP_PIXEL_FRAME_I2,
    E_HT_DISP_PIXEL_FRAME_I4,
    E_HT_DISP_PIXEL_FRAME_I8,

    E_HT_DISP_PIXEL_FRAME_YUV_SEMIPLANAR_422,
    E_HT_DISP_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    E_HT_DISP_PIXEL_FRAME_YUV_MST_420,

    //vdec mstar private video format
    E_HT_DISP_PIXEL_FRAME_YC420_MSTTILE1_H264,
    E_HT_DISP_PIXEL_FRAME_YC420_MSTTILE2_H265,
    E_HT_DISP_PIXEL_FRAME_YC420_MSTTILE3_H265,
    E_HT_DISP_PIXEL_FRAME_FORMAT_MAX,
} HT_DISP_PixelFormat_e;

typedef enum
{
    E_HT_DISP_INTF_CVBS = 0,
    E_HT_DISP_INTF_YPBPR,
    E_HT_DISP_INTF_VGA,
    E_HT_DISP_INTF_BT656,
    E_HT_DISP_INTF_BT1120,
    E_HT_DISP_INTF_HDMI,
    E_HT_DISP_INTF_LCD,
    E_HT_DISP_INTF_BT656_H,
    E_HT_DISP_INTF_BT656_L,
	E_HT_DISP_INTF_MAX,
}HT_DISP_Interface_e;
typedef enum
{
    E_HT_DISP_OUTPUT_PAL = 0,
    E_HT_DISP_OUTPUT_NTSC,
    E_HT_DISP_OUTPUT_960H_PAL,              /* ITU-R BT.1302 960 x 576 at 50 Hz (interlaced)*/
    E_HT_DISP_OUTPUT_960H_NTSC,             /* ITU-R BT.1302 960 x 480 at 60 Hz (interlaced)*/

    E_HT_DISP_OUTPUT_480i60,
    E_HT_DISP_OUTPUT_576i50,
    E_HT_DISP_OUTPUT_480P60,
    E_HT_DISP_OUTPUT_576P50,
    E_HT_DISP_OUTPUT_720P50,
    E_HT_DISP_OUTPUT_720P60,
    E_HT_DISP_OUTPUT_1080P24,
    E_HT_DISP_OUTPUT_1080P25,
    E_HT_DISP_OUTPUT_1080P30,
    E_HT_DISP_OUTPUT_1080I50,
    E_HT_DISP_OUTPUT_1080I60,
    E_HT_DISP_OUTPUT_1080P50,
    E_HT_DISP_OUTPUT_1080P60,

    E_HT_DISP_OUTPUT_640x480_60,            /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
    E_HT_DISP_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1024x768_60,           /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1366x768_60,           /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1440x900_60,           /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
    E_HT_DISP_OUTPUT_1280x800_60,           /* 1280*800@60Hz VGA@60Hz*/
    E_HT_DISP_OUTPUT_1680x1050_60,          /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1920x2160_30,          /* 1920x2160_30 */
    E_HT_DISP_OUTPUT_1600x1200_60,          /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
    E_HT_DISP_OUTPUT_1920x1200_60,          /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/

    E_HT_DISP_OUTPUT_2560x1440_30,          /* 2560x1440_30 */
    E_HT_DISP_OUTPUT_2560x1600_60,          /* 2560x1600_60 */
    E_HT_DISP_OUTPUT_3840x2160_30,          /* 3840x2160_30 */
    E_HT_DISP_OUTPUT_3840x2160_60,          /* 3840x2160_60 */
    E_HT_DISP_OUTPUT_USER,
    E_HT_DISP_OUTPUT_MAX,
} HT_DISP_OutputTiming_e;
typedef enum
{
    E_HT_DISP_CSC_MATRIX_BYPASS = 0,         /* do not change color space */

    E_HT_DISP_CSC_MATRIX_BT601_TO_BT709,       /* change color space from BT.601 to BT.709 */
    E_HT_DISP_CSC_MATRIX_BT709_TO_BT601,       /* change color space from BT.709 to BT.601 */

    E_HT_DISP_CSC_MATRIX_BT601_TO_RGB_PC,      /* change color space from BT.601 to RGB */
    E_HT_DISP_CSC_MATRIX_BT709_TO_RGB_PC,      /* change color space from BT.709 to RGB */

    E_HT_DISP_CSC_MATRIX_RGB_TO_BT601_PC,      /* change color space from RGB to BT.601 */
    E_HT_DISP_CSC_MATRIX_RGB_TO_BT709_PC,      /* change color space from RGB to BT.709 */

    E_HT_DISP_CSC_MATRIX_NUM
} HT_DISP_CscMattrix_e;
typedef enum
{
    E_HT_DISP_SYNC_MODE_INVALID = 0,
    E_HT_DISP_SYNC_MODE_CHECK_PTS,
    E_HT_DISP_SYNC_MODE_FREE_RUN,
    E_HT_DISP_SYNC_MODE_NUM,
} HT_DISP_SyncMode_e;
typedef enum
{
    E_HT_LAYER_INPUTPORT_STATUS_INVALID = 0,
    E_HT_LAYER_INPUTPORT_STATUS_PAUSE,
    E_HT_LAYER_INPUTPORT_STATUS_RESUME,
    E_HT_LAYER_INPUTPORT_STATUS_STEP,
    E_HT_LAYER_INPUTPORT_STATUS_REFRESH,
    E_HT_LAYER_INPUTPORT_STATUS_SHOW,
    E_HT_LAYER_INPUTPORT_STATUS_HIDE,
    E_HT_LAYER_INPUTPORT_STATUS_NUM,
} HT_DISP_InputPortStatus_e;

typedef enum
{
    E_HT_DISP_FRAME_NORMAL,
    E_HT_DISP_FRAME_WAIT,
    E_HT_DISP_FRAME_DROP,
    E_HT_DISP_FRAME_SHOW_LAST,
}HT_DISP_HandleFrame_e;
typedef enum
{
    E_HT_DISP_FRAME_TILE_MODE_NONE = 0,
    E_HT_DISP_FRAME_TILE_MODE_16x16,      // tile mode 16x16
    E_HT_DISP_FRAME_TILE_MODE_16x32,      // tile mode 16x32
    E_HT_DISP_FRAME_TILE_MODE_32x16,      // tile mode 32x16
    E_HT_DISP_FRAME_TILE_MODE_32x32,      // tile mode 32x32
    E_HT_DISP_FRAME_TILE_MODE_MAX
} HT_DISP_FrameTileMode_e;
typedef enum
{
    E_HT_DISP_COMPRESS_MODE_NONE,//no compress
    E_HT_DISP_COMPRESS_MODE_SEG,//compress unit is 256 bytes as a segment
    E_HT_DISP_COMPRESS_MODE_LINE,//compress unit is the whole line
    E_HT_DISP_COMPRESS_MODE_FRAME,//compress unit is the whole frame
    E_HT_DISP_COMPRESS_MODE_BUTT, //number
}HT_DISP_CompressMode_e;
typedef enum
{
    E_HT_DISP_FRAME_SCAN_MODE_PROGRESSIVE = 0x0,  // progessive.
    E_HT_DISP_FRAME_SCAN_MODE_INTERLACE   = 0x1,  // interlace.
    E_HT_DISP_FRAME_SCAN_MODE_MAX,
} HT_DISP_FrameScanMode_e;
typedef enum
{
    E_HT_DISP_FIELDTYPE_NONE,        //< no field.
    E_HT_DISP_FIELDTYPE_TOP,           //< Top field only.
    E_HT_DISP_FIELDTYPE_BOTTOM,    //< Bottom field only.
    E_HT_DISP_FIELDTYPE_BOTH,        //< Both fields.
    E_HT_DISP_FIELDTYPE_NUM
} HT_DISP_FieldType_e;

typedef struct HT_SYSCFG_MmapInfo_s
{
    HT_U8     u8Gid;                         // Mmap ID
    HT_U8     u8Layer;                       // Memory Layer
    HT_U8     u8MiuNo;                       // 0: MIU0 / 1: MIU1 / 2: MIU2
    HT_U8     u8CMAHid;                      // Memory CMAHID
    HT_U32    u32Addr;                       // Memory Address
    HT_U32    u32Size;                       // Memory Size
    HT_U32    u32Align;                      // Memory Align
    HT_U32    u32MemoryType;                 // Memory Type
} HT_SYSCFG_MmapInfo_t;
typedef struct
{

    HT_U16 u16tRx;
    HT_U16 u16tRy;
    HT_U16 u16tGx;
    HT_U16 u16tGy;
    HT_U16 u16tBx;
    HT_U16 u16tBy;
    HT_U16 u16tWx;
    HT_U16 u16tWy;
    HT_U16 u16MaxLuminance;
    HT_U16 u16MedLuminance;
    HT_U16 u16MinLuminance;
    HT_U8 u8ColorFormat;
    HT_U8 u8ColorDataFormat;
    HT_BOOL bIsFullRange;
    HT_BOOL bLinearRgb;
    HT_BOOL bCustomerColorPrimaries;
    HT_U16 u16SourceWx;
    HT_U16 u16SourceWy;
} HT_MAPI_PANEL_CFD_ATTRIBUTE;

typedef struct
{
    char pPanelName[128];                      ///<  PanelName

    HT_U8 bPanelDither;                ///<  PANEL_DITHER
    HT_MAPI_APIPNL_LINK_TYPE ePanelLinkType; ///<  define PANEL_LINK

    HT_U8 bPanelDualPort;             ///<  define PANEL_DUAL_PORT
    HT_U8 bPanelSwapPort;             ///<  define PANEL_SWAP_PORT
    HT_U8 bPanelSwapOdd_ML;         ///<  define PANEL_SWAP_ODD_ML
    HT_U8 bPanelSwapEven_ML;         ///<  define PANEL_SWAP_EVEN_ML
    HT_U8 bPanelSwapOdd_RB;         ///<  define PANEL_SWAP_ODD_RB
    HT_U8 bPanelSwapEven_RB;         ///<  define PANEL_SWAP_EVEN_RB

    HT_U8 bPanelSwapLVDS_POL;         //  #define PANEL_SWAP_LVDS_POL   0
    HT_U8 bPanelSwapLVDS_CH;         //  #define PANEL_SWAP_LVDS_CH    0
    HT_U8 bPanelPDP10BIT;         //  #define PANEL_PDP_10BIT       0
    HT_U8 bPanelLVDS_TI_MODE;         //  #define PANEL_LVDS_TI_MODE    _PNL_FUNC_EN_

    HT_U8 ucPanelDCLKDelay;                //  #define PANEL_DCLK_DELAY      0x00
    HT_U8 bPanelInvDCLK;             //  #define PANEL_INV_DCLK        0
    HT_U8 bPanelInvDE;                 //  #define PANEL_INV_DE          0
    HT_U8 bPanelInvHSync;             //  #define PANEL_INV_HSYNC       0
    HT_U8 bPanelInvVSync;             //  #define PANEL_INV_VSYNC       0

    HT_U8 ucPanelDCKLCurrent;              ///<  define PANEL_DCLK_CURRENT
    HT_U8 ucPanelDECurrent;                ///<  define PANEL_DE_CURRENT
    HT_U8 ucPanelODDDataCurrent;           ///<  define PANEL_ODD_DATA_CURRENT
    HT_U8 ucPanelEvenDataCurrent;          ///<  define PANEL_EVEN_DATA_CURRENT

    HT_U16 wPanelOnTiming1;                ///<  time between panel & data while turn on power
    HT_U16 wPanelOnTiming2;                ///<  time between data & back light while turn on power
    HT_U16 wPanelOffTiming1;               ///<  time between back light & data while turn off power
    HT_U16 wPanelOffTiming2;               ///<  time between data & panel while turn off power

    HT_U8 ucPanelHSyncWidth;               ///<  define PANEL_HSYNC_WIDTH
    HT_U8 ucPanelHSyncBackPorch;           ///<  define PANEL_HSYNC_BACK_PORCH

    HT_U8 ucPanelVSyncWidth;               ///<  define PANEL_VSYNC_WIDTH
    HT_U8 ucPanelVBackPorch;               ///<  define PANEL_VSYNC_BACK_PORCH

    HT_U16 wPanelHStart;                   ///<  define PANEL_HSTART (PANEL_HSYNC_WIDTH + PANEL_HSYNC_BACK_PORCH)
    HT_U16 wPanelVStart;                   ///<  define PANEL_VSTART (PANEL_VSYNC_WIDTH + PANEL_VSYNC_BACK_PORCH)
    HT_U16 wPanelWidth;                    ///<  define PANEL_WIDTH
    HT_U16 wPanelHeight;                   ///<  define PANEL_HEIGHT

    HT_U16 wPanelMaxHTotal;                ///<  define PANEL_MAX_HTOTAL
    HT_U16 wPanelHTotal;                   ///<  define PANEL_HTOTAL
    HT_U16 wPanelMinHTotal;                ///<  define PANEL_MIN_HTOTAL

    HT_U16 wPanelMaxVTotal;                ///<  define PANEL_MAX_VTOTAL
    HT_U16 wPanelVTotal;                   ///<  define PANEL_VTOTAL
    HT_U16 wPanelMinVTotal;                ///<  define PANEL_MIN_VTOTAL

    HT_U8 dwPanelMaxDCLK;                  ///<  define PANEL_MAX_DCLK
    HT_U8 dwPanelDCLK;                     ///<  define PANEL_DCLK
    HT_U8 dwPanelMinDCLK;                  ///<  define PANEL_MIN_DCLK

    HT_U16 wSpreadSpectrumStep;            ///<  Value for Spread_Spectrum_Control register(B7..3:Period,B2..0:Amplitude)
    HT_U16 wSpreadSpectrumSpan;            ///<  Value for Spread_Spectrum_Control register(B7..3:Period,B2..0:Amplitude)

    HT_U8 ucDimmingCtl;                    ///<  Initial Dimming Value
    HT_U8 ucMaxPWMVal;                     ///<  Max Dimming Value
    HT_U8 ucMinPWMVal;                     ///<  Min Dimming Value

    HT_U8 bPanelDeinterMode;         ///<  define PANEL_DEINTER_MODE
    HT_MAPI_PNL_ASPECT_RATIO ucPanelAspectRatio; ///<  Panel Aspect Ratio
    HT_U16 u16LVDSTxSwapValue;
    HT_MAPI_APIPNL_TIBITMODE ucTiBitMode;
    HT_MAPI_APIPNL_OUTPUTFORMAT_BITMODE ucOutputFormatBitMode;

    HT_U8 bPanelSwapOdd_RG;         ///<  define PANEL_SWAP_ODD_RG
    HT_U8 bPanelSwapEven_RG;         ///<  define PANEL_SWAP_EVEN_RG
    HT_U8 bPanelSwapOdd_GB;         ///<  define PANEL_SWAP_ODD_GB
    HT_U8 bPanelSwapEven_GB;         ///<  define PANEL_SWAP_EVEN_GB

    HT_U8 bPanelDoubleClk;            ///<  define Double Clock
    HT_U32 dwPanelMaxSET;                     ///<  define PANEL_MAX_SET
    HT_U32 dwPanelMinSET;                     ///<  define PANEL_MIN_SET
    HT_MAPI_APIPNL_OUT_TIMING_MODE ucOutTimingMode;   ///<Define which panel output timing change mode is used to change VFreq for same panel
	HT_U8 bPanelNoiseDith;    ///<  PAFRC mixed with noise dither disable

    HT_BOOL bPanel3DFreerunFlag;           ///<  define flag to check if this panel should force freerun or not under 3D mode    1:forcefreerun 0: use defualt mode
    HT_BOOL bPanel2DFreerunFlag;           ///<  define flag to check if this panel should force freerun or not under 2D mode    1:forcefreerun 0: use defualt mode
    HT_BOOL bPanelReverseFlag;             ///<  define flag to check if this panel should Set 3D LRSwitch or not under 3D mode  1:Set 3D LRSwitch once  0:use default LR mode
    HT_BOOL bSGPanelFlag;                  ///<  define flag to check if this panel is SG panel   1:SG panel  0: not SG panel
    HT_BOOL bXCOutput120hzSGPanelFlag;     ///<  define flag to check if this panel is scaler direct output 120hz SG panel   1:SG panel  0: not SG panel

    HT_U16 u16PanelMaxDCLK;
    HT_U16 u16PanelDCLK;
    HT_U16 u16PanelMinDCLK;
    HT_MAPI_PANEL_CFD_ATTRIBUTE stCfdAttribute;
    HT_U16 bMirrorMode;
    HT_U16 bMirrorModeH;
    HT_U16 bMirrorModeV;

} HT_MAPI_PanelType;

typedef struct
{

    HT_MAPI_PanelType PanelAttr; ///PanelType
    HT_U16 u16PanelLinkExtType;
    HT_EN_DISPLAYTIMING_RES_TYPE eTiming;
    HT_BOOL bHdmiTx;
} HT_PanelInfo_t;

typedef struct HT_DISP_SyncInfo_s
{
    HT_BOOL  bSynm;     /* sync mode(0:timing,as BT.656; 1:signal,as LCD) */
    HT_BOOL  bIop;      /* interlaced or progressive display(0:i; 1:p) */
    HT_U8    u8Intfb;   /* interlace bit width while output */

    HT_U16   u16Vact ;  /* vertical active area */
    HT_U16   u16Vbb;    /* vertical back blank porch */
    HT_U16   u16Vfb;    /* vertical front blank porch */

    HT_U16   u16Hact;   /* herizontal active area */
    HT_U16   u16Hbb;    /* herizontal back blank porch */
    HT_U16   u16Hfb;    /* herizontal front blank porch */
    HT_U16   u16Hmid;   /* bottom herizontal active area */

    HT_U16   u16Bvact;  /* bottom vertical active area */
    HT_U16   u16Bvbb;   /* bottom vertical back blank porch */
    HT_U16   u16Bvfb;   /* bottom vertical front blank porch */

    HT_U16   u16Hpw;    /* horizontal pulse width */
    HT_U16   u16Vpw;    /* vertical pulse width */

    HT_BOOL  bIdv;      /* inverse data valid of output */
    HT_BOOL  bIhs;      /* inverse horizontal synch signal */
    HT_BOOL  bIvs;      /* inverse vertical synch signal */
    HT_U32   u32FrameRate;
} HT_DISP_SyncInfo_t;

typedef struct HT_DISP_PubAttr_s
{
    HT_U32                   u32BgColor;          /* Background color of a device, in RGB format. */
    HT_DISP_Interface_e      eIntfType;         /* Type of a VO interface */
    HT_DISP_OutputTiming_e   eIntfSync;          /* Type of a VO interface timing */
    HT_DISP_SyncInfo_t       stSyncInfo;          /* Information about VO interface timings */
}HT_DISP_PubAttr_t;

typedef struct HT_DISP_DEV_Status_s
{
   HT_BOOL bDISPEnabled;
   wait_queue_head_t stWaitQueueHead;
   HT_U32 u32DevId;
   //Binded layer list, Use List for extented
   struct list_head stBindedLayer;
   HT_U32 u32SrcW; //layer width
   HT_U32 u32SrcH; //layer height
   HT_U32 u32BgColor;
   //HT_DISP_Interface_e eIntfType;
   HT_U32 u32Interface;
   HT_DISP_OutputTiming_e eDeviceTiming[E_HT_DISP_INTF_MAX + 1];
   HT_DISP_SyncInfo_t       stSyncInfo;          /* Information about VO interface timings */
   //Hdmi or Vga
   HT_DISP_CscMattrix_e eCscMatrix;
   HT_U32 u32Luma;                     /* luminance:   0 ~ 100 default: 50 */
   HT_U32 u32Contrast;                 /* contrast :   0 ~ 100 default: 50 */
   HT_U32 u32Hue;                      /* hue      :   0 ~ 100 default: 50 */
   HT_U32 u32Saturation;               /* saturation:  0 ~ 100 default: 50 */
   //vga only
   HT_U32 u32Gain;                          /* current gain of VGA signals. [0, 64). default:0x30 */
   HT_U32 u32Sharpness;                /* For VGA signals*/
   //cvbs
   HT_BOOL bCvbsEnable;
   // Hal Dev instance
   void* pstDevObj;
   //Dev List node
   struct list_head stDevNode;
   //Dev Task
   struct task_struct* pstIsrTask;
   struct task_struct* pstSendBufTask;
   //Check pts
   HT_U64 u64LastIntTimeStamp;
   HT_U64 u64CurrentIntTimeStamp;
   HT_U64 u64VsyncInterval;
}HT_DISP_DevStatus_t;
typedef struct HT_DISP_VidWin_Rect_s
{
    HT_U16 u16X;
    HT_U16 u16Y;
    HT_U16 u16Width;
    HT_U16 u16Height;
} HT_DISP_VidWinRect_t;
typedef struct HT_DISP_InputPortStatus_s
{
    HT_DISP_VidWinRect_t stDispWin;
    HT_DISP_VidWinRect_t stCropWin;
    HT_U16 u16Fence;
    HT_BOOL bGeFlip;
    HT_BOOL bPause;
    HT_BOOL bEnable;
    HT_DISP_SyncMode_e eMode;
    HT_DISP_InputPortStatus_e eStatus;
    //Check PTS
    HT_U64 u64LastFiredTimeStamp;
    HT_U64 u64LastFramePts;
    HT_U64 u64FiredDiff;
    HT_BOOL bFramePtsBefore;
    //Hal layer instance
    void* apInputObjs;
}HT_DISP_InputPortStatus_t;

typedef struct HT_DISP_FrameData_s
{
    HT_DISP_FrameTileMode_e eTileMode;
    HT_DISP_PixelFormat_e ePixelFormat;
    HT_DISP_CompressMode_e eCompressMode;
    HT_DISP_FrameScanMode_e eFrameScanMode;
    HT_DISP_FieldType_e eFieldType;
    HT_U16 u16Width;
    HT_U16 u16Height;
    HT_U32 u32Stride[2];
} HT_DISP_FrameData_t;

typedef struct HT_DISP_BufInfo_s
{
    HT_U64 u64Pts;
    HT_U64 u64SidebandMsg;
    HT_BOOL bEndOfStream;
    HT_BOOL bUsrBuf;
    HT_YUVInfo_t stYUVBufInfo;
    HT_DISP_FrameData_t stFrameData;
}HT_DISP_BufInfo_t;

typedef struct HT_DISP_LayerStatus_s
{
   //Video Layer properities
   HT_U8 u8LayerID;
   HT_BOOL  bLayerEnabled;
   HT_U16     u16LayerWidth;
   HT_U16     u16LayerHeight;
   HT_DISP_PixelFormat_e    ePixFormat;         /* Pixel format of the video layer */
   HT_DISP_VidWinRect_t     stVidLayerDispWin;                    /* Display resolution */
   HT_BOOL bCompress;
   HT_U32 u32Priority;
   HT_U32 u32Toleration;
   HT_U8 u8BindedDevID;
   //Input port status in video Layer
   HT_DISP_InputPortStatus_t astPortStatus[HT_DISP_INPUTPORT_MAX];
   //Input port pending buffer
   struct semaphore stDispLayerPendingQueueMutex;
   struct list_head stPortPendingBufQueue[HT_DISP_INPUTPORT_MAX];
   HT_DISP_BufInfo_t *pstOnScreenBufInfo[HT_DISP_INPUTPORT_MAX];
   HT_DISP_BufInfo_t *pstCurrentFiredBufInfo[HT_DISP_INPUTPORT_MAX];
   //Hal layer instance
   void* apLayerObjs;
   //Layer List node
   struct list_head stLayerNode;
   HT_BOOL bInited;
}HT_DISP_LayerStatus_t;

typedef struct HT_DISP_VideoLayerSize_s
{
    HT_U32 u16Width;
    HT_U32 u16Height;
} HT_DISP_VideoLayerSize_t;

typedef struct HT_DISP_VideoLayerAttr_s
{
    HT_DISP_VidWinRect_t     stVidLayerDispWin;                  /* Display resolution */
    HT_DISP_VideoLayerSize_t stVidLayerSize;                 /* Canvas size of the video layer */
    HT_DISP_PixelFormat_e ePixFormat;
} HT_DISP_VideoLayerAttr_t;
typedef struct HT_DISP_InputPortAttr_s
{
    HT_DISP_VidWinRect_t stDispWin;                     /* rect of video out chn */
} HT_DISP_InputPortAttr_t;

typedef struct HT_DISP_PortPendingBuf_s
{
    HT_DISP_BufInfo_t stBufInfo;
    struct list_head stPortPendingBufNode;
    HT_U64 u64GotTime;
    HT_U64 u64ReleaseTime;
}HT_DISP_PortPendingBuf_t;

typedef struct HT_DISP_TestValue_s
{
    HT_U16 u16Width;
    HT_U16 u16Height;
    HT_DISP_PixelFormat_e ePixel;
    HT_DISP_OutputTiming_e eOutputTiming;
}HT_DISP_TestValue_t;

DEFINE_SEMAPHORE(HT_DISP_globalVarSem);
#define HT_DISP_GetVarMutex()      down(&(HT_DISP_globalVarSem))
#define HT_DISP_ReleaseVarMutex()  up(&(HT_DISP_globalVarSem))

#define STATIC_ASSERT(_x_)                              \
                do {                                                \
                    char c[(_x_)?(1):(-1)];          \
                    c[0]='\0';                            \
                    c[0]= c[0];                            \
                } while (0)

#define WAKE_UP_QUEUE_IF_NECESSARY(waitqueue)  \
            do{   \
                if(waitqueue_active(&(waitqueue))) \
                    wake_up_all(&(waitqueue)); \
            }while(0)

#define ToMHAL_DISP_SyncInfo(hal, ht)\
        do \
    {\
        (hal)->bSynm = (ht)->bSynm;\
        (hal)->bIop = (ht)->bIop;\
        (hal)->u8Intfb = (ht)->u8Intfb;\
        (hal)->u16Vact  = (ht)->u16Vact ;\
        (hal)->u16Vbb = (ht)->u16Vbb;\
        (hal)->u16Vfb = (ht)->u16Vfb;\
        (hal)->u16Hact = (ht)->u16Hact;\
        (hal)->u16Hbb = (ht)->u16Hbb;\
        (hal)->u16Hfb = (ht)->u16Hfb;\
        (hal)->u16Hmid = (ht)->u16Hmid;\
        (hal)->u16Bvact = (ht)->u16Bvact;\
        (hal)->u16Bvbb = (ht)->u16Bvbb;\
        (hal)->u16Bvfb = (ht)->u16Bvfb;\
        (hal)->u16Hpw = (ht)->u16Hpw;\
        (hal)->u16Vpw = (ht)->u16Vpw;\
        (hal)->bIdv = (ht)->bIdv;\
        (hal)->bIhs = (ht)->bIhs;\
        (hal)->bIvs = (ht)->bIvs;\
        (hal)->u32FrameRate = (ht)->u32FrameRate;\
    }while(0)

static HT_U8 gu8FilledBuffCnt=0;
static HT_U8 gu8BuffIdx=0;
static HT_S64 gs64ReadFilePos=0;
static HT_BOOL _bDispInit = FALSE;

static HT_DISP_BufInfo_t gastBufInfo[HT_DISP_MAX_BUFFCNT];


static struct timer_list gstDispTimer;
static struct completion gstDispJobFinishWakeup;
static struct completion gstDispSendBuffWakeup;

static HT_DISP_TestValue_t gstDispTestValue;
static HT_DISP_BufInfo_t gstOnScreenBufInfo;
static HT_DISP_DevStatus_t* pstDevice0Param = NULL;
static HT_DISP_DevStatus_t astDevStatus[HT_DISP_DEV_MAX];
static HT_DISP_LayerStatus_t astLayerStatus[HT_DISP_LAYER_MAX];

static HT_S32 _mi_sys_MMA_Alloc(MS_U8 *pu8Name, MS_U32 size, unsigned long long *pu64PhyAddr)
{
    return HT_SUCCESS;
}
static HT_S32 _mi_sys_MMA_Free(unsigned long long u64PhyAddr)
{
    return HT_SUCCESS;
}

static HT_PxlFmt_e _HT_DISP_Convert2HtCommonPixelFmt(HT_DISP_PixelFormat_e eDispPixelFmt)
{
    HT_PxlFmt_e ePixelFmt;
    switch(eDispPixelFmt)
    {
        case E_HT_DISP_PIXEL_FRAME_YUV422_YUYV:
            ePixelFmt=E_HT_PIXEL_FORMAT_YUV422_YUYV;
            break;
        case E_HT_DISP_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            ePixelFmt=E_HT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            break;
        default:
            ePixelFmt=E_HT_PIXEL_FORMAT_YUV422_YUYV;
            break;
    }
    return ePixelFmt;
}

static MHAL_DISP_DeviceTiming_e _HT_DISP_ConvertDisp2MhalTiming(HT_DISP_OutputTiming_e eIntfSync)
{
    MHAL_DISP_DeviceTiming_e eHalDeviceTiming = E_HT_DISP_OUTPUT_MAX;
    switch(eIntfSync)
    {
        case E_HT_DISP_OUTPUT_PAL:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_PAL;
            break;
        case E_HT_DISP_OUTPUT_NTSC:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_NTSC;
            break;
        case E_HT_DISP_OUTPUT_960H_PAL:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_960H_PAL;
            break;
        case E_HT_DISP_OUTPUT_960H_NTSC:
            //eHalDeviceTiming = E_MHAL_DISP_OUTPUT_960H_NTSC;
            break;
        case E_HT_DISP_OUTPUT_480i60:
            //eHalDeviceTiming = E_MHAL_DISP_OUTPUT_PAL;
            break;
        case E_HT_DISP_OUTPUT_576i50:
            //eHalDeviceTiming = E_MHAL_DISP_OUTPUT_NTSC;
            break;
        case E_HT_DISP_OUTPUT_576P50:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_576P50;
            break;
        case E_HT_DISP_OUTPUT_720P50:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_720P50;
            break;
        case E_HT_DISP_OUTPUT_720P60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_720P60;
            break;
        case E_HT_DISP_OUTPUT_1080P24:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080P24;
            break;
        case E_HT_DISP_OUTPUT_1080P25:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080P25;
            break;
        case E_HT_DISP_OUTPUT_1080P30:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080P30;
            break;
        case E_HT_DISP_OUTPUT_1080I50:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080I50;
            break;
        case E_HT_DISP_OUTPUT_1080P50:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080P50;
            break;
        case E_HT_DISP_OUTPUT_1080P60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1080P60;
            break;
        case E_HT_DISP_OUTPUT_640x480_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_640x480_60;
            break;
        case E_HT_DISP_OUTPUT_800x600_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_800x600_60;
            break;
        case E_HT_DISP_OUTPUT_1024x768_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1024x768_60;
            break;
        case E_HT_DISP_OUTPUT_1280x1024_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1280x1024_60;
            break;
        case E_HT_DISP_OUTPUT_1366x768_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1366x768_60;
            break;
        case E_HT_DISP_OUTPUT_1440x900_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1440x900_60;
            break;
        case E_HT_DISP_OUTPUT_1280x800_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1280x800_60;
            break;
        case E_HT_DISP_OUTPUT_1680x1050_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1680x1050_60;
            break;
        case E_HT_DISP_OUTPUT_1920x2160_30:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1920x2160_30;
            break;
        case E_HT_DISP_OUTPUT_1600x1200_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1600x1200_60;
            break;
        case E_HT_DISP_OUTPUT_1920x1200_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_1920x1200_60;
            break;
        case E_HT_DISP_OUTPUT_2560x1440_30:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_2560x1440_30;
            break;
        case E_HT_DISP_OUTPUT_2560x1600_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_2560x1600_60;
            break;
        case E_HT_DISP_OUTPUT_3840x2160_30:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_3840x2160_30;
            break;
        case E_HT_DISP_OUTPUT_3840x2160_60:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_3840x2160_60;
            break;
        case E_HT_DISP_OUTPUT_USER:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_USER;
            break;
        default:
            eHalDeviceTiming = E_MHAL_DISP_OUTPUT_MAX;
            break;
    }
    return eHalDeviceTiming;
}

static HT_S32 _HT_DISP_SetPubAttr(HT_DISP_DEV DispDev, const HT_DISP_PubAttr_t *pstPubAttr)
{
    HT_S32 s32Ret = HT_FAILURE;
    MHAL_DISP_DeviceTimingInfo_t stDevTimingInfo;
    MHAL_DISP_SyncInfo_t stDispSyncInfo;
    HT_U32 u32HalDevice;

    HT_DISP_GetVarMutex();
    if(MHAL_DISP_DeviceSetBackGroundColor((astDevStatus[DispDev].pstDevObj), pstPubAttr->u32BgColor) != TRUE)
    {
        printk("Hal Set backcolor fail!!!\n");
        goto UP_DEV_MUTEX;
    }
    astDevStatus[DispDev].u32BgColor = pstPubAttr->u32BgColor;

    switch(pstPubAttr->eIntfType)
    {
        case E_HT_DISP_INTF_HDMI:
            u32HalDevice = MHAL_DISP_INTF_HDMI;
            break;
        case E_HT_DISP_INTF_CVBS:
            u32HalDevice = MHAL_DISP_INTF_CVBS;
            break;
        case E_HT_DISP_INTF_VGA:
            u32HalDevice = MHAL_DISP_INTF_VGA;
            break;
        case E_HT_DISP_INTF_LCD:
            u32HalDevice = MHAL_DISP_INTF_LCD;
            break;
        case E_HT_DISP_INTF_YPBPR:
            u32HalDevice = MHAL_DISP_INTF_YPBPR;
            break;
        default:
            u32HalDevice = MHAL_DISP_INTF_HDMI;
            break;
    }
    if(MHAL_DISP_DeviceAddOutInterface((astDevStatus[DispDev].pstDevObj), u32HalDevice) != TRUE)
    {
        printk("Hal Add Output device fail!!!\n");
        goto UP_DEV_MUTEX;
    }
    astDevStatus[DispDev].u32Interface |= (1 << pstPubAttr->eIntfType);

    memset(&stDevTimingInfo, 0, sizeof(MHAL_DISP_DeviceTimingInfo_t));
    stDevTimingInfo.eTimeType = _HT_DISP_ConvertDisp2MhalTiming(pstPubAttr->eIntfSync);
    memset(&stDispSyncInfo, 0, sizeof(stDispSyncInfo));
    stDevTimingInfo.pstSyncInfo = &stDispSyncInfo;
    ToMHAL_DISP_SyncInfo(stDevTimingInfo.pstSyncInfo, &pstPubAttr->stSyncInfo);
    if(TRUE != MHAL_DISP_DeviceSetOutputTiming((astDevStatus[DispDev].pstDevObj), u32HalDevice, &stDevTimingInfo))
    {
        printk("Hal Set Output Timing fail!!!\n");
        goto UP_DEV_MUTEX;
    }
    astDevStatus[DispDev].eDeviceTiming[pstPubAttr->eIntfType] =_HT_DISP_ConvertDisp2MhalTiming(pstPubAttr->eIntfSync);
    memcpy(&astDevStatus[DispDev].stSyncInfo, &pstPubAttr->stSyncInfo, sizeof(pstPubAttr->stSyncInfo));

    s32Ret = HT_SUCCESS;
UP_DEV_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_S32 _HT_DISP_Enable(HT_DISP_DEV DispDev)
{
    HT_S32 s32Ret = HT_FAILURE;
    HT_DISP_GetVarMutex();
    if(MHAL_DISP_DeviceEnable(astDevStatus[DispDev].pstDevObj, TRUE) != TRUE)
    {
        printk("Hal Enable Display Dev Fail!!!\n");
        goto UP_DEV_MUTEX;
    }
    astDevStatus[DispDev].bDISPEnabled = TRUE;
    s32Ret = HT_SUCCESS;
UP_DEV_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}

static HT_S32 _HT_DISP_SetVideoLayerAttr(HT_DISP_LAYER DispLayer, const HT_DISP_VideoLayerAttr_t *pstLayerAttr)
{
    HT_S32 s32Ret =HT_FAILURE;

    MHAL_DISP_VideoLayerAttr_t stHalLayerAttr;

    HT_DISP_GetVarMutex();
    memset(&stHalLayerAttr, 0, sizeof(MHAL_DISP_VideoLayerAttr_t));
    stHalLayerAttr.ePixFormat = pstLayerAttr->ePixFormat;
    stHalLayerAttr.stVidLayerDispWin.u16X = pstLayerAttr->stVidLayerDispWin.u16X;
    stHalLayerAttr.stVidLayerDispWin.u16Y = pstLayerAttr->stVidLayerDispWin.u16Y;
    stHalLayerAttr.stVidLayerDispWin.u16Height = pstLayerAttr->stVidLayerDispWin.u16Height;
    stHalLayerAttr.stVidLayerDispWin.u16Width = pstLayerAttr->stVidLayerDispWin.u16Width;
    stHalLayerAttr.stVidLayerSize.u32Width    = pstLayerAttr->stVidLayerSize.u16Width;
    stHalLayerAttr.stVidLayerSize.u32Height   = pstLayerAttr->stVidLayerSize.u16Height;
    if(!MHAL_DISP_VideoLayerSetAttr((astLayerStatus[DispLayer].apLayerObjs), &stHalLayerAttr))
    {
        printk("MHAL_DISP_VideoLayerSetAttr Fail!!!\n");
        s32Ret = HT_FAILURE;
        goto UP_DEV_LAYER_MUTEX;
    }
    astLayerStatus[DispLayer].u16LayerWidth = pstLayerAttr->stVidLayerSize.u16Width;
    astLayerStatus[DispLayer].u16LayerHeight = pstLayerAttr->stVidLayerSize.u16Height;
    astLayerStatus[DispLayer].ePixFormat = pstLayerAttr->ePixFormat;
    astLayerStatus[DispLayer].stVidLayerDispWin = pstLayerAttr->stVidLayerDispWin;
    astLayerStatus[DispLayer].bInited = TRUE;
#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
    init_MUTEX(&(astLayerStatus[DispLayer].stDispLayerPendingQueueMutex));
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
    sema_init(&(astLayerStatus[DispLayer].stDispLayerPendingQueueMutex), 1);
#endif
    s32Ret = HT_SUCCESS;
UP_DEV_LAYER_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_S32 _HT_DISP_BindVideoLayer(HT_DISP_LAYER DispLayer, HT_DISP_DEV DispDev)
{
    HT_S32 s32Ret = HT_FAILURE;
    HT_DISP_GetVarMutex();
    if(!MHAL_DISP_VideoLayerBind((astLayerStatus[DispLayer].apLayerObjs), (astDevStatus[DispDev].pstDevObj)))
    {
        printk("Bind Video Layer  Fail!!!\n");
        s32Ret = HT_FAILURE;
        goto UP_DEV_LAYER_MUTEX;
    }
    list_add_tail(&(astLayerStatus[DispLayer].stLayerNode), &(astDevStatus[DispDev].stBindedLayer));
    astLayerStatus[DispLayer].u8BindedDevID = DispDev;
    s32Ret = HT_SUCCESS;
UP_DEV_LAYER_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_S32 _HT_DISP_EnableVideoLayer(HT_DISP_LAYER DispLayer)
{
    HT_S32 s32Ret = HT_FAILURE;
    HT_U8 u8PortCount = 0;
    HT_U8 u8DevID = 0;
    MHAL_DISP_AllocPhyMem_t stHalAlloc;

    u8DevID = astLayerStatus[DispLayer].u8BindedDevID;
    HT_DISP_GetVarMutex();
    if(!MHAL_DISP_VideoLayerEnable((astLayerStatus[DispLayer].apLayerObjs), TRUE))
    {
        printk("Enable Video Layer Fail!!!\n");
        s32Ret = HT_FAILURE;
        goto UP_LAYER_MUTEX;
    }
    astLayerStatus[DispLayer].bLayerEnabled = TRUE;
    for(u8PortCount = 0; u8PortCount < HT_DISP_VIDEO_LAYER_INPUT_PORT_MAX; u8PortCount++)
    {
        if(!MHAL_DISP_InputPortCreateInstance(&stHalAlloc, (astLayerStatus[DispLayer].apLayerObjs), u8PortCount, &(astLayerStatus[DispLayer].astPortStatus[u8PortCount].apInputObjs)))
        {
            printk("InputPort Create Instance fail!!!\n");
            continue;
        }
    }
    s32Ret = HT_SUCCESS;
UP_LAYER_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_S32 _HT_DISP_SetInputPortAttr(HT_DISP_LAYER DispLayer, HT_DISP_INPUTPORT LayerInputPort, const HT_DISP_InputPortAttr_t *pstInputPortAttr)
{
    HT_S32 s32Ret = HT_FAILURE;
    MHAL_DISP_InputPortAttr_t stHalInputPortAttr;

    HT_DISP_GetVarMutex();
    memset(&stHalInputPortAttr, 0, sizeof(MHAL_DISP_InputPortAttr_t));
    stHalInputPortAttr.stDispWin.u16X = pstInputPortAttr->stDispWin.u16X;
    stHalInputPortAttr.stDispWin.u16Y = pstInputPortAttr->stDispWin.u16Y;
    stHalInputPortAttr.stDispWin.u16Height = pstInputPortAttr->stDispWin.u16Height;
    stHalInputPortAttr.stDispWin.u16Width = pstInputPortAttr->stDispWin.u16Width;
    if(!MHAL_DISP_InputPortSetAttr((astLayerStatus[DispLayer].astPortStatus[LayerInputPort].apInputObjs), &stHalInputPortAttr))
    {
        printk("MHAL_DISP_InputPortSetAttr!!!\n");
        s32Ret = HT_FAILURE;
        goto UP_PORT_MUTEX;
    }
    astLayerStatus[DispLayer].astPortStatus[LayerInputPort].stDispWin = pstInputPortAttr->stDispWin;
    s32Ret = HT_SUCCESS;
UP_PORT_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_S32 _HT_DISP_EnableInputPort (HT_DISP_LAYER DispLayer, HT_DISP_INPUTPORT LayerInputPort)
{
    HT_S32 s32Ret = HT_FAILURE;

    HT_DISP_GetVarMutex();
    if(!MHAL_DISP_InputPortEnable((astLayerStatus[DispLayer].astPortStatus[LayerInputPort].apInputObjs), TRUE))
    {
        printk("MHAL_DISP_InputPortEnable Fail!!!\n");
        s32Ret = HT_FAILURE;
        goto UP_PORT_MUTEX;
    }
    astLayerStatus[DispLayer].astPortStatus[LayerInputPort].bEnable = TRUE;
    s32Ret = HT_SUCCESS;
UP_PORT_MUTEX:
    HT_DISP_ReleaseVarMutex();
    return s32Ret;
}
static HT_BOOL _HT_DISP_checkCropInfo(HT_U8 u8LayerId,HT_U8 u8PortId)
{
    HT_BOOL bValid = TRUE;
    HT_DISP_VidWinRect_t stCropWin;
    HT_DISP_VidWinRect_t stDispWin;
    memset(&stCropWin, 0, sizeof(HT_DISP_VidWinRect_t));
    memset(&stDispWin, 0, sizeof(HT_DISP_VidWinRect_t));
    stDispWin = astLayerStatus[u8LayerId].astPortStatus[u8PortId].stDispWin;
    stCropWin = astLayerStatus[u8LayerId].astPortStatus[u8PortId].stCropWin;
    if(stCropWin.u16Width > stDispWin.u16Width)
    {
        stCropWin.u16Width = stDispWin.u16Width;
    }
    if(stCropWin.u16Height > stDispWin.u16Height)
    {
        stCropWin.u16Height = stDispWin.u16Height;
    }
    if((stCropWin.u16X > stDispWin.u16Width)
        || (stCropWin.u16Y > stDispWin.u16Height)
        || ((stCropWin.u16X + stCropWin.u16Width) > stDispWin.u16Width)
        || ((stCropWin.u16Y + stCropWin.u16Height) > stDispWin.u16Height)
        || (stCropWin.u16Width == 0)
        || (stCropWin.u16Height == 0)
        || ((stCropWin.u16Width == stDispWin.u16Width) && (stCropWin.u16Height == stDispWin.u16Height))
        )
    {
        bValid = FALSE;
    }
    return bValid;
}
static HT_S32 _HT_DISP_InitDevStatus(HT_DISP_DevStatus_t* pstDevStatus, HT_U32 u32DevId)
{
    INIT_LIST_HEAD(&pstDevStatus->stBindedLayer);
    pstDevStatus->bDISPEnabled = FALSE;
    init_waitqueue_head(&pstDevStatus->stWaitQueueHead);
    pstDevStatus->u32DevId = u32DevId;
    pstDevStatus->u32SrcW = 0; //layer width
    pstDevStatus->u32SrcH = 0; //layer height
    pstDevStatus->u32BgColor = 0;
    pstDevStatus->u32Luma = 50;
    pstDevStatus->u32Contrast = 50;
    pstDevStatus->u32Hue = 50;
    pstDevStatus->u32Saturation = 50;
    pstDevStatus->u32Gain = 0;
    pstDevStatus->u32Sharpness = 0;
    pstDevStatus->bCvbsEnable = FALSE;
    pstDevStatus->pstDevObj = NULL;
    INIT_LIST_HEAD(&pstDevStatus->stDevNode);
    pstDevStatus->pstIsrTask = NULL;
    return HT_SUCCESS;

}
static HT_S32 _HT_DISP_InitLayerStatus(HT_DISP_LayerStatus_t* pstLayerStatus, HT_U32 u32LayerId)
{
    HT_U16 u16Index = 0;
    if(pstLayerStatus == NULL)
    {
        printk("HT_DISP_InitLayerStatus Fail !!!\n");
        return HT_FAILURE;
    }

    pstLayerStatus->bCompress = false;
    pstLayerStatus->bInited = true;
    pstLayerStatus->bLayerEnabled = false;
    pstLayerStatus->ePixFormat = E_HT_DISP_PIXEL_FRAME_YUV422_YUYV;
    INIT_LIST_HEAD(&pstLayerStatus->stLayerNode);
    for(u16Index = 0; u16Index < HT_DISP_INPUTPORT_MAX; u16Index ++)
    {
        INIT_LIST_HEAD(&pstLayerStatus->stPortPendingBufQueue[u16Index]);
        pstLayerStatus->pstCurrentFiredBufInfo[u16Index] = NULL;
        pstLayerStatus->pstOnScreenBufInfo[u16Index] = NULL;
    }
    pstLayerStatus->u32Priority = 0;
    pstLayerStatus->u32Toleration = 100;
    pstLayerStatus->u8BindedDevID = 0;
    pstLayerStatus->u8LayerID = u32LayerId;

#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
    init_MUTEX(&(astLayerStatus[u32LayerId].stDispLayerPendingQueueMutex));
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
    sema_init(&(astLayerStatus[u32LayerId].stDispLayerPendingQueueMutex), 1);
#endif

    return HT_SUCCESS;

}
#if 0
static HT_BOOL _HT_display_CheckGeFlip(HT_U8 u8LayerId)
{
    HT_BOOL bRet = FALSE;
    HT_U8 u8PortCount = 0;
    for(u8PortCount = 0; u8PortCount < HT_DISP_VIDEO_LAYER_INPUT_PORT_MAX; u8PortCount++)
    {
        if(astLayerStatus[u8LayerId].astPortStatus[u8PortCount].bGeFlip)
        {
            bRet = TRUE;
        }
    }
    return bRet;
}
#endif
static HT_RESULT _HT_display_UpdateOnScreenFrame(HT_U8 u8LayerId, HT_U8 u8PortId)
{
    HT_BOOL bGeFlip = astLayerStatus[u8LayerId].astPortStatus[u8PortId].bGeFlip;
    HT_DISP_LayerStatus_t *pstTmpLayer = NULL;
#if 0
    HT_YUVInfo_t *pstYUVBufInfo;
#endif
    pstTmpLayer = &(astLayerStatus[u8LayerId]);
    if(bGeFlip)
    {
        if(pstTmpLayer->pstCurrentFiredBufInfo[u8PortId])
        {
            if(pstTmpLayer->pstOnScreenBufInfo[u8PortId])
            {
                //mi_sys_FinishBuf(pstTmpLayer->pstCurrentFiredBufInfo[u8PortId]);
            }
            else
            {
                pstTmpLayer->pstOnScreenBufInfo[u8PortId] = pstTmpLayer->pstCurrentFiredBufInfo[u8PortId];
            }
            pstTmpLayer->pstCurrentFiredBufInfo[u8PortId] = NULL;
        }
    }
    else
    {
        if(pstTmpLayer->pstCurrentFiredBufInfo[u8PortId])
        {
            if(pstTmpLayer->pstOnScreenBufInfo[u8PortId])
            {
#if 0
                pstYUVBufInfo=&pstTmpLayer->pstOnScreenBufInfo[u8PortId]->stYUVBufInfo;
                printk("OnScreenBuf phyaddr:%llx\r\n",pstYUVBufInfo->stYUV422YUYV.stVbInfo_yuv.phyAddr);
                HT_FreeYUVOneFrame(pstYUVBufInfo);
                //mi_sys_FinishBuf(pstTmpLayer->pstOnScreenBufInfo[u8PortId]);
#endif
            }
            gstOnScreenBufInfo=*(pstTmpLayer->pstCurrentFiredBufInfo[u8PortId]);
            pstTmpLayer->pstOnScreenBufInfo[u8PortId] = &gstOnScreenBufInfo;
            pstTmpLayer->pstCurrentFiredBufInfo[u8PortId] = NULL;
        }
    }
    return HT_SUCCESS;
}
static HT_BOOL _HT_display_SuitableDispWin(HT_DISP_BufInfo_t* pstBufInfo, HT_DISP_VidWinRect_t *pstDispWin)
{
    HT_BOOL bSuitable = FALSE;
    if((pstBufInfo == NULL) || (pstDispWin == NULL))
    {
        printk("NULL Pointer, mi_display_SuitableDispWin!!!\n");
        return FALSE;
    }
    bSuitable = (pstBufInfo->stFrameData.u16Width == pstDispWin->u16Width)
        && (pstBufInfo->stFrameData.u16Height == pstDispWin->u16Height);
    return bSuitable;
}
static HT_BOOL _HT_display_FlipFrame(HT_U8 u8LayerId, HT_U8 u8PortId, HT_DISP_BufInfo_t* pstFrameBufInfo)
{
    MHAL_DISP_VideoFrameData_t stHalFrameData;
    memset(&stHalFrameData, 0, sizeof(MHAL_DISP_VideoFrameData_t));
    stHalFrameData.eCompressMode = pstFrameBufInfo->stFrameData.eCompressMode;
    stHalFrameData.ePixelFormat = pstFrameBufInfo->stFrameData.ePixelFormat;
    stHalFrameData.aPhyAddr[0] = pstFrameBufInfo->stYUVBufInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr;
    stHalFrameData.u32Height = pstFrameBufInfo->stFrameData.u16Height;
    stHalFrameData.u32Width = pstFrameBufInfo->stFrameData.u16Width;
    stHalFrameData.au32Stride[0] = pstFrameBufInfo->stFrameData.u32Stride[0];
    ExecFunc(MHAL_DISP_InputPortFlip((astLayerStatus[u8LayerId].astPortStatus[u8PortId].apInputObjs), &stHalFrameData),TRUE);
    astLayerStatus[u8LayerId].astPortStatus[u8PortId].bGeFlip = FALSE;
    return TRUE;
}
static HT_RESULT _HT_DISP_GetPanelAttr(MHAL_DISP_DeviceTiming_e eOutputTiming, MHAL_DISP_PanelConfig_t *pstPanelCfg)
{
    MHAL_DISP_PanelConfig_t astPanelConfig[3];
    memset(astPanelConfig, 0, sizeof(astPanelConfig));

    astPanelConfig[0].eTiming = E_MHAL_DISP_OUTPUT_1080P60;
    astPanelConfig[0].u32OutputDev = MHAL_DISP_INTF_HDMI;
    astPanelConfig[0].stPanelAttr.m_pPanelName            = "DACOUT_1080P_60";
    astPanelConfig[0].stPanelAttr.m_bPanelDither          =   0;
    astPanelConfig[0].stPanelAttr.m_ePanelLinkType        =   8;
    astPanelConfig[0].stPanelAttr.m_bPanelDualPort        =   1;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapPort        =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapOdd_ML      =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapEven_ML     =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapOdd_RB      =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapEven_RB     =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelLVDS_TI_MODE    =   1;
    astPanelConfig[0].stPanelAttr.m_ucPanelDCLKDelay      =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelInvDCLK         =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelInvDE           =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelInvHSync        =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelInvVSync        =   0;
    astPanelConfig[0].stPanelAttr.m_ucPanelDCKLCurrent    =   1;
    astPanelConfig[0].stPanelAttr.m_ucPanelDECurrent      =   1;
    astPanelConfig[0].stPanelAttr.m_ucPanelODDDataCurrent =   1;
    astPanelConfig[0].stPanelAttr.m_ucPanelEvenDataCurrent=   1;
    astPanelConfig[0].stPanelAttr.m_wPanelOnTiming1       =   30;
    astPanelConfig[0].stPanelAttr.m_wPanelOnTiming2       =   400;
    astPanelConfig[0].stPanelAttr.m_wPanelOffTiming1      =   80;
    astPanelConfig[0].stPanelAttr.m_wPanelOffTiming2      =   30;
    astPanelConfig[0].stPanelAttr.m_ucPanelHSyncWidth     =   44;
    astPanelConfig[0].stPanelAttr.m_ucPanelHSyncBackPorch =   148;
    astPanelConfig[0].stPanelAttr.m_ucPanelVSyncWidth     =   5;
    astPanelConfig[0].stPanelAttr.m_ucPanelVBackPorch     =   36;
    astPanelConfig[0].stPanelAttr.m_wPanelHStart          =   192;
    astPanelConfig[0].stPanelAttr.m_wPanelVStart          =   0;
    astPanelConfig[0].stPanelAttr.m_wPanelWidth           =   1920;
    astPanelConfig[0].stPanelAttr.m_wPanelHeight          =   1080;
    astPanelConfig[0].stPanelAttr.m_wPanelMaxHTotal       =   2300;
    astPanelConfig[0].stPanelAttr.m_wPanelHTotal          =   2200;
    astPanelConfig[0].stPanelAttr.m_wPanelMinHTotal       =   2100;
    astPanelConfig[0].stPanelAttr.m_wPanelMaxVTotal       =   1225;
    astPanelConfig[0].stPanelAttr.m_wPanelVTotal          =   1125;
    astPanelConfig[0].stPanelAttr.m_wPanelMinVTotal       =   1025;
    astPanelConfig[0].stPanelAttr.m_dwPanelMaxDCLK        =   158;
    astPanelConfig[0].stPanelAttr.m_dwPanelDCLK           =   148;
    astPanelConfig[0].stPanelAttr.m_dwPanelMinDCLK        =   138;
    astPanelConfig[0].stPanelAttr.m_wSpreadSpectrumStep   =   25;
    astPanelConfig[0].stPanelAttr.m_wSpreadSpectrumSpan   =   192;
    astPanelConfig[0].stPanelAttr.m_ucDimmingCtl          =   160;
    astPanelConfig[0].stPanelAttr.m_ucMaxPWMVal           =   255;
    astPanelConfig[0].stPanelAttr.m_ucMinPWMVal           =   80;
    astPanelConfig[0].stPanelAttr.m_bPanelDeinterMode     =   0;
    astPanelConfig[0].stPanelAttr.m_ucPanelAspectRatio    =   1;
    astPanelConfig[0].stPanelAttr.m_ucTiBitMode           =   2;
    astPanelConfig[0].stPanelAttr.m_ucOutputFormatBitMode =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapOdd_RG      =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapEven_RG     =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapOdd_GB      =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelSwapEven_GB     =   0;
    astPanelConfig[0].stPanelAttr.m_bPanelDoubleClk       =   1;
    astPanelConfig[0].stPanelAttr.m_dwPanelMaxSET         =   0x001c848e;
    astPanelConfig[0].stPanelAttr.m_dwPanelMinSET         =   0x0018eb59;
    astPanelConfig[0].stPanelAttr.m_ucOutTimingMode       =   2;
    astPanelConfig[0].stPanelAttr.m_bPanelNoiseDith       =   0;
    astPanelConfig[0].bValid=TRUE;
    memcpy(&astPanelConfig[1],&astPanelConfig[0],sizeof(MHAL_DISP_PanelConfig_t));
    memcpy(&astPanelConfig[2],&astPanelConfig[0],sizeof(MHAL_DISP_PanelConfig_t));
    astPanelConfig[1].eTiming = E_MHAL_DISP_OUTPUT_720P60;
    astPanelConfig[1].stPanelAttr.m_pPanelName= "DACOUT_720P_60";
    astPanelConfig[1].stPanelAttr.m_bPanelDualPort=0;
    astPanelConfig[1].stPanelAttr.m_ucPanelHSyncWidth=40;
    astPanelConfig[1].stPanelAttr.m_ucPanelHSyncBackPorch=220;
    astPanelConfig[1].stPanelAttr.m_ucPanelVSyncWidth =6;
    astPanelConfig[1].stPanelAttr.m_ucPanelVBackPorch=20;
    astPanelConfig[1].stPanelAttr.m_wPanelHStart =260;
    astPanelConfig[1].stPanelAttr.m_wPanelWidth =1280;
    astPanelConfig[1].stPanelAttr.m_wPanelHeight =720;
    astPanelConfig[1].stPanelAttr.m_wPanelMaxHTotal =1750;
    astPanelConfig[1].stPanelAttr.m_wPanelHTotal =1650;
    astPanelConfig[1].stPanelAttr.m_wPanelMinHTotal=1550;
    astPanelConfig[1].stPanelAttr.m_wPanelMaxVTotal=850;
    astPanelConfig[1].stPanelAttr.m_wPanelVTotal=750;
    astPanelConfig[1].stPanelAttr.m_wPanelMinVTotal=650;
    astPanelConfig[1].stPanelAttr.m_dwPanelMaxDCLK=84;
    astPanelConfig[1].stPanelAttr.m_dwPanelDCLK=74;
    astPanelConfig[1].stPanelAttr.m_dwPanelMinDCLK=64;
    astPanelConfig[1].stPanelAttr.m_bPanelDoubleClk=0;
    astPanelConfig[1].stPanelAttr.m_dwPanelMaxSET=0x001ebcb1;
    astPanelConfig[1].stPanelAttr.m_dwPanelMinSET = 0x001770c0;

    astPanelConfig[2].eTiming = E_MHAL_DISP_OUTPUT_1024x768_60;
    astPanelConfig[2].stPanelAttr.m_pPanelName= "DACOUT_1024X768P_60";
    astPanelConfig[2].stPanelAttr.m_ucPanelHSyncWidth=136;
    astPanelConfig[2].stPanelAttr.m_ucPanelHSyncBackPorch=160;
    astPanelConfig[2].stPanelAttr.m_ucPanelVSyncWidth =6;
    astPanelConfig[2].stPanelAttr.m_ucPanelVBackPorch=29;
    astPanelConfig[2].stPanelAttr.m_wPanelHStart =296;
    astPanelConfig[2].stPanelAttr.m_wPanelWidth =1024;
    astPanelConfig[2].stPanelAttr.m_wPanelHeight =768;
    astPanelConfig[2].stPanelAttr.m_wPanelMaxHTotal =1444;
    astPanelConfig[2].stPanelAttr.m_wPanelHTotal =1344;
    astPanelConfig[2].stPanelAttr.m_wPanelMinHTotal=1244;
    astPanelConfig[2].stPanelAttr.m_wPanelMaxVTotal=906;
    astPanelConfig[2].stPanelAttr.m_wPanelVTotal=806;
    astPanelConfig[2].stPanelAttr.m_wPanelMinVTotal=706;
    astPanelConfig[2].stPanelAttr.m_dwPanelMaxDCLK=75;
    astPanelConfig[2].stPanelAttr.m_dwPanelDCLK=65;
    astPanelConfig[2].stPanelAttr.m_dwPanelMinDCLK=55;
    astPanelConfig[2].stPanelAttr.m_bPanelDoubleClk=1;
    astPanelConfig[2].stPanelAttr.m_dwPanelMaxSET=0x1EBCB1;
    astPanelConfig[2].stPanelAttr.m_dwPanelMinSET = 0x1770C0;
    //astPanelConfig[2].stPanelAttr.m_ResolutionNum = 1;
    switch(eOutputTiming)
    {
        case E_MHAL_DISP_OUTPUT_1080P60:
            memcpy(pstPanelCfg, &astPanelConfig[0], sizeof(MHAL_DISP_PanelConfig_t));
            break;
        case E_MHAL_DISP_OUTPUT_720P60:
            memcpy(pstPanelCfg, &astPanelConfig[1], sizeof(MHAL_DISP_PanelConfig_t));
            break;
        case E_MHAL_DISP_OUTPUT_1024x768_60:
            memcpy(pstPanelCfg, &astPanelConfig[2], sizeof(MHAL_DISP_PanelConfig_t));
            break;
        default:
            break;
    }
    return HT_SUCCESS;
}

static HT_S32 _HT_DISP_ConfigInit(void)
{
    HT_S32 s32Ret = HT_SUCCESS;
    HT_SYSCFG_MmapInfo_t stMmapInfo;
    MHAL_DISP_MmapType_e eMmType = E_MHAL_DISP_MMAP_MAX;
    MHAL_DISP_MmapInfo_t stMhalMmapInfo;
    MHAL_DISP_PanelConfig_t *pstPanelAttr;
    MHAL_DISP_PanelConfig_t *pstPanelAttr1;
    MHAL_DISP_DeviceTiming_e eOutputTiming = E_MHAL_DISP_OUTPUT_MAX;

    pstPanelAttr = kmalloc((sizeof(MHAL_DISP_PanelConfig_t) * E_MHAL_DISP_OUTPUT_MAX), GFP_KERNEL);
    memset(pstPanelAttr, 0, (sizeof(MHAL_DISP_PanelConfig_t) * E_MHAL_DISP_OUTPUT_MAX));
    for(eOutputTiming = E_MHAL_DISP_OUTPUT_PAL; eOutputTiming < E_MHAL_DISP_OUTPUT_MAX; eOutputTiming++)
    {
        pstPanelAttr1 = pstPanelAttr + eOutputTiming;
        _HT_DISP_GetPanelAttr(eOutputTiming, pstPanelAttr1);
    }
    if(!MHAL_DISP_InitPanelConfig(pstPanelAttr, E_MHAL_DISP_OUTPUT_MAX))
    {
        printk("MHAL_DISP_InitPanelConfig Fail \n");
        s32Ret = HT_FAILURE;
    }
    kfree(pstPanelAttr);

    memset(&stMhalMmapInfo, 0, sizeof(MHAL_DISP_MmapInfo_t));
    HT_MmapParser("E_MMAP_ID_XC_MAIN_FB", &(stMmapInfo.u32Addr), &(stMmapInfo.u32Size), &(stMmapInfo.u8MiuNo));
    eMmType = E_MHAL_DISP_MMAP_XC_MAIN;
    stMhalMmapInfo.u32Addr = stMmapInfo.u32Addr;
    stMhalMmapInfo.u32Align = stMmapInfo.u32Align;
    stMhalMmapInfo.u32MemoryType = stMmapInfo.u32MemoryType;
    stMhalMmapInfo.u32Size = stMmapInfo.u32Size;
    stMhalMmapInfo.u8CMAHid = stMmapInfo.u8CMAHid;
    stMhalMmapInfo.u8Gid = stMmapInfo.u8Gid;
    stMhalMmapInfo.u8Layer = stMmapInfo.u8Layer;
    stMhalMmapInfo.u8MiuNo = stMmapInfo.u8MiuNo;
    MHAL_DISP_InitMmapConfig(eMmType, &stMhalMmapInfo);

    memset(&stMhalMmapInfo, 0, sizeof(MHAL_DISP_MmapInfo_t));
    HT_MmapParser("E_MMAP_ID_XC_MLOAD", &(stMmapInfo.u32Addr), &(stMmapInfo.u32Size), &(stMmapInfo.u8MiuNo));
    eMmType = E_MHAL_DISP_MMAP_XC_MENULOAD;
    stMhalMmapInfo.u32Addr = stMmapInfo.u32Addr;
    stMhalMmapInfo.u32Align = stMmapInfo.u32Align;
    stMhalMmapInfo.u32MemoryType = stMmapInfo.u32MemoryType;
    stMhalMmapInfo.u32Size = stMmapInfo.u32Size;
    stMhalMmapInfo.u8CMAHid = stMmapInfo.u8CMAHid;
    stMhalMmapInfo.u8Gid = stMmapInfo.u8Gid;
    stMhalMmapInfo.u8Layer = stMmapInfo.u8Layer;
    stMhalMmapInfo.u8MiuNo = stMmapInfo.u8MiuNo;
    MHAL_DISP_InitMmapConfig(eMmType, &stMhalMmapInfo);

    return s32Ret;
}

static HT_RESULT _HT_DISP_Add_PendingBufNode(void)
{
    HT_DISP_LAYER DispLayer = 0;
    HT_DISP_INPUTPORT LayerInputPort = 0;
    HT_DISP_PortPendingBuf_t* pstPortPendingBuf = NULL;

    pstPortPendingBuf = kmalloc(sizeof(HT_DISP_PortPendingBuf_t), GFP_KERNEL);
    if(pstPortPendingBuf != NULL)
    {
        pstPortPendingBuf->stBufInfo= gastBufInfo[gu8BuffIdx++];
        if(gu8BuffIdx>=gu8FilledBuffCnt)
        {
            gu8BuffIdx=0;
        }
        down(&(astLayerStatus[DispLayer].stDispLayerPendingQueueMutex));
        list_add_tail(&(pstPortPendingBuf->stPortPendingBufNode), &astLayerStatus[DispLayer].stPortPendingBufQueue[LayerInputPort]);
        up(&(astLayerStatus[DispLayer].stDispLayerPendingQueueMutex));
        printk("send one frame buff success !!!\r\n");
    }
    else
    {
        return HT_FAILURE;
    }

    return HT_SUCCESS;
}

static int _HT_DISP_Dev_ISR_Thread(void *pdata)
{
    int ret = HT_SUCCESS;
    HT_U8 u8LayerID = 0;
    HT_U8 u8PortCount = 0;
    struct timespec sttime;
    HT_DISP_DevStatus_t* pstDispDevParam = (HT_DISP_DevStatus_t*)pdata;
    //MI_GFX_Open();
    while(!kthread_should_stop())
    {
#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
        interruptible_sleep_on_timeout(&(pstDispDevParam->stWaitQueueHead), 10);
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
        wait_event_interruptible_timeout((pstDispDevParam->stWaitQueueHead), TRUE, msecs_to_jiffies(10));
#endif
        _HT_DISP_Add_PendingBufNode();
        HT_DISP_GetVarMutex();
        if(!pstDispDevParam->bDISPEnabled)
        {
            HT_DISP_ReleaseVarMutex();
            continue;
        }
        if(!list_empty(&pstDispDevParam->stBindedLayer))
        {
            struct list_head *pstPos = NULL;
            struct list_head *n = NULL;
            HT_DISP_LayerStatus_t *pstTmpLayer = NULL;
            list_for_each_safe(pstPos, n, &pstDispDevParam->stBindedLayer)
            {
                pstTmpLayer = list_entry(pstPos, HT_DISP_LayerStatus_t, stLayerNode);
                u8LayerID = pstTmpLayer->u8LayerID;
                if(!MHAL_DISP_VideoLayerCheckBufferFired(pstTmpLayer->apLayerObjs))//_HT_display_CheckGeFlip(u8LayerID)
                {
                    continue;
                }
                for(u8PortCount = 0; u8PortCount < HT_DISP_VIDEO_LAYER_INPUT_PORT_MAX; u8PortCount++)
                {
                    down(&(pstTmpLayer->stDispLayerPendingQueueMutex));
                    if(!list_empty(&pstTmpLayer->stPortPendingBufQueue[u8PortCount]))
                    {
                        struct list_head *pstPendingBufferPos = NULL;
                        struct list_head *m = NULL;
                        HT_DISP_PortPendingBuf_t *pstTmpPendingBuf;
                        HT_DISP_BufInfo_t stTmpBufInfo;
                        HT_DISP_HandleFrame_e eHandleFrame;
                        list_for_each_safe(pstPendingBufferPos, m, &(pstTmpLayer->stPortPendingBufQueue[u8PortCount]))
                        {
                            _HT_display_UpdateOnScreenFrame(u8LayerID, u8PortCount);
                            eHandleFrame = E_HT_DISP_FRAME_NORMAL;
                            pstTmpPendingBuf = list_entry(pstPendingBufferPos, HT_DISP_PortPendingBuf_t, stPortPendingBufNode);
                            stTmpBufInfo=pstTmpPendingBuf->stBufInfo;
                            printk("get buff phyaddr:%llx buffsize:%d\r\n",stTmpBufInfo.stYUVBufInfo.stYUV422YUYV.stVbInfo_yuv.phyAddr,stTmpBufInfo.stYUVBufInfo.stYUV422YUYV.stVbInfo_yuv.u32Size);
                            list_del(pstPendingBufferPos);
                            kfree(pstTmpPendingBuf);
                            if(astLayerStatus[u8LayerID].astPortStatus[u8PortCount].bPause)
                            {
                                /*if(mi_sys_FinishBuf(pstTmpInputBuffer) != MI_SUCCESS)
                                {
                                    DISP_DBG_ERR("mi_sys_FinishBuf Fail!!!\n");
                                }*/
                                continue;
                            }
                            if(astLayerStatus[u8LayerID].astPortStatus[u8PortCount].eStatus == E_HT_LAYER_INPUTPORT_STATUS_REFRESH)
                            {
                                eHandleFrame = E_HT_DISP_FRAME_SHOW_LAST;
                            }
                            else if((astLayerStatus[u8LayerID].astPortStatus[u8PortCount].eMode == E_HT_DISP_SYNC_MODE_FREE_RUN)
                                ||(astLayerStatus[u8LayerID].astPortStatus[u8PortCount].eStatus == E_HT_LAYER_INPUTPORT_STATUS_STEP))
                            {
                                eHandleFrame = E_HT_DISP_FRAME_NORMAL;
                            }
                            else
                            {
                                //eHandleFrame = _HT_display_CheckInputPortPts(pstTmpYUVBufInfo, &(astLayerStatus[u8LayerID].astPortStatus[u8PortCount]), pstDispDevParam, astLayerStatus[u8LayerID].u32Toleration);
                            }
                            if(HT_DISP_FORCE_ENABLE_PTS_CHK)
                            {
                                if(eHandleFrame == E_HT_DISP_FRAME_DROP)
                                {
                                    /*if(mi_sys_FinishBuf(pstTmpInputBuffer) != MI_SUCCESS)
                                    {
                                        printk("mi_sys_FinishBuf Fail!!!\n");
                                    }*/
                                    continue;
                                }
                                else if(eHandleFrame == E_HT_DISP_FRAME_SHOW_LAST)
                                {
                                    if(list_empty(&(pstTmpLayer->stPortPendingBufQueue[u8PortCount])))
                                    {
                                        printk("[%s %d]No others buffer ,show Current buffer\n", __FUNCTION__, __LINE__);
                                    }
                                    else
                                    {
                                        printk("[%s %d]Has more buffer,Finish current buffer\n", __FUNCTION__, __LINE__);
                                        /*if(mi_sys_FinishBuf(pstTmpInputBuffer) != MI_SUCCESS)
                                        {
                                            printk("mi_sys_FinishBuf Fail!!!\n");
                                        }*/
                                        continue;
                                    }
                                }
                                else if(eHandleFrame == E_HT_DISP_FRAME_NORMAL)
                                {
                                    //Normal case
                                }
                            }
                            if(_HT_display_SuitableDispWin(&stTmpBufInfo, &astLayerStatus[u8LayerID].astPortStatus[u8PortCount].stDispWin))
                            {
                                HT_BOOL bPreCropSideBandAcked=TRUE;
                                memset(&sttime, 0, sizeof(&sttime));
                                //bPreCropSideBandAcked = MI_SYS_SIDEBAND_MSG_ACKED(pstTmpInputBuffer->u64SidebandMsg);
                                //BUG_ON(bPreCropSideBandAcked && MI_SYS_GET_SIDEBAND_MSG_TYPE(pstTmpInputBuffer->u64SidebandMsg)!=MI_SYS_SIDEBAND_MSG_TYPE_PREFER_CROP_RECT);
                                if(!bPreCropSideBandAcked && _HT_DISP_checkCropInfo(u8LayerID, u8PortCount) && (astLayerStatus[u8LayerID].pstOnScreenBufInfo[u8PortCount] != NULL))
                                {
                                    //mi_display_ScalingByGe(u8LayerID, u8PortCount, pstTmpInputBuffer);
                                }
                                else
                                {
                                    _HT_display_FlipFrame(u8LayerID, u8PortCount, &stTmpBufInfo);
                                    printk("display FlipFrame !!!\r\n");
                                }
                                astLayerStatus[u8LayerID].pstCurrentFiredBufInfo[u8PortCount] = &stTmpBufInfo;
                                do_posix_clock_monotonic_gettime(&sttime);
                                astLayerStatus[u8LayerID].astPortStatus[u8PortCount].u64LastFiredTimeStamp = sttime.tv_sec * 1000 * 1000 + (sttime.tv_nsec / 1000);
                                astLayerStatus[u8LayerID].astPortStatus[u8PortCount].u64LastFramePts = stTmpBufInfo.u64Pts;
                            }
                            else
                            {
                                printk("pending buffer[%d, %d] !!!\n", stTmpBufInfo.stFrameData.u16Width, stTmpBufInfo.stFrameData.u16Height);
                                printk("Video Layer %d [%d, %d] !!!\n", u8LayerID, astLayerStatus[u8LayerID].u16LayerWidth, astLayerStatus[u8LayerID].u16LayerHeight);
                                printk("Frame Not Suitable video layer= %d !!!\n", u8LayerID);
                                //mi_sys_FinishBuf(pstTmpInputBuffer);
                            }
                        }
                     }
                     up(&(pstTmpLayer->stDispLayerPendingQueueMutex));
                }
            }
        }
        HT_DISP_ReleaseVarMutex();
    }
    ret = 0;
    return ret;
}
static irqreturn_t _HT_DISP_Dev0ISR(int eIntNum, void* dev_id)
{
    HT_BOOL bEnable = FALSE;
    HT_DISP_DevStatus_t *pstDispDevParam = NULL;
    struct timespec sttime;
    pstDispDevParam = (HT_DISP_DevStatus_t*)pstDevice0Param;
    bEnable = TRUE;
    ExecFunc(MHAL_DISP_ClearDevInterrupt(pstDispDevParam->pstDevObj, &bEnable), TRUE);
    if(pstDispDevParam->bDISPEnabled)
    {
        memset(&sttime, 0, sizeof(sttime));
        do_posix_clock_monotonic_gettime(&sttime);
        if(pstDispDevParam->u64VsyncInterval == 0)
        {
            pstDispDevParam->u64CurrentIntTimeStamp = sttime.tv_sec * 1000 * 1000 + (sttime.tv_nsec / 1000);
            pstDispDevParam->u64LastIntTimeStamp = pstDispDevParam->u64CurrentIntTimeStamp;
            pstDispDevParam->u64VsyncInterval = 20 * 1000;
        }
        else
        {
            pstDispDevParam->u64LastIntTimeStamp = pstDispDevParam->u64CurrentIntTimeStamp;
            pstDispDevParam->u64CurrentIntTimeStamp = sttime.tv_sec * 1000 * 1000 + (sttime.tv_nsec / 1000);
        }
        WAKE_UP_QUEUE_IF_NECESSARY(pstDispDevParam->stWaitQueueHead);
    }
    return IRQ_HANDLED;
}

static int _HT_DISP_ReadFile_Thread(void *pdata)
{
    HT_U8 i=0;
    HT_U32 u32YUVBufSize=0;
    HT_U8 *pFilePath=NULL;
    HT_PxlFmt_e ePixel;
    struct timespec sttime;

    for(i=HT_DISP_INIT_BUFFCNT;i<HT_DISP_MAX_BUFFCNT;i++)
    {
        ePixel=_HT_DISP_Convert2HtCommonPixelFmt(gstDispTestValue.ePixel);
        gastBufInfo[i].stYUVBufInfo.ePixelFormat=ePixel;
        HT_MallocYUVOneFrame(&gastBufInfo[i].stYUVBufInfo,gstDispTestValue.u16Width,gstDispTestValue.u16Height);
        do_posix_clock_monotonic_gettime(&sttime);
        gastBufInfo[i].u64Pts = sttime.tv_sec * 1000 * 1000 + (sttime.tv_nsec / 1000);
        gastBufInfo[i].stFrameData.u16Width=gstDispTestValue.u16Width;
        gastBufInfo[i].stFrameData.u16Height=gstDispTestValue.u16Height;
        gastBufInfo[i].stFrameData.u32Stride[0]=gstDispTestValue.u16Width*2;
        gastBufInfo[i].stFrameData.ePixelFormat=E_HT_DISP_PIXEL_FRAME_YUV422_YUYV;
        gastBufInfo[i].stFrameData.eFrameScanMode=E_HT_DISP_FRAME_SCAN_MODE_PROGRESSIVE;
        u32YUVBufSize=gstDispTestValue.u16Width*gstDispTestValue.u16Height*2;
        printk("malloc one frame buffer success buffnum:%d\r\n",i+1);
        if(gstDispTestValue.u16Width==1920)
            pFilePath=HT_DISP_1920x1080_FILEPATH;
        else if(gstDispTestValue.u16Width==1280)
            pFilePath=HT_DISP_1280x720_FILEPATH;
        if(HT_ReadYUVOneFrameToBuffer(pFilePath,&gastBufInfo[i].stYUVBufInfo,&gs64ReadFilePos) == HT_SUCCESS)
        {
            printk("read one frame to buff success\r\n");
        }
        else
        {
            printk("read one frame to buff failed\r\n");
            return HT_FAILURE;
        }
        gu8FilledBuffCnt++;
    }
    return HT_SUCCESS;
}

static void _HT_DISP_TimerISR(HT_VIRT ulArg)
{
    mod_timer(&gstDispTimer,jiffies+HZ/100);//set 10ms timeout
    complete(&gstDispSendBuffWakeup);
}

static HT_RESULT _HT_DISP_Init(void)
{
    HT_U8 i=0;
    HT_PxlFmt_e ePixel;
    HT_U8 *pFilePath=NULL;
    HT_U32 u32YUVBufSize=0;
    struct timespec sttime;
    HT_DISP_DEV DispDev = 0;
    HT_DISP_LAYER DispLayer = 0;
    MHAL_DISP_AllocPhyMem_t stHalAlloc;

    if(_bDispInit)
    {
        printk("MI_DISPLAY already Inited, return ok !\n");
        return HT_SUCCESS;
    }

    _HT_DISP_ConfigInit();

    pstDevice0Param = &(astDevStatus[DispDev]);
    memset(pstDevice0Param, 0, sizeof(*pstDevice0Param));
    if(_HT_DISP_InitDevStatus(pstDevice0Param, DispDev) != HT_SUCCESS)
    {
        printk("_HT_DISP_InitDevStatus FAILED\r\n");
        return HT_FAILURE;
    }
    if(_HT_DISP_InitLayerStatus(&astLayerStatus[DispLayer],DispLayer)!=HT_SUCCESS)
    {
        printk("_HT_DISP_InitLayerStatus FAILED\r\n");
        return HT_FAILURE;
    }
    stHalAlloc.free = _mi_sys_MMA_Free;
    stHalAlloc.alloc = _mi_sys_MMA_Alloc;
    ExecFunc(MHAL_DISP_DeviceCreateInstance(&stHalAlloc, DispDev, &(astDevStatus[DispDev].pstDevObj)),TRUE);
    stHalAlloc.free = _mi_sys_MMA_Free;
    stHalAlloc.alloc = _mi_sys_MMA_Alloc;
    ExecFunc(MHAL_DISP_VideoLayerCreateInstance(&stHalAlloc, 0, &(astLayerStatus[0].apLayerObjs)), TRUE);

    for(i=0;i<HT_DISP_INIT_BUFFCNT;i++)
    {
        ePixel=_HT_DISP_Convert2HtCommonPixelFmt(gstDispTestValue.ePixel);
        gastBufInfo[i].stYUVBufInfo.ePixelFormat=ePixel;
        HT_MallocYUVOneFrame(&gastBufInfo[i].stYUVBufInfo,gstDispTestValue.u16Width,gstDispTestValue.u16Height);
        do_posix_clock_monotonic_gettime(&sttime);
        gastBufInfo[i].u64Pts = sttime.tv_sec * 1000 * 1000 + (sttime.tv_nsec / 1000);
        gastBufInfo[i].stFrameData.u16Width=gstDispTestValue.u16Width;
        gastBufInfo[i].stFrameData.u16Height=gstDispTestValue.u16Height;
        gastBufInfo[i].stFrameData.u32Stride[0]=gstDispTestValue.u16Width*2;
        gastBufInfo[i].stFrameData.ePixelFormat=E_HT_DISP_PIXEL_FRAME_YUV422_YUYV;
        gastBufInfo[i].stFrameData.eFrameScanMode=E_HT_DISP_FRAME_SCAN_MODE_PROGRESSIVE;
        u32YUVBufSize=gstDispTestValue.u16Width*gstDispTestValue.u16Height*2;
        printk("malloc one frame buffer success buffnum:%d\r\n",i+1);
        if(gstDispTestValue.u16Width==1920)
            pFilePath=HT_DISP_1920x1080_FILEPATH;
        else if(gstDispTestValue.u16Width==1280)
            pFilePath=HT_DISP_1280x720_FILEPATH;
        if(HT_ReadYUVOneFrameToBuffer(pFilePath,&gastBufInfo[i].stYUVBufInfo,&gs64ReadFilePos) == HT_SUCCESS)
        {
            printk("read one frame to buff success\r\n");
        }
        else
        {
            printk("read one frame to buff failed\r\n");
        }
    }
    gu8FilledBuffCnt=HT_DISP_INIT_BUFFCNT;
    _bDispInit = TRUE;
    return HT_SUCCESS;
}

static HT_RESULT _HT_DISP_DeInit(void)
{
    HT_DISP_DEV DispDev = 0;
    HT_DISP_LAYER DispLayer = 0;
    HT_DISP_INPUTPORT LayerInputPort = 0;
    if(_bDispInit == FALSE)
    {
        printk("HT_DISPLAY already deinit, return ok !\n");
        return HT_SUCCESS;
    }
    MHAL_DISP_GetDevIrq(astDevStatus[0].pstDevObj, &DispDev);
    free_irq(DispDev, pstDevice0Param);
    kthread_stop(pstDevice0Param->pstIsrTask);
    kthread_stop(pstDevice0Param->pstSendBufTask);
    MHAL_DISP_DeviceDestroyInstance(astDevStatus[DispDev].pstDevObj);
    MHAL_DISP_InputPortDestroyInstance(astLayerStatus[DispLayer].apLayerObjs);
    MHAL_DISP_InputPortDestroyInstance(astLayerStatus[DispLayer].astPortStatus[LayerInputPort].apInputObjs);
    memset(pstDevice0Param, 0, sizeof(*pstDevice0Param));
    pstDevice0Param = NULL;
    del_timer_sync(&gstDispTimer);
    _bDispInit = FALSE;
    return HT_SUCCESS;
}
static void _HT_DISP_InitVar(void)
{
    gu8BuffIdx=0;
    gu8FilledBuffCnt=0;
    gs64ReadFilePos=0;
    _bDispInit=0;
    memset(gastBufInfo,0,sizeof(gastBufInfo));
    init_completion(&gstDispJobFinishWakeup);
    init_completion(&gstDispSendBuffWakeup);
    init_timer(&gstDispTimer);
    gstDispTimer.function=_HT_DISP_TimerISR;
    add_timer(&gstDispTimer);
}
static void _HT_DISP_ParseStrings(HT_U16 *pau16CmdValue,HT_U8 u8CmdCnt,HT_DISP_TestValue_t *pstDispTestValue)
{
    pstDispTestValue->u16Width=pau16CmdValue[0];
    pstDispTestValue->u16Height=pau16CmdValue[1];
    switch(pau16CmdValue[2])
    {
        case 0:
            pstDispTestValue->ePixel=E_HT_DISP_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            break;
        case 1:
            pstDispTestValue->ePixel=E_HT_DISP_PIXEL_FRAME_YUV422_YUYV;
            break;
        default:
            pstDispTestValue->ePixel=E_HT_DISP_PIXEL_FRAME_YUV422_YUYV;
            break;
    }
    switch(pau16CmdValue[3])
    {
        case 0:
            pstDispTestValue->eOutputTiming=E_HT_DISP_OUTPUT_720P60;
            break;
        case 1:
            pstDispTestValue->eOutputTiming=E_HT_DISP_OUTPUT_1024x768_60;
            break;
        case 2:
            pstDispTestValue->eOutputTiming=E_HT_DISP_OUTPUT_1080P60;
            break;
        default:
            pstDispTestValue->eOutputTiming=E_HT_DISP_OUTPUT_1080P60;
            break;
    }

}
void HT_DISP_DisplayHelp(void)
{

}
HT_RESULT HT_DISP(HT_U16 *pau16CmdValue, HT_U8 u8CmdCnt)
{
    HT_DISP_DEV DispDev = 0;
    HT_DISP_LAYER DispLayer = 0;
    HT_DISP_INPUTPORT LayerInputPort = 0;
    HT_U32 u32DevIrq = 0;
    HT_DISP_PubAttr_t stPubAttr;
    HT_DISP_VideoLayerAttr_t stLayerAttr;
    HT_DISP_InputPortAttr_t stInputPortAttr;

    _HT_DISP_ParseStrings(pau16CmdValue,u8CmdCnt,&gstDispTestValue);
    _HT_DISP_InitVar();
    _HT_DISP_Init();

    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = gstDispTestValue.eOutputTiming;
    stPubAttr.eIntfType = E_HT_DISP_INTF_HDMI;
    _HT_DISP_SetPubAttr(DispDev,  &stPubAttr);
    _HT_DISP_Enable(DispDev);

    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    stLayerAttr.ePixFormat=gstDispTestValue.ePixel;
    stLayerAttr.stVidLayerSize.u16Width = gstDispTestValue.u16Width;
    stLayerAttr.stVidLayerSize.u16Height= gstDispTestValue.u16Height;
    stLayerAttr.stVidLayerDispWin.u16X = 0;
    stLayerAttr.stVidLayerDispWin.u16Y = 0;
    stLayerAttr.stVidLayerDispWin.u16Width = 1920;
    stLayerAttr.stVidLayerDispWin.u16Height = 1080;
    _HT_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
    _HT_DISP_BindVideoLayer(DispLayer, DispDev);
    _HT_DISP_EnableVideoLayer(DispLayer);

    memset(&stInputPortAttr,0, sizeof(stInputPortAttr));
    stInputPortAttr.stDispWin.u16X=0;
    stInputPortAttr.stDispWin.u16Y=0;
    stInputPortAttr.stDispWin.u16Width = gstDispTestValue.u16Width;
    stInputPortAttr.stDispWin.u16Height = gstDispTestValue.u16Height;
    _HT_DISP_SetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr);
    _HT_DISP_EnableInputPort(DispLayer, LayerInputPort);

    pstDevice0Param->pstSendBufTask = kthread_run(_HT_DISP_ReadFile_Thread,NULL,"dispreadfile");
    pstDevice0Param->pstIsrTask = kthread_run(_HT_DISP_Dev_ISR_Thread, pstDevice0Param, "Dev0IsrThread");
    MHAL_DISP_GetDevIrq(astDevStatus[0].pstDevObj, &u32DevIrq);
    request_irq(u32DevIrq, _HT_DISP_Dev0ISR, IRQF_SHARED | IRQF_ONESHOT, "mi_disp_isr", pstDevice0Param);

    //mod_timer(&gstDispTimer,jiffies+HZ/100);//set 10ms timeout

    wait_for_completion(&gstDispJobFinishWakeup);

    _HT_DISP_DeInit();
    return HT_SUCCESS;
}


