
// path : absolute for flash (like "flash0:/XXXXXXX"), without device for ms (like "/PSP/GAME/XXXX")
// cmd_mode : either KERMIT_MODE_MSFS or KERMIT_MODE_FLASHFS
u64 KermitIo_open( char *path, int flags, SceMode mode, int cmd_mode, int cmd )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	u32 argc = 0;
	
	// Prepare to receive and set path as 1st arg
	sceKermitMemory_driver_AAF047AC(packet, argc, path, strlen(path)+1, KERMIT_INPUT_MODE);
	
	// 2nd arg
	*((u32*)((u32)packet+0x18)) = flags;
	// 3rd arg
	*((u32*)((u32)packet+0x20)) = mode;
	
	
	argc = 3;
	sceKermit_driver_4F75AA05(packet, cmd_mode, cmd, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	return resp;
}

u64 KermitIoOpen( char *path, int flags, SceMode mode, int cmd_mode )
{
	return KermitIo_open(path, flags, mode, cmd_mode, KERMIT_CMD_OPEN_FS );
}

u64 KermitIoDopen( char *dirName, int cmd_mode )
{
	return KermitIo_open(dirName, 0, 0, cmd_mode, KERMIT_CMD_DOPEN_FS );
}

u64 KermitIoWrite(u32 kermitFileId, const char *data, int len, int cmd_mode )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	int ret;
	u32 argc;
	const void *addr = (void*)((u32)data - ((u32)data % 64));
	ret = sceKernelDcacheWritebackRange(addr, ALIGN_64(((u32)data % 64) + len + 63) );
	if ( ret != 0 )	PRTSTR2("Failed data cache write back (addr 0x%08lX) %08lX\n", addr, ret);
	
	// 1st arg
	*((u32*)((u32)packet+0x10)) = kermitFileId;
	
	argc = 1;
	
	// Prepare to receive and set data as 2nd arg
	sceKermitMemory_driver_AAF047AC(packet, argc, data, len, KERMIT_INPUT_MODE);
	
	
	
	// 3rd arg
	*((u32*)((u32)packet+0x20)) = len;
	
	argc = 3;
	sceKermit_driver_4F75AA05(packet, cmd_mode, KERMIT_CMD_WRITE_FS, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	return resp;
}

u64 KermitIoRead(u32 kermitFileId, char *data, int len, int cmd_mode )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	u8 *buf_ = (u8*)0xAA124000;	// using flashfs kermit read buffer
	
	u32 argc = 1;
	
	// 1st arg
	*((u32*)((u32)packet+0x10)) = kermitFileId;
	
	// Prepare to receive and set buf as 2nd arg
	sceKermitMemory_driver_AAF047AC(packet, argc, buf_, len, KERMIT_OUTPUT_MODE);
	
	
	
	// 3rd arg
	*((u32*)((u32)packet+0x20)) = len;
	
	argc = 3;
	sceKermit_driver_4F75AA05(packet, cmd_mode, KERMIT_CMD_READ_FS, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	// Receive data
	sceKermitMemory_driver_90B662D0(buf_, (u32)resp);
	
	memcpy(data, buf_, (u32)resp);
	
	
	return resp;
}

u64 KermitIo_close(u32 kermitFileId, int cmd_mode, int cmd )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	u32 argc = 1;
	
	// 1st arg
	*((u32*)((u32)packet+0x10)) = kermitFileId;
	
	sceKermit_driver_4F75AA05(packet, cmd_mode, cmd, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	return resp;
}

u64 KermitIoClose(u32 kermitFileId, int cmd_mode )
{
	return KermitIo_close(kermitFileId, cmd_mode, KERMIT_CMD_CLOSE_FS );
}

u64 KermitIoDclose(u32 kermitFileId, int cmd_mode )
{
	return KermitIo_close(kermitFileId, cmd_mode, KERMIT_CMD_DCLOSE_FS );
}

