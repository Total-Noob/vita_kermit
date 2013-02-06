
// Authors : freddy_156, Yosh


#include <pspiofilemgr.h>
#include <pspsysclib.h>
#include <pspsysevent.h>
#include <pspthreadman_kernel.h>
#include <psputilsforkernel.h>
#include <pspkdebug.h>
#include <pspkernel.h>
#include <pspintrman_kernel.h>

#define SCE_ERROR_ERRNO_OPERATION_NOT_PERMITTED 0x80010001
#define SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES 0x80010018
#define MAX_OPENED_FILES 32


PSP_MODULE_INFO("sceKermitMsfs_driver", 0x1007, 1, 0);


typedef struct FileInfo_
{
	// 00
	struct SceIoIob_ *fileIob;
	// 04
	int mutex;
	int kermitFileId;
	int fileFlags;
	u32 unk16;	// size ?
	u32 unk20;	// nb errors ?
} FileInfo;

typedef struct SceIoIob_
{
    int unk000; // some ID
    int fsNum; // 4
    SceIoDeviceArg *dev; // 8
    int dev_type; // 12
    struct FileInfo_ *fileInfo; // 16
    int unk020; // 20
    int unk024; // 24
    int unk028; // 28
    int unk032; // 32
    int unk036; // 36
    int unk040; // 40
    SceUID curThread; // 44
    char userMode; // 48
    char powerLocked; // 49
    char unk050;
    char asyncPrio; // 51
    SceUID asyncThread; // 52
    SceUID asyncSema; // 56
    SceUID asyncEvFlag; // 60
    SceUID asyncCb; // 64
    void *asyncCbArgp; // 68
    int unused72; // 72
    int k1; // 76
    s64 asyncRet; // 80
    int asyncArgs[6]; // 88
    int asyncCmd; // 112
    int userLevel; // 116
    SceIoHook hook; // 120
    int unk132; // 132
    char *newPath; // 136
    int retAddr; // 140
} SceIoIob;

int sub_0228(SceIoIob *iob, int cmd); // do_close
int sub_27B0(void);	// ThreadMsfsClose
int sub_2860(void);
int sub_2518(void);
void sub_260C(void);
int sub_0108(int ev_id, char *ev_name, void *param, int *result);
int sub_1C20(PspIoDrvArg *arg); //IoInit
int sub_1CB4(PspIoDrvArg *arg); //IoExit
int sub_1D44(SceIoIob *iob, char *file, int flags, SceMode mode); //IoOpen
int sub_1D60(SceIoIob *iob); //IoClose
int sub_032C(SceIoIob *iob, char *data, int len); //IoRead
int sub_0718(SceIoIob *iob, const char *data, int len); //IoWrite
SceOff sub_0918(SceIoIob *iob, SceOff ofs, int whence); //IoLseek
int sub_0A90(SceIoIob *iob, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen); //IoIoctl
int sub_0CA4(SceIoIob *iob, const char *name); //IoRemove
int sub_0DB4(SceIoIob *iob, const char *name, SceMode mode); //IoMkdir
int sub_0ED4(SceIoIob *iob, const char *name); //IoRmdir
int sub_1D7C(SceIoIob *iob, const char *dirname); //IoDopen
int sub_1DA0(SceIoIob *iob); //IoDclose
int sub_0FE4(SceIoIob *iob, SceIoDirent *dir); //IoDread
int sub_1260(SceIoIob *iob, const char *file, SceIoStat *stat); //IoGetstat
int sub_140C(SceIoIob *iob, const char *file, SceIoStat *stat, int bits); //IoChstat
int sub_1594(SceIoIob *iob, const char *oldname, const char *newname); //IoRename
int sub_1714(SceIoIob *iob, const char *dir); //IoChdir
int sub_1DBC(SceIoIob *iob); //IoMount
int sub_1DC4(SceIoIob *iob); //IoUmount
int sub_1A20(SceIoIob *iob, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen); //IoDevctl

FileInfo openedFiles[MAX_OPENED_FILES];	// 2F48

u32 g_msfs_mutex;	// 2F38
u32 g_2F3C;
u8 *g_msfs_buf;	// 2F40
u32 g_msfs_bufsize;	// 2F44
SceUID g_2F20 = -1;
SceUID g_2F24 = -1;
SceUID g_2F28 = -1;
SceUID g_2F2C = -1;

