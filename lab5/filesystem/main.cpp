#include "myfile.h"
#define CMD_NUM 11

// input
char cmds[CMD_NUM][10] = {"ls", "touch", "mkdir", "cd", "exit", "mkfs", "vim", "map", "rm", "rmdir", "ll"};
char arg[32];
char cmd[32];
int op = 1;
int myexit = 0;
int temp;

int main() {
    usage();
    init();
    while (1) {
        op = 100;
        printf("\033[1;34m%s\033[0m$ ", pwd);
        scanf("%s", cmd);
        strcpy(arg, "unname");
        if (strcmp(cmd, "touch") == 0 || strcmp(cmd, "mkdir") == 0 || \
            strcmp(cmd, "cd") == 0 || strcmp(cmd, "vim") == 0 || \
            strcmp(cmd, "rm") == 0 || strcmp(cmd, "rmdir") == 0) {
            scanf("%s", arg);
        }
        for (int i = 0; i < CMD_NUM; i++) {
            if (strcmp(cmds[i], cmd) == 0) {
                op = i;
                break;
            }
        }
        switch (op) {
        case 0:
            // ls
            show();
            break;
        case 1:
            // touch
            mkfile(cur_inum, arg, _FILE);
            break;
        case 2:
            // mkdir
            mkfile(cur_inum, arg, _DIR);
            break;
        case 3:
            // cd
            change_dir(arg);
            break;
        case 4:
            // exit
            close_dir(cur_inum);
            myexit = 1;
            break;
        case 5:
            // mkfs
            init_root();
            break;
        case 6:
            // vim
            // printf("arg = %s, inode = %d\n", arg, iget_name(arg));
            if (iget_name(arg) == -1) {
                printf("%s not exists\n", arg);
                break;
            }
            read_file(iget_name(arg));
            write_file(iget_name(arg));
            break;
        case 7:
            // map
            show_map();
            break;
        case 8:
            // rm
            if (get_itype(iget_name(arg)) == _FILE) {
                rm_file(iget_name(arg));
                rm_cur(iget_name(arg));
            } else {
                printf("not file or file not exist\n");
            }
            break;
        case 9:
            // rmdir
            if (iget_name(arg) != -1) {
                if (get_itype(iget_name(arg)) == _DIR) {
                    rm_dir(iget_name(arg));
                    rm_cur(iget_name(arg));
                } else {
                    printf("not dir or file not exist\n");
                }
            }
            break;
        case 10:
            // ll
            show_more();
            break;
        default:
            // printf("file_num = %d\n", cur_fnum);
            // printf("cur_inum = %d\n", cur_inum);
            printf("No command \'%s\' found\n", cmd);
            break;
        }
        if (myexit == 1) {
            break;
        }
    }
    end();
}

int init() {
    //  init filesystem
    fs = fopen("fs", "r+");
    fread(&sb, sizeof(SuperBlock), 1, fs);
    //  fseek(fs, BlockSeg, SEEK_SET);
    strcpy(pwd, "/");
    cur_inum = 0;

    if (open_dir(cur_inum) == -1) {
        init_root();
    }
    return 0;
}

int init_root() {
    // init superBlock
    memset(sb.inode_map, 0, sizeof(sb.inode_map));
    memset(sb.block_map, 0, sizeof(sb.block_map));
    sb.inode_map[0] = 1;
    sb.inode_used = 1;
    sb.block_map[0] = 1;
    sb.block_used = 1;
    Inode inode;
    inode.size = sizeof(Dir);
    inode.block_num = 1;
    inode.blocks[0] = 0;
    inode.type = _DIR;
    strcpy(pwd, "/");
    // write . ..
    Dir dir;
    dir.inum = 0;
    strcpy(dir.name, ".");
    fseek(fs, BlockSeg, SEEK_SET);
    fwrite(&dir, sizeof(Dir), 1, fs);
    // write new_inode
    fseek(fs, InodeSeg, SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fs);
    cur_fnum = 0;
    cur_inode = inode;
    cur_inum = 0;
    cur_files[cur_fnum++] = dir;
    change_dir(cur_files[0].name);
    return 0;
}

int end() {
    close_dir(cur_inum);
    fseek(fs, SuperSeg, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, fs);
    fclose(fs);
}

