#include <pspiofilemgr.h>
#include <pspsysclib.h>
#include <pspsysevent.h>
#include <pspthreadman_kernel.h>
#include <psputilsforkernel.h>
#include <pspkdebug.h>
#include <pspkernel.h>
#include <pspintrman_kernel.h>

#define SCE_ERROR_ERRNO_DEVICE_NOT_FOUND 0x80010013
#define SCE_ERROR_ERRNO_FILE_INVALID_ADDR 0x8001000E
#define SCE_USER_ERROR_ILLEGAL_ARGUMENT 0x80010016
#define SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR 0x80010009



PSP_MODULE_INFO("sceKermitFlashfs_driver", 0x1007, 1, 0);


FileInfo openedFiles[MAX_OPENED_FILES];	// 23A0

PspIoDrvFuncs g_207C = {
	.IoInit = &sub_14E8,
	.IoExit = &sub_157C,
	.IoOpen = &sub_00E4,
	.IoClose = &sub_160C,
	.IoRead = &sub_01DC,
	.IoWrite = &sub_05C8,
	.IoLseek = &sub_07C8,
	.IoIoctl = &sub_1628,
	.IoRemove = &sub_0948,
	.IoMkdir = &sub_0A74,
	.IoRmdir = &sub_0BB0,
	.IoDopen = &sub_16A0,
	.IoDclose = &sub_16C4,
	.IoDread = &sub_0CDC,
	.IoGetstat = &sub_0EB0,
	.IoChstat = &sub_1068,
	.IoRename = &sub_120C,
	.IoChdir = &sub_13BC,
	.IoMount = NULL,
	.IoUmount = NULL,
	.IoDevctl = &sub_16E0,
	.IoUnk21 = NULL,
};

PspIoDrvFuncs g_20D4 = {
	.IoInit = &sub_14E8,
	.IoExit = &sub_157C,
	.IoOpen = &sub_00E4,
	.IoClose = &sub_160C,
	.IoRead = &sub_01DC,
	.IoWrite = NULL,
	.IoLseek = NULL,
	.IoIoctl = NULL,
	.IoRemove = NULL,
	.IoMkdir = NULL,
	.IoRmdir = NULL,
	.IoDopen = NULL,
	.IoDclose = NULL,
	.IoDread = NULL,
	.IoGetstat = &sub_0EB0,
	.IoChstat = NULL,
	.IoRename = NULL,
	.IoChdir = NULL,
	.IoMount = &sub_1714,
	.IoUmount = &sub_171C,
	.IoDevctl = NULL,
	.IoUnk21 = NULL,
};

PspIoDrv g_212C = {"flash", 0x10, 0x01, "FLASH file", &g_207C};
PspIoDrv g_2140 = {"flashfat", 0x10, 0x01, "FLASH fat file", &g_20D4};

PspSysEventHandler g_2350 = {
	.size = 0x04,
	.name = "sceKermitFlashfs",
	.type_mask = 0x00FFFF00,
	.handler = &sub_170C;
	.r28 = 0,
	.busy = 0,
	.next = NULL,
	.reserved[0] = 0,
	.reserved[1] = 0,
	.reserved[2] = 0,
	.reserved[3] = 0,
	.reserved[4] = 0,
	.reserved[5] = 0,
	.reserved[6] = 0,
	.reserved[7] = 0,
	.reserved[8] = 0,
};

u32 g_2390;
u32 g_2394;
u32 g_2398;
u32 g_239C;

int module_bootstart(void)
{
	sceKernelMemset(0xAA120000, 0, 0x8000);
	g_2398 = 0xAA124000;
	g_2394 = 0xAA120000;
	
	g_239C = sceKernelCreateMutex("SceKermitFlashfsFile", 0x100, 0, 0);
	g_2390 = g_239C;
	if(g_2390 <= 0)	return 1;
	
	sceIoDelDrv("flashfat");
	if(sceIoAddDrv(&g_2140) >= 0)
	{
		sceIoDelDrv("flash");
		if(sceIoAddDrv(&g_212C) >= 0)
		{
			sceKernelRegisterSysEventHandler(&g_2350);
			return 0;
		}
	}
	
	sceKernelDeleteMutex(g_2390);
	return 1;
}

int sub_14E8(PspIoDrvArg *arg) //IoInit
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
	sceKermit_driver_4F75AA05(packet, KERMIT_MODE_FLASHFS, KERMIT_CMD_INIT_FS, argc, KERMIT_CALLBACK_DISABLE, resp);
	
	// Receive Vita data
	sceKermitMemory_driver_90B662D0(arg, size);
	
	
	return resp[0];
}

// IoWrite
int sub_05C8(SceIoIob *iob, const char *data, int len)
{
	u8 buf[128];
	u32 resp[2];
	int errCode = 0;
	void *alignedBuf = ALIGN_64(buf + 63);
	u32 cust = alignedBuf;
	
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	
	// asm("sra %0, %1, 31" : "=r" (cust) : "r" (cust));
	// KermitPacket *packet = alignedBuf | (((u32)cust + 2) << 29);
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	if ( iob->fs_num < 5 )
	{
		//	Check address
		if ( ( (data & 0x1FFFC000 == 0x10000) && ((0b110101 >> (data >> 29)) & 1) ) || /* Scratchpad &  0-1 4-5 8-9 A-B */ \
				( (data & 0x1F800000 == 0x04000000) && ((0b110101 >> (data >> 29)) & 1) ) || /* VRAM &  0-1 4-5 8-9 A-B */ \
				((0x220202 >> (data >> 27)) & 1) )	// 0 4 8 A (and next bit 1)
		{
			if ( len >= 0 )
			{
				if ( iob->fileInfo >= openedFiles && \
						iob->fileInfo < (openedFiles + sizeof(openedFiles)) && \
						iob->fileInfo->fileIob == iob )
				{
					int ret;
					u32 argc;
					const void *addr = data - (data % 64);
					ret = sceKernelDcacheWritebackRange(addr, ALIGN_64((data % 64) + len + 63) );
					if ( ret != 0 )	Kprintf("Failed data cache write back (addr 0x%08x) %08x\n", addr, ret);
					
					// first arg
					*(packet+0x10) = iob->fileInfo->kermitFileId;
					
					argc = 1;
					
					// Prepares to receive and sets data as 2nd arg
					sceKermitMemory_driver_AAF047AC(packet, argc, data, len, KERMIT_INPUT_MODE);
					
					
					
					// 3rd arg
					*(packet+0x20) = len;
					
					argc = 3;
					sceKermit_driver_4F75AA05(packet, KERMIT_MODE_FLASHFS, KERMIT_CMD_WRITE_FS, argc, KERMIT_CALLBACK_DISABLE, resp);
					
					if ( resp[0] > 0)
					{
						iob->fileInfo->unk16 += resp[0];
						iob->fileInfo->unk20 += - KERNEL(resp[0]);
						if ( iob->fileInfo->unk16 < (u32)resp[0] )	(iob->fileInfo->unk20)++;
						
						iob->unk024 = iob->fileInfo->unk16;
						iob->unk028 = iob->fileInfo->unk20;
					}
					
					errCode = resp[0];
					
				}
				else	errCode = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
			}
			else	errCode = SCE_USER_ERROR_ILLEGAL_ARGUMENT;
		}
		else	errCode = SCE_ERROR_ERRNO_FILE_INVALID_ADDR;
	}
	else	errCode = SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
	
	
	return errCode;
}

