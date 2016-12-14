/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>
  Further modifications by William Christie (2016) <william.christie@colorado.edu>
		to create encrypted FUSE file system using SSL.
  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
  gcc -Wall `pkg-config fuse --cflags` fusepa5_encfs.c -o fusepa5_encfs `pkg-config fuse --libs`
  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.
*/


#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

/*for crypto*/
#include "aes-crypt.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

/* Per Private Data
 * http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/private.html
 *
 * #define BB_DATA ((struct bb_state *) fuse_get_context() -> private_data)
 * struct bb_state {
 *   char *rootdir;
 *   FILE *logfile;
 * };
 * 
 * All information within the type file_info is private data */
#define ENCFS_DATA ((file_data *) fuse_get_context() -> private_data)

typedef struct {
	char *rootdir;
	char *ekey;
} file_data;

/* for file operations */
FILE *open_memstream(char **ptr, size_t *sizeloc);

/* Per bb_fullpath 
 * http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/src/bbfs.c */
static void pa5_encfs_fullpath(char fpath[PATH_MAX], const char *path)
{
	/* copy string into ENCFS_DATA, concatenate together */
	strcpy(fpath, ENCFS_DATA -> rootdir);
	strncat(fpath, path, PATH_MAX);	
}

static int pa5_encfs_getattr(const char *path, struct stat *stbuf)
{
	int res;	
	/* http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/callbacks.html
	 * When function called, passed a file path (default is root directory) and maintains file info. 
	 * This function called every time path referenced.
	 * update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = lstat(fpath, stbuf); /* get file status */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_access(const char *path, int mask)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = access(fpath, mask); /* check user's permissions for file */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_readlink(const char *path, char *buf, size_t size)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	res = readlink(fpath, buf, size - 1); /* Print value of a symbolic link or canonical file name */
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int pa5_encfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	(void) offset;
	(void) fi;
	/* struct dirent {
	   ino_t          d_ino;       // inode number 
	   off_t          d_off;       // not an offset; see NOTES 
	   unsigned short d_reclen;    // length of this record 
	   unsigned char  d_type;      // type of file; not supported
									  by all filesystem types 
	   char           d_name[256]; // filename 
       };*/

	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	dp = opendir(fpath); /* open a directory */
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) { /*returns a pointer to a dirent structure representing the next directory entry */
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp); /* close a directory */
	return 0;
}

static int pa5_encfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);	/* make FIFO's, named pipes */
	else
		res = mknod(fpath, mode, rdev); /* make block or special character files */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_mkdir(const char *path, mode_t mode)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	res = mkdir(fpath, mode); /* create a directory */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_unlink(const char *path)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	res = unlink(fpath); /* Call the unlink function to remove the specified FILE. */ 
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_rmdir(const char *path)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	res = rmdir(fpath); /* remove an empty directory */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to); /* create a symbolic link named (to) which contains the string (from)*/
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to); /* rename file */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_link(const char *from, const char *to)
{
	int res;

	res = link(from, to); /* Call the link function to create a link named FILE2 to an existing FILE1 */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_chmod(const char *path, mode_t mode)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = chmod(fpath, mode); /* change permissions on a file */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = lchown(fpath, uid, gid); /* change the owner and group of a file */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_truncate(const char *path, off_t size)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = truncate(fpath, size); /* shrink or extend the size of a file to the specified size */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];
	/* update file path  */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	/* 0 = access, 1 = modify */
	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv); /* change file last access and modification times */
	if (res == -1)
		return -errno;

	return 0;
}	

static int pa5_encfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = open(fpath, fi->flags); /* open file */
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int pa5_encfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int res;
	/* set up file and mirrored file */
	FILE* fd;
	FILE* fdM;
	/* buffer value and length to be written into for mirroring */
	char* mv;
	size_t mvlen;
	char xval[5];
	(void) fi;

	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	/* open file with read permissions */
	fd = fopen(fpath, "r"); 
	if (fd == NULL) {
		return -errno;
	}

	/* Set file buffer and length for mirror file directory */
	fdM = open_memstream(&mv, &mvlen);
	if(fdM == NULL) {
		return -errno;
	}

	/* check if file is encrypted */
	if(getxattr(fpath, "user.encrypted", xval, 5) != -1) {	
		/* if encrypted, decrypt and copy */
		do_crypt(fd, fdM, 0, ENCFS_DATA -> ekey);
	} else {
		/* if not encrypted, pass through case and copies */
		do_crypt(fd, fdM, -1, ENCFS_DATA -> ekey);
	}
	
	/* wait for ouput file */
	fflush(fdM);

	/* determine load location in mirror */
	fseek(fdM, offset, SEEK_SET);

	/* read mirror file */
	res = fread(buf, 1, size, fdM);
	if (res == -1) {
		res = -errno;
	}

	/* close files and return */
	fclose(fd);
	fclose(fdM);	
	return res;
}

