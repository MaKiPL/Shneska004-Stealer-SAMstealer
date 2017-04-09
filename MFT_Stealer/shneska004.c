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

struct NTFS_MFT_FILE
{
	char		szSignature[4];		// Signature "FILE"
	WORD		wFixupOffset;		// offset to fixup pattern
	WORD		wFixupSize;			// Size of fixup-list +1
	LONGLONG	n64LogSeqNumber;	// log file seq number
	WORD		wSequence;			// sequence nr in MFT
	WORD		wHardLinks;			// Hard-link count
	WORD		wAttribOffset;		// Offset to seq of Attributes
	WORD		wFlags;				// 0x01 = NonRes; 0x02 = Dir
	DWORD		dwRecLength;		// Real size of the record
	DWORD		dwAllLength;		// Allocated size of the record
	LONGLONG	n64BaseMftRec;		// ptr to base MFT rec or 0
	WORD		wNextAttrID;		// Minimum Identificator +1
	WORD		wFixupPattern;		// Current fixup pattern
	DWORD		dwMFTRecNumber;		// Number of this MFT Record
									// followed by resident and
									// part of non-res attributes
};

struct NTFS_ATTRIBUTE	// if resident then + RESIDENT
{					//  else + NONRESIDENT
	DWORD	dwType;
	DWORD	dwFullLength;
	BYTE	uchNonResFlag;
	BYTE	uchNameLength;
	WORD	wNameOffset;
	WORD	wFlags;
	WORD	wID;

	union ATTR
	{
		struct RESIDENT
		{
			DWORD	dwLength;
			WORD	wAttrOffset;
			BYTE	uchIndexedTag;
			BYTE	uchPadding;
		} Resident;

