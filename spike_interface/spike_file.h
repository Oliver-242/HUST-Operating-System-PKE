#ifndef _SPIKE_FILE_H_
#define _SPIKE_FILE_H_

#include <unistd.h>
#include <sys/stat.h>

#include "util/types.h"

typedef struct file {
  int kfd;  // file descriptor of the host file
  uint32 refcnt;
} spike_file_t;

extern spike_file_t spike_files[];

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02
#define ENOMEM 12 /* Out of memory */

#define stdin (spike_files + 0)
#define stdout (spike_files + 1)
#define stderr (spike_files + 2)

#define INIT_FILE_REF 3

struct frontend_stat {
  uint64 dev;
  uint64 ino;
  uint32 mode;
  uint32 nlink;
  uint32 uid;
  uint32 gid;
  uint64 rdev;
  uint64 __pad1;
  uint64 size;
  uint32 blksize;
  uint32 __pad2;
  uint64 blocks;
  uint64 atime;
  uint64 __pad3;
  uint64 mtime;
  uint64 __pad4;
  uint64 ctime;
  uint64 __pad5;
  uint32 __unused4;
  uint32 __unused5;
};

void copy_stat(struct stat* dest, struct frontend_stat* src);
spike_file_t* spike_file_open(const char* fn, int flags, int mode);
int spike_file_close(spike_file_t* f);
spike_file_t* spike_file_openat(int dirfd, const char* fn, int flags, int mode);
ssize_t spike_file_lseek(spike_file_t* f, size_t ptr, int dir);
ssize_t spike_file_read(spike_file_t* f, void* buf, size_t size);
ssize_t spike_file_pread(spike_file_t* f, void* buf, size_t n, off_t off);
ssize_t spike_file_write(spike_file_t* f, const void* buf, size_t n);
void spike_file_decref(spike_file_t* f);
void spike_file_init(void);
int spike_file_dup(spike_file_t* f);
int spike_file_truncate(spike_file_t* f, off_t len);
int spike_file_stat(spike_file_t* f, struct stat* s);

#endif
