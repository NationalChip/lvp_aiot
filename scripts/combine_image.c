#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
//将image2 固件放在 image1 的 offset 处
void usage()
{
    printf("./combine_image \"image1's mcu_nor.bin\" \"image2's mcu_nor.bin\" offset\n");
    exit(-1);
}

static unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
    unsigned long result = 0,value;

    if (*cp == '0') {
        cp++;
        if ((*cp == 'x') && isxdigit(cp[1])) {
            base = 16;
            cp++;
        }
        if (!base) {
            base = 8;
        }
    }
    if (!base) {
        base = 10;
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
                    ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

int main(int argc, char const *argv[])
{
    if(argc < 4)
        usage();
    int size1, size2, offset, fill_size1;
    const unsigned char *image1, *image2;
    unsigned char *fill_ptr;
    unsigned char read_buf[1000];
    // offset = atoi(argv[3]);
    offset = simple_strtoul((const char *)argv[3], NULL, 0);
    image1 = argv[1];
    image2 = argv[2];
    struct stat statbuf;
    stat(image1, &statbuf);
    size1 = statbuf.st_size;
    stat(image2, &statbuf);
    size2 = statbuf.st_size;
    if(size1 > offset)
    {
        printf("offset size1 < image1 error\n");
        return -1;
    }
    FILE *fp1, *fp2;
    fp1 = fopen(image1, "rb+");
    if (!fp1) {
        printf("open file %s error\n", image1);
    }
    fp2 = fopen(image2, "rb+");
    if (!fp1) {
        printf("open file %s error\n", image1);
    }
    fill_size1 = offset - size1;
    fill_ptr = malloc(fill_size1);
    fseek(fp1, 0, SEEK_END);
    memset(fill_ptr, 0xff, fill_size1);
    fwrite(fill_ptr, 1, fill_size1, fp1);
    free(fill_ptr);
    offset = ftell(fp1);
    while(1) {
        offset = fread(read_buf, 1, 1000, fp2);
        int ret = fwrite(read_buf, 1, offset, fp1);
        if (offset < 1000)
            break;
    }
    fclose(fp1);
    fclose(fp2);
    return 0;
}
