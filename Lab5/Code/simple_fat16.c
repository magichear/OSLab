#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#include "fat16.h"
#include "fat16_utils.h"

/* FAT16 volume data with a file handler of the FAT16 image file */
// 存储文件系统所需要的元数据的数据结构
// "抄袭"了https://elixir.bootlin.com/linux/latest/source/fs/fat/inode.c#L44
typedef struct {
	uint32_t sector_size;           // 逻辑扇区大小（字节）
	uint32_t sec_per_clus;          // 每簇扇区数
	uint32_t reserved;              // 保留扇区数
	uint32_t fats;                  // FAT表的数量
	uint32_t dir_entries;           // 根目录项数量
	uint32_t sectors;               // 文件系统总扇区数
	uint32_t sec_per_fat;           // 每个FAT表所占扇区数

    sector_t fat_sec;               // FAT表开始扇区
    sector_t root_sec;              // 根目录区域开始扇区
    uint32_t root_sectors;          // 根目录区域扇区数
    sector_t data_sec;              // 数据区域开始扇区
    
    uint32_t clusters;              // 文件系统簇数
    uint32_t cluster_size;          // 簇大小（字节）

    uid_t fs_uid;               // 可忽略，挂载FAT的用户ID，所有文件的拥有者都显示为该用户
    gid_t fs_gid;               // 可忽略，挂载FAT的组ID，所有文件的用户组都显示为该组
    struct timespec atime;      // 访问时间
    struct timespec mtime;      // 修改时间
    struct timespec ctime;      // 创建时间
} FAT16;

FAT16 meta;

sector_t cluster_first_sector(cluster_t clus) {
    assert(is_cluster_inuse(clus));
    return ((clus - 2) * meta.sec_per_clus) + meta.data_sec;
}

cluster_t sector_cluster(sector_t sec) {
    if(sec < meta.data_sec) {
        return 0;
    }
    cluster_t clus = 2 + (sec - meta.data_sec) / meta.sec_per_clus;
    assert(is_cluster_inuse(clus));
    return clus;
}

cluster_t read_fat_entry(cluster_t clus)
{
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    /**
     * TODO: 4.1 读取FAT表项 [约5行代码]
     * Hint: 你需要读取FAT表中clus对应的表项，然后返回该表项的值。
     *       表项在哪个扇区？在扇区中的偏移量是多少？表项的大小是多少？
     */
    // ================== Your code here =================
    /* 计算扇区号，每个FAT表项占用2字节 */
    sector_t sec = meta.fat_sec + clus / (meta.sector_size / 2);
    /* 计算扇区内偏移量 */
    size_t off = (clus % (meta.sector_size / 2)) * 2;
    /* 获取表项的大小 */
    int ret = sector_read(sec, sector_buffer);
    if(ret < 0) {
        return ret;
    }
    // ===================================================
    return *(cluster_t*)(sector_buffer + off);
//    return CLUSTER_END; // TODO: 记得删除或者修改这一行
}

typedef struct {
    DIR_ENTRY dir;
    sector_t sector;
    size_t offset;
} DirEntrySlot;

/**
 * @brief 寻找对应的目录项，从 name 开始的 len 字节为要搜索的文件/目录名
 * 
 * @param name 指向文件名的指针，不一定以'\0'结尾，因此需要len参数
 * @param len 文件名的长度
 * @param from_sector 开始搜索的扇区
 * @param sectors_count 需要搜索的扇区数
 * @param slot 输出参数，存放找到的目录项和位置
 * @return long 找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，扇区均满了返回 FIND_FULL，错误返回错误代码的负值
 */
int find_entry_in_sectors(const char* name, size_t len, 
            sector_t from_sector, size_t sectors_count, 
            DirEntrySlot* slot) {
    char buffer[MAX_LOGICAL_SECTOR_SIZE];

    /**
     * TODO: 2.1 在指定扇区中找到文件名为 name 的目录项 [约20行代码]  
     * Hint: 你可以参考 fill_entries_in_sectors 函数的实现。（非常类似！）
     *       区别在于，你需要找到对应目录项并给slot赋值，然后返回正确的返回值，而不是调用 filler 函数。
     *       你可以使用 check_name 函数来检查目录项是否符合你要找的文件名。
     *       注意：找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，所有扇区都满了返回 FIND_FULL。
     */
    // ================== Your code here =================
    for(size_t i=0; i < sectors_count; i++) {       // 搜索 sectors_count 个扇区
        sector_t sec = from_sector + i;             // 从 from_sector 开始搜索
        int ret = sector_read(sec, buffer);         // 读取扇区内容到缓存中
        if(ret < 0) {
            return -EIO;
        }
        /* 对单个扇区内的目录项进行搜索 */
        for(size_t off = 0; off < meta.sector_size; off += DIR_ENTRY_SIZE ) {
            DIR_ENTRY* entry = (DIR_ENTRY*)(buffer + off);
            if(de_is_valid(entry)) {    // 有效目录项
                // 将 filler 函数的调用改为check_name的检查
                // 无需调用to_longname， check_name中会自动处理
                if(check_name(name, len, entry)) {
                    // 找到匹配项
                    // 修改 slot 的三个属性
                    slot->dir = *entry;                    
                    slot->sector = sec;
                    slot->offset = off;
                    return FIND_EXIST;
                }
            }
            if(de_is_free(entry)) {
                // 由于找到空槽会直接返回
                // 所以空槽也要记录下来
                    slot->dir = *entry;                    
                    slot->sector = sec;
                    slot->offset = off;
                return FIND_EMPTY;
            }
        }
    }
    // ===================================================
    return FIND_FULL;
}

/**
 * @brief 找到path所对应路径的目录项，如果最后一级路径不存在，则找到能创建最后一级文件/目录的空目录项。
 * 
 * @param path    要查找的路径
 * @param slot    输出参数，存放找到的目录项和位置
 * @param remains 输出参数，指向未找到的路径部分。如路径为 "/a/b/c"，a存在，b不存在，remains会指指向 "b/c"。
 * @return int    找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，扇区均满了返回 FIND_FULL，
 *                错误返回错误代码的负值，可能的错误参见brief部分。
 */