PspIoDrvFuncs g_2E04 = {
	.IoInit = &sub_1C20,
	.IoExit = &sub_1CB4,
	.IoOpen = &sub_1D44,
	.IoClose = &sub_1D60,
	.IoRead = &sub_032C,
	.IoWrite = &sub_0718,
	.IoLseek = &sub_0918,
	.IoIoctl = &sub_0A90,
	.IoRemove = &sub_0CA4,
	.IoMkdir = &sub_0DB4,
	.IoRmdir = &sub_0ED4,
	.IoDopen = &sub_1D7C,
	.IoDclose = &sub_1DA0,
	.IoDread = &sub_0FE4,
	.IoGetstat = &sub_1260,
	.IoChstat = &sub_140C,
	.IoRename = &sub_1594,
	.IoChdir = &sub_1714,
	.IoMount = &sub_1DBC,
	.IoUmount = &sub_1DC4,
	.IoDevctl = &sub_1A20,
	.IoUnk21 = NULL,
};

PspIoDrvFuncs g_2E5C = {
	.IoInit = &sub_1C20,
	.IoExit = &sub_1CB4,
	.IoOpen = &sub_1D44,
	.IoClose = &sub_1D60,
	.IoRead = &sub_032C,
	.IoWrite = &sub_0718,
	.IoLseek = &sub_0918,
	.IoIoctl = &sub_0A90,
	.IoRemove = &sub_0CA4,
	.IoMkdir = &sub_0DB4,
	.IoRmdir = &sub_0ED4,
	.IoDopen = &sub_1D7C,
	.IoDclose = &sub_1DA0,
	.IoDread = &sub_0FE4,
	.IoGetstat = &sub_1260,
	.IoChstat = &sub_140C,
	.IoRename = &sub_1594,
	.IoChdir = &sub_1714,
	.IoMount = &sub_1DBC,
	.IoUmount = &sub_1DC4,
	.IoDevctl = &sub_1A20,
	.IoUnk21 = NULL,
}; //same pointers?

PspIoDrv g_2EB4 = { "ms", 0x10, 0x01, "Memory Stick File", &g_2E04 }; //0x01?
PspIoDrv g_2EC8 = { "fatms", 0x10, 0x01, "fatms emulation", &g_2E5C };

PspSysEventHandler g_2EE0 = {
	.size = 0x04,
	.name = "sceMsFs",
	.type_mask = 0x00FFFF00,
	.handler = &sub_0108;
	.r28 = 0,
	.busy = 0,
	.next = NULL,
};

int module_bootstart(void)
{
	sceKernelMemset(0xAA118000, 0, 0x8000);
	g_msfs_bufsize = 0x4000;
	g_msfs_buf = 0xAA11C000;
	g_2F3C = 0xAA118000;
	
	if((g_msfs_mutex = sceKernelCreateMutex("SceKermitMsfsFile", 0x100, 0, NULL)) <= 0)
	{
		return 1;
	}
	
	if(sub_2518() < 0)
	{
		sceKernelDeleteMutex(g_msfs_mutex);
		return 1;
	}
	
	sceIoDelDrv("ms");
	
	if(sceIoAddDrv(&g_2EB4) >= 0)
	{
		sceIoDelDrv("fatms");
		if(sceIoAddDrv(&g_2EC8) >= 0)
		{
			if(sub_2860() >= 0)
			{
				sceKernelRegisterSysEventHandler(&g_2EE0);
				return 0;
			}
		}
	}
	
	sub_260C();
	sceKernelDeleteMutex(g_msfs_mutex);
	
	return 1;
}

int sub_1C20(PspIoDrvArg *arg) //IoInit
{
	u8 buf[128];
	u32 resp[2];
	u32 argc;
	u32 size = 0xC;
	void *alignedBuf = ALIGN_64(buf + 63);
	
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	argc = 0;
	sceKermitMemory_driver_AAF047AC(packet, argc, arg, size, KERMIT_OUTPUT_MODE | KERMIT_INPUT_MODE);
	
	argc = 1;
	sceKermit_driver_4F75AA05(packet, KERMIT_MODE_MSFS, KERMIT_CMD_INIT_FS, argc, KERMIT_CALLBACK_DISABLE, resp);
	
	// Receive Vita data
	sceKermitMemory_driver_90B662D0(arg, size);
	
	
	return resp[0];
}

