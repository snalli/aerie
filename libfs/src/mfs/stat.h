#ifndef _STAT_H_KJI901
#define _STAT_H_KJI901

#define T_FREE 0   // Unallocated inode ? FIXME We probably need another way to
                   //   deduce whether an inode is free: e.g. no link to inode
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device

struct stat {
	short type;  // Type of file
	int dev;     // Device number
	uint ino;    // Inode number on device
	short nlink; // Number of links to file
	uint size;   // Size of file in bytes
};

#endif
