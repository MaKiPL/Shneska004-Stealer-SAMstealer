#include <stdio.h>
#include <Windows.h>


void outerr(DWORD errmess);
char rootdriver;
DWORD CheckFile(USN_RECORD *rec);

ULONG64 filescount = 0;

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

DWORD CheckFile(USN_RECORD *rec) //DEBUG version
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