// inum : parent's inum
int mkfile(int pa_inum, char *name, int type) {
    if (same_name(name, type) == -1) return -1;
    int new_inum = get_inum();
    if (type == _DIR) {
        init_dir(pa_inum, new_inum);
    } else {
        init_file(new_inum);
    }
    Dir dir;
    strcpy(dir.name, name);
    dir.inum = new_inum;
    cur_files[cur_fnum++] = dir;
    return 0;
}

int same_name(char *name, int type) {
    for (int i = 0; i < cur_fnum; i++) {
        if (strcmp(name, cur_files[i].name) == 0) {
            printf("file exists\n");
            return -1;
        }
    }
    return 0;
}

int init_dir(int pa_inum, int new_inum) {
    // init inode
    Inode inode;
    int bnum;
    memset(inode.blocks, 0, sizeof(inode.blocks));
    inode.type = _DIR;
    inode.block_num = 1;
    bnum = get_bnum();
    inode.blocks[0] = bnum;
    inode.size = 2 * sizeof(Dir);
    // write . ..
    Dir dir[2];
    strcpy(dir[0].name, ".");
    dir[0].inum = new_inum;
    strcpy(dir[1].name, "..");
    dir[1].inum = pa_inum;
    fseek(fs, BlockSeg + (BlockSize * bnum), SEEK_SET);
    fwrite(dir, sizeof(Dir), 2, fs);
    // write new_inode
    fseek(fs, InodeSeg + (sizeof(Inode) * new_inum), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fs);
    // write pa_inode
    fseek(fs, InodeSeg + (sizeof(Inode) * pa_inum), SEEK_SET);
    fwrite(&cur_inode, sizeof(Inode), 1, fs);
    return 0;
}

int init_file(int inum) {
    Inode inode;
    memset(inode.blocks, 0, sizeof(inode.blocks));
    inode.block_num = 0;
    inode.size = 0;
    inode.type = _FILE;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fs);
    return 0;
}

int get_itype(int inum) {
    Inode inode;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, fs);
    return inode.type;
}

int get_inum() {
    // check if used up
    if (sb.inode_used >= InodeNum) {
        printf("inode is used up\n");
        return -1;
    }
    // get an inode num
    for (int i = 1; i < InodeNum; i++) {
        if (sb.inode_map[i] == 0) {
            sb.inode_map[i] |= 1;
            sb.inode_used++;
            return i;
        }
    }
}

int get_bnum() {
    // check if used up
    if (sb.block_used >= BlockNum) {
        printf("block is used up\n");
        return -1;
    }
    // get an block num
    for (int i = 1; i < BlockNum; i++) {
        if (sb.block_map[i] == 0) {
            sb.block_map[i] |= 1;
            sb.block_used++;
            return i;
        }
    }
}

int open_dir(int inum) {
    // read
    fseek(fs, InodeSeg + (inum  * sizeof(Inode)), SEEK_SET);
    fread(&cur_inode, sizeof(Inode), 1, fs);
    // null
    if (cur_inode.block_num == 0) {
        printf("cur_inode.block_num = 0\n\n");
        return -1;
    }
    // is a file
    if (cur_inode.type == _FILE) {
        printf("This is a file, not a dir\n");
        return -2;
    }
    //
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&cur_inode, sizeof(Inode), 1, fs);
    cur_inum = inum;
    cur_fnum = cur_inode.size / sizeof(Dir);
    int bnum = cur_inode.blocks[0];
    fseek(fs, BlockSeg + (BlockSize * bnum), SEEK_SET);
    fread(cur_files, sizeof(Dir), cur_fnum, fs);
    return 0;
}

int iget_name(char *name) {
    for (int i = 0; i < cur_fnum; i++) {
        if (strcmp(name, cur_files[i].name) == 0) {
            return cur_files[i].inum;
        }
    }
    printf("no file : %s\n", name);
    return -1;
}

