#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("AuroraChat\n");
    printf("Press START to exit.\n");
    printf("Press B to say something\n");

    const int bufferSize = 0x20000; // 128KB
    void* buffer = malloc(bufferSize);
    if (!buffer) {
        printf("Failed to allocate memory for networking.\n");
        goto cleanup_gfx;
    }

    int ret = socInit(buffer, bufferSize);
    if (ret == 0) {
        printf("Networking initialized!\n");
    } else {
        printf("socInit() failed: %d\n", ret);
        free(buffer);
        goto cleanup_gfx;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        gspWaitForVBlank();
    }

    socExit();
    free(buffer);

cleanup_gfx:
    gfxExit();
    return 0;
}
