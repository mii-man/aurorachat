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
#include <opusfile.h> // i almost forgor

#include <3ds/applets/swkbd.h>

#include <citro2d.h>

// Audio functions and definitions

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
        LightEvent_Wait(&audioEvent);
    }
    return;
}

// it took me ages to figure this out, btw, if you compile this you're gonna need to install opusfile / libopusfile in devkitpro (might provide more info)


// also audio doesn't work on emulators, only real hardware.











C2D_TextBuf sbuffer;
C2D_Text stext;

C2D_TextBuf chatbuffer;
C2D_Text chat;

char chatstring[6000] = "-chat-";
char usernameholder[64];

float chatscroll = 20;

int scene = 1;

bool inacc = false;



int main(int argc, char **argv) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetRate(0, SAMPLE_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspSetCallback(audioCallback, NULL);

    OggOpusFile *file = op_open_file("romfs:/incomingmessage.opus", NULL); // you HAVE to use .opus, just convert files to opus and you're good.

    audioBuffer = linearAlloc(WAVEBUF_SIZE * 2);
    memset(waveBufs, 0, sizeof(waveBufs));
    for (int i = 0; i < 2; i++) {
        waveBufs[i].data_pcm16 = audioBuffer + (i * SAMPLES_PER_BUF * CHANNELS);
        waveBufs[i].status = NDSP_WBUF_DONE;
    }

    LightEvent_Init(&audioEvent, RESET_ONESHOT);
    Thread thread = threadCreate(audioThread, file, 32 * 1024, 0x18, 1, false);

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
    server.sin_port = htons(8961); // Whoops. Forgot to change that when I changed the server port.
    server.sin_addr.s_addr = inet_addr("104.236.25.60"); // put a server here

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        // placeholder
    }

    char username[11];

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms

    char buffer[512];
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        if (hidKeysDown() & KEY_A) {
            char message[64];
            char input[64];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 11);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username)); 
        }

        if (hidKeysDown() & KEY_B) {
            char message[64];
            char msg[128];

            char input[80];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 80);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, message, sizeof(message));
            if (button == SWKBD_BUTTON_CONFIRM) {
                if (inacc) {
                    sprintf(msg, "<%s>: %s", username, message);
                }
                if (!inacc) {
                    sprintf(msg, "<%s>: %s", username, message);
                }
                send(sock, msg, strlen(msg), 0);
                printf("Message sent!\n");
            }
        }

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {
                // play sound (note: set sound in settings to stereo on 3ds, helps with volume cause it can be hard to hear sometimes.)
                quit = true;
                LightEvent_Signal(&audioEvent);
                if (thread) {
                    threadJoin(thread, U64_MAX);
                    threadFree(thread);
                }
    
                // Cleanup
                if (file) op_free(file);
                linearFree(audioBuffer);
    
                // Reopen file
                file = op_open_file("romfs:/incomingmessage.opus", NULL);
                if (!file) continue;
    
                // Reallocate and reset buffers
                audioBuffer = linearAlloc(WAVEBUF_SIZE * 2);
                memset(waveBufs, 0, sizeof(waveBufs));
                for (int i = 0; i < 2; i++) {
                    waveBufs[i].data_pcm16 = audioBuffer + (i * SAMPLES_PER_BUF * CHANNELS);
                    waveBufs[i].status = NDSP_WBUF_DONE;
                }
    
                // Pre-fill both buffers
                quit = false;
                if (!fillBuffer(file, &waveBufs[0])) continue;
                if (!fillBuffer(file, &waveBufs[1])) continue;
    
                // Restart thread
                thread = threadCreate(audioThread, NULL, 32 * 1024, 0x18, 1, false);



                
                buffer[len] = '\0';

                sprintf(chatstring, "%s\n%s", chatstring, buffer);

                C2D_TextParse(&chat, chatbuffer, chatstring);
                C2D_TextOptimize(&chat);
                chatscroll = chatscroll - 25;

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
            strcpy(chatstring, "-chat-\n");
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


        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(top);




        if (waveBufs[0].status == NDSP_WBUF_DONE) {
            if (!fillBuffer(file, &waveBufs[0]));
        }
        if (waveBufs[1].status == NDSP_WBUF_DONE) {
            if (!fillBuffer(file, &waveBufs[1]));
        }




        


        if (scene == 1) {
            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "aurorachat");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 250.0f, 0.0f, 0.5f, 1.0f, 1.0f);

            sprintf(usernameholder, "%s %s", "Username:", username);

            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, usernameholder);
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 200.0f, 0.5f, 1.0f, 1.0f);


            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "v0.0.2");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 352.0f, 25.0f, 0.5f, 0.6f, 0.6f);



            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "A: Change Username\nB: Send Message\nL: Rules)");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 0.0f, 0.5f, 0.6f, 0.6f);

        }


        if (scene == 2) {
            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "(Press X to Go Back)\n\nRule 1: No Spamming\n\nRule 2: No Swearing\n\nRule 3: No Impersonating\n\nRule 4: No Politics\n\nAll of these could result in a ban.");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 0.0f, 0.5f, 0.6f, 0.6f);
        }


        C2D_TargetClear(bottom, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(bottom);

        C2D_DrawText(&chat, C2D_WordWrap, 0.0f, chatscroll, 0.5f, 0.5f, 0.5f, 290.0f);



        C3D_FrameEnd(0);


        svcSleepThread(1000000L); // required, otherwise audio can be glitchy, distorted, and chopped up.

    }


    if (file) op_free(file);
    linearFree(audioBuffer);

    
    closesocket(sock);
    ndspExit();
    socExit();
    gfxExit();
    return 0;
}
