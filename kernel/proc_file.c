/*
 * Interface functions between file system and kernel/processes. added @lab4_1
 */

#include "proc_file.h"

#include "hostfs.h"
#include "pmm.h"
#include "process.h"
#include "ramdev.h"
#include "rfs.h"
#include "riscv.h"
#include "spike_interface/spike_file.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"
#include "util/string.h"

/**
 * 分三种情况讨论
 * @return './'——1 、'..'——2、'../'——3、other——0
 */
int isRelativePath(const char *path)
{
  if (strlen(path) >= 2 && path[0] == '.' && path[1] == '/')
  {
    return 1;
  }
  else if (strcmp(path, "..") == 0)
  {
    return 2;
  }
  else if (strlen(path) >= 3 && path[0] == '.' && path[1] == '.' && path[2] == '/')
  {
    return 3;
  }
  else
  {
    return 0;
  }
}

/**
 * 传入的最好是路径的副本，因为函数会对path做修改
 */
void changeToAbsolutePath(char *path)
{
  struct dentry *tmp = current->pfiles->cwd;
  // 循环获取路径信息
  int i = 1;
  switch (isRelativePath(path))
  {
  case 1:
    for (; path[i]; i++)
    {
      path[i - 1] = path[i];
    }
    path[i - 1] = '\0';
    break;
  case 2:
    tmp = tmp->parent;
    i = 2;
    for (; path[i]; i++)
    {
      path[i - 2] = path[i];
    }
    path[i - 2] = '\0';
    break;
  case 3:
    tmp = tmp->parent;
    i = 2;
    for (; path[i]; i++)
    {
      path[i - 2] = path[i];
    }
    path[i - 2] = '\0';
    break;
  default:
    return;
  }
  do
  {
    char tmp_path[30];
    if (tmp->parent)
    {
      strcat(tmp_path, "/");
    }
    strcpy(tmp_path, tmp->name);
    strcat(tmp_path, path);
    strcpy(path, tmp_path);
    tmp = tmp->parent;

  } while (tmp);
}

//
// initialize file system
//
void fs_init(void)
{
  // initialize the vfs
  vfs_init();

  // register hostfs and mount it as the root
  if (register_hostfs() < 0)
    panic("fs_init: cannot register hostfs.\n");
  struct device *hostdev = init_host_device("HOSTDEV");
  vfs_mount("HOSTDEV", MOUNT_AS_ROOT);

  // register and mount rfs
  if (register_rfs() < 0)
    panic("fs_init: cannot register rfs.\n");
  struct device *ramdisk0 = init_rfs_device("RAMDISK0");
  rfs_format_dev(ramdisk0);
  vfs_mount("RAMDISK0", MOUNT_DEFAULT);
}

//
// initialize a proc_file_management data structure for a process.
// return the pointer to the page containing the data structure.
//
proc_file_management *init_proc_file_management(void)
{
  proc_file_management *pfiles = (proc_file_management *)alloc_page();
  pfiles->cwd = vfs_root_dentry; // by default, cwd is the root
  pfiles->nfiles = 0;

  for (int fd = 0; fd < MAX_FILES; ++fd)
    pfiles->opened_files[fd].status = FD_NONE;

  sprint("FS: created a file management struct for a process.\n");
  return pfiles;
}

//
// reclaim the open-file management data structure of a process.
// note: this function is not used as PKE does not actually reclaim a process.
//
void reclaim_proc_file_management(proc_file_management *pfiles)
{
  free_page(pfiles);
  return;
}

//
// get an opened file from proc->opened_file array.
// return: the pointer to the opened file structure.
//
struct file *get_opened_file(int fd)
{
  struct file *pfile = NULL;

  // browse opened file list to locate the fd
  for (int i = 0; i < MAX_FILES; ++i)
  {
    pfile = &(current->pfiles->opened_files[i]); // file entry
    if (i == fd)
      break;
  }
  if (pfile == NULL)
    panic("do_read: invalid fd!\n");
  return pfile;
}

//
// open a file named as "pathname" with the permission of "flags".
// return: -1 on failure; non-zero file-descriptor on success.
//
int do_open(char *pathname, int flags)
{
  char tmp[50];
  strcpy(tmp, pathname);
  // 判断是否使用了相对路径
  changeToAbsolutePath(tmp);
  // sprint("%s\n", tmp);
  struct file *opened_file = NULL;
  if ((opened_file = vfs_open(tmp, flags)) == NULL)
    return -1;

  int fd = 0;
  if (current->pfiles->nfiles >= MAX_FILES)
  {
    panic("do_open: no file entry for current process!\n");
  }
  struct file *pfile;
  for (fd = 0; fd < MAX_FILES; ++fd)
  {
    pfile = &(current->pfiles->opened_files[fd]);
    if (pfile->status == FD_NONE)
      break;
  }

  // initialize this file structure
  memcpy(pfile, opened_file, sizeof(struct file));

  ++current->pfiles->nfiles;
  return fd;
}

