// aurorachat 
// Made by mii-man
// Description: A chatting app for 3DS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h> // delicious
#include <citro2d.h> // delectable
// text.h doesn't exist (well it does, but as a part of citro2d.)
#include <malloc.h>

#include <sys/socket.h> // networking
#include <arpa/inet.h> // networking again

// keyboard
#include <3ds/applets/swkbd.h>

C2D_TextBuf tbuffer;
C2D_Text text;


// bro, that is NOT where the brackets go :sob:
int main(int argc, char* argv[]) {
	
	gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare(); // good
	C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT); // you need these two things




	u32 *soc_buffer = memalign(0x1000, 0x100000);
    if (!soc_buffer) {
        // placeholder
    }
    if (socInit(soc_buffer, 0x100000) != 0) {
        // placeholder
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        // placeholder
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(3071); // I copied this directly from hbchat that's why this is what it is
    server.sin_addr.s_addr = inet_addr(""); // server goes here (might help you with that bit, I'll explain)

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        // placeholder
    }



	char buffer[512];
    
	// Main loop
	while (aptMainLoop()) {

		fd_set readfds;
		struct timeval timeout;
	
		FD_ZERO(&readfds);
	    FD_SET(sock, &readfds);
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 10000; // 10ms
		
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_R) {
			send(sock, "lanburger", strlen("lanburger"), 0); // sends lanburger to server (without the quotes btw)
		}

		// recieve from server
		if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {
                buffer[len] = '\0';
            }
        }





		
		// C2D_Text() does not exist btw

		// start rendering
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		// begin top screen
        C2D_TargetClear(top, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(top);

		C2D_TextBufClear(tbuffer);
        C2D_TextParse(&text, tbuffer, "aurorachat");
        C2D_TextOptimize(&text);

		C2D_DrawText(&text, 0, 150.0f, 0.0f, 0.5f, 0.7f, 0.7f);

		// idk why i use this twice but i do normally and it's been fine so far so it's whatever
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		// begin bottom screen
        C2D_TargetClear(bottom, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(bottom);


		C2D_TextBufClear(tbuffer);
        C2D_TextParse(&text, tbuffer, "bottom text");
        C2D_TextOptimize(&text);

		C2D_DrawText(&text, 0, 100.0f, 0.0f, 0.5f, 0.7f, 0.7f);

		
			
        



		if (kDown & KEY_START)
			break; // leave
	}

	gfxExit();
	return 0;
}