static int pa5_encfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi) {
	int res;
	/* set up file and mirrored file */
	FILE* fd;
	FILE* fdM;
	/* Buffer value and length to be written into for mirroring */
	char* mv;
	size_t mvlen;
	/* for xtended attribute encrypted*/
	char xval[5];

	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	(void) fi;
	
	/* open file with read permissions first to check encryption */
	fd = fopen(fpath, "r"); 
	if (fd == NULL) {
		return -errno;
	}
	
	/* set file buffer and length for mirror file dir */
	fdM = open_memstream(&mv, &mvlen);
	if (fdM == NULL) {
		return -errno;
	}

	/* detemine if already encrypted by checking xattr, if yes then decrypt */
	if (getxattr(fpath, "user.encrypted", xval, 5) != -1) {
		do_crypt(fd, fdM, 0, ENCFS_DATA -> ekey);
	}
	
	/* search for load location of mirror */
	fseek(fdM, offset, SEEK_SET);
	
	/* write to mirror file */
	res = fwrite(buf, 1, size, fdM);
	if (res == -1) {
		res = -errno;
	}
	
	/* wait for completion */
	fflush(fdM);

	/* Close file and reopen with write permissions */
	fclose(fd);
	fd = fopen(fpath, "w");

        /*Searches for load location of mirror, without an offset, this will be used for encryption */
	fseek(fdM, 0, SEEK_SET);

	/* write to fd, writing will always encrypt, pass 1 to do_crypt */
	do_crypt(fdM, fd, 1, ENCFS_DATA -> ekey);

	/* setxattr
	 * http://man7.org/linux/man-pages/man2/setxattr.2.html
	 * update user.encrypted extended attr to true 
	 * <path, name, value, size(bytes), flags > */
	setxattr(fpath, "user.encrypted", "true", 4, 0);

	/* close and return */
	fclose(fd);
	fclose(fdM);	
	return res;
}

static int pa5_encfs_statfs(const char *path, struct statvfs *stbuf) {
	int res;
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	res = statvfs(fpath, stbuf); /* get file system statistics */
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
	(void) fi;
	(void) mode;

	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);

	int res;
	/* set up file and mirrored file (aka temp to make do_crypt work) */
	FILE* fd;
	FILE* fdM; 
	/* buffer value and length to be written into */
	char* mv;
	size_t mvlen;

	/* open file */
	fd = fopen(fpath, "w");
	if(fd == NULL)
		return -errno;
	
	/*set up mirroring */
	fdM = open_memstream(&mv, &mvlen);
	if(fdM == NULL) {
		return -errno;
	}
	
	/* encrypt new file */
	do_crypt(fdM, fd, 1, ENCFS_DATA -> ekey);
	
	/* set extended attribute user.encrypted  to true */
	res = setxattr(fpath, "user.encrypted", "true", 4, 0);
	if(res) {
		return -errno;
	}

	/* close and return */
	fclose(fd); 
	fclose(fdM);
	return 0;
}


static int pa5_encfs_release(const char *path, struct fuse_file_info *fi) {
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int pa5_encfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi) {
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int pa5_encfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags) {
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int pa5_encfs_getxattr(const char *path, const char *name, char *value,
			size_t size) {
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int pa5_encfs_listxattr(const char *path, char *list, size_t size) {
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int pa5_encfs_removexattr(const char *path, const char *name) {
	/* update file path */
	char fpath[PATH_MAX];
	pa5_encfs_fullpath(fpath, path);
	
	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations pa5_encfs_oper = {
	.getattr		= pa5_encfs_getattr,
	.access			= pa5_encfs_access,
	.readlink		= pa5_encfs_readlink,
	.readdir		= pa5_encfs_readdir,
	.mknod			= pa5_encfs_mknod,
	.mkdir			= pa5_encfs_mkdir,
	.symlink		= pa5_encfs_symlink,
	.unlink			= pa5_encfs_unlink,
	.rmdir			= pa5_encfs_rmdir,
	.rename			= pa5_encfs_rename,
	.link			= pa5_encfs_link,
	.chmod			= pa5_encfs_chmod,
	.chown			= pa5_encfs_chown,
	.truncate		= pa5_encfs_truncate,
	.utimens		= pa5_encfs_utimens,
	.open			= pa5_encfs_open,
	.read			= pa5_encfs_read,
	.write			= pa5_encfs_write,
	.statfs			= pa5_encfs_statfs,
	.create         = pa5_encfs_create,
	.release		= pa5_encfs_release,
	.fsync			= pa5_encfs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr		= pa5_encfs_setxattr,
	.getxattr		= pa5_encfs_getxattr,
	.listxattr		= pa5_encfs_listxattr,
	.removexattr	= pa5_encfs_removexattr,
#endif
};

int main(int argc, char *argv[]) 
{
	/* 
	 * Citation: CS135 FUSE Documentation - Parsing the Command Line and Initializing FUSE
	 * http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/init.html
	 * bb_data->rootdir = realpath(argv[argc-2], NULL);
	 * argv[argc-2] = argv[argc-1];
	 * argv[argc-1] = NULL;
	 * argc--;
	 */

	file_data *fdata;
	umask(0);

	/* check args
	 * Usage: ./pa5-pa5_encfs <encryption key><mirror dir><mount dir> */
	if(argc < 4) {
		fprintf(stderr, "Not enough arguments.\nUsage: ./pa5-pa5_encfs <key> <mirror dir> <mount dir>\n");
		return 1;
	}

	 /* Per CS135 FUSE Documentation: Private Data*/
	fdata = malloc(sizeof(file_data));
	if(fdata == NULL) {
		perror("ERROR: Failed memory allocation.\n");
		abort();
	}

	/* Parsing the Command Line and Initializing FUSE: Stores path and encryption key. Pull the root directory out of the argument list and saves it
	realpath() function to translate to a canonicalized absolute pathname */
	fdata -> rootdir = realpath(argv[argc - 2], NULL);
	fdata -> ekey = argv[argc - 3]; 

	/* Reorganize command arguments in order to pass the arguments into fuse_main */
	argv[argc - 3] = argv[argc - 1];
	argv[argc - 2] = NULL;
	argv[argc - 1] = NULL;
	argc -= 2;

	return fuse_main(argc, argv, &pa5_encfs_oper, fdata);
} 	