//
// read content of a file ("fd") into "buf" for "count".
// return: actual length of data read from the file.
//
int do_read(int fd, char *buf, uint64 count)
{
  struct file *pfile = get_opened_file(fd);

  if (pfile->readable == 0)
    panic("do_read: no readable file!\n");

  char buffer[count + 1];
  int len = vfs_read(pfile, buffer, count);
  buffer[count] = '\0';
  strcpy(buf, buffer);
  return len;
}

//
// write content ("buf") whose length is "count" to a file "fd".
// return: actual length of data written to the file.
//
int do_write(int fd, char *buf, uint64 count)
{
  struct file *pfile = get_opened_file(fd);

  if (pfile->writable == 0)
    panic("do_write: cannot write file!\n");

  int len = vfs_write(pfile, buf, count);
  return len;
}

//
// reposition the file offset
//
int do_lseek(int fd, int offset, int whence)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_lseek(pfile, offset, whence);
}

//
// read the vinode information
//
int do_stat(int fd, struct istat *istat)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_stat(pfile, istat);
}

//
// read the inode information on the disk
//
int do_disk_stat(int fd, struct istat *istat)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_disk_stat(pfile, istat);
}

//
// close a file
//
int do_close(int fd)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_close(pfile);
}

//
// open a directory
// return: the fd of the directory file
//
int do_opendir(char *pathname)
{
  char tmp[50];
  strcpy(tmp, pathname);
  // 判断是否使用了相对路径
  changeToAbsolutePath(tmp);
  struct file *opened_file = NULL;
  if ((opened_file = vfs_opendir(tmp)) == NULL)
    return -1;

  int fd = 0;
  struct file *pfile;
  for (fd = 0; fd < MAX_FILES; ++fd)
  {
    pfile = &(current->pfiles->opened_files[fd]);
    if (pfile->status == FD_NONE)
      break;
  }
  if (pfile->status != FD_NONE) // no free entry
    panic("do_opendir: no file entry for current process!\n");

  // initialize this file structure
  memcpy(pfile, opened_file, sizeof(struct file));

  ++current->pfiles->nfiles;
  return fd;
}

//
// read a directory entry
//
int do_readdir(int fd, struct dir *dir)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_readdir(pfile, dir);
}

//
// make a new directory
//
int do_mkdir(char *pathname)
{
  char tmp[50];
  strcpy(tmp, pathname);
  // 判断是否使用了相对路径
  changeToAbsolutePath(tmp);
  return vfs_mkdir(tmp);
}

//
// close a directory
//
int do_closedir(int fd)
{
  struct file *pfile = get_opened_file(fd);
  return vfs_closedir(pfile);
}

//
// create hard link to a file
//
int do_link(char *oldpath, char *newpath)
{
  char tmp_old[50], tmp_new[50];
  strcpy(tmp_old, oldpath);
  strcpy(tmp_new, newpath);
  // 判断是否使用了相对路径
  changeToAbsolutePath(tmp_old);
  changeToAbsolutePath(tmp_new);
  return vfs_link(tmp_old, tmp_new);
}

//
// remove a hard link to a file
//
int do_unlink(char *path)
{
  return vfs_unlink(path);
}

int do_rcwd(char *path)
{
  memset(path, '\0', sizeof(path));
  struct dentry *tmp = current->pfiles->cwd;
  // 循环获取路径信息
  do
  {
    char tmp_path[30];
    if (tmp->parent)
    {
      strcat(tmp_path, "/");
    }
    strcpy(tmp_path, tmp->name);
    // 因为这里会将path直接粘贴到tmp_path上，所以有必要对path初始化清零
    strcat(tmp_path, path);
    // sprint("%s %s %d %d\n", tmp->name, tmp_path, strlen(tmp->name), strlen(tmp_path));
    strcpy(path, tmp_path);
    tmp = tmp->parent;
  } while (tmp);
  return 0;
}

int do_ccwd(const char *path)
{
  struct dentry *new_cwd = NULL;
  char miss_name[MAX_PATH_LEN];
  switch (isRelativePath(path))
  {
  case 1:
    new_cwd = lookup_final_dentry(path + 2, &current->pfiles->cwd, miss_name);
    break;
  case 2:
    new_cwd = current->pfiles->cwd->parent;
    break;
  case 3:
    new_cwd = lookup_final_dentry(path + 3, &current->pfiles->cwd->parent, miss_name);
    break;
  default:
    // 都不是则说明是用了绝对路径,传入根目录作为parent
    new_cwd = lookup_final_dentry(path, &vfs_root_dentry, miss_name);
    break;
  }

  // new_cwd为空则说明没有该文件或者已经是根目录了
  if (new_cwd == NULL)
  {
    return 1;
  }
  else
  {
    // 若不为空则将当前进程的cwd替换为new_cwd
    current->pfiles->cwd = new_cwd;
    return 0;
  }
}