		struct NONRESIDENT
		{
			LONGLONG	n64StartVCN;
			LONGLONG	n64EndVCN;
			WORD		wDatarunOffset;
			WORD		wCompressionSize; // compression unit size
			BYTE		uchPadding[4];
			LONGLONG	n64AllocSize;
			LONGLONG	n64RealSize;
			LONGLONG	n64StreamSize;
			// data runs...
		}NonResident;
	}Attr;
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
DWORD bShouldStop = 0;
DWORD bStartsWithBootloader = 0;
LONGLONG WholeHDDsector = 0;

#define WholeHDD 1
#define _DEBUG 1

int main()
{
	//MFT_ENUM_DATA_V0 mftenumdata;


	char * c = getenv("SYSTEMROOT");
	rootdriver = *c ;
	wchar_t * py = (wchar_t*)malloc(32);
	if(!WholeHDD)
		wsprintf(py, L"%s%c%s", L"\\\\.\\", rootdriver, ":");
	else wsprintf(py, L"%ls%d", L"\\\\.\\PhysicalDrive", 0);
	HANDLE dev;
	if(!WholeHDD)
		dev = CreateFile(py, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
	else dev = CreateFile(py, FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (dev == INVALID_HANDLE_VALUE)
		return -1;
	if(_DEBUG)
		printf("1. Device opened\n");

	DWORD brre = 1;
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

	if (_DEBUG)
	printf("2. Obtained device geometry!\n");

	VOID * ntfsboot = malloc(sizeof(struct NTFS_PART_BOOT_SEC));
	DWORD bootsz = bytespersector;
	if (bytespersector > 512)
		bootsz = 512;
	SetFilePointer(dev, 0, 0,FILE_BEGIN);
	if (WholeHDD)
		while (1)
		{
			int errtest = ReadFile(dev, ntfsboot, bootsz, &brre, NULL);
			struct NTFS_PART_BOOT_SEC * ntboot = (struct NTFS_PART_BOOT_SEC*)ntfsboot;
			if (ntboot->wSecMark == 0xAA55)
				bStartsWithBootloader = 1;
			if (!memcmp(ntboot->chOemID, "NTFS", 4))
				break;
			//system("pause");
			WholeHDDsector++;
			SetFilePointer(dev, 512, 0, FILE_CURRENT); //pass 512 bytes boot loader
			if (WholeHDDsector > sectorspertrack*trackpercylinder * (cylinders.QuadPart/256))
				return -1;
		}
	int errtest = 0;
	if (WholeHDD)
		SetFilePointer(dev, -512, 0, FILE_CURRENT);

	errtest = ReadFile(dev, ntfsboot, bootsz, &brre, NULL); 
	if(errtest==0)
		outerr(GetLastError());
	struct NTFS_PART_BOOT_SEC * ntboot = (struct NTFS_PART_BOOT_SEC*)ntfsboot;

	
	if (_DEBUG)
	printf("3. Got the disk boot sector!\n");

	if (memcmp(ntboot->chOemID, "NTFS", 4))
		return -1;

	DWORD bytespercluster = ntboot->bpb.uchSecPerClust * ntboot->bpb.wBytesPerSec;

	DWORD MFTRecordSize = 0x01 << ((-1)*((char)ntboot->bpb.nClustPerMFTRecord));

	int * mftbb = malloc(sizeof(byte) * MFTRecordSize);


	LARGE_INTEGER li;
		li.QuadPart = (LONGLONG)ntboot->bpb.n64MFTLogicalClustNum * bytespercluster;
	if(!WholeHDD)
	errtest = SetFilePointer(dev, li.LowPart, &li.HighPart, FILE_BEGIN);
	else 
	{
		SetFilePointer(dev, -512, 0, FILE_CURRENT); //whole disk so NTFS is relative to boot sector which is further the disk (when example the OS bootloader is)
		errtest = SetFilePointer(dev, li.LowPart, &li.HighPart, FILE_CURRENT);
	}
	if (errtest == -1)
		outerr(GetLastError());

	if (_DEBUG)
	printf("4. Reading $MFT\n");
	errtest = ReadFile(dev, mftbb, MFTRecordSize, &brre, NULL);
	if (errtest == 0)
		outerr(GetLastError());
	BYTE * mftrecordtempvar;
	__asm
	{
		PUSH EAX
		MOV EAX, mftbb
		MOV mftrecordtempvar, EAX
		POP EAX
	}
	struct NTFS_MFT_FILE *MFT = (struct NTFS_MFT_FILE*)mftrecordtempvar;
	if (_DEBUG)
	printf("5. Read $MFT; Querying files\n");

	struct NTFS_ATTRIBUTE * ntatr_std;
	struct NTFS_ATTRIBUTE * ntatr_fnm;

	DWORD curpos = MFT->wAttribOffset;

	while (1) {
		__asm
		{
			PUSH EAX
			MOV EAX, mftbb
			ADD EAX, [curpos]
			MOV mftrecordtempvar, EAX
			POP EAX
		}
		struct NTFS_ATTRIBUTE * ntatr = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;

		DWORD nextatr = ntatr->dwFullLength;

		if (ntatr->dwType == 0x10)
			ntatr_std = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;
		if (ntatr->dwType == 0x30) 
		{
			ntatr_fnm = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;			
			wchar_t *c = (mftrecordtempvar + (0x42 + ntatr_fnm->wNameOffset));
			if (!lstrcmpW(c, L"$FMT"))
				return -1;


		}

		if (ntatr->dwType == -1)
			break;

		curpos += nextatr;
	}

	FILE * fwriteout;
	if (_DEBUG)
	fwriteout = fopen("D:\debug.log", "a+");


	//LOOPSTER
	while (1)
	{
	returnhere:
		if (bShouldStop > 256)
			break;
		errtest = ReadFile(dev, mftbb, 1024, &brre, NULL);
		if (errtest == 0)
			outerr(GetLastError());
		__asm
		{
			PUSH EAX
			MOV EAX, mftbb
			MOV mftrecordtempvar, EAX
			POP EAX
		}
		MFT = (struct NTFS_MFT_FILE*)mftrecordtempvar;
		if (MFT->szSignature[0] != 'F')
		{
			bShouldStop++;
			goto returnhere;
		}
		DWORD baseadr;
		__asm
		{
			PUSH EAX
			MOV EAX, mftrecordtempvar
			MOV [baseadr], EAX
			POP EAX
		}
		curpos = MFT->wAttribOffset;
		while (1) {
			__asm
			{
				PUSH EAX
				MOV EAX, mftbb
				ADD EAX, [curpos]
				MOV mftrecordtempvar, EAX
				POP EAX
			}
			if (mftrecordtempvar > baseadr + 1024)
				break;

			struct NTFS_ATTRIBUTE * ntatr = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;
			
			DWORD nextatr = ntatr->dwFullLength;
			if (nextatr == 0)
				break;
			if (ntatr->dwType != 0x10 && ntatr->dwType != 0x30)
				break;

			if (ntatr->dwType == 0x10)
				ntatr_std = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;
			if (ntatr->dwType == 0x30)
			{
				ntatr_fnm = (struct NTFS_ATTRIBUTE*)mftrecordtempvar;
				DWORD relFpath = ntatr_fnm->wNameOffset;
				relFpath = relFpath == 0 ? 0x0F : relFpath;

				wchar_t *c = (mftrecordtempvar + (0x5A));
				if(!lstrcmpW(c, L"SAM"))
					wprintf(L"%ls\n", c);
				if (_DEBUG)
				fwprintf(fwriteout, L"%ls\n", c);

			}

			curpos += nextatr;
		}
	}
	


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