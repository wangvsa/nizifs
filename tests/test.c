#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>      /* for O_CREAT */
#include <fcntl.h>          /* for open() */
#include <unistd.h>         /* for close() */

static char path[] = "/mnt/nizifs/test.txt";
static char str[] = "Hello World";


/**
 * This function tests fwrite/fread/fseek/rewind
 */
void test_fwrite_fread(char *path) {
    FILE *fp;
    char *buf;
    long lSize;

    fp = fopen(path, "w");
    fwrite(str, sizeof(char), sizeof(str), fp);
    fclose(fp);

    fp = fopen(path, "r");
    fseek(fp, 0, SEEK_END);
    lSize = ftell(fp);          // Get the size of file
    rewind(fp);                 // Set back the offset
    printf("lszie: %ld\n", lSize);

    buf = (char *)malloc(sizeof(char) * lSize);
    fread(buf, sizeof(char), lSize, fp);
    printf("content: %s\n", buf);
    fclose(fp);
    free(buf);
}

void test_pwrite_pread(char *path) {
    int fd;
    ssize_t res;
    char *buf = (char *)malloc(sizeof(char) * sizeof(str));

    fd = open(path, O_CREAT|O_WRONLY);
    res = pwrite(fd, str, sizeof(str), 0);
    printf("pwrite: %ld\n", res);
    close(fd);

    fd = open(path, O_RDONLY);
    res = pread(fd, buf, sizeof(str), 0);
    printf("pread: %ld, %s\n", res, buf);
    close(fd);
}

int main(int argc, char *argv[]) {
    printf("Test suit\n");

    //test_fwrite_fread(path);
    test_pwrite_pread(path);
    return 0;
}
