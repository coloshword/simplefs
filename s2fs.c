#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/fs.h>

#define S2FS_MAGIC_NUM 0xFFF34 //magic num to represent our file system
#define HELLO_WORLD_STR "Hello World!"
#define HELLO_STR_SIZE (sizeof(HELLO_WORLD_STR) - 1)    

/* part 2.1 : create inode function */
/*
*s2fs_make_inode: accepts 2 inputs, the superblock, mode (directory, vs a file)
*/
static struct inode *s2fs_make_inode(struct super_block *sb, int mode) {
    struct inode *inode;
    inode = new_inode(sb);
    if(!inode) {
        printk(KERN_INFO "ERROR creating inode in s2fs_make_inode");
        return NULL;
    }
    // set the fields of the new inode
    inode->i_mode = mode;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    //REMEMBER TO SET FILE OPS AFTER DEFINING THEM IN 2.3, change header to include file ops
    inode->i_fop = &simple_dir_operations; // assuming this is the default, we will change the permissions manually on the others 
    inode->i_ino = get_next_ino();
    return inode;
}


/*
* 2.3: Define file operations
*/

/* 
* s2fs_open: returns 0
*/
static int s2fs_open(struct inode *inode, struct file *file_ptr) {
    return 0;
}

/*
* s2fs_write_file: returns 0
*/
static ssize_t s2fs_write_file(struct file *file_ptr, const char __user *buf, size_t len, loff_t *offset) {
    return 0;
}
/*
* s2fs_read_file(): returns "Hello World!"
*/
static ssize_t s2fs_read_file(struct file *file_ptr, char __user *buf, size_t count, loff_t *offset) {
    if(*offset >= HELLO_STR_SIZE) {
        // past end of str, done reading  
        return 0;
    }

    if(count > HELLO_STR_SIZE - *offset) {
        count = HELLO_STR_SIZE - *offset;
    }
    // copy to buffer
    if(copy_to_user(buf, HELLO_WORLD_STR + *offset, count)) {
        return -EFAULT;
    }
    *offset += count;
    return count;
}


/*
* s2fs_fops: defines the file operations for s2fs
*/
static struct file_operations s2fs_fops = {
    .open = s2fs_open,
    .write = s2fs_write_file,
    .read = s2fs_read_file,
};


/* part 2.2: create a directory*/
/*s2fs_create_dir: creates a directory, returns 0 if fail and also handles cleaning up of dentry. */
static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name) {
    struct dentry *new_dentry; // the dentry we are connecting to the parent dentry
    struct inode *new_inode; // inode to represent the directory

    new_dentry = d_alloc_name(parent, dir_name); // initialize dentry to name, dir_name, and connected to parent dentry (parent directory)
    if(!new_dentry) {
        return 0;
    }
    //make new inode as type directory with permission to read and write. 
    new_inode = s2fs_make_inode(sb, S_IFDIR | 0755);
    if(!new_inode) {
        dput(new_dentry);
        return 0;
    }
    new_inode->i_op = &simple_dir_inode_operations;
    // associate new_dentry with new_inode
    d_add(new_dentry, new_inode);
    return new_dentry;
}

/* part 2.4: create a file 
*s2fs_create_file --> creates a file, returns 0 if unsuccesful
*/
static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *dir, const char *file_name) {
    //dentry *dir is should point to the inode for the file 
    struct dentry *new_dentry;
    struct inode *new_inode;

    new_dentry = d_alloc_name(dir, file_name); //associate new dentry with parent dentry (dir) + name here because inode does not hold name
    if(!new_dentry) {
        return 0; // don't dput cause dentry not created
    }
    // make the inode
    new_inode = s2fs_make_inode(sb, S_IFREG | 0644); 
    if(!new_inode) {
        dput(new_dentry); // remove bc not used
        return 0;
    }
    // have to set the operations to something else
    new_inode->i_fop = &s2fs_fops;
    d_add(new_dentry, new_inode);
    return new_dentry;
}


/*
*define superblock operations
*/
static struct super_operations s2fs_sb_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

/*
*s2fs_fill_super: defines and fills a superblock
returns 0 if success, 1 if failure
*/
static int s2fs_fill_super(struct super_block *sb, void *data, int silent){
    struct inode *root_inode;
    struct dentry *root_dentry;
    struct dentry *foo_dentry;
    struct dentry *bar_dentry;
    /// fill super_block with info 
    sb->s_blocksize = 4096;
    sb->s_blocksize_bits = 12;
    sb->s_magic = S2FS_MAGIC_NUM;
    sb->s_op = &s2fs_sb_ops;

    //create root inode. Need to define inode for part 2.1 first. Also have permissions. 
    root_inode = s2fs_make_inode(sb, S_IFDIR | 0755);
    inode_init_owner(&init_user_ns, root_inode, NULL, S_IFDIR | 0755); //NULL because no parent dir for root, permission --> is a directory also gives read and write permissions only within the hiearchy
    if(!root_inode) {
        return -ENOMEM;
    }
    root_inode->i_op = &simple_dir_inode_operations;
    set_nlink(root_inode, 2);
    root_dentry = d_make_root(root_inode);
    if(!root_dentry) {
        iput(root_inode);
        return -ENOMEM;
    }

    /* if i need to create files, i will put them here*/
    sb->s_root = root_dentry;

    foo_dentry = s2fs_create_dir(sb, root_dentry, "foo");
    bar_dentry = s2fs_create_file(sb, foo_dentry, "bar");

    return 0;
}
    

//need a mount function and a kill_sb function
static struct dentry *s2fs_mount(struct file_system_type *fst, int flags, const char *devname, void *data) {
    return mount_nodev(fst, flags, data, s2fs_fill_super);
}

// define file system type
static struct file_system_type s2fs_type = {
    .owner = THIS_MODULE,
    .name = "s2fs",
    .mount = s2fs_mount,
    .kill_sb = kill_anon_super,
};


int init_module(void) {
    int ret;
    ret = register_filesystem(&s2fs_type);
    if (ret != 0) {
        printk(KERN_INFO "Failed to register s2fs: %d\n", ret);
        return ret;
    }

    return 0;
}


void cleanup_module(void) {
    unregister_filesystem(&s2fs_type);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Acero Liang Li");