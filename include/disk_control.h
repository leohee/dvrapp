/********************************
** hdd_control.h
**
** by CSNAM, 2010/11/24
**
** Copyright nSolutions Co.
**
*********************************/

#ifndef __NSDVR_DISK_DEF__
#define __NSDVR_DISK_DEF__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common_def.h"

#define NSDVR_FIRST_SCSI_HDD_PATH		"/dev/sda"
#define NSDVR_FIRST_SCSI_CDROM_PATH		"/dev/sr"

#define NSDVR_DEFAULT_MOUNT_DIR	"/dvr/hdd1"

#define NSDVR_MAX_REC_HDD_CNT			4
#define NSDVR_MAX_MASTER_HDD_CNT	1
#define NSDVR_MAX_SLAVE_HDD_CNT		(NSDVR_MAX_REC_HDD_CNT-NSDVR_MAX_MASTER_HDD_CNT)
#define NSDVR_MAX_BACKUP_HDD_CNT	4
#define NSDVR_MAX_HDD_CNT					(NSDVR_MAX_REC_HDD_CNT+NSDVR_MAX_BACKUP_HDD_CNT)
#define NSDVR_MASTER_HDD_INDEX		0
#define NSDVR_SLAVE_HDD_INDEX			(NSDVR_MASTER_HDD_INDEX+NSDVR_MAX_MASTER_HDD_CNT)
#define NSDVR_BACKUP_HDD_INDEX		(NSDVR_SLAVE_HDD_INDEX+NSDVR_MAX_SLAVE_HDD_CNT)

#define NSDVR_MAX_SCSI_PATH_LENGTH		32
#define NSDVR_MAX_SCSI_VENDOR_LENGTH	32
#define NSDVR_MAX_SCSI_MODEL_LENGTH		32
#define NSDVR_MAX_SCSI_DEVICES				10
#define NSDVR_MAX_HDD_PARTITIONS			4
#define NSDVR_MAX_MOUNT_DIR_NAME			NSDVR_MAX_PATH_LENGTH

typedef enum {
	NSDVR_SCSI_DEVICE_TYPE_HDD=0,
	NSDVR_SCSI_DEVICE_TYPE_CDROM=5,
} enumNSDVR_SCSI_DEVICE_TYPE;

typedef enum {
	NSDVR_MEDIA_TYPE_UNKNOWN	= 0,
	NSDVR_MEDIA_TYPE_CD_ROM,
	NSDVR_MEDIA_TYPE_CD_RW,
	NSDVR_MEDIA_TYPE_DVD_ROM,
	NSDVR_MEDIA_TYPE_DVD_RW,
	NSDVR_MEDIA_TYPE_DVD_RAM,
} enumNSDVR_CDROM_MEDIA_TYPE;

typedef enum {
	NSDVR_PARTITON_UNKNOWN=0,
	NSDVR_PARTITON_TYPE_FAT_32 = 0x0C,
	NSDVR_PARTITON_TYPE_LINUX_SWAP = 0x82,
	NSDVR_PARTITON_TYPE_LINUX_NATIVE = 0x83,
	NSDVR_PARTITON_TYPE_RAW_DATA = 0xDA,
} enumNSDVR_PARTITON_TYPE;

typedef enum {
	NSDVR_FILE_SYSTEM_TYPE_UNKNOWN=0,
	
	// Linux
	NSDVR_FILE_SYSTEM_TYPE_EXT2,
	NSDVR_FILE_SYSTEM_TYPE_EXT3,
	NSDVR_FILE_SYSTEM_TYPE_XFS,
	NSDVR_FILE_SYSTEM_TYPE_SWAP,
	
	// DOS/Windows
	NSDVR_FILE_SYSTEM_TYPE_VFAT,
} enumNSDVR_FILE_SYSTEM_TYPE;

typedef enum {
	NSDVR_HDD_STATUS_UNUSED		= 0,
	NSDVR_HDD_STATUS_NORMAL,
	NSDVR_HDD_STATUS_NOT_CONNECTED,
	NSDVR_HDD_STATUS_NOT_FORMATTED,
	NSDVR_HDD_STATUS_IO_ERROR,
} enumNSDVR_HDD_STATUS;

#define NSDVR_HDD_SIZE_MAX		((long)-1)

typedef enum {
	NSDVR_DISK_USAGE_TYPE_NONE		= 0,
	NSDVR_DISK_USAGE_TYPE_REC_MASTER,
	NSDVR_DISK_USAGE_TYPE_REC_SLAVE,
	NSDVR_DISK_USAGE_TYPE_BACKUP,
} enumNSDVR_DISK_USAGE;

typedef struct {
	int scsi_id;
	int	is_usb;
	enumNSDVR_SCSI_DEVICE_TYPE 	device_type;
	//
	char device_path[NSDVR_MAX_SCSI_PATH_LENGTH];
	char model[NSDVR_MAX_SCSI_MODEL_LENGTH];
	char vendor[NSDVR_MAX_SCSI_VENDOR_LENGTH];
} stNSDVR_ScsiDeviceInfo;

