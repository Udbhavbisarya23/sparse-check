#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/stat.h>
#include<sys/ioctl.h>
#include<errno.h>
#include<inttypes.h>
#include<fcntl.h>

#include<linux/fs.h>
#include<linux/fiemap.h>
#include<unistd.h>

int main(int argc, char* argv[]) {
    int fd;
    fd = open("gistfile1.txt", O_RDONLY | O_NOCTTY);
    int newfile = open("newFile.txt", O_RDWR | O_CREAT | O_NOCTTY);


    // Check whether a file is sparse or not
    struct stat info;
    int cause = 0;
    if (fstat(fd, &info) == -1) {
        cause = errno;
    }
    __u64 total, stored, sparse;

    total = (__u64)info.st_size;
    if (total % (__u64)info.st_blksize)
        total += (__u64)info.st_blksize - ((__u64)total % (__u64)info.st_blksize);
    stored = (__u64)512 * (__u64)info.st_blocks;

    sparse = total - stored;
    __u64 sparse_blocks = sparse / (__u64)info.st_blksize;
    if(sparse % (__u64)info.st_blksize)
        sparse_blocks += 1;
    
    printf("Before error?\n");
    printf("%-16.16llx\n", total);
    printf("%-16.16llx\n", sparse);

    //If the file is sparse, use the extents to generate a new file
    if(sparse > 0) {
        printf("Sparse blocks exist\n");
        struct fiemap* fiemap;

        if((fiemap = (struct fiemap*)malloc(sizeof(struct fiemap))) == NULL) {
            fprintf(stderr, "Can't allocate fiemap\n");
            return 0;
        }

        memset(fiemap,0,sizeof(struct fiemap));
        fiemap->fm_start = 0;
        fiemap->fm_length = ~0;	
        fiemap->fm_flags = 0;
        fiemap->fm_extent_count = 0;
        fiemap->fm_mapped_extents = 0;

        if(ioctl(fd,FS_IOC_FIEMAP,fiemap) < 0) {
            fprintf(stderr,"Ioctl() not working\n");
            return 0;
        }

        int total_extent_size = sizeof(struct fiemap_extent) * (fiemap->fm_mapped_extents);
        
        if((fiemap = (struct fiemap*)realloc(fiemap,sizeof(struct fiemap) + total_extent_size)) == NULL) {
            fprintf(stderr,"Error in reallocation of fiemap");
        }

        memset(fiemap->fm_extents,0,sizeof(struct fiemap_extent));
        fiemap->fm_extent_count = fiemap->fm_mapped_extents;
	    fiemap->fm_mapped_extents = 0;

        if(ioctl(fd,FS_IOC_FIEMAP,fiemap) < 0) {
            fprintf(stderr,"Ioctl() not working\n");
            return 0;
        }
        __u64 total_file_length = 0;
        for (int i=0;i<fiemap->fm_mapped_extents;i++) {
            total_file_length += fiemap->fm_extents[i].fe_length;
        }
        printf("Total length is :- %-16.16llx \n",total_file_length);

        for(int i=0;i<fiemap->fm_mapped_extents;i++) {
            long curr_pos = lseek(fd,fiemap->fm_extents[i].fe_logical,SEEK_SET);
            printf("Current position is :- %-4.4lx \n", curr_pos);

            int size = fiemap->fm_extents[i].fe_length;
            unsigned char buffer[size];
            read(fd,buffer,size);

            for(int j=0;j<size;j++) {
                printf("%c",buffer[j]);
            }
            printf("\n");

            write(newfile,buffer,size);
        }
        
    }
    return 0;
}