int find_entry_internal(const char* path, DirEntrySlot* slot, const char** remains) {
    *remains = path;
    *remains += strspn(*remains, "/");    // 跳过开头的'/'
    
    // 根目录
    sector_t first_sec = meta.root_sec;
    size_t nsec = meta.root_sectors;
    size_t len = strcspn(*remains, "/"); // 目前要搜索的文件名长度
    int state = find_entry_in_sectors(*remains, len, first_sec, nsec, slot);    // 请补全 find_entry_in_sectors 函数

    // 找到下一层名字开头
    const char* next_level = *remains + len;
    next_level += strspn(next_level, "/");

    // 以下是根目录搜索的结果判断和错误处理
    if(state < 0 || *next_level == '\0') {   // 出错，或者只有一层已经找到了，直接返回结果
        return state;
    }
    if(state != FIND_EXIST) {   // 不是最后一层，且没找到
        return -ENOENT;
    }
    if(!attr_is_directory(slot->dir.DIR_Attr)) { // 不是最后一级，且不是目录
        return -ENOTDIR;
    }

    cluster_t clus = slot->dir.DIR_FstClusLO;  // 文件首个簇的簇号
    *remains = next_level;
    while (true) {
        size_t len = strcspn(*remains, "/"); // 目前要搜索的文件名长度
        /**
         * TODO: 5.1 查找非根目录的路径的目录项 [约10行代码]
         * Hint: 补全以下代码，实现对非根目录的路径的查找。提示，你要写一个 while 循环，依次搜索当前目录对应的簇里的目录项。
         */
        // ================== Your code here =================
        /* 原理： */
/*
        // 找到当前簇的第一个扇区
        sector_t sec = cluster_first_sector(clus);
        // 遍历当前簇中的所有扇区
        for(size_t i = 0; i < meta.sec_per_clus; i++) {
            state = find_entry_in_sectors(*remains, len, sec + i, 1, slot);
            // 找到匹配或空槽直接退出查找
            if(state != FIND_FULL) {
                break;
            }
        }
*/
        // 下面这样（也许？）效率更高         感觉怎么想都凑不齐10行
        state = find_entry_in_sectors(*remains, len, cluster_first_sector(clus), meta.sec_per_clus, slot);
        // ===================================================

        // 此时，slot 中存放了下一层的目录项（通过find_entry_in_sectors设置）

        const char* next_level = *remains + len;
        next_level += strspn(next_level, "/");

        if(state < 0 || *next_level == '\0') {   // 出错或者是最后一层，直接返回结果
            return state;
        }
        if(state != FIND_EXIST) {
            return -ENOENT;
        }
        if(!attr_is_directory(slot->dir.DIR_Attr)) {
            return -ENOTDIR;
        }

        *remains = next_level;  // remains 指向下一层的名字开头
        clus = slot->dir.DIR_FstClusLO; // 切换到下一个簇
    }
    return -EUCLEAN;
}


/**
 * @brief 将 path 对应的目录项写入 slot 中，slot 实际上记录了目录项本身，以及目录项的位置（扇区号和在扇区中的偏移）。
 *        该函数的主体实现在 find_entry_internal 函数中。你不需要修改这个函数。
 * @param path 
 * @param slot 
 * @return int 如果找到目录项，返回0；如果文件不存在，返回-ENOENT；如果出现其它错误，返回错误代码的负值。
 */
int find_entry(const char* path, DirEntrySlot* slot) {
    const char* remains = NULL;
    int ret = find_entry_internal(path, slot, &remains); // 请查看并补全 find_entry_internal 函数
    if(ret < 0) {
        return ret;
    }
    if(ret == FIND_EXIST) {
        return 0;
    }
    return -ENOENT;
}

/**
 * @brief 找到一个能创建 path 对应文件/目录的空槽（空目录项），如果文件/目录已经存在，则返回错误。
 * 
 * @param path 
 * @param slot 
 * @param last_name 
 * @return int 如果找到空槽，返回0；如果文件已经存在，返回-EEXIST；如果目录已满，返回-ENOSPC。
 */
int find_empty_slot(const char* path, DirEntrySlot *slot, const char** last_name) {
    int ret = find_entry_internal(path, slot, last_name);
    if(ret < 0) {
        return ret;
    }
    if(ret == FIND_EXIST) { // 文件已经存在
        return -EEXIST;
    }
    if(ret == FIND_FULL) { // 所有槽都已经满了
        return -ENOSPC;
    }
    return 0;
}

mode_t get_mode_from_attr(uint8_t attr) {
    mode_t mode = 0;
    mode |= attr_is_readonly(attr) ? S_IRUGO : S_NORMAL;
    mode |= attr_is_directory(attr) ? S_IFDIR : S_IFREG;
    return mode;
}

// ===========================文件系统接口实现===============================

/**
 * @brief 文件系统初始化，无需修改。但你可以阅读这个函数来了解如何使用 sector_read 来读出文件系统元数据信息。
 * 
 * @param conn 
 * @return void* 
 */
void *fat16_init(struct fuse_conn_info * conn, struct fuse_config *config) {
    /* Reads the BPB */
    BPB_BS bpb;
    sector_read(0, &bpb);
    meta.sector_size = bpb.BPB_BytsPerSec;
    meta.sec_per_clus = bpb.BPB_SecPerClus;
    meta.reserved = bpb.BPB_RsvdSecCnt;
    meta.fats = bpb.BPB_NumFATS;
    meta.dir_entries = bpb.BPB_RootEntCnt;
    meta.sectors = bpb.BPB_TotSec16 != 0 ? bpb.BPB_TotSec16 : bpb.BPB_TotSec32;
    meta.sec_per_fat = bpb.BPB_FATSz16;


    meta.fat_sec = meta.reserved;
    meta.root_sec = meta.fat_sec + (meta.fats * meta.sec_per_fat);
    meta.root_sectors = (meta.dir_entries * DIR_ENTRY_SIZE) / meta.sector_size;
    meta.data_sec = meta.root_sec + meta.root_sectors;
    meta.clusters = (meta.sectors - meta.data_sec) / meta.sec_per_clus;
    meta.cluster_size = meta.sec_per_clus * meta.sector_size;

    meta.fs_uid = getuid();
    meta.fs_gid = getgid();

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    meta.atime = meta.mtime = meta.ctime = now;
    return NULL;
}

/**
 * @brief 释放文件系统，无需修改
 * 
 * @param data 
 */
void fat16_destroy(void *data) { }

