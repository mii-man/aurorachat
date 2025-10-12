#include <3ds.h>
#include <stdio.h>

int main() {
  gfxInitDefault()
  consoleInit(GFX_TOP, NULL);

printf("AuroraChat\n");
printf("Press START to exit.\n");
printf("Press B to say something\n");

int ret = socInit(buffer, bufferSize);
    if ret == 0 {
      printf("Networking initialized!\n")
        } else {
      printf("socInit() falied: %d\n", ret);
    

while (aptMainLoop())
  hidScanInput();
u32 kDown = hidKeysDown();

if (kDown & KEY_START) break;

gspWaitForVBlank();
  }
  socExit();
  gfxExit();
  return 0;
}
