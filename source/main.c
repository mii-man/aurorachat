// aurorachat
// Authored by mii-man, Virtualle, cool-guy-awesome, ItsFuntum, and manti-09.
// NOTE: You CANNOT use swkbd while rendering! So don't! Otherwise you'll create the stupidest crash in history!

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <opusfile.h>

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

int theme = 8;

bool switched = false;





#define SAMPLE_RATE 48000
#define CHANNELS 2
#define BUFFER_MS 120
#define SAMPLES_PER_BUF (SAMPLE_RATE * BUFFER_MS / 1000)
#define WAVEBUF_SIZE (SAMPLES_PER_BUF * CHANNELS * sizeof(int16_t))

ndspWaveBuf waveBufs[2];
int16_t *audioBuffer = NULL;
LightEvent audioEvent;
volatile bool quit = false;

bool fillBuffer(OggOpusFile *file, ndspWaveBuf *buf) {
    int total = 0;
    while (total < SAMPLES_PER_BUF) {
        int16_t *ptr = buf->data_pcm16 + (total * CHANNELS);
        int ret = op_read_stereo(file, ptr, (SAMPLES_PER_BUF - total) * CHANNELS);
        if (ret <= 0) break;
        total += ret;
    }
    if (total == 0) return false;
    buf->nsamples = total;
    DSP_FlushDataCache(buf->data_pcm16, total * CHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(0, buf);
    return true;
}

void audioCallback(void *arg) {
    if (!quit) LightEvent_Signal(&audioEvent);
}

void audioThread(void *arg) {
    OggOpusFile *file = (OggOpusFile*)arg;
    while (!quit) {
        for (int i = 0; i < 2; i++) {
            if (waveBufs[i].status == NDSP_WBUF_DONE) {
                if (!fillBuffer(file, &waveBufs[i])) { quit = true; return; }
            }
        }
        svcSleepThread(10000000L);
//        LightEvent_Wait(&audioEvent);
    }
    return;
}




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

bool isSpriteTapped(C2D_Sprite* sprite, float scaleX, float scaleY) {
    static bool wasTouched = false;
    bool isTouched = (hidKeysHeld() & KEY_TOUCH);

    if (!wasTouched && isTouched) {
        touchPosition touch;
        hidTouchRead(&touch);
        float w = sprite->image.subtex->width * scaleX;
        float h = sprite->image.subtex->height * scaleY;
        float x = sprite->params.pos.x;
        float y = sprite->params.pos.y;
        float left = x - w/2, right = x + w/2;
        float top = y - h/2, bottom = y + h/2;

        if (touch.px >= left && touch.px <= right && touch.py >= top && touch.py <= bottom) {
            wasTouched = true;
            return true;
        }
    }

    if (!isTouched) wasTouched = false;
    return false;
}


















int main(int argc, char **argv) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    int themamt = 9;

    u32 sampleCount;

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetRate(0, SAMPLE_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspSetCallback(audioCallback, NULL);

    OggOpusFile *file = op_open_file("romfs:/Project_4.opus", NULL);

    sbuffer = C2D_TextBufNew(4096);

    audioBuffer = linearAlloc(WAVEBUF_SIZE * 2);
    memset(waveBufs, 0, sizeof(waveBufs));
    for (int i = 0; i < 2; i++) {
        waveBufs[i].data_pcm16 = audioBuffer + (i * SAMPLES_PER_BUF * CHANNELS);
        waveBufs[i].status = NDSP_WBUF_DONE;
    }

    LightEvent_Init(&audioEvent, RESET_ONESHOT);
    Thread thread = threadCreate(audioThread, file, 32 * 1024, 0x3F, 1, false);

    // chatgpt
    if (!fillBuffer(file, &waveBufs[0]));
    if (!fillBuffer(file, &waveBufs[1]));
    
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
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // one below 61 (new niche meme)

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
    char lastbuffer[512];

    char acc_password[21];
    char acc_username[21];

    send(sock, "LOGGEDIN?", strlen("LOGGEDIN?"), 0);
    int loginstatus = 1;

    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

//        if (hidKeysDown() & KEY_A) { // this is commented because usernames are now handled by the server. Not the client (this will change when I add display names!)
//            char message[64];
//            char input[64];
//            SwkbdState swkbd;
//            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
//            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
//            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
//
//            SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username)); 
//        }

        if (scene == 1) {
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
                    sprintf(msg, "CHAT,%s", message);
                    send(sock, msg, strlen(msg), 0);
                }
            }
        }

        if (scene == 4) {
            if (hidKeysDown() & KEY_L) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                SwkbdButton button = swkbdInputText(&swkbd, acc_username, sizeof(acc_username));
            }
            if (hidKeysDown() & KEY_R) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                SwkbdButton button = swkbdInputText(&swkbd, acc_password, sizeof(acc_password));
            }
            if (hidKeysHeld() & KEY_X) {
                char msg[80];
                sprintf(msg, "MAKEACC,%s,%s", acc_username, acc_password);
                send(sock, msg, strlen(msg), 0);
            }
        }
        if (scene == 5) {
            if (hidKeysDown() & KEY_L) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                SwkbdButton button = swkbdInputText(&swkbd, acc_username, sizeof(acc_username));
            }
            if (hidKeysDown() & KEY_R) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                SwkbdButton button = swkbdInputText(&swkbd, acc_password, sizeof(acc_password));
            }
            if (hidKeysHeld() & KEY_X) {
                char msg[80];
                sprintf(msg, "LOGINACC,%s,%s", acc_username, acc_password);
                send(sock, msg, strlen(msg), 0);
            }
        }


