// aurorachat
// Author: mii-man
// Description: A chatting application for 3DS

// yes I did copy basically the entire hbchat codebase here and call it a day because ain't no way your original base was ever gonna work :sob:
// as mii man, yes my base would never work ;(

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <opusfile.h> // apparently this didnt mess with the compilation it was just a warning for me -.-

#include <3ds/applets/swkbd.h>

#include <citro2d.h>

u32 __stacksize__ = 0x100000; // 1 MB





C2D_TextBuf sbuffer;
C2D_Text stext;

C2D_TextBuf chatbuffer;
C2D_Text chat;

char chatstring[6000] = "-chat-";
char usernameholder[64];

float chatscroll = 20;

int scene = 1;

bool inacc = false;

int theme = 1;

bool switched = false;










// function time

void DrawText(char *text, float x, float y, int z, float scaleX, float scaleY, u32 color, bool wordwrap) {
    C2D_TextBufClear(sbuffer);
    C2D_TextParse(&stext, sbuffer, text);
    C2D_TextOptimize(&stext);

    if (!wordwrap) {
        C2D_DrawText(&stext, C2D_WithColor, x, y, z, scaleX, scaleY, color);
    }
    if (wordwrap) {
        C2D_DrawText(&stext, C2D_WithColor | C2D_WordWrap, x, y, z, scaleX, scaleY, color, 290.0f);
    }
}








// intentional
// haha