int change_dir(char *name) {
    int old_inum = cur_inum;
    // save current
    close_dir(cur_inum);

    // change pwd[128]
    // current
    if (strcmp(name, ".") == 0) {
        return 0;
    }
    // open new
    int inum = iget_name(name);
    if (inum == -1) return -1;
    if (open_dir(inum) != 0) {
        printf("open file %s failed\n", name);
        return -1;
    }
    // father
    if (strcmp(name, "..") == 0) {
        int i = 0;
        int pos = 0;
        char c;
        while (c = pwd[i++]) {
            if (c == '/') {
                pos = i-1;
            }
        }
        pwd[pos] = '\0';
        if (pos == 0) {
            strcpy(pwd, "/");
        }
    } else {
        // add
        if (old_inum != 0)  strcat(pwd, "/");
        strcat(pwd, name);
    }
    return 0;
}

int close_dir(int inum) {
    // save cur_files
    fseek(fs, BlockSeg + (BlockSize * cur_inode.blocks[0]), SEEK_SET);
    fwrite(cur_files, sizeof(Dir), cur_fnum, fs);
    // save inode
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    cur_inode.size = cur_fnum * sizeof(Dir);
    cur_inode.block_num = 1;
    fwrite(&cur_inode, sizeof(Inode), 1, fs);
    return 0;
}

int read_file(int inum) {
    // read inode
    Inode inode;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, fs);
    // read blocks
    FILE *fp = fopen("buf", "w+");

    if (inode.block_num == 0) {
        // show
        printf("inode.block_num = 0\n");
        fclose(fp);
        system("vim buf");
        return 0;
    }

    int offset = 0;
    int i = 0;
    for (i = 0; i < inode.block_num - 1; i++) {
        fseek(fs, BlockSeg + (BlockSize * inode.blocks[i]), SEEK_SET);
        fseek(fp, offset, SEEK_SET);
        fread(BUF, BlockSize, 1, fs);
        fwrite(BUF, BlockSize, 1, fp);
        offset += BlockSize;
    }
    int size = inode.size - offset;
    fseek(fs, BlockSeg + (BlockSize * inode.blocks[i]), SEEK_SET);
    fseek(fp, offset, SEEK_SET);
    fread(BUF, size, 1, fs);
    fwrite(BUF, size, 1, fp);

    /*
    printf("read begin! size = %d, bnum = %d, b = %d\n", size, inode.block_num, inode.blocks[i]); //inode.blocks[i]);
    char temp[100];
    fseek(fs, BlockSeg + (BlockSize * inode.blocks[i]), SEEK_SET);
    fread(temp, size, 1, fs);
    printf("size = %d read temp = %s\n", size, temp);
    */

    // show
    fclose(fp);
    system("vim buf");
    return 0;
}

int write_file(int inum) {
    // read inode
    Inode inode;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, fs);
    // get buf size
    struct stat stbuf;
    int fd = open("buf", O_RDONLY);
    if (fd == -1) {
        printf("open buf error\n");
        return -1;
    }
    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        printf("get buf info error or buf is not a file\n");
        return 0;
    }
    inode.size = stbuf.st_size;
    close(fd);
    // write blocks
    FILE *fp = fopen("buf", "r");

    int max_bnum = inode.size / BlockSize;
    int offset = 0;
    int i;
    for (i = 0; i < max_bnum; i++) {
        if (i+1 > inode.block_num) {
            inode.blocks[i] = get_bnum();
            inode.block_num++;
        }
        fseek(fs, BlockSeg + (BlockSize * inode.blocks[i]), SEEK_SET);
        fseek(fp, offset, SEEK_SET);
        fread(BUF, BlockSize, 1, fp);
        fwrite(BUF, BlockSize, 1, fs);
        offset += BlockSize;
    }
    int size = inode.size - offset;
    if (size != 0) {
        if (i+1 > inode.block_num) {
            inode.blocks[i] = get_bnum();
            inode.block_num++;
        }
        // printf("write begin! size = %d, bnum = %d, b = %d\n", size, inode.block_num, inode.blocks[i]);
        fseek(fs, BlockSeg + (BlockSize * inode.blocks[i]), SEEK_SET);
        fseek(fp, offset, SEEK_SET);
        fread(BUF, size, 1, fp);
        fwrite(BUF, size, 1, fs);
        /*
        char temp[100];
        fseek(fp, offset, SEEK_SET);
        fread(temp, size, 1, fp);
        printf(" write temp = %s\n", temp);
        */
    }
    fclose(fp);
    // write inode
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fs);
    return 0;
}

