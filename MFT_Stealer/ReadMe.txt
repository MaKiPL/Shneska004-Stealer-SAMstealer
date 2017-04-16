Low level application to communicate with NTFS driver in order to get $MFT, locate wanted file and read raw sector

Status: In-dev (Only querying)

Currently, for system32/config/* files it only works on any Windows except Windows 10. They did some extremely clever trick and hid important files from $MFT (they are not in boot partition also)
You can't even access config files from Linux on Win10 partition (well, NTFS is their FS, so they did something there)!
Don't be afraid, everything is possible, I work it out! :)