/**
 * @brief 获取path对应的文件的属性，无需修改。
 * 
 * @param path    要获取属性的文件路径
 * @param stbuf   输出参数，需要填充的属性结构体
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
    printf("getattr(path='%s')\n", path);
    // 清空所有属性
    memset(stbuf, 0, sizeof(struct stat));

    // 这些属性被忽略
    stbuf->st_dev = 0;
    stbuf->st_ino = 0;
    stbuf->st_nlink = 0;
    stbuf->st_rdev = 0;

    // 这些属性被提前计算好，不会改变
    stbuf->st_uid = meta.fs_uid;
    stbuf->st_gid = meta.fs_gid;
    stbuf->st_blksize = meta.cluster_size;

    // 这些属性需要根据文件设置
    // st_mode, st_size, st_blocks, a/m/ctim
    if (path_is_root(path)) {
        stbuf->st_mode = S_IFDIR | S_NORMAL;
        stbuf->st_size = 0;
        stbuf->st_blocks = 0;
        stbuf->st_atim = meta.atime;
        stbuf->st_mtim = meta.mtime;
        stbuf->st_ctim = meta.ctime;
        return 0;
    }

    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    stbuf->st_mode = get_mode_from_attr(dir->DIR_Attr);
    stbuf->st_size = dir->DIR_FileSize;
    stbuf->st_blocks = dir->DIR_FileSize / PHYSICAL_SECTOR_SIZE;
    
    time_fat_to_unix(&stbuf->st_atim, dir->DIR_LstAccDate, 0, 0);
    time_fat_to_unix(&stbuf->st_mtim, dir->DIR_WrtDate, dir->DIR_WrtTime, 0);
    time_fat_to_unix(&stbuf->st_ctim, dir->DIR_CrtDate, dir->DIR_CrtTime, dir->DIR_CrtTimeTenth);
    return 0;
}

/**
 * @brief 读取扇区号为 first_sec 开始的 nsec 个扇区的目录项，并使用 filler 函数将目录项填充到 buffer 中
 *        调用 filler 的方式为 filler(buffer, 完整的文件名/目录名, NULL, 0, 0)
 *        例如，filler(buffer, "file1.txt", NULL, 0, 0) 表示该目录中有文件 file1.txt
 * @param first_sec 开始的扇区号
 * @param nsec      扇区数
 * @param filler    用于填充结果的函数
 * @param buf       结果缓冲区
 * @return int      成功返回0，失败返回POSIX错误代码的负值
 */
int fill_entries_in_sectors(sector_t first_sec, size_t nsec, fuse_fill_dir_t filler, void* buf) {
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    char name[MAX_NAME_LEN];
    for(size_t i=0; i < nsec; i++) {
        /**
         * TODO: 1.2 读取扇区中的目录项 [3行代码]
         * Hint：（为降低难度，我们实现了大部分代码，你只需要修改和补全以下带 TODO 的几行。）
         *       你需要补全这个循环。这个循环读取每个扇区的内容，然后遍历扇区中的目录项，并正确调用 filler 函数。
         *       你可以参考以下步骤（大部分已经实现）：
         *          1. 使用 sector_read 读取第i个扇区内容到 sector_buffer 中（扇区号是什么？）
         *          2. 遍历 sector_buffer 中的每个目录项，如果是有效的目录项，转换文件名并调用 filler 函数。（每个目录项多长？）
         *             可能用到的函数（请自行阅读函数实现来了解用法）：
         *              de_is_valid: 判断目录项是否有效。
         *              to_longname: 将 FAT 短文件名转换为长文件名。
         *              filler: 用于填充结果的函数。
         *          3. 如果遇到空目录项，说明该扇区的目录项已经读取完，无需继续遍历。（为什么？）
         *             可能用到的函数：
         *              de_is_free: 判断目录项是否为空。
         *       为降低难度，我们实现了大部分代码，你只需要修改和补全以下带 TODO 的几行。
         */
        /* 由于是在`for`循环中，且有起始地址，故直接将起始地址加上循环变量赋值出去即可 */
        sector_t sec = first_sec + i; // TODO: 请填写正确的扇区号
        int ret = sector_read(sec, sector_buffer);
        if(ret < 0) {
            return -EIO;
        }
        for(size_t off = 0; off < meta.sector_size; off += DIR_ENTRY_SIZE ) { // TODO: 请补全循环条件（每个扇区多大？目录项多大？）
            DIR_ENTRY* entry = (DIR_ENTRY*)(sector_buffer + off);
            if(de_is_valid(entry)) {
                int ret = to_longname(entry->DIR_Name, name, MAX_NAME_LEN); // TODO: 请调用 to_longname 函数，将 entry->DIR_Name 转换为长文件名，结果存放在 name 中。
                if(ret < 0) {
                    return ret;
                }
                filler(buf, name, NULL, 0, 0);
            }
            if(de_is_free(entry)) {
                return 0;
            }
        }
    }
    return 0;
}

/**
 * @brief 读取path对应的目录，结果通过filler函数写入buffer中
 * 
 * @param path    要读取目录的路径
 * @param buf     结果缓冲区
 * @param filler  用于填充结果的函数，本次实验按filler(buffer, 文件名, NULL, 0, 0)的方式调用即可。
 *                你也可以参考<fuse.h>第58行附近的函数声明和注释来获得更多信息。
 * @param offset  忽略
 * @param fi      忽略
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, 
                    struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    printf("readdir(path='%s')\n", path);

    if(path_is_root(path)) {
        /**
         * TODO: 1.1 读取根目录区域 [2行代码]
         * Hint: 请正确修改以下两行，删掉 _placeholder_() 并替换为正确的数值。提示，使用 meta 中的成员变量。
         *       （注：_placeholder_() 在之后的 TODO 中也会出现。它只是个占位符，相当于你要填的空，你应该删除它，修改成正确的值。）
         */
        sector_t first_sec = meta.root_sec; // TODO: 请填写正确的根目录区域开始扇区号，你可以参考 meta 的定义。
        size_t nsec = meta.root_sectors;    // TODO: 请填写正确的根目录区域扇区数
        fill_entries_in_sectors(first_sec, nsec, filler, buf);
        return 0;
    }

    // 不是根目录的情况，需要找到第一个 clus
    cluster_t clus = CLUSTER_END;
    DirEntrySlot slot;
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }

    DIR_ENTRY* dir = &(slot.dir);
    clus = dir->DIR_FstClusLO;    // 不是根目录
    if(!attr_is_directory(dir->DIR_Attr)) {
        return -ENOTDIR;
    }

    // 改成 if root else 的结构
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    char name[MAX_NAME_LEN];
    while (is_cluster_inuse(clus)) {
        sector_t first_sec = cluster_first_sector(clus);
        size_t nsec = meta.sec_per_clus;
        fill_entries_in_sectors(first_sec, nsec, filler, buf);

        clus = read_fat_entry(clus);
    }
    
    return 0;
}

/**
 * @brief 从簇号为 clus 的簇的 offset 处开始读取 size 字节的数据到 data 中，并返回实际读取的字节数。
 * 
 * @param clus 簇号
 * @param offset 开始读取的偏移
 * @param data 输出缓冲区
 * @param size 要读取数据的长度
 * @return int 成功返回实际读取数据的长度，错误返回 POSIX 错误代码的负值
 */
