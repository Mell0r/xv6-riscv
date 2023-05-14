#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "memlayout.h"
#include "syscall.h"

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    if(type == T_SYMLINK)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0){
    iunlockput(dp);
    return 0;
  }

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      goto fail;
  }

  if(dirlink(dp, name, ip->inum) < 0)
    goto fail;

  if(type == T_DIR){
    // now that success is guaranteed:
    dp->nlink++;  // for ".."
    iupdate(dp);
  }

  iunlockput(dp);

  return ip;

 fail:
  // something went wrong. de-allocate ip.
  ip->nlink = 0;
  iupdate(ip);
  iunlockput(ip);
  iunlockput(dp);
  return 0;
}

uint64 sys_symlink(void)
{  
    char target[MAXPATH], linkpath[MAXPATH];

    int target_length = argstr(0, target, MAXPATH);
    if (target_length >= MAXPATH || target_length == -1)
        return -1;

    int linkpath_length = argstr(1, linkpath, MAXPATH);
    if (linkpath_length < 0)
        return -1;

    begin_op();

    struct inode *ilink = create(linkpath, T_SYMLINK, 0, 0);
    if(ilink == 0) {
        end_op();
        return -1;
    }

    int written = writei(ilink, 0, (uint64)target, 0, target_length);
    iunlock(ilink);
    iput(ilink);

    end_op();

    if (written != target_length)
        return -1;
    return 0;
}

uint64 sys_readlink() {

  char linkpath[MAXPATH];
  if (argstr(0, linkpath, MAXPATH) < 0)
    return -1;

  uint64 user_buf;
  argaddr(1, &user_buf);

  begin_op(); 

  struct inode *ilink = namei(linkpath);
  if (ilink == 0) {
    iput(ilink);
    end_op();
    return -1;
  }
  if (ilink->type != T_SYMLINK) {
    iput(ilink);
    end_op();
    return -1;
  }

  ilock(ilink);
  int read = readi(ilink, 1, user_buf, 0, ilink->size);
  iunlock(ilink);
  iput(ilink);

  end_op();
  if (read <= 0)
    return -1;
  return read;
}