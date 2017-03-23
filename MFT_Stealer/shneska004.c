#include <stdio.h>
#include <Windows.h>


void outerr(DWORD errmess);
char rootdriver;

const wchar_t * sam = L"SAM";
const wchar_t * sec = L"SECURITY";

int main()
{
	char * c = getenv("SYSTEMROOT");
	rootdriver = *c ;
	wchar_t * py = (wchar_t*)malloc(32);
	wsprintf(py, L"%s%c%s", L"\\\\.\\", rootdriver, ":");
	HANDLE dev = CreateFile(py, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
	if (dev == INVALID_HANDLE_VALUE)
		return -1;
	
	MFT_ENUM_DATA * mftenumdata = malloc(sizeof( MFT_ENUM_DATA));
	
	mftenumdata->StartFileReferenceNumber = 0;
	mftenumdata->LowUsn = 0;

	
	DWORD bufffsz = 1024 * 1024;
	VOID * outbuff = malloc(bufffsz);
	DWORD brre = 1;
	if (!DeviceIoControl(dev, FSCTL_QUERY_USN_JOURNAL, NULL, 0, outbuff, bufffsz, &brre, NULL))
		outerr(GetLastError());

	USN_JOURNAL_DATA * jrnl;
	USN_RECORD *usn;
	jrnl = (USN_JOURNAL_DATA*)outbuff;

	mftenumdata->LowUsn = jrnl->LowestValidUsn;
	mftenumdata->HighUsn = jrnl->MaxUsn;

	if (DeviceIoControl(dev, FSCTL_ENUM_USN_DATA, &mftenumdata, sizeof(mftenumdata), outbuff, bufffsz, &brre, NULL))
	{
		usn = (USN_RECORD*)outbuff;
		wprintf("%s", usn->FileName);
	}
	else
		outerr(GetLastError());



    return 0;
}

void outerr(DWORD errmess)
{
	char * p = malloc(10);
	sprintf(p,"ERROR: %u", errmess);
	OutputDebugStringA(p);
}