int read_from_cluster_at_offset(cluster_t clus, off_t offset, char* data, size_t size) {
    assert(offset + size <= meta.cluster_size);  // offset + size 必须小于簇大小
    char sector_buffer[PHYSICAL_SECTOR_SIZE];
    /**
     * TODO: 2.2 从簇中读取数据 [约5行代码]
     * Hint: 步骤如下: 
     *       1. 计算 offset 对应的扇区号和扇区内偏移量。你可以用 cluster_first_sector 函数找到簇的第一个扇区。
     *          但 offset 可能超过一个扇区，所以你要计算出实际的扇区号和扇区内偏移量。
     *       2. 读取扇区（使用 sector_read）
     *       3. 将扇区正确位置的内容移动至 data 中正确位置
     *       你只需要补全以下 TODO 部分。主要是计算扇区号和扇区内偏移量；以及使用 memcpy 将数据移动到 data 中。
     */
    /* 偏移量未超过一个扇区，那就在当前扇区内查找 */
    uint32_t sec = cluster_first_sector(clus) + (offset / meta.sector_size);   // TODO: 请填写正确的扇区号。
    /* 扇区号移动过后，多出的那部分就是实际的偏移 */
    size_t sec_off = offset % meta.sector_size; // TODO: 请填写正确的扇区内偏移量。
    size_t pos = 0; // 实际已经读取的字节数
    while(pos < size) { // 还没有读取完毕
        int ret = sector_read(sec, sector_buffer); // 读取扇区内容到缓冲区
        if(ret < 0) {
            return ret;
        }
        // Hint: 使用 memcpy 挪数据，从 sec_off 开始挪。挪到 data 中哪个位置？挪多少？挪完后记得更新 pos （约3行代码）
        // ================== Your code here =================
        /* 计算需要复制的长度，若当前扇区剩余长度不足，则需要全部复制后再搜索下一个扇区 */
        size_t copy_bytes = min(meta.sector_size - sec_off, size - pos);
        /* 将扇区偏移后的copy_bytes个字节数据复制到data + pos 位置 */
        memcpy(data + pos, sector_buffer + sec_off, copy_bytes);
        /* 更新已读取的字节数 */
        pos += copy_bytes;
        // ===================================================
        sec_off = 0;    // 下一个扇区需要从头开始读取
        sec ++ ;        // 搜索下一个扇区
    }
    return pos;
}

/**
 * @brief 从path对应的文件的 offset 字节处开始读取 size 字节的数据到 buffer 中，并返回实际读取的字节数。
 * Hint: 文件大小属性是 Dir.DIR_FileSize。
 * 
 * @param path    要读取文件的路径
 * @param buffer  结果缓冲区
 * @param size    需要读取的数据长度
 * @param offset  要读取的数据所在偏移量
 * @param fi      忽略
 * @return int    成功返回实际读写的字符数，失败返回0。
 */
int fat16_read(const char *path, char *buffer, size_t size, off_t offset,
               struct fuse_file_info *fi) {
    printf("read(path='%s', offset=%ld, size=%lu)\n", path, offset, size);
    if(path_is_root(path)) {
        return -EISDIR;
    }

    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);  // 寻找文件对应的目录项，请查看并补全 find_entry 函数
    if(ret < 0) { // 寻找目录项出错
        return ret;
    }
    if(attr_is_directory(dir->DIR_Attr)) { // 找到的是目录
        return -EISDIR;
    }
    if(offset > dir->DIR_FileSize) { // 要读取的偏移量超过文件大小
        return -EINVAL;
    }
    size = min(size, dir->DIR_FileSize - offset);  // 读取的数据长度不能超过文件大小


    if(offset + size <= meta.cluster_size) {    // 文件在一个簇内的情况
        cluster_t clus = dir->DIR_FstClusLO;
        int ret = read_from_cluster_at_offset(clus, offset, buffer, size); // 请补全该函数
        return ret;
    }

    // 读取跨簇的文件
    cluster_t clus = dir->DIR_FstClusLO;
    size_t p = 0;   // 实际读取的字节数
    while(offset >= meta.cluster_size) {    // 移动到正确的簇号
        if(!is_cluster_inuse(clus)) {
            return -EUCLEAN;
        }
        offset -= meta.cluster_size;    // 如果 offset 大于一个簇的大小，需要移动到下一个簇，offset 减去一个簇的大小
        clus = read_fat_entry(clus);    // 请查看并补全 read_fat_entry 函数
    }

    /**
     * TODO: 4.2 读取多个簇中的数据 [约10行代码]
     * Hint: 你需要写一个循环，读取当前簇中的正确数据。步骤如下
     *       0. 写一个 while 循环，确认还有数据要读取，且簇号 clus 有效。
     *            1. 你可以使用 is_cluster_inuse() 函数
     *       1. 计算你要读取的（从offset开始的）数据长度。这有两种情况：
     *            1. 要读到簇的结尾，长度是？（注意 offset）
     *            2. 剩余的数据不需要到簇的结尾，长度是？
     *       2. 使用 read_from_cluster_at_offset 函数读取数据。注意检查返回值。
     *       3. 更新 p （已读字节数）、offset （下一个簇偏移）、clus（下一个簇号）。
     *          实际上，offset肯定是0，因为除了第一个簇，后面的簇都是从头开始读取。
     *          clus 则需要读取 FAT 表，别忘了你已经实现了 read_fat_entry 函数。
     */
    // ================== Your code here =================
    /* (还有数据需要读取) && 簇号有效 */
    while((p < size) && is_cluster_inuse(clus)) {
        /* min(不需要读取到簇末尾， 需要读取到簇末尾) */
        size_t Dread_len = min(size - p, meta.cluster_size - offset);   // Data_read_lenth
        /* 考虑到跨簇读取，应该写入缓存中实际已经写好的位置之后 */
        int ret = read_from_cluster_at_offset(clus, offset, buffer + p, Dread_len);
        if(ret < 0) {
            return ret;
        }
        /* p 自增本轮实际读取的长度 */
        p += ret;
        /* 读取一轮之后，下一簇必然是从起始位置开始访问的 */
        offset = 0;
        /* 更新clus */
        clus = read_fat_entry(clus);
    }
    // ===================================================
    return p;
}