int sub_2518(void) 
{
	if((g_2F20 = sceKernelCreateThread("SceKermitMsfsClose", &sub_27B0, 0x10, 0x1000, 0x00100000 | 1, NULL)) < 0)
	{
		sub_260C();
		return g_2F20;
	}
	if((g_2F24 = sceKernelCreateMutex("SceKermitMsfsClose", 0x100, 0, NULL)) < 0)
	{
		sub_260C();
		return g_2F24;
	}
	if((g_2F28 = sceKernelCreateMsgPipe("SceCompatMsfsCloseRequest", 1, 0, 0x10, NULL)) < 0)
	{
		sub_260C();
		return g_2F28;
	}
	if((g_2F2C = sceKernelCreateMsgPipe("SceKermitMsfsCloseResult", 1, 0, 0x8, NULL)) < 0)
	{
		sub_260C();
		return g_2F2C;
	}
	return 0;
}

int sub_00002860()
{
	if ( g_2F20 > 0 )	return MIN( sceKernelStartThread(g_2F20, 0, NULL), 0 );
	else	return SCE_ERROR_ERRNO_OPERATION_NOT_PERMITTED;
}

//	IoOpen
int sub_1D44(SceIoIob *iob, char *file, int flags, SceMode mode)
{
	return sub_00001F64(iob, file, flags, mode, KERMIT_CMD_OPEN_FS);
}

//	IoDopen
int sub_1D7C(SceIoIob *iob, const char *dirname)
{
	return sub_00001F64(iob, dirname, 0, 0, KERMIT_CMD_DOPEN_FS);
}

int sub_00001F64(SceIoIob *iob, char *path, int flags, SceMode mode, int cmd)
{
	u8 buf[128];
	u32 resp[2];
	int errCode = 0;
	void *alignedBuf = ALIGN_64(buf + 63);
	
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	if ( iob->fsNum < 5 )
	{
		int intr = sceKernelCpuSuspendIntr();
		int nbFilesOpen;
		
		nbFilesOpen = 0;
		
		if ( openedFiles[0].fileIob != NULL )
		{
			FileInfo *curFileInfo = openedFiles;
			
			do
			{
				nbFilesOpen++;
				curFileInfo += sizeof(FileInfo);
			} while ( nbFilesOpen < MAX_OPENED_FILES && curFileInfo->fileIob != NULL );
			
			
			if ( nbFilesOpen >= MAX_OPENED_FILES )	errCode = SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES;
		}
		
		if ( errCode >= 0 )
		{
			FileInfo *curFileInfo = nbFilesOpen * sizeof(FileInfo);
			
			curFileInfo->unk16 = NULL;
			curFileInfo->unk20 = NULL;
			iob->fileInfo = curFileInfo;
			curFileInfo->fileIob = iob;
			curFileInfo->fileFlags = flags;
		}
		
		sceKernelCpuResumeIntr(intr);
		
		
		if ( errCode >= 0 )
		{
			int ret;
			
			curFileInfo->mutex = sceKernelCreateMutex("SceMsfs", 0x100, 1, NULL);
			
			ret = sceKernelLockMutex(g_msfs_mutex, 1, NULL);
			
			if ( !ret )
			{
				u32 maxSize = g_msfs_bufsize/2;
				strncpy(g_msfs_buf, path, maxSize);
				
				*((char*)(g_msfs_buf + maxSize - 1)) = '\0';
				
				u32 argc = 0;
				sceKermitMemory_driver_AAF047AC(packet, argc, g_msfs_buf, strlen(g_msfs_buf)+1, KERMIT_INPUT_MODE);
				
				// 2nd arg
				*(packet+0x18) = flags;
				// 3rd arg
				*(packet+0x20) = mode;
				
				
				argc = 3;
				sceKermit_driver_4F75AA05(packet, KERMIT_MODE_MSFS, cmd, argc, KERMIT_CALLBACK_DISABLE, resp);
				
				curFileInfo->kermitFileId = resp[0];
				ret = sceKernelUnlockMutex(g_msfs_mutex, 1);
				
				if ( !ret )
				{
					sceKernelUnlockMutex(curFileInfo->mutex, 1);
					
					if ( resp[0] < 0 )
					{
						sceKernelDeleteMutex(curFileInfo->mutex);
						
						intr = sceKernelCpuSuspendIntr();
						
						iob->fileInfo = NULL;
						
						sceKernelCpuResumeIntr(intr);
					}
					
					errCode = resp[0];
				}
				else	errCode = ret;
			}
			else	errCode = ret;
		}
	}
	else	errCode = SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
	
	
	
	return errCode;
}



