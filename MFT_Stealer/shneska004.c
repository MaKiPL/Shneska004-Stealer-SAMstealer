#include <stdio.h>
#include <stdlib.h>
//#include "NTFSstructs.h"
#include <Windows.h>


//Undelete 
#pragma pack(1)
struct NTFS_PART_BOOT_SEC
{
	char		chJumpInstruction[3];
	char		chOemID[4];
	char		chDummy[4];

	struct NTFS_BPB
	{
		WORD		wBytesPerSec;
		BYTE		uchSecPerClust;
		WORD		wReservedSec;
		BYTE		uchReserved[3];
		WORD		wUnused1;
		BYTE		uchMediaDescriptor;
		WORD		wUnused2;
		WORD		wSecPerTrack;
		WORD		wNumberOfHeads;
		DWORD		dwHiddenSec;
		DWORD		dwUnused3;
		DWORD		dwUnused4;
		LONGLONG	n64TotalSec;
		LONGLONG	n64MFTLogicalClustNum;
		LONGLONG	n64MFTMirrLogicalClustNum;
		int			nClustPerMFTRecord;
		int			nClustPerIndexRecord;
		LONGLONG	n64VolumeSerialNum;
		DWORD		dwChecksum;
	} bpb;

	char		chBootstrapCode[426];
	WORD		wSecMark;
};

void outerr(DWORD errmess);
char rootdriver;
DWORD CheckFile(USN_RECORD *rec);

const wchar_t * sam = L"SAM";
const wchar_t * sec = L"SECURITY";

//disc
DWORD bytespersector = 0;
LARGE_INTEGER cylinders = { 0 };
DWORD sectorspertrack = 0;
DWORD trackpercylinder = 0;

int main()
{
	//MFT_ENUM_DATA_V0 mftenumdata;


	char * c = getenv("SYSTEMROOT");
	rootdriver = *c ;
	wchar_t * py = (wchar_t*)malloc(32);
	wsprintf(py, L"%s%c%s", L"\\\\.\\", rootdriver, ":");
	HANDLE dev = CreateFile(py, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
	if (dev == INVALID_HANDLE_VALUE)
		return -1;
	printf("1. Device opened\n");

	DWORD brre = 1;

	//MFT_ENUM_DATA * mftenumdata = malloc(sizeof( MFT_ENUM_DATA));
	
	/*mftenumdata.StartFileReferenceNumber = 0;
	mftenumdata.LowUsn = 0;
	

	
	DWORD bufffsz = 1024 * 1024;
	VOID * outbuff = malloc(bufffsz);
	if (!DeviceIoControl(dev, FSCTL_QUERY_USN_JOURNAL, NULL, 0, outbuff, bufffsz, &brre, NULL))
		outerr(GetLastError());

	printf("2. USN Journal grabbed\n");

	USN_JOURNAL_DATA * jrnl;
	USN_RECORD *usn;
	jrnl = (USN_JOURNAL_DATA*)outbuff;
	mftenumdata.LowUsn = 0;
	mftenumdata.HighUsn = (USN)jrnl->MaxUsn;

	if (DeviceIoControl(dev, FSCTL_ENUM_USN_DATA, &mftenumdata, sizeof(mftenumdata), outbuff, bufffsz, &brre, NULL))
	{
		usn = (USN_RECORD*)outbuff;
		wprintf("%ws", usn->FileName);
	}
	else
		outerr(GetLastError());

	printf("3. Test ENUM USN jrnl success!\n");

	*/
	DISK_GEOMETRY * dg = malloc(sizeof(DISK_GEOMETRY));

	if (DeviceIoControl(dev, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, dg, sizeof(DISK_GEOMETRY), &brre, NULL))
	{
		bytespersector = dg->BytesPerSector;
		cylinders = dg->Cylinders;
		sectorspertrack = dg->SectorsPerTrack;
		trackpercylinder = dg->TracksPerCylinder;
	}
	else
		outerr(GetLastError());

	printf("2. Obtained device geometry!\n");

	VOID * ntfsboot = malloc(sizeof(struct NTFS_PART_BOOT_SEC));
	DWORD bootsz = bytespersector;
	if (bytespersector > 512)
		bootsz = 512;
	int errtest = ReadFile(dev, ntfsboot, bootsz, &brre, NULL); //Error 87 if OutSize is not 1024 const. WTF???? 512 fixes critical kernel error.. lol wtf is happening here?
	//okay, that's probably because you must read buffer as one sector (that's why non pow(2) values doesn't work)
	//also I overwrote buffer and it could hit the important data so it caused critical kernel error on further memory operations
	//and also main return
	if(errtest==0)
		outerr(GetLastError());
	struct NTFS_PART_BOOT_SEC * ntboot = (struct NTFS_PART_BOOT_SEC*)ntfsboot;

	

	printf("3. Got the disk boot sector!");

	if (memcmp(ntboot->chOemID, "NTFS", 4))
		return -1;

	DWORD bytespercluster = ntboot->bpb.uchSecPerClust * ntboot->bpb.wBytesPerSec;

	DWORD MFTRecordSize = 0x01 << ((-1)*((char)ntboot->bpb.nClustPerMFTRecord));


	/*
	while (1)
	{

		if (DeviceIoControl(dev, FSCTL_ENUM_USN_DATA, &mftenumdata, sizeof(mftenumdata), outbuff, bufffsz, &brre, NULL))
		{
			usn = (USN_RECORD*)outbuff;
			if (CheckFile(usn))
				break;
			else
				mftenumdata.StartFileReferenceNumber = *((DWORDLONG*)outbuff);
		}
		else
			outerr(GetLastError());
	}
	*/

	int * mftbb = malloc(sizeof(byte) * 1024);


    return 0;
}

void outerr(DWORD errmess)
{
	char * p = malloc(10);
	sprintf(p,"ERROR: %u", errmess);
	OutputDebugStringA(p);
	__asm {
		MOV AH, 01h
		INT 21h
	}
}

/*DWORD CheckFile(USN_RECORD *rec) //DEBUG version
{
	__asm
	{
		INC [filescount]
	}
	if (rec->FileName)
	{
		printf("%ws", rec->FileName);
	}
		return 0;
}
*/