int dir_entry_write(DirEntrySlot slot) {
    /**
     * TODO: 3.2 写入目录项 [3行代码]
     * Hint: （你只需补全带 TODO 的几行）
     *       使用 sector_write 将目录项写入文件系统。
     *       sector_write 只能写入一个扇区，但一个目录项只占用一个扇区的一部分。
     *       为了不覆盖其他目录项，你需要先读取该扇区，修改目录项对应的位置，然后再将整个扇区写回。
     */

    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    /* 传入： 扇区号， 缓存地址 */
    int ret = sector_read(slot.sector, sector_buffer); // TODO: 使用 sector_read 读取扇区
    if(ret < 0) {
        return ret;
    }
    /* slot.offset 是当前已经使用的总长度 */
    /* 传入： 修改位置地址（缓存）， 修改来源地址， 修改量大小 */
    memcpy(sector_buffer + slot.offset, &(slot.dir), sizeof(DIR_ENTRY)); // TODO: 使用 memcpy 将 slot.dir 里的目录项写入 buffer 中的正确位置
    /* 当前扇区的最新所有数据都在缓存中，应该全部写回 */
    /* 传入： 扇区号， 缓存地址 */
    ret = sector_write(slot.sector, sector_buffer); // TODO: 使用 sector_write 写回扇区
    if(ret < 0) {
        return ret;
    }
    return 0;
}

int dir_entry_create(DirEntrySlot slot, const char *shortname, 
            attr_t attr, cluster_t first_clus, size_t file_size) {
    DIR_ENTRY* dir = &(slot.dir);
    memset(dir, 0, sizeof(DIR_ENTRY));      // 设置一个空目录
    
    /**
     * TODO: 3.1 创建目录项 [约5行代码]
     * Hint: 请给目录项的 DIR_Name、Dir_Attr、DIR_FstClusHI、DIR_FstClusLO、DIR_FileSize 设置正确的值。
     *       你可以用 memcpy 函数来设置 DIR_Name。
     *       DIR_FstClusHI 在我们的系统中永远为 0。
     */
    // ================== Your code here =================
    /* 把传入的参数全部对 DIR 赋值即可 */
    memcpy(dir->DIR_Name, shortname, FAT_NAME_LEN); // 使用 memcpy 函数来设置 DIR_Name
    dir->DIR_Attr = attr;            // 设置 DIR_Attr
    dir->DIR_FstClusHI = 0;          // 设置 DIR_FstClusHI （永远为0）
    dir->DIR_FstClusLO = first_clus; // 设置 DIR_FstClusLO
    dir->DIR_FileSize = file_size;   // 设置 DIR_FileSize
    // ===================================================
    // 设置文件时间戳，无需修改
    struct timespec ts;
    int ret = clock_gettime(CLOCK_REALTIME, &ts);
    if(ret < 0) {
        return -errno;
    }
    time_unix_to_fat(&ts, &(dir->DIR_CrtDate), &(dir->DIR_CrtTime), &(dir->DIR_CrtTimeTenth));
    time_unix_to_fat(&ts, &(dir->DIR_WrtDate), &(dir->DIR_WrtTime), NULL);
    time_unix_to_fat(&ts, &(dir->DIR_LstAccDate), NULL, NULL);

    ret = dir_entry_write(slot);    // 请实现该函数
    if(ret < 0) {
        return ret;
    }
    return 0;
}


/**
 * @brief 在 path 对应的路径创建新文件（一个目录项对应一个文件，创建文件实际上就是创建目录项）
 * 
 * @param path    要创建的文件路径
 * @param mode    要创建文件的类型，本次实验可忽略，默认所有创建的文件都为普通文件
 * @param devNum  忽略，要创建文件的设备的设备号
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_mknod(const char *path, mode_t mode, dev_t dev) {
    printf("mknod(path='%s', mode=%03o, dev=%lu)\n", path, mode, dev);
    DirEntrySlot slot;
    const char* filename = NULL;
    int ret = find_empty_slot(path, &slot, &filename);  // 找一个空的目录项，如果正确实现了 find_entry_internal 函数，这里不需要修改
    if(ret < 0) {
        return ret;
    }

    char shortname[11];
    ret = to_shortname(filename, MAX_NAME_LEN, shortname); // 将长文件名转换为短文件名
    if(ret < 0) {
        return ret;
    }
    ret = dir_entry_create(slot, shortname, ATTR_REGULAR, 0, 0); // 创建目录项，请查看并补全 dir_entry_create 函数
    if(ret < 0) {
        return ret;
    }
    return 0;
}

/**
 * @brief 将data写入簇号为clusterN的簇对应的FAT表项，注意要对文件系统中所有FAT表都进行相同的写入。
 * 
 * @param clus  要写入表项的簇号
 * @param data  要写入表项的数据，如下一个簇号，CLUSTER_END（文件末尾），或者0（释放该簇）等等
 * @return int  成功返回0
 */
int write_fat_entry(cluster_t clus, cluster_t data) {
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    size_t clus_off = clus * sizeof(cluster_t);
    sector_t clus_sec = clus_off / meta.sector_size;
    size_t sec_off = clus_off % meta.sector_size;
    for(size_t i = 0; i < meta.fats; i++) {
        /**
         * TODO: 6.2 修改FAT表项 [约10行代码，核心代码约4行]
         * Hint: 修改第 i 个 FAT 表中，clus_sec 扇区中，sec_off 偏移处的表项，使其值为 data
         *         1. 计算第 i 个 FAT 表所在扇区，进一步计算clus应的FAT表项所在扇区
         *         2. 读取该扇区并在对应位置修改数据
         *         3. 将该扇区写回
         */
        // ================== Your code here =================   
        /* 计算第 i 个 FAT 表所在扇区，进一步计算clus对应的FAT表项所在扇区 */
        sector_t sec = meta.fat_sec + i * meta.sec_per_fat + clus_sec; 
        int ret = sector_read(sec, sector_buffer); // 读取该扇区
        if(ret < 0) {
            return ret;
        }
        // *(cluster_t*)(sector_buffer + sec_off) = data;在对应位置修改数据
        memcpy(sector_buffer + sec_off, &data, sizeof(cluster_t));
        ret = sector_write(sec, sector_buffer); // 将该扇区写回
        if(ret < 0) {
            return ret;
        }
        // ===================================================
    }
    return 0;
}

int free_clusters(cluster_t clus) {
    while(is_cluster_inuse(clus)) {
        cluster_t next = read_fat_entry(clus);
        int ret = write_fat_entry(clus, CLUSTER_FREE);
        if(ret < 0) {
            return ret;
        }
        clus = next;
    }
    return 0;
}


static const char ZERO_SECTOR[PHYSICAL_SECTOR_SIZE] = {0};
int cluster_clear(cluster_t clus) {
    sector_t first_sec = cluster_first_sector(clus);
    for(size_t i = 0; i < meta.sec_per_clus; i++) {
        sector_t sec = first_sec + i;
        int ret = sector_write(sec, ZERO_SECTOR);
        if(ret < 0) {
            return ret;
        }
    }
    return 0;
}

