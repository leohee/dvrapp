/** ===========================================================================
* @file mem_mng.h
*
* @path $(IPNCPATH)\multimedia\encode_stream\stream
*
* @desc 
* .
*
* =========================================================================== */

#ifndef __MEM_MNG__
#define __MEM_MNG__

#define MJPG_CACHE_SIZE 			(0x200000)
/* size of 3 GOP */
#define MP4_CACHE_SIZE 				(0x100000)
#define MP4_EXT_CACHE_SIZE 			(0x50000)
#define AUDIO_CACHE_SIZE 			(0x2000)


#define MJPG_CACHE_BLK_SIZE 		(100*1024)
#define MP4_CACHE_BLK_SIZE 			(30*1024)
#define MP4_EXT_CACHE_BLK_SIZE 		(10*1024)
#define AUDIO_CACHE_BLK_SIZE 		(1*512)

#define CACHE_MEM_SIZE	(MJPG_CACHE_SIZE+MP4_CACHE_SIZE+MP4_EXT_CACHE_SIZE+AUDIO_CACHE_SIZE)


#define MJPG_MEM_SIZE 				(0x200000)
#define MP4_MEM_SIZE 				(0x100000)
#define MP4_EXT_MEM_SIZE 			(0x50000)
#define AUDIO_MEM_SIZE 				(0x2000)

#define MJPG_BLK_SIZE				(100*1024)
#define MP4_BLK_SIZE				(30*1024)
#define MP4_EXT_BLK_SIZE			(10*1024)

#define AUDIO_BLK_SIZE				(1*512)


#define MP4_1_EXTRA_SIZE				(128)
#define MP4_2_EXTRA_SIZE				(128)
#define TOTAL_EXTRA_SIZE				(MP4_1_EXTRA_SIZE+MP4_2_EXTRA_SIZE)

#define TOTAL_MEM_SIZE 	(MJPG_MEM_SIZE+MP4_MEM_SIZE+MP4_EXT_MEM_SIZE+AUDIO_MEM_SIZE+CACHE_MEM_SIZE+TOTAL_EXTRA_SIZE)

#define SHMEM_KEY	(0x4321)




enum{
	VIDEO_MJPG = 0,
	VIDEO_MP4,
	AUDIO_G711
};


enum{
	DUMMY_FRAME = (-2),
	EMPTY_FRAME = (-1),
	AUDIO_FRAME = 0,
	I_FRAME,
	P_FRAME,
	B_FRAME,
	END_FRAME_TYPE
};

enum{
	VIDOE_INFO_CH1 = 0,
	VIDOE_INFO_CH2,
	VIDOE_INFO_CH3,
	VIDOE_INFO_CH4,
 //RTSP_EXT_CHANNEL	
	VIDOE_INFO_CH5,
	VIDOE_INFO_CH6,
 	VIDOE_INFO_CH7,
	VIDOE_INFO_CH8,
	VIDOE_INFO_CH9,
	VIDOE_INFO_CH10,
	VIDOE_INFO_CH11,
	VIDOE_INFO_CH12,
	VIDOE_INFO_CH13,
	VIDOE_INFO_CH14,
	VIDOE_INFO_CH15,
	VIDOE_INFO_CH16,
	
	AUDIO_INFO_G711_CH1,
	AUDIO_INFO_G711_CH2,
	AUDIO_INFO_G711_CH3,
	AUDIO_INFO_G711_CH4,
	AUDIO_INFO_G711_CH5,
	AUDIO_INFO_G711_CH6,
	AUDIO_INFO_G711_CH7,
	AUDIO_INFO_G711_CH8,
	AUDIO_INFO_G711_CH9,
	AUDIO_INFO_G711_CH10,
	AUDIO_INFO_G711_CH11,
	AUDIO_INFO_G711_CH12,
	AUDIO_INFO_G711_CH13,
	AUDIO_INFO_G711_CH14,
	AUDIO_INFO_G711_CH15,
	AUDIO_INFO_G711_CH16,
	VIDOE_INFO_END	
};


enum{
	CACHE_PARM_ERR = -100,
	CACHE_NOMEM,
	CACHE_MEM_FAIL,
	CACHE_ERR,
	CACHE_OK = 0,

};

enum{
	EMPTY_CACHE = (-1),
	I_FRAME_CACHE = I_FRAME,
	P_FRAME_CACHE = P_FRAME,
	B_FRAME_CACHE = B_FRAME,
	END_CACHE_TYPE
};