u64 KermitIoLseek(u32 kermitFileId, SceOff ofs, int whence, int cmd_mode )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	
	u32 argc = 3;
	
	// 1st arg
	*((u32*)((u32)packet+0x10)) = kermitFileId;
	// 2nd arg
	*((u32*)((u32)packet+0x18)) = (u32)ofs;
	// 2nd arg
	*((u32*)((u32)packet+0x1C)) = (u32)(ofs >> 32);
	// 3rd arg
	*((u32*)((u32)packet+0x20)) = whence;
	
	sceKermit_driver_4F75AA05(packet, cmd_mode, KERMIT_CMD_SEEK_FS, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	return resp;
}

u64 KermitIoDread(u32 kermitDirId, SceIoDirent *dir, int cmd_mode )
{
	u8 buf[128];
	u64 resp;
	void *alignedBuf = ALIGN_64(buf + 63);
	sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
	
	KermitPacket *packet = KERMIT_PACKET(alignedBuf);
	
	u8 *buf_ = (u8*)0xAA124000;	// using flashfs kermit read buffer
	
	u32 argc = 1;
	
	// 1st arg
	*((u32*)((u32)packet+0x10)) = kermitDirId;
	
	// Prepare to receive and set buf as 2nd arg
	sceKermitMemory_driver_AAF047AC(packet, argc, buf_, sizeof(SceIoDirent), KERMIT_OUTPUT_MODE);
	
	
	
	argc = 2;
	sceKermit_driver_4F75AA05(packet, cmd_mode, KERMIT_CMD_DREAD_FS, argc, KERMIT_CALLBACK_DISABLE, &resp);
	
	
	// Receive data
	sceKermitMemory_driver_90B662D0(buf_, sizeof(SceIoDirent));
	
	memcpy(dir, buf_, sizeof(SceIoDirent));
	
	
	return resp;
}

u64 kermit_msfs_devctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
    u8 buf[128];
    u64 resp;
    void *alignedBuf = ALIGN_64((u32)buf + 63);
    sceKernelDcacheInvalidateRange(alignedBuf, 0x40);
    
    KermitPacket *packet = KERMIT_PACKET((u32)alignedBuf);


    const u32 max = 10;     // Enough for some device size ..
    u8 *buf_ = (u8*)0xAA124000;  // using flashfs kermit read buffer (works just as good as msfs's one)
    u8 *buf_2 = (u8*)((u32)buf_ + max);
    u8 *buf_3 = (u8*)((u32)buf_ + 0x100);   // Keep 0x100 buffer bytes for indata

    strcpy(buf_, dev);
    memcpy(buf_2, indata, inlen);
    memset(buf_3,0,outlen);


    // kermit cmd
    *((u32*)((u32)packet+0x00)) = KERMIT_CMD_DEVCTL;


    u32 argc = 0;
    
    // Prepare to send and set device's buf_ as 1st arg
    _sceKermitMemory_driver_AAF047AC(packet, argc, buf_, strlen(buf_) + 1, KERMIT_INPUT_MODE);
    
    // 2nd arg
    *((u32*)((u32)packet+0x18)) = cmd;
    argc = 2;
    
    // Prepare to send and set indata's buf_2 as 3rd arg
    _sceKermitMemory_driver_AAF047AC(packet, argc, buf_2, inlen, KERMIT_INPUT_MODE);

    // 4th arg
    *((u32*)((u32)packet+0x28)) = inlen;
    argc = 4;
    
    // Prepare to receive and set outdata's buf_3 as 5th arg
    _sceKermitMemory_driver_AAF047AC(packet, argc, buf_3, outlen, KERMIT_OUTPUT_MODE);

    // 6th arg
    *((u32*)((u32)packet+0x38)) = outlen;



    argc = 6;
    _sceKermit_driver_4F75AA05(packet, KERMIT_MODE_MSFS, KERMIT_CMD_DEVCTL, argc, KERMIT_CALLBACK_DISABLE, &resp);
    
    

    // Receive outdata
    _sceKermitMemory_driver_90B662D0(buf_3, outlen);
    
    memcpy(outdata, buf_3, outlen);
    
    
    
    return resp;
}