/**
 * @brief 分配一个空闲簇，将簇号写入 clus 中
 * 
 * @param clus 输出参数，用于保存分配的簇号
 * @return int 成功返回0，失败返回错误代码负值
 */
int alloc_one_cluster(cluster_t* clus) {
    /**
     * TODO: 6.3 分配一个空闲簇 [约15行代码]
     * Hint: 步骤如下
     *       1. 扫描FAT表，找到1个空闲的簇。
     *         1.1 找不到空簇，分配失败，返回 -ENOSPC。
     *       2. 修改空簇对应的FAT表项，将其指向 CLUSTER_END。同时清零该簇（使用 cluster_clear 函数）。
     * 
     */
    // ================== Your code here =================
    /* 扫描FAT表找一个空闲簇，从MIN扫到MAX*/
    cluster_t ClusN = CLUSTER_MIN;
    /* 感觉这样会比调用is_cluster_inuse节约一点资源？ */
    while(ClusN <= CLUSTER_MAX) {
        /* 找到空闲簇 */
        if (read_fat_entry(ClusN) == CLUSTER_FREE) {
            /* 记录簇号 */
            *clus = ClusN;
            /* 将FAT表项指向CLUSTER_END */
            write_fat_entry(*clus, CLUSTER_END);    
            /* 清零 */
            cluster_clear(*clus);
            /* 返回 */
            return 0;
        }
        ClusN++;
    }
    /* 找不到 */
    return -ENOSPC;
    // ===================================================
//    return -ENOTSUP;    // TODO 请删除这一行或者修改为正确的返回值
}

/**
 * @brief 分配n个空闲簇，分配过程中将n个簇通过FAT表项连在一起，然后返回第一个簇的簇号。
 *        最后一个簇的FAT表项将会指向0xFFFF（即文件中止）。
 * @param n         要分配的簇数
 * @param first_clus 输出参数，用于保存第一个簇的簇号
 * @return int      成功返回0，失败返回错误代码负值
 */
int alloc_clusters(size_t n, cluster_t* first_clus) {
    if (n == 0)
        return CLUSTER_END;

    // 用于保存找到的n个空闲簇，另外在末尾加上CLUSTER_END，共n+1个簇号
    cluster_t *clusters = malloc((n + 1) * sizeof(cluster_t));
    size_t allocated = 0; // 已找到的空闲簇个数

    /**
     * TODO: 8.3 分配 n 个空闲簇
     * Hint: 步骤如下
     *       1. 扫描FAT表，找到n个空闲的簇，存入cluster数组。注意此时不需要修改对应的FAT表项。
     *         1.1 找不到n个簇，分配失败，记得 free(clusters)，返回 -ENOSPC。
     *       2. 修改clusters中存储的N个簇对应的FAT表项，将每个簇与下一个簇连接在一起。同时清零每一个新分配的簇。
     *         2.1 记得将最后一个簇连接至 CLUSTER_END。
     *       3. 将 first_clus 设置为第一个簇的簇号，释放 clusters。
     */


    // ================== Your code here =================
    cluster_t clusNum = CLUSTER_MIN;
    /* 未找齐N个空闲簇，并且簇未扫描完毕 */
    while((allocated < n) && (is_cluster_inuse(clusNum))) { // 扫描FAT表，找到n个空闲的簇，存入cluster数组。
        if(read_fat_entry(clusNum) == CLUSTER_FREE) {
            clusters[allocated] = clusNum;
            allocated++;
        }
        clusNum++;
    }
    /* 找不到n个簇，分配失败， free(clusters)，返回 -ENOSPC*/
    if(allocated != n) { 
        free(clusters);
        return -ENOSPC;
    }
    clusters[n] = CLUSTER_END; // 将最后一个簇连接至 CLUSTER_END
    /* 修改clusters中存储的N个簇对应的FAT表项，将每个簇与下一个簇连接在一起。
    同时清零每一个新分配的簇。*/
    for(size_t i = 0; i < n; i++) {
        cluster_clear(clusters[i]);
        write_fat_entry(clusters[i], clusters[i + 1]);
    }
    *first_clus = clusters[0]; // 将 first_clus 设置为第一个簇的簇号
    // ===================================================
    free(clusters);
    return 0;
}


/**
 * @brief 创建path对应的文件夹
 * 
 * @param path 创建的文件夹路径
 * @param mode 文件模式，本次实验可忽略，默认都为普通文件夹
 * @return int 成功:0， 失败: POSIX错误代码的负值
 */
int fat16_mkdir(const char *path, mode_t mode) {
    printf("mkdir(path='%s', mode=%03o)\n", path, mode);
    DirEntrySlot slot = {{}, 0, 0};
    const char* filename = NULL;   
    cluster_t dir_clus = 0; // 新建目录的簇号
    int ret = 0;

    /**
     * TODO: 6.1 创建目录（文件夹） [约15行代码，核心代码约4行]
     * Hint: 参考 mknod，区别在于，这次你应该分配一个空闲簇，然后在这个簇中创建 . 和 .. 目录项。
     *       请调用 alloc_one_cluster(&dir_clus); 函数来分配簇。然后实现 alloc_one_cluster 函数。
     */

    // ================== Your code here =================
    /* 先找到一个空闲目录项 */
    ret = find_empty_slot(path, &slot, &filename);
    if(ret < 0) {
        return ret;
    }
    /* 将长文件名转化为短文件名 */
    char shortname[11];
    ret = to_shortname(filename, MAX_NAME_LEN, shortname);
    if(ret < 0) {
        return ret;
    }
    /* 准备工作完毕，分配一个空闲簇 */
    ret = alloc_one_cluster(&dir_clus);
    if(ret < 0) {
        return ret;
    }
    /* 创建目录项 */
    ret = dir_entry_create(slot, shortname, ATTR_DIRECTORY, dir_clus, 0);
    if(ret < 0) {
        return ret;
    }
    // ===================================================

    // 设置 . 和 .. 目录项
    const char DOT_NAME[] =    ".          ";
    const char DOTDOT_NAME[] = "..         ";
    sector_t sec = cluster_first_sector(dir_clus);
    DirEntrySlot dot_slot = {.sector=sec, .offset=0};
    ret = dir_entry_create(dot_slot, DOT_NAME, ATTR_DIRECTORY, dir_clus, 0);
    if(ret < 0) {
        return ret;
    }
    DirEntrySlot dotdot_slot = {.sector=sec, .offset=DIR_ENTRY_SIZE};
    ret = dir_entry_create(dotdot_slot, DOTDOT_NAME, ATTR_DIRECTORY, sector_cluster(slot.sector), 0);
    if(ret < 0) {
        return ret;
    }
    return 0;
}

