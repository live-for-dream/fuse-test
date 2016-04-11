#define FUSE_USE_VERSION 26
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <dirent.h>

#include "list.h"
 
#define MAX_NAMELEN 255

#define dst_prefix_path "/root/fuse"
#define dst_prefix_path_len 10
 
struct ou_entry {
    mode_t mode;
    struct list_node node;
    char name[MAX_NAMELEN + 1];
};
 
static struct list_node entries;

static int ou_redirect(const char *src, const char *prefix, char **dst) {
	int src_len;
	int prefix_len;
	
	src_len = strlen(src);
	prefix_len = strlen(prefix);
	*dst = (char *)malloc((src_len + prefix_len + 1) * sizeof(char));
	if (!*dst) {
		return -ENOMEM;
//		return -1;
	}

	sprintf(*dst, "%s%s", prefix, src);
	return 0;
}
 
static int ou_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info* fi)
{
 	DIR *dp;
	struct dirent *de;
	char *redirect_name;
	int ret;
	
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

	ret = ou_redirect(path, dst_prefix_path, &redirect_name);
	if (ret < 0) {
		return ret;
	}
	
	dp = opendir(redirect_name);
	if (dp == NULL) {
		free(redirect_name);
		return -errno;
	}

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}
 	free(redirect_name);
    return 0;
}

static int ou_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char *redirect_name;
	int ret;

	ret = ou_redirect(path, dst_prefix_path, &redirect_name);
	if (ret < 0) {
		return ret;
	}

	res = lstat(redirect_name, stbuf);
	if (res == -1) {
		free(redirect_name);
		return -errno;
	}
	free(redirect_name);
	return 0;
}

static int ou_truncate(const char *path, off_t size)
{
	int res;
	char *redirect_name;
	int ret;

	ret = ou_redirect(path, dst_prefix_path, &redirect_name);
	if (ret < 0) {
		return ret;
	}

	res = truncate(path, size);
	if (res == -1)
		return -errno;
	
	free(redirect_name);
	return 0;
}


static int ou_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int 		fd;
	char 	   *redirect_name;
	int 		ret;
	
	ret = ou_redirect(path, dst_prefix_path, &redirect_name);
	if (ret < 0) {
		return ret;
	}

	fd = open(redirect_name, fi->flags, mode);
	if (fd == -1)
		return -errno;

	free(redirect_name);
	fi->fh = fd;
	return 0;
}

static int ou_open(const char *path, struct fuse_file_info *fi)
{
	int 		fd;
	char 	   *redirect_name;
	int 		ret;
		
	ret = ou_redirect(path, dst_prefix_path, &redirect_name);
	if (ret < 0) {
		return ret;
	}

	fd = open(redirect_name, fi->flags);
	if (fd == -1) {
		free(redirect_name);
		return -errno;
	}
	
	free(redirect_name);
	fi->fh = fd;
	return 0;
}


#define prefix_path "/fuse/mnts"
#define prefix_path_len 10
#define prefix_sum (dst_prefix_path_len - prefix_path_len)

static int ou_read(const char* name, char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	off_t 		off;
	int 		ret;
	
	off = lseek(info->fh, offset, SEEK_SET);
	if (off < 0) {
		return -errno;
	}
	ret = read(info->fh, buf, size);
	return ret;
}

static int ou_write(const char *name, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {

	off_t off;
	int ret = -1;

	off = lseek(info->fh, offset, SEEK_SET);
	if (off < 0) {
		return -errno;
	}
		
	ret = write(info->fh, buf, size);

	return ret;
}
 
static int ou_unlink(const char* path)
{
    struct list_node *n, *p;
 
    list_for_each_safe (n, p, &entries) {
        struct ou_entry* o = list_entry(n, struct ou_entry, node);
        if (strcmp(path + 1, o->name) == 0) {
            __list_del(n);
            free(o);
            return 0;
        }
    }
 
    return -ENOENT;
}
 
static struct fuse_operations oufs_ops = {
    .getattr    =   ou_getattr,
    .readdir    =   ou_readdir,
    .create     =   ou_create,
    .unlink     =   ou_unlink,
    .read		= 	ou_read,
    .write 		=	ou_write,
    .open		= 	ou_open,
    .truncate	=	ou_truncate,
};
 
int main(int argc, char* argv[])
{
    list_init(&entries);
 
    return fuse_main(argc, argv, &oufs_ops, NULL);
}
