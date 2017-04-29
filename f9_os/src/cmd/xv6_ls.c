#include "../xx6/types.h"
#include "../xx6/fs.h"

char*
fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void ls(const char *path)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        printk( "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        printf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.st_mode & _IFMT){
        case _IFREG:
        	printk( "%s %d %d %d\n", fmtname(path), st.st_mode, st.st_ino, st.st_size);
            break;

        case _IFDIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            	printk( "ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                	printk(  "ls: cannot stat %s\n", buf);
                    continue;
                }
                printk( "%s %d %d %d\n", fmtname(buf), st.st_mode , st.st_ino, st.st_size);
            }
            break;
    }
    close(fd);
}
#if 0
int
main(int argc, char *argv[])
{
    int i;

    if(argc < 2){
        ls(".");
        exit();
    }
    for(i=1; i<argc; i++)
        ls(argv[i]);
    exit();
}
#endif