int main(int argc, char **argv) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    int themamt = 9;
    
    sbuffer = C2D_TextBufNew(4096);
    chatbuffer = C2D_TextBufNew(4096);


    C2D_TextParse(&chat, chatbuffer, chatstring);
    C2D_TextOptimize(&chat);


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
    server.sin_port = htons(8961); // new niche meme?
    server.sin_addr.s_addr = inet_addr("104.236.25.60"); // one below 61 (new niche meme)

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        // placeholder
    }

    char username[21]; //swkbd registers name, but client doesnt, now it does
    // okay
    // was i the one who made that comment???

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0; // NOT 10ms

    u32 textcolor;
    u32 themecolor;

    char buffer[512];
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        if (hidKeysDown() & KEY_A) {
            char message[64];
            char input[64];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username)); 
        }

        if (hidKeysDown() & KEY_B) {
            char message[80];
            char msg[128];

            char input[80];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 80);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, message, sizeof(message));
            if (button == SWKBD_BUTTON_CONFIRM) {
                sprintf(msg, "<%s>: %s", username, message);
                send(sock, msg, strlen(msg), 0);
            }
        }

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0; // gives more FPS, aka yummy

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {

                
                buffer[len] = '\0';

                char temp[6000];
                snprintf(temp, sizeof(temp), "%s\n%s", chatstring, buffer);
                strncpy(chatstring, temp, sizeof(chatstring));
                chatscroll = chatscroll - 15; // testing with this // hey hackertron here, are you done "testign" "with" "this"

                const char* parseResult = C2D_TextParse(&chat, chatbuffer, chatstring);
                if (parseResult != NULL && *parseResult != '\0') {
                    chatbuffer = C2D_TextBufResize(chatbuffer, 8192);
                    if (chatbuffer) {
                        C2D_TextBufClear(chatbuffer);
                        C2D_TextParse(&chat, chatbuffer, chatstring);
                    }
                }
                C2D_TextOptimize(&chat);
            }
        }


        if (strlen(chatstring) > 3500) {
            strcpy(chatstring, "-chat cleared!-\n");
            C2D_TextBufClear(chatbuffer);
            C2D_TextParse(&chat, chatbuffer, chatstring);
            C2D_TextOptimize(&chat);
            chatscroll = 20;
        }


        if (hidKeysDown() & KEY_START) break;

        if (hidKeysHeld() & KEY_CPAD_DOWN) {
            chatscroll = chatscroll - 5;
        }

        if (hidKeysHeld() & KEY_CPAD_UP) {
            chatscroll = chatscroll + 5;
        }

        if ((hidKeysDown() & KEY_L) && scene == 1) {
            scene = 2;
        }

        if ((hidKeysDown() & KEY_X) && scene == 2) {
            scene = 1;
        }

        if (hidKeysDown() & KEY_DUP) {
            if (!switched) {
                theme++;
                switched = true;
            }
            if (theme > themamt) {
                theme = 1;
            }
        }
        if (hidKeysDown() & KEY_DDOWN) {
            if (!switched) {
                theme--;
                switched = true;
            }
            if (theme < 1) {
                theme = themamt;
            }
        }

        switched = false;


        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, themecolor);
        C2D_SceneBegin(top);


        char *themename;
        if (theme == 1) {
            themename = "Aurora White";
            themecolor = C2D_Color32(255, 255, 255, 255);
            textcolor = C2D_Color32(0, 0, 0, 255);
        }
        if (theme == 2) {
            themename = "Deep Gray";
            themecolor = C2D_Color32(73, 73, 73, 255);
            textcolor = C2D_Color32(0, 0, 0, 255);
        }
        if (theme == 3) {
            themename = "Homeblue Chat";
            themecolor = C2D_Color32(0, 26, 242, 255);
            textcolor = C2D_Color32(0, 0, 0, 255);
        }
        if (theme == 4) {
            themename = "Hackertron Style";
            themecolor = C2D_Color32(0, 0, 0, 255);
            textcolor = C2D_Color32(17, 255, 0, 255);
        }
        if (theme == 5) {
            themename = "Dark Mode";
            themecolor = C2D_Color32(23, 27, 57, 255);
            textcolor = C2D_Color32(255, 255, 255, 255);
        }
        if (theme == 6) {
            themename = "Blurange";
            themecolor = C2D_Color32(0, 25, 117, 255);
            textcolor = C2D_Color32(255, 189, 97, 255);
        }
        if (theme == 7) {
            themename = "Red Paint";
            themecolor = C2D_Color32(255, 80, 80, 255);
            textcolor = C2D_Color32(255, 255, 255, 255);
        }
        if (theme == 8) {
            themename = "Deep Blue.";
            themecolor = C2D_Color32(6, 0, 57, 255);
            textcolor = C2D_Color32(255, 255, 255, 255);
        }
        if (theme == 9) {
            themename = "True Dark Mode";
            themecolor = C2D_Color32(0, 0, 0, 255);
            textcolor = C2D_Color32(255, 255, 255, 255);
        }

        if (scene == 1) {
            DrawText("aurorachat", 260.0f, 0.0f, 0, 1.0f, 1.0f, textcolor, false);
            

            sprintf(usernameholder, "%s %s", "Username:", username);

            DrawText(usernameholder, 0.0f, 200.0f, 0, 1.0f, 1.0f, textcolor, false);



            
            DrawText("v0.0.4.0", 335.0f, 25.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            
            




            DrawText("A: Change Username\nB: Send Message\nL: Rules\nD-PAD: Change Theme", 0.0f, 0.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            


            DrawText(themename, 170.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);

        }


        if (scene == 2) {
            DrawText("(Press X to Go Back)\n\nRule 1: No Spamming\n\nRule 2: No Swearing\n\nRule 3: No Impersonating\n\nRule 4: No Politics\n\nAll of these could result in a ban.", 0.0f, 0.0f, 0, 0.5f, 0.6f, textcolor, false);
        }

        C2D_TargetClear(bottom, themecolor);
        
        C2D_SceneBegin(bottom);

        C2D_DrawText(&chat, C2D_WithColor | C2D_WordWrap, 0.0f, chatscroll, 0, 0.5, 0.5, textcolor, 290.0f);



        C3D_FrameEnd(0);


        // svcSleepThread(1000000L); // required, otherwise audio can be glitchy, distorted, and chopped up.
        // audio is gone rn
        // will bring it back soon

    }



    
    closesocket(sock);
    ndspExit();
    socExit();
    gfxExit();
    return 0;
}
