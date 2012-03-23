/**
 * COMPLETELY Untested (not even compiled) module implementation idea.
 *  I don't have the linux headers on this machine...
 */

#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/proc_fs.h>	/* Necessary because we use proc fs */

#include "httpc.h"

#define PROC_ENTRY_FILENAME 	"rssfs"

#define RSSFS_URL               "http://news.google.com/?output=rss"

struct rssfs_info {
    /** connection identifier */
    int conn;
};

/**
 * The structure keeping information about the /proc file
 *
 */
static struct proc_dir_entry *Our_Proc_File;

/**
 * This function is called when the /proc file is read
 *
 */
static ssize_t procfs_read(struct file *filp,	/* see include/linux/fs.h   */
			     char *buffer,	/* buffer to fill with data */
			     size_t length,	/* length of the buffer     */
			     loff_t * offset)
{
    struct rssfs_info *info = (struct rssfs_info*)filp->private_data;
	return httpc_read(info->conn, buffer, length);
}

/* 
 * This function decides whether to allow an operation
 * (return zero) or not allow it (return a non-zero
 * which indicates why it is not allowed).
 *
 * The operation can be one of the following values:
 * 0 - Execute (run the "file" - meaningless in our case)
 * 2 - Write (input to the kernel module)
 * 4 - Read (output from the kernel module)
 *
 * This is the real function that checks file
 * permissions. The permissions returned by ls -l are
 * for referece only, and can be overridden here.
 */

static int module_permission(struct inode *inode, int op, struct nameidata *foo)
{
	/* 
	 * We allow everybody to read from our module, and that's it
	 */
	if (op == 4)
		return 0;

	/* 
	 * If it's anything else, access is denied 
	 */
	return -EACCES;
}

/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
    int conn = httpc_open(RSSFS_URL);
    if (conn > -1) {
        struct rssfs_info *info = (struct rssfs_info *)malloc(
            sizeof(struct rssfs_info));
        info->conn = conn;
        file->private_data = info;
        return 0;
    }

    return -1;
}

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
    free(file->private_data);
	module_put(THIS_MODULE);
	return 0;		/* success */
}

static struct file_operations File_Ops_4_Our_Proc_File = {
	.read 	 = procfs_read,
	.open 	 = procfs_open,
	.release = procfs_close,
};

/* 
 * Inode operations for our proc file. We need it so
 * we'll have some place to specify the file operations
 * structure we want to use, and the function we use for
 * permissions. It's also possible to specify functions
 * to be called for anything else which could be done to
 * an inode (although we don't bother, we just put
 * NULL). 
 */

static struct inode_operations Inode_Ops_4_Our_Proc_File = {
	.permission = module_permission,	/* check for permissions */
};

/* 
 * Module initialization and cleanup 
 */
int init_module()
{
	/* create the /proc file */
	Our_Proc_File = create_proc_entry(PROC_ENTRY_FILENAME, 0644, NULL);
	
	/* check if the /proc file was created successfuly */
	if (Our_Proc_File == NULL){
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       PROC_ENTRY_FILENAME);
		return -ENOMEM;
	}
	
	Our_Proc_File->owner = THIS_MODULE;
	Our_Proc_File->proc_iops = &Inode_Ops_4_Our_Proc_File;
	Our_Proc_File->proc_fops = &File_Ops_4_Our_Proc_File;
	Our_Proc_File->mode = S_IFREG | S_IRUGO | S_IWUSR;
	Our_Proc_File->uid = 0;
	Our_Proc_File->gid = 0;
	Our_Proc_File->size = 80;

	printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);

	return 0;	/* success */
}

void cleanup_module()
{
	remove_proc_entry(PROC_ENTRY_FILENAME, &proc_root);
	printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);
}