/**
 * @brief 删除path对应的文件
 * 
 * @param path  要删除的文件路径
 * @return int  成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_unlink(const char *path) {
    printf("unlink(path='%s')\n", path);
    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);

    /**
     * TODO: 7.1 删除文件 [约15行代码，核心代码约5行]
     * Hint: 和创建文件类似，删除文件实际上就是删除目录项。步骤如下：
     *       找到目录项；确认目录项是个文件；释放占用的簇；修改目录项为删除；写回目录项；
     *       可能使用的函数：find_entry, attr_is_directory, free_clusters, dir_entry_write
     *       记得检查调用函数后的返回值。
     */
    // ================== Your code here =================
    /* 找到目录项 */
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    /* 确认目录项是个文件 */
    if(attr_is_directory(dir->DIR_Attr)) {
        return -EISDIR;
    }
    /* 释放占用的簇 */
    free_clusters(dir->DIR_FstClusLO);
    /* 修改目录项为删除 */
    dir->DIR_Name[0] = NAME_DELETED;
    /* 写回目录项 */
    ret = dir_entry_write(slot);
    // ===================================================
    return 0; // TODO: 请修改返回值
}

/**
 * @brief 删除path对应的文件夹
 * 
 * @param path 要删除的文件夹路径
 * @return int 成功:0， 失败: POSIX错误代码的负值
 */
int fat16_rmdir(const char *path) {
    printf("rmdir(path='%s')\n", path);
    if(path_is_root(path)) {  // 根目录无法删除
        return -EBUSY;
    }

    /**
     * TODO: 7.2 删除目录（文件夹）[约50行代码]
     * Hint: 总体思路类似 readdir + unlink。具体如下：
     *       删除过程和删除文件类似，但注意，请**检查目录是否为空**，如果目录不为空，**不能删除**，且返回 -ENOTEMPTY。
     *       检查目录是否为空的过程和 readdir 类似，但是你需要检查每个目录项是否为空，而不是填充结果。
     */
    // ================== Your code here =================
    // 检查是否合法**********************************************
    cluster_t clus = CLUSTER_END;
    DirEntrySlot slot;
    /* 找到目录项 */
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    DIR_ENTRY* dir = &(slot.dir);
    /* 若不是目录，返回错误 */
    if(!attr_is_directory(dir->DIR_Attr)) {
        return -ENOTDIR;
    }
    
    // 检查目录是否为空******************************************

    /* 从目录项中获取簇号 */
    clus = dir->DIR_FstClusLO;
    
    while(is_cluster_inuse(clus)) {
        /* 从簇的第一个扇区开始扫描 */
        sector_t fst_clus_sec = cluster_first_sector(clus);
        /* 获取扇区总数 */
        size_t nsec = meta.sec_per_clus;
        char sector_buffer[MAX_LOGICAL_SECTOR_SIZE]; 
        /* 遍历扇区 */
        for (size_t i = 0; i < nsec; i++){
            /* 当前操作扇区 */
            sector_t sec = fst_clus_sec + i;
            /* 读入缓存 */
            int ret = sector_read(sec, sector_buffer);
            if(ret < 0) {
                return -EIO;
            }
            /* 检查每一个目录项 */
            for (size_t off = 0; off < meta.sector_size; off += DIR_ENTRY_SIZE) {
                /* buffer中存储了扇区中的全部数据，每次检查一个目录项 */
                DIR_ENTRY* entry = (DIR_ENTRY*)(sector_buffer + off);
                /* 目录不为空 */
                if(de_is_valid(entry)) {
                    /* 不是 . 或 ..  */
                    if (!de_is_dot(entry)) {
                        return -ENOTEMPTY;
                    }
                }                
            }
        }
        /* 进入目录的下一个簇 */
        clus = read_fat_entry(clus);
    }
    /* 释放目录的簇 */
    free_clusters(dir->DIR_FstClusLO);
    /* 标记为删除 */
    char buff[MAX_LOGICAL_SECTOR_SIZE];
    dir->DIR_Name[0] = NAME_DELETED;
    /* 读出、修改、写回 */
    sector_read(slot.sector, buff);
    memcpy(buff + slot.offset, &(slot.dir), DIR_ENTRY_SIZE);
    sector_write(slot.sector, buff);
    // ===================================================
    return 0; 
}

/**
 * @brief 修改path对应文件的时间戳，本次实验不做要求，可忽略该函数
 * 
 * @param path  要修改时间戳的文件路径
 * @param tv    时间戳
 * @return int 
 */
