Low level application to communicate with NTFS driver in order to get USN record in order to obtain the raw sector of files that are kernel-locked

It opens system disk partition as device, enumerates USN journal and loops through records to find sectors in which wanted files are
It is very dangerous software as direct access to hard drive can harm your system and make it unstable!

Status: IN-DEV!


DONE:
-Open device to querying
-Get USN journal 

NEXT:
-Get USN records from journal
-Enum records to find wanted files index
-Read SAM/SECURITY sector locations
-Read raw sectors