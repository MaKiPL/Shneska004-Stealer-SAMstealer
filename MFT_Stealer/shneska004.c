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

	

	printf("3. Got the disk boot sector!\n");

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

	int * mftbb = malloc(sizeof(byte) * MFTRecordSize);


	LARGE_INTEGER li;
	li.QuadPart = (LONGLONG)ntboot->bpb.n64MFTLogicalClustNum * bytespercluster;
	errtest = SetFilePointer(dev, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (errtest == -1)
		outerr(GetLastError());
	printf("4. Reading $MFT\n");
	errtest = ReadFile(dev, mftbb, MFTRecordSize, &brre, NULL);
	if (errtest == 0)
		outerr(GetLastError());
	//struct NTFS_MFT_FILE *MFT = (struct NTFS_MFT_FILE*)mftbb;
	//BYTE * mftrecordtempvar = &mftbb;
	BYTE * mftrecordtempvar;
	__asm
	{
		PUSH EAX
		MOV EAX, mftbb
		MOV mftrecordtempvar, EAX
		POP EAX
	}
	struct NTFS_MFT_FILE *MFT = (struct NTFS_MFT_FILE*)mftrecordtempvar;
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
			/*DWORD filenamepointer = ntatr_fnm->wNameOffset;
			char * dc = *(mftrecordtempvar + filenamepointer);*/
				//BYTE filesize = mftrecordtempvar + 0x40; invalid?
			
			wchar_t *c = (mftrecordtempvar + (0x42 + ntatr_fnm->wNameOffset)); // $MFT is near, use memory viewer!
			if (!lstrcmpW(c, L"$FMT"))
				return -1;


		}

		if (ntatr->dwType == -1)
			break;

		curpos += nextatr;
	}

	FILE * fwriteout = fopen("D:\debug.log", "a+");


	//LOOPSTER
	while (1)
	{
	returnhere:
		if (bShouldStop > 30)
			break;
		//SetFilePointer(dev, 1024, NULL, FILE_CURRENT); //I don't fucking believe I actually left it here
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
			//LEA EAX, mftrecordtempvar
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
			/*if (IsBadReadPtr(mftrecordtempvar, 0x30) != 0)
				break;*/
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

				//wchar_t *c = (mftrecordtempvar + (0x42 + relFpath));
				wchar_t *c = (mftrecordtempvar + (0x5A));
				//wprintf(L"%ls\n", c);
				if(!lstrcmpW(c, L"SAM"))
					wprintf(L"%ls\n", c);
				//wprintf(L"%ls\n", c);
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