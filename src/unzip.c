
/* gcc unzip.c -lzip */
#include <stdio.h>
#include <stdlib.h>
#include <zip.h>

int main(int argc, char **argv) {
    struct zip *zip_file;
    struct zip_file *file_in_zip;
    int err;
    int files_total;
    int r;
    char buffer[10000];

    if (argc < 2) {
        fprintf(stderr,"usage: %s <zipfile>\n",argv[0]);
        return -1;
    };

    zip_file = zip_open(argv[1], 0, &err);
    if (!zip_file) {
        fprintf(stderr,"Error: can't open file %s\n",argv[1]);
        return -1;
    };

    files_total = zip_get_num_files(zip_file);
    printf("%d files\n", files_total);

    int i = 0;
    for (i = 0; i<files_total; i++) {
        const char *filename =  zip_get_name(zip_file, i, 0);
        printf("getting file index %d: %s...\n", i, filename);
        file_in_zip = zip_fopen_index(zip_file, i, 0);
        if (file_in_zip) {
            while ( (r = zip_fread(file_in_zip, buffer, sizeof(buffer))) > 0) {
                printf("read %d bytes...\n", r);
            }
            zip_fclose(file_in_zip);
        } else {
            fprintf(stderr,"Error: can't open file %d in zip\n", i);
        }
    }
    zip_close(zip_file);

    return 0;
}