int free_inum(int inum) {
    sb.inode_map[inum] = 0;
    sb.inode_used--;
    return 0;
}

int free_bnum(int bnum) {
    sb.block_map[bnum] = 0;
    sb.block_used--;
    return 0;
}

int rm_file(int inum) {
    Inode inode;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, fs);
    for (int i = 0; i < inode.block_num; i++) {
        free_bnum(inode.blocks[i]);
    }
    free_inum(inum);
    return 0;
}


// use fseek before fread, otherwise fs is not right
int rm_dir(int inum) {
    // free inode
    Inode inode;
    fseek(fs, InodeSeg + (sizeof(Inode) * inum), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, fs);
    // free sub
    Dir dir;
    for (int i = 0; i < inode.size / sizeof(Dir); i++) {
        fseek(fs, BlockSeg + (BlockSize * inode.blocks[0]) + (sizeof(Dir) * i), SEEK_SET);
        fread(&dir, sizeof(Dir), 1, fs);
        if (strcmp(dir.name, ".") == 0 || strcmp(dir.name, "..") == 0) {
            continue;
        }
        // printf("rm %s, inum = %d\n", dir.name, dir.inum);
        if (get_itype(dir.inum) == _FILE) {
            rm_file(dir.inum);
        } else {
            rm_dir(dir.inum);
        }
    }
    free_bnum(inode.blocks[0]);
    free_inum(inum);
    return 0;
}

int rm_cur(int inum) {
    for (int i = 0; i < cur_fnum; i++) {
        if (cur_files[i].inum == inum) {
            for (int j = i; j < cur_fnum-1; j++) {
                cur_files[j] = cur_files[j+1];
            }
            break;
        }
    }
    cur_fnum--;
    return 0;
}


int show() {
    int j = 0;
    for (int i = 0; i < cur_fnum; i++) {
        if (j++ == 4) {
            j = 0;
            printf("\n");
        }
        if (get_itype(cur_files[i].inum) == _DIR) {
            // printf("%s\n", cur_files[i].name);
            printf("\033[1;34m%-20s\033[0m", cur_files[i].name);
            //printf("\033[1;34m%-15s %d\033[0m", cur_files[i].name, cur_files[i].inum);
        } else {
            printf("%-20s", cur_files[i].name);
            //printf("%-15s %d", cur_files[i].name, cur_files[i].inum);
        }
    }
    printf("\n");
    return 0;
}

int show_more() {
    close_dir(cur_inum);
    int j = 0;
    for (int i = 0; i < cur_fnum; i++) {
        Inode inode;
        fseek(fs, InodeSeg + (sizeof(Inode) * cur_files[i].inum), SEEK_SET);
        fread(&inode, sizeof(Inode), 1, fs);
        if (inode.type == _DIR) {
            printf("drw\t");
        } else {
            printf("frw\t");
        }
        printf("%d\t", inode.block_num);
        printf("%d\t", inode.size);


        if (get_itype(cur_files[i].inum) == _DIR) {
            // printf("%s\n", cur_files[i].name);
            printf("\033[1;34m%s\033[0m", cur_files[i].name);
            //printf("\033[1;34m%-15s %d\033[0m", cur_files[i].name, cur_files[i].inum);
        } else {
            printf("%s", cur_files[i].name);
            //printf("%-15s %d", cur_files[i].name, cur_files[i].inum);
        }

        printf("\n");
    }
    printf("\n");
    return 0;
}

int show_map() {
    printf("------------------------Inode map--------------------------\n");
    for (int i = 0; i < 100; i++) {
        printf("%d ", sb.inode_map[i]);
    }
    printf("\n------------------------Block map--------------------------\n");
    for (int i = 0; i < 200; i++) {
        printf("%d ", sb.block_map[i]);
    }
    printf("\n");
}

void usage() {
    // red
    printf("\n\033[32;31mjust use it as linux shell\033[0m\n\n");
    // blue
    //printf("\n\033[0;34mjust use it as linux shell\033[0m\n");
    // printf("\033[显示方式;前景色;背景色m输出字符串\033[0m/n");
}