typedef struct {
	int id;
	int type;	// enumNSDVR_PARTITON_TYPE
	long size;	// 1K
	char mount_dir[NSDVR_MAX_MOUNT_DIR_NAME];
} stNSDVR_PartitionInfo;

typedef struct {
	long total_size;	// 1K
	long used_size;	// 1K
} stNSDVR_DiskUsage;

typedef struct {
	int dvr_id;
	int hdd_id;	// CFG-DB index
	int type;
	time_t format_time;
	long size;	// 1K
	char model[NSDVR_MAX_SCSI_MODEL_LENGTH];
	char vendor[NSDVR_MAX_SCSI_VENDOR_LENGTH];
} stNSDVR_hdd_id;

typedef struct {
	// Permanent info
	stNSDVR_hdd_id id;
	long disk_size;	// 1K
	int num_partitions;
	stNSDVR_PartitionInfo part[NSDVR_MAX_HDD_PARTITIONS];
	
	// Temporal info
	int status;
	stNSDVR_DiskUsage usage;
	stNSDVR_ScsiDeviceInfo device;
	char mount_dir[NSDVR_MAX_MOUNT_DIR_NAME];
} stNSDVR_hdd_info;

typedef struct {
	int temperature;
} stNSDVR_DiskSMARTStatus;

typedef struct {
	int writable;
	int erasable;
	enumNSDVR_CDROM_MEDIA_TYPE 		media_type;
} stNSDVR_CDROMStatus;

extern char* nsDVR_CDROM_media_type_name(int type);


extern char* nsDVR_disk_status_name(int status);
extern char* nsDVR_disk_partition_type_name(int type);
extern char* nsDVR_CDROM_media_type_name(int type);

extern int nsDVR_disk_get_scsi_count();
extern int nsDVR_disk_get_scsi_list(stNSDVR_ScsiDeviceInfo* pInfo, int numInfo);
extern int nsDVR_disk_get_hdd_count();
extern int nsDVR_disk_get_hdd_list(stNSDVR_hdd_info* pInfo, int numInfo);

extern int nsDVR_disk_get_hdd_info(char* devics_path, stNSDVR_hdd_info* hdd);
extern int nsDVR_disk_hdd_magic_write(char* device_path, stNSDVR_hdd_id* hdd_info);
extern int nsDVR_disk_hdd_magic_read(char* device_path, stNSDVR_hdd_id* hdd_info);
//extern int nsDVR_disk_make_partition(char* device_path, int type);
//extern int nsDVR_disk_make_partition(char* device, stNSDVR_PartitionInfo *pInfo, int n_partitios);
extern handle nsDVR_disk_mkfs_open(char* partition_path, int fs_type, int *error_code);
extern int nsDVR_disk_mkfs_status(handle hMKFS);
extern void nsDVR_disk_mkfs_close(handle hMKFS);
extern int nsDVR_disk_get_partition_list(char* devicePath, long* total_size, stNSDVR_PartitionInfo* pInfo, int n_partitons);
extern int nsDVR_disk_get_scsi_info(stNSDVR_hdd_id* id, stNSDVR_ScsiDeviceInfo *scsi_dev_name);
extern int nsDVR_disk_get_fs_size_KB(char* dir_name);
extern int nsDVR_disk_mount(char* hdd_path, char* mount_dir);
extern int nsDVR_disk_get_mount_dir(char* devicePath, char* dir_name_buf, int n_dir_buf_len);

extern int nsDVR_disk_get_usage(char* devicePath, stNSDVR_DiskUsage* pInfo);
extern int nsDVR_disk_SMART_status(char* devicePath, stNSDVR_DiskSMARTStatus* pInfo);
extern int nsDVR_disk_check_error(char* devicePath);

extern int nsDVR_usb_get_count();
extern int nsDVR_usb_get_list(stNSDVR_ScsiDeviceInfo* pInfo, int numInfo);

extern int nsDVR_disk_SMART_status(char* devicePath, stNSDVR_DiskSMARTStatus* pInfo);
extern int nsDVR_disk_check_error(char* devicePath);


extern int nsDVR_CDROM_get_media_info(char* devicePath, stNSDVR_CDROMStatus* pInfo);
extern int nsDVR_CDROM_check_ready(char* devicePath);
extern int nsDVR_CDROM_eject(char* devicePath);
extern handle nsDVR_CDROM_mkiso_open(char* isoFilePath, char** fileList, int nFiles, int *error_code);
extern int nsDVR_CDROM_mkiso_status(handle hMKISO);
extern void nsDVR_CDROM_mkiso_close(handle hMKISO);
extern handle nsDVR_CDROM_writer_open(char* devicePath, char* isoFilePath, int *error_code);
extern int nsDVR_CDROM_writer_status(handle hWriter);
extern void nsDVR_CDROM_writer_close(handle hWriter);
extern handle nsDVR_CDROM_eraser_open(char* devicePath, int *error_code);
extern int nsDVR_CDROM_eraser_status(handle hEraser);
extern void nsDVR_CDROM_eraser_close(handle hEraser);

#endif // __NSDVR_DISK_DEF__