//        if (hidKeysHeld() & KEY_X) {
//            send(sock, "MAKEACC,RIZZ,GYATT", strlen("MAKEACC,RIZZ,GYATT"), 0);
//        }

//        if (hidKeysHeld() & KEY_Y) { // keep these for reference but don't uncomment them
//            send(sock, "LOGINACC,RIZZ,GYATT", strlen("LOGINACC,RIZZ,GYATT"), 0);
//        }
//        if (hidKeysHeld() & KEY_R) {
//            send(sock, "LOGGEDIN?", strlen("LOGGEDIN?"), 0);
//        }

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {

                
                buffer[len] = '\0';

                lastbuffer[512] = buffer[len];

                if (loginstatus == 0) {
                    char temp[6000];
                    snprintf(temp, sizeof(temp), "%s\n%s", chatstring, buffer);
                    strncpy(chatstring, temp, sizeof(chatstring));
                    chatscroll = chatscroll - 20; // increase
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
                if (loginstatus == 1) {
                    if (strcmp(buffer, "False") == 0) {
                        loginstatus = 2;
                        scene = 3;
                    }
                    if (strcmp(buffer, "True") == 0) {
                        loginstatus = 0;
                        scene = 1;
                    }
                }
                if (loginstatus == 2) {
                    if (strcmp(buffer, "USR_CREATED") == 0) {
                        loginstatus = 1;
                        scene = 3;
                    }
                    if (strcmp(buffer, "USR_IN_USE") == 0) {
                        loginstatus = 1;
                        scene = 3;
                    }
                }
                if (loginstatus == 3) {
                    if (strcmp(buffer, "LOGIN_OK") == 0) {
                        loginstatus = 0;
                        scene = 1;
                    }
                    if (strcmp(buffer, "LOGIN_WRONG_PASS") == 0) {
                        loginstatus = 1;
                        scene = 3;
                    }
                }
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

        DrawText(lastbuffer, 140.0f, 0.0f, 0, 1.0f, 1.0f, textcolor, false);


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


            DrawText("aurorachat", 140.0f, 0.0f, 0, 1.0f, 1.0f, textcolor, false);
            

            sprintf(usernameholder, "%s %s", "Username:", username);

            DrawText(usernameholder, 0.0f, 200.0f, 0, 1.0f, 1.0f, textcolor, false);



            
            DrawText("v0.0.4.0", 175.0f, 25.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            
            




//            DrawText("A: Change Username\nB: Send Message\nL: Rules\nD-PAD: Change Theme", 0.0f, 0.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            


//            DrawText(themename, 0.0f, 270.0f, 0, 0.4f, 0.4f, textcolor, false);

        }


        if (scene == 2) {
            DrawText("(Press X to Go Back)\n\nRule 1: No Spamming\n\nRule 2: No Swearing\n\nRule 3: No Impersonating\n\nRule 4: No Politics\n\nAll of these could result in a ban.", 0.0f, 0.0f, 0, 0.5f, 0.6f, textcolor, false);
        }

        C2D_TargetClear(bottom, themecolor);
        
        C2D_SceneBegin(bottom);
        
        if (scene == 1 || scene == 2) {
            C2D_DrawRectSolid(20, 0, 0, 275, 250, C2D_Color32(200, 200, 200, 70));
            C2D_DrawText(&chat, C2D_WithColor | C2D_WordWrap, 35.0f, chatscroll, 0, 0.5, 0.5, textcolor, 200.0f);
        }

        if (scene == 3) {
            C2D_SceneBegin(top);
            DrawText("Press A to Sign Up.", 150.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);
            DrawText("Press B to Log in.", 150.0f, 40.0f, 0, 0.4f, 0.4f, textcolor, false);

            if (hidKeysHeld() & KEY_A) {
                loginstatus = 2;
                scene = 4;
            }
            if (hidKeysHeld() & KEY_B) {
                loginstatus = 3;
                scene = 5;
            }
        }

        if (scene == 4) {
            C2D_SceneBegin(top);
            DrawText("Press L to set your username.", 0.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);
            DrawText("Press R to set your password.", 0.0f, 40.0f, 0, 0.4f, 0.4f, textcolor, false);
            DrawText("Press X to register.", 0.0f, 80.0f, 0, 0.4f, 0.4f, textcolor, false);
        }

        if (scene == 5) {
            C2D_SceneBegin(top);
            DrawText("Press L to input your username.", 0.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);
            DrawText("Press R to input your password.", 0.0f, 40.0f, 0, 0.4f, 0.4f, textcolor, false);
            DrawText("Press X to login after you've inputted your username and password.", 0.0f, 80.0f, 0, 0.4f, 0.4f, textcolor, false);
        }

        if (scene == 6) {
            C2D_SceneBegin(top);
            DrawText("Account created! Press A to proceed to the login screen.", 0.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);
            if (hidKeysDown() & KEY_A) {
                loginstatus = 3;
                scene = 5;
            }
        }


        C3D_FrameEnd(0);

        if (waveBufs[0].status == NDSP_WBUF_DONE) {
            if (!fillBuffer(file, &waveBufs[0]));
        }
        if (waveBufs[1].status == NDSP_WBUF_DONE) {
            if (!fillBuffer(file, &waveBufs[1]));
        }


        svcSleepThread(1000000L); // required, otherwise audio can be glitchy, distorted, and chopped up.

    }



    
    closesocket(sock);
    ndspExit();
    socExit();
    gfxExit();
    return 0;
}
