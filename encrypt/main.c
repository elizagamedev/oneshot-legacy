#include <stdio.h>
#include "../encrypt_key.h"

int main(int argc, char** argv) {
    if(argc < 2) {
        return 1;
    }

    FILE* in = fopen(argv[1], "rb");
    FILE* out = fopen(argv[2], "wb");

    int i = 0, c;
    while((c = fgetc(in)) != EOF) {
        fputc((unsigned char)c ^ key[i++ % KEY_SIZE], out);
    }
    fclose(in);
    fclose(out);

    return 0;
}