int fat16_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info* fi) {
    printf("utimens(path='%s', tv=[%ld.%09ld, %ld.%09ld])\n", path, 
                tv[0].tv_sec, tv[0].tv_nsec, tv[1].tv_sec, tv[1].tv_nsec);
    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }

    time_unix_to_fat(&tv[1], &(dir->DIR_WrtDate), &(dir->DIR_WrtTime), NULL);
    time_unix_to_fat(&tv[0], &(dir->DIR_LstAccDate), NULL, NULL);
    ret = dir_entry_write(slot);
    if(ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief 将data中的数据写入编号为clusterN的簇的offset位置。
 *        注意size+offset <= 簇大小
 * 
 * @param clus      要写入数据的块号
 * @param data      要写入的数据
 * @param size      要写入数据的大小（字节）
 * @param offset    要写入簇的偏移量
 * @return ssize_t  成功写入的字节数，失败返回错误代码负值。可能部分成功，此时仅返回成功写入的字节数，不提供错误原因（POSIX标准）。
 */
ssize_t write_to_cluster_at_offset(cluster_t clus, off_t offset, const char* data, size_t size) {
    assert(offset + size <= meta.cluster_size);  // offset + size 必须小于簇大小
    char sector_buffer[PHYSICAL_SECTOR_SIZE];

    /**
     * TODO: 8.2 写入数据到簇中 [约20行代码]
     * Hint: 参考 read_from_cluster_at_offset 函数。每个扇区实际上都要 读 -> 修改 -> 写。
     */

    // ================== Your code here =================
    uint32_t sec = cluster_first_sector(clus) + offset / meta.sector_size;
    size_t sec_off = offset % meta.sector_size; 
    size_t pos = 0; // 实际已经写入的字节数
    while(pos < size) { // 还没有写入完毕
        int ret = sector_read(sec, sector_buffer);
        if(ret < 0) {
            return ret;
        }
        size_t to_write = min(size - pos, meta.sector_size - sec_off); // 计算本次需要写入的字节数
        memcpy(sector_buffer + sec_off, data + pos, to_write); // 将数据写入扇区中的正确位置
        ret = sector_write(sec, sector_buffer); // 将扇区写回
        if(ret < 0) {
            return ret;
        }
        pos += to_write;
        sec_off = 0;
        sec++;
    }
    // ===================================================
    return pos; // TODO: 请修改返回值
}

/**
 * @brief 将长度为size的数据data写入path对应的文件的offset位置。注意当写入数据量超过文件本身大小时，
 *        需要扩展文件的大小，必要时需要分配新的簇。
 * 
 * @param path    要写入的文件的路径
 * @param data    要写入的数据
 * @param size    要写入数据的长度
 * @param offset  文件中要写入数据的偏移量（字节）
 * @param fi      本次实验可忽略该参数
 * @return int    成功返回写入的字节数，失败返回POSIX错误代码的负值。
 */
int fat16_write(const char *path, const char *data, size_t size, off_t offset,
                struct fuse_file_info *fi) {
    printf("write(path='%s', offset=%ld, size=%lu)\n", path, offset, size);
    if(path_is_root(path)) {
        return -EISDIR;
    }

    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    if(attr_is_directory(dir->DIR_Attr)) {
        return -EISDIR;
    }
    if(offset > dir->DIR_FileSize) {
        return -EINVAL;
    }
    if(size == 0) {
        return 0;
    }

    /**
     * TODO: 8.1 写入数据到文件中,必要时分配新簇。[约50行代码]
     * Hint: 参考实验文档中的实现思路。
     * 
     */
    // ================== Your code here =================
    /* 检查是否需要新分配簇 */

    size_t clus_cnt = 0;                         // 文件的簇总数
    cluster_t lst_clus = CLUSTER_END;       // 文件的最后一个被使用簇号
    cluster_t clus = dir->DIR_FstClusLO;    
    while(is_cluster_inuse(clus)){
        clus_cnt++;
        lst_clus = clus;
        clus = read_fat_entry(clus);
    }

    /* 需要分配新簇 */
    if ((dir->DIR_FileSize + size) > (clus_cnt * meta.sec_per_clus * meta.sector_size)) {
        size_t new_len = (size + meta.cluster_size - 1) / meta.cluster_size;
        /* 分配 */
        int ret = alloc_clusters(new_len, &clus);
        if (ret < 0) {
            return ret;
        }
        /* 连接 */
        if (lst_clus == CLUSTER_END) {
            /* 空文件 */
            dir->DIR_FstClusLO = clus;
        }
        else {
            write_fat_entry(lst_clus, clus);
        }
    }

    /* 分配检查完毕，写入对应偏移处 */
    /* 找到写入位置的起始簇 */
    sector_t clus_start = offset / (meta.sector_size * meta.sec_per_clus);
    
    /* 让clus从文件簇头开始移动到起始簇的位置 */
    clus = dir->DIR_FstClusLO;
    for (size_t i = 0; i < clus_start; i++) {
        if (!is_cluster_inuse(clus)){
            return 0;
        }
        clus = read_fat_entry(clus);
    }
    /* 当前已写入字节数 */
    size_t cur_off = 0;
    /* 簇内偏移 */
    size_t clus_off = offset % (meta.sector_size * meta.sec_per_clus);

    while (cur_off < size) {
        /* 当前写入长度 */
        size_t write_len = min(size - cur_off, meta.sector_size * meta.sec_per_clus - clus_off);
        /* 写入簇偏移位置 */
        write_to_cluster_at_offset(clus, clus_off, data + cur_off, write_len);
        /* 进入下一个簇 */
        clus = read_fat_entry(clus);
        clus_off = 0;
        cur_off += write_len;
    }

    dir_entry_write(slot); // 将更新后的目录项写回
    // ===================================================
    return cur_off; // TODO: 请修改返回值
}

/**
 * @brief 将path对应的文件大小改为size，注意size可以大于小于或等于原文件大小。
 *        若size大于原文件大小，需要将拓展的部分全部置为0，如有需要，需要分配新簇。
 *        若size小于原文件大小，将从末尾截断文件，若有簇不再被使用，应该释放对应的簇。
 *        若size等于原文件大小，什么都不需要做。
 * 
 * @param path 需要更改大小的文件路径 
 * @param size 新的文件大小
 * @return int 成功返回0，失败返回POSIX错误代码的负值。
 */
int fat16_truncate(const char *path, off_t size, struct fuse_file_info* fi) {
    printf("truncate(path='%s', size=%lu)\n", path, size);
    if(path_is_root(path)) {
        return -EISDIR;
    }

    DirEntrySlot slot;
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    if(attr_is_directory(dir->DIR_Attr)) {
        return -EISDIR;
    }

    size_t old_size = dir->DIR_FileSize;
    if(old_size == size) {
        return 0;
    } else if(size > old_size) {
        size_t need_clus = (size + meta.cluster_size - 1) / meta.cluster_size;
        cluster_t clus = dir->DIR_FstClusLO;
        cluster_t last_clus = 0;
        while(is_cluster_inuse(clus)) {
            last_clus = clus;
            need_clus --;
            clus = read_fat_entry(clus);
        }

        cluster_t new;
        int ret = alloc_clusters(need_clus, &new);
        if(ret < 0) {
            return ret;
        }
        ret = write_fat_entry(last_clus, new);

        if(last_clus == 0) {
            dir->DIR_FstClusLO = new;
        }
    } else if(size < old_size) {
        size_t need_clus = (size + meta.cluster_size - 1) / meta.cluster_size;
        cluster_t clus = dir->DIR_FstClusLO;
        cluster_t last_clus = 0;
        while(need_clus > 0) {
            need_clus --;
            last_clus = clus;
            clus = read_fat_entry(clus);
        }
        if(last_clus == 0) {
            dir->DIR_FstClusLO = CLUSTER_FREE;
        } else {
            int ret;
            ret = free_clusters(clus);
            if(ret < 0) {
                return ret;
            }
            ret = write_fat_entry(last_clus, CLUSTER_END);
            if(ret < 0) {
                return ret;
            }
        }
    }

    dir->DIR_FileSize = size;
    dir_entry_write(slot);

    return 0;
}


struct fuse_operations fat16_oper = {
    .init = fat16_init,         // 文件系统初始化
    .destroy = fat16_destroy,   // 文件系统注销
    .getattr = fat16_getattr,   // 获取文件属性

    .readdir = fat16_readdir,   // 读取目录
    .read = fat16_read,         // 读取文件

    .mknod = fat16_mknod,       // 创建文件
    .unlink = fat16_unlink,     // 删除文件
    .utimens = fat16_utimens,   // 修改文件时间戳

    .mkdir = fat16_mkdir,       // 创建目录
    .rmdir = fat16_rmdir,       // 删除目录

    .write = fat16_write,       // 写文件
    .truncate = fat16_truncate  // 修改文件大小
};
