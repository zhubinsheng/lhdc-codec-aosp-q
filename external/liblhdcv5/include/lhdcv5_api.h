#ifndef __LHDCV5_API_H__
#define __LHDCV5_API_H__


#ifdef __cplusplus
extern "C" {
#endif


typedef enum __LHDCV5_SAMPLE_FREQ__
{
  LHDCV5_SR_44100HZ  =  44100,
  LHDCV5_SR_48000HZ  =  48000,
  LHDCV5_SR_96000HZ  =  96000,
  LHDCV5_SR_192000HZ = 192000,
} LHDCV5BT_SAMPLE_FREQ_T;


typedef enum __LHDCV5BT_SMPL_FMT__
{
  LHDCV5BT_SMPL_FMT_S16 = 16,
  LHDCV5BT_SMPL_FMT_S24 = 24,
  LHDCV5BT_SMPL_FMT_S32 = 32,
} LHDCV5BT_SMPL_FMT_T;


typedef enum __LHDCV5_SAMPLE_FRAME__
{
  LHDCV5_SAMPLE_FRAME_5MS_44100KHZ  = 240,
  LHDCV5_SAMPLE_FRAME_5MS_48000KHZ  = 240,
  LHDCV5_SAMPLE_FRAME_5MS_96000KHZ  = 480,
  LHDCV5_SAMPLE_FRAME_5MS_192000KHZ = 960,
  LHDCV5_MAX_SAMPLE_FRAME = 960,
} LHDCV5_SAMPLE_FRAME_T;


typedef enum __LHDCV5_FRAME_DURATION__
{
  LHDCV5_FRAME_5MS   = 50,
  LHDCV5_FRAME_7P5MS = 75,
  LHDCV5_FRAME_10MS  = 100,
  LHDCV5_FRAME_1S    = 10000,
} LHDCV5_FRAME_DURATION_T;


typedef enum __LHDCV5_ENC_INTERVAL__
{
  LHDCV5_ENC_INTERVAL_10MS = 10,
  LHDCV5_ENC_INTERVAL_20MS = 20,
} LHDCV5_ENC_INTERVAL_T;


typedef enum __LHDCV5_QUALITY__
{
  LHDCV5_QUALITY_LOW0 = 0,
  LHDCV5_QUALITY_LOW1,
  LHDCV5_QUALITY_LOW2,
  LHDCV5_QUALITY_LOW3,
  LHDCV5_QUALITY_LOW4,
  LHDCV5_QUALITY_LOW,
  LHDCV5_QUALITY_MID,
  LHDCV5_QUALITY_HIGH,
  LHDCV5_QUALITY_HIGH1,
  LHDCV5_QUALITY_AUTO,
  LHDCV5_QUALITY_RESET_AUTO,
  LHDCV5_QUALITY_INVALID
} LHDCV5_QUALITY_T;


typedef enum __LHDCV5_MTU_SIZE__
{
  LHDCV5_MTU_MIN   = 300,
  LHDCV5_MTU_2MBPS = 660,
  LHDCV5_MTU_MAX   = 1023,
} LHDCV5_MTU_SIZE_T;


typedef enum __LHDCV5_VERSION__
{
  LHDCV5_VERSION_1 = 1,
  LHDCV5_VERSION_INVALID
} LHDCV5_VERSION_T;


typedef enum __LHDCV5_ENC_TYPE__ 
{
  LHDCV5_ENC_TYPE_UNKNOWN = 0,
  LHDCV5_ENC_TYPE_LHDCV5,
  LHDCV5_ENC_TYPE_INVALID
} LHDCV5_ENC_TYPE_T;


typedef enum __LHDCV5_EXT_FUNC__
{
  LHDCV5_EXT_FUNC_AR = 0,
  LHDCV5_EXT_FUNC_LARC,
  LHDCV5_EXT_FUNC_JAS,
  LHDCV5_EXT_FUNC_META,
  LHDCV5_EXT_FUNC_INVALID
} LHDCV5_EXT_FUNC_T;


typedef enum __LHDCV5_META_PARAM__
{
  LHDCV5_META_LOOP_CNT_MAX = 100,
  LHDCV5_META_LOOP_CNT_STD = 20,
  LHDCV5_META_LEN_FIXED = 8,
  LHDCV5_META_LEN_MAX = 128
} LHDCV5_META_PARAM_T;


typedef enum __LHDCV5_ABR_TYPE__ 
{
  LHDCV5_ABR_44K_RES = 0,
  LHDCV5_ABR_48K_RES,
  LHDCV5_ABR_96K_RES,
  LHDCV5_ABR_192K_RES,
  LHDCV5_ABR_INVALID
} LHDCV5_ABR_TYPE_T;


typedef struct _lhdcv5_abr_para_t 
{
  uint32_t	version;			// version of LHDC 5.0 CODEC
  uint32_t	sample_rate;		// sample rate (Hz)
  uint32_t 	bits_per_sample;	// bits per sample (bit)
  uint32_t 	bits_per_sample_ui;	// bits per sample (bit) (from UI)

  uint32_t 	upBitrateCnt;		// (ABR) up bitrate check parameter
  uint32_t 	upBitrateSum;		// (ABR) up bitrate check parameter

  uint32_t 	dnBitrateCnt;		// (ABR) down bitrate check parameter
  uint32_t 	dnBitrateSum;		// (ABR) down bitrate check parameter

  uint32_t 	lastBitrate;     	// Last bit rate (kbps) set for LHDC 5.0 encoder
  uint32_t 	qualityStatus;      // current bit rate (index) (LOW0 ~ AUTO) is same as UI setting

  //----------------------------------// Above is shared ABR parameters

} lhdcv5_abr_para_t;



typedef void * HANDLE_LHDCV5_BT;



/*
 ******************************************************************
 LHDC v5 common utilities functions group
 ******************************************************************
 */

extern int32_t lhdcv5_util_free_handle 
(
    HANDLE_LHDCV5_BT	handle
);

extern int32_t lhdcv5_util_get_mem_req 
(
    uint32_t			version,
    uint32_t			* mem_req_bytes
);

extern int32_t lhdcv5_util_get_handle 
(
    uint32_t			version,
    HANDLE_LHDCV5_BT	handle,
    uint32_t            mem_size
);

extern int32_t lhdcv5_util_get_target_bitrate
(
    HANDLE_LHDCV5_BT	handle,
    uint32_t			* bitrate
);

extern int32_t lhdcv5_util_set_target_bitrate_inx
(
    HANDLE_LHDCV5_BT 	handle,
    uint32_t 			bitrate_inx,
    uint32_t			* bitrate_inx_set,
    bool				upd_qual_status
);

extern int32_t lhdcv5_util_set_max_bitrate_inx
(
    HANDLE_LHDCV5_BT	handle,
    uint32_t			max_bitrate_inx,
    uint32_t			* max_bitrate_inx_set
);

extern int32_t lhdcv5_util_set_min_bitrate_inx
(
    HANDLE_LHDCV5_BT	handle,
    uint32_t			min_bitrate_inx,
    uint32_t			* min_bitrate_inx_set
);

extern int32_t lhdcv5_util_adjust_bitrate
(
    HANDLE_LHDCV5_BT 	handle,
    LHDCV5_ENC_TYPE_T	* enc_type_ptr,
    lhdcv5_abr_para_t	** abr_para_ptr
);

extern int32_t lhdcv5_util_reset_up_bitrate
(
    lhdcv5_abr_para_t	* handle
);

extern int32_t lhdcv5_util_reset_down_bitrate
(
    lhdcv5_abr_para_t 	* handle
);

extern int32_t lhdcv5_util_get_ext_func_state
(
    HANDLE_LHDCV5_BT 	handle,
    LHDCV5_EXT_FUNC_T 	ext_type,
    bool				* enable_ptr
);

extern int32_t lhdcv5_util_set_ext_func_state
(
    HANDLE_LHDCV5_BT 	handle,
    LHDCV5_EXT_FUNC_T 	ext_type,
    bool 				func_enable,
    uint8_t				* data_ptr,
    uint32_t			data_len,
    uint32_t			loop_cnt
);

extern int32_t lhdcv5_util_init_encoder
(
    HANDLE_LHDCV5_BT 	handle,
    uint32_t 			sampling_freq,
    uint32_t 			bits_per_sample,
    uint32_t 			bitrate_inx,
    uint32_t 			frame_duration,
    uint32_t 			mtu,
    uint32_t 			interval
);

extern int32_t lhdcv5_util_get_block_Size
(
    HANDLE_LHDCV5_BT 	handle,
    uint32_t			* block_size
);

extern int32_t lhdcv5_util_enc_process
(
    HANDLE_LHDCV5_BT 	handle,
    void				* p_pcm,
    uint32_t			pcm_bytes,
    uint8_t				* out_put,
    uint32_t			out_buf_bytes,
    uint32_t 			* written,
    uint32_t 			* out_frames
);

extern int32_t lhdcv5_util_get_bitrate
(
    uint32_t	bitrate_inx,
    uint32_t	* bitrate
);

extern int32_t lhdcv5_util_get_bitrate_inx
(
    uint32_t	bitrate,
    uint32_t	* bitrate_inx
);

extern int32_t lhdcv5_util_ar_set_gyro_pos 
(
    HANDLE_LHDCV5_BT 	handle,
    int32_t 			world_coordinate_x,
    int32_t 			world_coordinate_y,
    int32_t				world_coordinate_z
);

extern int32_t lhdcv5_util_ar_set_cfg
(
    HANDLE_LHDCV5_BT 	handle,
    int32_t 			* pos_ptr,
    uint32_t			pos_item_num,
    float 				* gain_ptr,
    uint32_t			gain_item_num,
    uint32_t	 		app_ar_enabled
);

extern int32_t lhdcv5_util_ar_get_cfg 
(
    HANDLE_LHDCV5_BT 	handle,
    int32_t 			* pos_ptr,
    uint32_t			pos_item_num,
    float 				* gain_ptr,
    uint32_t			gain_item_num
);


typedef enum __LHDCV5_FUNC_RET__
{
  LHDCV5_FRET_SUCCESS 				=	  0,
  LHDCV5_FRET_INVALID_INPUT_PARAM		=	 -1,
  LHDCV5_FRET_INVALID_HANDLE_CB		=	 -2,
  LHDCV5_FRET_INVALID_HANDLE_PARA		=	 -3,
  LHDCV5_FRET_INVALID_HANDLE_ENC		=	 -4,
  LHDCV5_FRET_INVALID_HANDLE_CBUF		=	 -5,
  LHDCV5_FRET_INVALID_HANDLE_AR		=	 -6,
  LHDCV5_FRET_INVALID_CODEC			=	 -7,
  LHDCV5_FRET_CODEC_NOT_READY			=	 -8,
  LHDCV5_FRET_AR_NOT_READY			=	 -9,
  LHDCV5_FRET_ERROR					=	-10,
  LHDCV5_FRET_BUF_NOT_ENOUGH			=	-11,
} LHDCV5_FUNC_RET_T;


#ifdef __cplusplus
}
#endif
#endif //End of __LHDCV5_API_H__