enum{
	MEM_LAYOUT_DEFAULT = 0,
	MEM_LAYOUT_1 = 0,
	MEM_LAYOUT_2,
	MEM_LAYOUT_NUM
};



typedef struct _CACHE_DATA_INFO{
	int 			serial;
	int 			fram_type;
	unsigned long 	start_addr;
	unsigned long 	start_phy;
	int				realsize;
	int 			flag;
	unsigned int		timestamp;
	int				ref_serial[VIDOE_INFO_END];
}CACHE_DATA_INFO;


typedef struct _CACHE_DATA{
	int 	serial;
	int 	fram_type;
	int		blkindex;
	int		blks;
	int		realsize;
	int		cnt;
	int		flag;
	unsigned int	timestamp;
	int		ref_serial[VIDOE_INFO_END];
}CACHE_DATA;

typedef struct _CACHE_BLK{
	int IsFree;
	int offset;
	int serial;
}CACHE_BLK;

typedef struct _CACHE_MNG_INFO{
	int 			video_type;
	unsigned long 	start_addr;
	unsigned long 	start_phy;
	int				size;
	int				blk_sz;
	int				blk_num;
	int				cache_num;
	CACHE_DATA		*cache;
	CACHE_BLK		*blk;
} CACHE_MNG;


typedef struct _VIDEO_FRAME
{
	int 	serial;
	int 	fram_type;
	int		blkindex;
	int		blks;
	int		realsize;
	int		flag;
	unsigned int	timestamp;
	int		ref_serial[VIDOE_INFO_END];
} VIDEO_FRAME;

typedef struct _VIDEO_GOP
{
	int				last_Start;
	int				last_Start_serial;
	int				last_End;
	int				last_End_serial;
	int				lastest_I;
	int				lastest_I_serial;
}VIDEO_GOP;

typedef struct _VIDEO_BLK_INFO
{
	int 			video_type;
	int				width;
	int				height;
	unsigned long	start;
	unsigned long	start_phy;
	unsigned long	size;
	void			*extradata;
	int				extrasize;
	int				IsCache;
	
	int				blk_sz;
	int				blk_num;
	int				blk_free;
	int				frame_num;
	int				cur_frame;
	int				cur_serial;
	int				cur_blk;
	unsigned int	timestamp;
	VIDEO_GOP		gop;
	VIDEO_FRAME		*frame;
	CACHE_MNG		*cachemng;
} VIDEO_BLK_INFO;



typedef struct _MEM_INFO{
	unsigned long 	start_addr;
	unsigned long 	start_phyAddr;
	int				freesize;
	int				totalsize;
	int				offset;
	int				mem_layout;
	int				shmem_key;
	int				video_info_nums;
	VIDEO_BLK_INFO	*video_info;
	
	
} MEM_MNG_INFO;

typedef struct _MEM_SIZE{
	unsigned int cache_size;
	unsigned int cache_blk_size;
	unsigned int mem_size;
	unsigned int mem_blk_size;	
	unsigned int ext_size;
}MEM_SIZE;

typedef struct _MEM_LAYOUT{
	MEM_SIZE profiles[VIDOE_INFO_END];
	unsigned int totalsizes;
}MEM_LAYOUT;


int MemMng_Video_Write( void *pData, int size, int frame_type,
						VIDEO_BLK_INFO *pVidInfo );
						
int MemMng_release( MEM_MNG_INFO *pInfo );
int MemMng_Init( MEM_MNG_INFO *pInfo );

int MemMng_Video_ReadFrame(void *pDest, int *pSize, int *pFrm_type,
			int bufflimit, VIDEO_FRAME *pFrame, VIDEO_BLK_INFO *pVidInfo );
unsigned long GetMemMngPhyBaseAddr(MEM_MNG_INFO *pInfo);
unsigned long GetMemMngShmemKey(MEM_MNG_INFO *pInfo);
unsigned long GetMemMngTotalMemSize(MEM_MNG_INFO *pInfo);

VIDEO_FRAME * MemMng_GetFrameBySerial( int serial , VIDEO_BLK_INFO *pVidInfo);
int GetCurrentSerialNo(VIDEO_BLK_INFO *video_info);
int GetCurrentOffset(VIDEO_BLK_INFO *video_info);
VIDEO_FRAME * GetCurrentFrame(VIDEO_BLK_INFO *video_info);
VIDEO_FRAME * GetLastI_Frame(VIDEO_BLK_INFO *video_info);
#endif

