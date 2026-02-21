/*
                                       _           _   
                                      | |         | |  
   __ _ _   _ _ __ ___  _ __ __ _  ___| |__   __ _| |_ 
  / _` | | | | '__/ _ \| '__/ _` |/ __| '_ \ / _` | __|
 | (_| | |_| | | | (_) | | | (_| | (__| | | | (_| | |_ 
  \__,_|\__,_|_|  \___/|_|  \__,_|\___|_| |_|\__,_|\__|                    
             __  __     __  ______ __     
            |__)|_ |  ||__)| |  | |_ |\ | 
            | \ |__|/\|| \ | |  | |__| \| 
                              

    Project initialized and owned by: mii-man
    Lead Developer: Virtualle / VirtuallyExisting
    Server Developed by: hackertron and Orstando
    Music by: Virtualle, manti-09
    Commentary by: Virtualle




    This project was inspired by hbchat, a project created by Virtualle in October 2025, the project collapsed due to a doxxing that had 4 victims.


    Aurorachat is designed to remove the flawed, tangled mess of bugs and exploits that were in hbchat and make something that will inspire many and grow a community once more.





    This code is owned underneath the license given with the project, if you are to distribute this code the license is required to be with the code.
    


    This is a rewrite of the original aurorachat, remade to use aurorahttp by Orstando and to have much better choices. The comment you've just read was created by VirtuallyExisting / Virtualle, have a good day!


*/

/*
.__          .__                   __                  __                      ._.
|__| _____   |  |__ _____    ____ |  | __ ____________/  |________  ____   ____| |
|  |/     \  |  |  \\__  \ _/ ___\|  |/ // __ \_  __ \   __\_  __ \/  _ \ /    \ |
|  |  Y Y  \ |   Y  \/ __ \\  \___|    <\  ___/|  | \/|  |  |  | \(  <_> )   |  \|
|__|__|_|  / |___|  (____  /\___  >__|_ \\___  >__|   |__|  |__|   \____/|___|  /_
         \/       \/     \/     \/     \/    \/                               \/\/
The comment you've just read was created by cool-guy-awesome / hackertron. I hope you liked it.


You better like it.


Or else.






Have a great day!
*/

/* Oh yeah, and don't remove my newlines. Thanks! -hackertron */



#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <citro2d.h>
#include <malloc.h>
#include <opusfile.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <3ds/applets/swkbd.h>

/*

    HTTP(S) Functions
    Coded by: Virtualle
    Extra credit: devKitPro examples

*/


u32 size;
u32 siz;

u32 bw;

u8 *buf;

volatile bool downloadDone;

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} cstring;

cstring cstr_new(const char* str) {
    cstring s = {0};
    if (str) {
        s.len = strlen(str);
        s.cap = s.len + 1;
        s.data = malloc(s.cap);
        if (s.data) strcpy(s.data, str);
    }
    return s;
}

void cstr_free(cstring* s) {
    if (s->data) free(s->data);
    s->data = NULL;
    s->len = s->cap = 0;
}

bool CHECK_RESULT(const char* name, Result res) {
    bool failed = R_FAILED(res);
    printf("%s: %s! (0x%08lX)\n", name, failed ? "failed" : "success", res);
    return failed;
}

bool download(const char* url_str, const char* path) {
    httpcContext context;
    u32 status = 0;
    cstring url = cstr_new(url_str);
    downloadDone = false;
    bool success = false;

    while (true) {
        if (CHECK_RESULT("httpcOpenContext", httpcOpenContext(&context, HTTPC_METHOD_GET, url.data, 0))) goto cleanup;
        if (CHECK_RESULT("httpcSetSSLOpt",   httpcSetSSLOpt(&context, SSLCOPT_DisableVerify))) goto close;
        if (CHECK_RESULT("httpcSetKeepAlive", httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED))) goto close;
        if (CHECK_RESULT("httpcAddRequestHeaderField", httpcAddRequestHeaderField(&context, "User-Agent", "skibidi toilet"))) goto close;
        if (CHECK_RESULT("httpcAddRequestHeaderField", httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive"))) goto close;

        if (CHECK_RESULT("httpcBeginRequest", httpcBeginRequest(&context))) goto close;

        if (CHECK_RESULT("httpcGetResponseStatusCode", httpcGetResponseStatusCode(&context, &status))) goto close;

        if ((status >= 301 && status <= 303) || (status >= 307 && status <= 308)) {
            char newurl[0x1000];
            if (CHECK_RESULT("httpcGetResponseHeader", httpcGetResponseHeader(&context, "Location", newurl, sizeof(newurl)))) goto close;
            cstr_free(&url);
            url = cstr_new(newurl);
            httpcCloseContext(&context);
            continue;
        }
        break;
    }

    if (status != 200) goto close;

    if (CHECK_RESULT("httpcGetDownloadSizeState", httpcGetDownloadSizeState(&context, NULL, &siz))) goto close;

    FS_Archive sdmcRoot;
    if (CHECK_RESULT("FSUSER_OpenArchive", FSUSER_OpenArchive(&sdmcRoot, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) goto close;

    Handle fileHandle;
    FS_Path filePath = fsMakePath(PATH_ASCII, path);
    if (CHECK_RESULT("FSUSER_OpenFile", FSUSER_OpenFile(&fileHandle, sdmcRoot, filePath, FS_OPEN_CREATE | FS_OPEN_READ | FS_OPEN_WRITE, FS_ATTRIBUTE_ARCHIVE))) {
        FSUSER_CloseArchive(sdmcRoot);
        goto close;
    }

    if (CHECK_RESULT("FSFILE_SetSize", FSFILE_SetSize(fileHandle, 0))) {
        FSFILE_Close(fileHandle);
        FSUSER_CloseArchive(sdmcRoot);
        goto close;
    }

    u8* data = malloc(0x4000);
    if (!data) {
        FSFILE_Close(fileHandle);
        FSUSER_CloseArchive(sdmcRoot);
        goto close;
    }

    bw = 0;
    u32 readSize;
    Result ret;

    do {
        ret = httpcDownloadData(&context, data, 0x4000, &readSize);
        if (readSize > 0) {
            FSFILE_Write(fileHandle, NULL, bw, data, readSize, FS_WRITE_FLUSH);
            bw += readSize;
        }
    } while (ret == HTTPC_RESULTCODE_DOWNLOADPENDING);

    success = (ret == 0);

    FSFILE_Close(fileHandle);
    FSUSER_CloseArchive(sdmcRoot);

close:
    httpcCloseContext(&context);
cleanup:
    free(data);
    cstr_free(&url);
    downloadDone = true;
    return success;
}

void downloadThreadFunc(void* arg) {
    char** args = (char**)arg;
    char* url = args[0];
    char* path = args[1];

    download(url, path);

    free(url);
    free(path);
    free(args);
}

Thread startDownload(const char* url, const char* path) {
    char** args = malloc(2 * sizeof(char*));
    args[0] = strdup(url);
    args[1] = strdup(path);

    Thread thread = threadCreate(downloadThreadFunc, args, 0x8000, 0x38, -2, false);
    if (thread == NULL) {
        printf("Failed to create thread!\n");
        free(args[0]); free(args[1]); free(args);
    }
    return thread;
}




Result http_post(const char* url, const char* data) {
    Result ret = 0;
    httpcContext context;
    char *newurl = NULL;
    u32 statuscode = 0;
    u32 contentsize = 0, readsize = 0, size = 0;
    buf = NULL;
    u8 *lastbuf = NULL;
    char length[256];
    int contentlength;

    do {
        ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
        if (ret != 0) break;

        ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
        if (ret != 0) break;

        ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        if (ret != 0) break;

        ret = httpcAddRequestHeaderField(&context, "User-Agent", "skibidi toilet");
        if (ret != 0) break;

        ret = httpcAddRequestHeaderField(&context, "Content-Type", "application/json");
        if (ret != 0) break;

        contentlength = strlen(data);
        sprintf(length, "%d", contentlength);

        ret = httpcAddRequestHeaderField(&context, "Content-Length", length);
        if (ret != 0) break;

        ret = httpcAddPostDataRaw(&context, (u32*)data, strlen(data));
        if (ret != 0) break;

        ret = httpcBeginRequest(&context);
        if (ret != 0) break;

        ret = httpcGetResponseStatusCode(&context, &statuscode);
        if (ret != 0) break;

        // Handle redirects
        if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
            if (newurl == NULL) newurl = malloc(0x1000);
            if (newurl == NULL) { ret = -1; break; }

            ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
            url = newurl;
            httpcCloseContext(&context);
            continue;
        }

        if (statuscode != 200) {
            printf("HTTP Error: %x\n", statuscode);
            ret = -2;
            break;
        }

        ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
        if (ret != 0) break;

        buf = (u8*)malloc(0x1000);
        if (buf == NULL) { ret = -1; break; }

        do {
            ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
            size += readsize;
            if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
                lastbuf = buf;
                buf = realloc(buf, size + 0x1000);
                if (buf == NULL) { free(lastbuf); ret = -1; break; }
            }
        } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

        if (ret == 0) {
            buf = realloc(buf, size); // Resize to actual size
            printf("Response: %s\n", buf);
        }

    } while(0);

    if (newurl) free(newurl);
    httpcCloseContext(&context);
    return ret;
}


/*

    VCOI (Virtualle's Custom Opus Implementation)
    Coded by: Virtualle
    Extra Credit: libopusfile, devKitPro    


*/

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
        
        if (ret <= 0) {
            // End of stream or error — try to loop
            if (ret == OP_HOLE || ret == OPUS_INVALID_PACKET) {
                continue; // Skip invalid data, keep reading
            }
            // Attempt to seek to beginning of stream
            if (op_pcm_seek(file, 0) < 0) {
                break; // Failed to seek — can't loop
            }
            // After seeking, try to read again
            continue;
        }
        total += ret;
    }

    if (total == 0) return false; // No data filled (failed to recover)

    buf->nsamples = total;
    DSP_FlushDataCache(buf->data_pcm16, total * CHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(0, buf);
    return true;
}

bool fillBufferNoLoop(OggOpusFile *file, ndspWaveBuf *buf) {
    int total = 0;
    while (total < SAMPLES_PER_BUF) {
        int16_t *ptr = buf->data_pcm16 + (total * CHANNELS);
        int ret = op_read_stereo(file, ptr, (SAMPLES_PER_BUF - total) * CHANNELS);
        
        if (ret <= 0) {
            // End of stream or error — try to loop
            if (ret == OP_HOLE || ret == OPUS_INVALID_PACKET) {
                continue; // Skip invalid data, keep reading
            }
            // Attempt to seek to beginning of stream
            break; // Failed to seek — can't loop
            // After seeking, try to read again
            continue;
        }
        total += ret;
    }

    if (total == 0) return false; // No data filled (failed to recover)

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

typedef struct {
    OggOpusFile* file;
    bool quit;
} AudioThreadData;

void audioThreadFunc(void* arg) {
    AudioThreadData* data = (AudioThreadData*)arg;

    ndspChnReset(0);
    ndspChnSetRate(0, 48000);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspSetCallback(audioCallback, NULL);

    for (int i = 0; i < 2; i++) {
        waveBufs[i].data_pcm16 = audioBuffer + (i * SAMPLES_PER_BUF * CHANNELS);
        waveBufs[i].status = NDSP_WBUF_DONE;
    }

    while (!data->quit) {
        for (int i = 0; i < 2; i++) {
            if (waveBufs[i].status == NDSP_WBUF_DONE) {
                if (!fillBufferNoLoop(data->file, &waveBufs[i])) {
                    data->quit = true;
                    break;
                }
            }
        }
        svcSleepThread(1000000L);
    }

    op_free(data->file);
    linearFree(audioBuffer);
    svcExitThread();
}

bool playsound = true;

Thread playSound(const char* path) {
    if (playsound) {
        int err;
        OggOpusFile* file = op_open_file(path, &err);
        if (!file) return NULL;

        audioBuffer = (int16_t*)linearAlloc(WAVEBUF_SIZE * 2);
        if (!audioBuffer) { op_free(file); return NULL; }

        AudioThreadData* data = (AudioThreadData*)malloc(sizeof(AudioThreadData));
        data->file = file;
        data->quit = false;

        Thread thread = threadCreate(audioThreadFunc, data, 0x10000, 0x1F, -2, true);
        return thread; // Returns NULL on failure
    }
}



/*

    Sprite Tap Detection
    Coded by: Virtualle
    Note: This is pretty flawed in a lot of cases, but it works to some extent.

*/

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




/*

    DrawText()
    Coded by: Virtualle
    Note: This is not very good, however it gets the job done fairly easily!

*/

C2D_TextBuf sbuffer;
C2D_Text stext;

void DrawText(char *text, float x, float y, int z, float scaleX, float scaleY, u32 color, bool wordwrap) {
//    if (!sbuffer) {return;}
    C2D_TextBufClear(sbuffer);
    C2D_TextParse(&stext, sbuffer, text);
    C2D_TextOptimize(&stext);
    float wordwrapsize = 290.0f;

    if (!wordwrap) {
        C2D_DrawText(&stext, C2D_WithColor, x, y, z, scaleX, scaleY, color);
    }
    if (wordwrap) {
        C2D_DrawText(&stext, C2D_WithColor | C2D_WordWrap, x, y, z, scaleX, scaleY, color, wordwrapsize);
    }
}


/*


    Append Chat Message
    Coded by: Virtualle
    Extra credit: Funtum
    Note: This is based off of Funtum's version, even named the same, but it is heavily different.


*/

float chatscroll = 10.0f;

char chat[3000] = "-chat-\n";

void append_message(char* to_add) {
    size_t len = strlen(chat);
    size_t add_len = strlen(to_add);
    int space_left = 2000 - len - 1;

    if (add_len + 1 > space_left) {
        chat[0] = '\0';
        chatscroll = 10.0f;
    }

    // Copy to_add into chat with line breaks every 30 chars
    int i = 0;
    while (i < add_len) {
        int chunk = (add_len - i >= 50) ? 50 : add_len - i;
        strncat(chat, to_add + i, chunk);
        len = strlen(chat);
        if (i + chunk < add_len && len + 2 < 3000) {
            strcat(chat, "\n");
        }
        i += chunk;
    }
    chatscroll = chatscroll - 10;
}

C2D_SpriteSheet spriteSheet;
C2D_Sprite button;
C2D_Sprite button2;
C2D_Sprite button3;
C2D_Sprite button4;

int scene = 1;

char password[21];
char username[21];

bool showpassword = false;

char buftext[256];

bool showpassjustpressed = false;

bool outdated = false;



/*

    This is the main function, this is what code will run when you actually start the program.

*/


int main() {

    fsInit();
	romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    httpcInit(4 * 1024 * 1024);

    spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    C2D_SpriteFromImage(&button, C2D_SpriteSheetGetImage(spriteSheet, 0));
    C2D_SpriteFromImage(&button2, C2D_SpriteSheetGetImage(spriteSheet, 0));
    C2D_SpriteFromImage(&button3, C2D_SpriteSheetGetImage(spriteSheet, 0));
    C2D_SpriteFromImage(&button4, C2D_SpriteSheetGetImage(spriteSheet, 0));

    http_post("http://104.236.25.60:3072/api", "{\"cmd\":\"CONNECT\", \"version\":\"4.2\"}");
    sprintf(buftext, "%s", buf);
    if (strstr(buftext, "OUTDATED") != 0) {
        outdated = true;
    }

    sbuffer = C2D_TextBufNew(4096);

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

    /*
    
        Sockets have been majorly deprecated, however they are still used for grabbing messages.
        The reason for this is because these raw TCP sockets are suspected to be a cause of the doxxing.
    
    */

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(4040);
    server.sin_addr.s_addr = inet_addr("104.236.25.60");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        // placeholder
    }

    u32 textcolor = C2D_Color32(0, 0, 0, 200);
    u32 themecolor = C2D_Color32(255, 255, 255, 255);
    u32 textcolorb = C2D_Color32(0, 0, 0, 200);
    u32 themecolorb = C2D_Color32(255, 255, 255, 255);
    u32 logocolor = C2D_Color32(0, 0, 0, 200);

    u32 textcolor_temp = C2D_Color32(0, 0, 0, 255);
    u32 themecolor_temp = C2D_Color32(0, 0, 0, 255);

    int darkmode = 0; /* keeping the name just in case but most of these are probably too bright lol */
    int themecount = 18; // just in case--it's actually one less
    bool reversi;
    char currenttheme[30] = "none"; /* it'd be nice if it could tell you which one you're using... */

    int rulesvisible = 0;
    
    int silly = 0; //used for the silly rules check


    /*
    
        This is what will run every frame. You have to be careful with things that MUST only be triggered once here.
    
    */

    char buffer[1024] = {0};

    while (aptMainLoop()) {
        /*
        
            Scan inputs, allows the program to detect button presses.
        
        */
        hidScanInput();



        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {

                buffer[len] = '\0';


                append_message(buffer);
            }
        }

        /* MORE! THEMES! NOW!
        >NORMAL THEMES<
        
        0 - Basic Light
        1 - Basic Dark
        2 - HBChat Legacy

        3 - Glacier Plus (formerly Ivory Plus)
        4 - Velvet Plus
        5 - Slate Plus
        6 - Pearl Plus
        7 - Emerald Plus

        >REVERSI-APPLICABLE THEMES<

        8 - Lemon Soda
        9 - Cinnamon
        10 - Tropical
        11 - Glacier
        12 - Eyestrain Deluxe

        >USER-SUBMISSION THEMES<
        13 - Bread Guy's Theme (Bread Guy)
        14 - hacker torn tyehme. (cool guy)
        15 - P3 Phosphor (waydoing)
		16 - Slowpoke Pink (Slow)
        17 - Sky (DJ)
        18 - GB Green (Slow)
        19 - Homebrew Blue (Slow)

        + theme reversing with X cause you asked for it
        */

        if (hidKeysDown() & KEY_L) {
            if (darkmode > 0) {
                darkmode -= 1;
            }
            else {
                darkmode = themecount;
            }
        } /* Cycle through themes, counting up */

        if (hidKeysDown() & KEY_R) {
            if (darkmode < themecount) {
                darkmode += 1;
            }
            else {
                darkmode = 0;
            }
        } /* Cycle through themes, counting down */

        if (hidKeysDown() & KEY_X) {
            reversi = !reversi;
        } /* it keeps updating so this is all i got */

        if (darkmode == 0) {
            textcolor = C2D_Color32(0, 0, 0, 200);
            themecolor = C2D_Color32(255, 255, 255, 255);
            textcolorb = C2D_Color32(0, 0, 0, 200);
            themecolorb = C2D_Color32(255, 255, 255, 255);
            logocolor = C2D_Color32(0, 0, 100, 200);
            strcpy(currenttheme, "Basic Light");
        } /* Basic Light */

        if (darkmode == 1) {
            textcolor = C2D_Color32(255, 255, 255, 200);
            themecolor = C2D_Color32(0, 0, 0, 255);
            textcolorb = C2D_Color32(255, 255, 255, 200);
            themecolorb = C2D_Color32(0, 0, 0, 255);
            logocolor = C2D_Color32(0, 0, 100, 200);
            strcpy(currenttheme, "Basic Dark");
        } /* Basic Dark */

        if (darkmode == 2) {
            textcolor = C2D_Color32(0, 0, 0, 200);
            themecolor = C2D_Color32(0, 0, 255, 255);
            textcolorb = C2D_Color32(0, 0, 0, 200);
            themecolorb = C2D_Color32(0, 0, 255, 255);
            logocolor = C2D_Color32(0, 0, 0, 255);
            strcpy(currenttheme, "HBChat Legacy");
        } /* HBChat Legacy */

        if (darkmode == 3) {
            textcolor = C2D_Color32(106, 142, 174, 200);
            themecolor = C2D_Color32(209, 250, 255, 255);
            textcolorb = C2D_Color32(106, 142, 174, 200);
            themecolorb = C2D_Color32(209, 250, 255, 255);
            logocolor = C2D_Color32(54, 38, 167, 200);
            strcpy(currenttheme, "Glacier Plus");
        } /* Glacier Plus */

        if (darkmode == 4) {
            textcolor = C2D_Color32(157, 99, 129, 200);
            themecolor = C2D_Color32(28, 2, 33, 255);
            textcolorb = C2D_Color32(157, 99, 129, 200);
            themecolorb = C2D_Color32(28, 2, 33, 255);
            logocolor = C2D_Color32(255, 234, 238, 200);
            strcpy(currenttheme, "Velvet Plus");
        } /* Velvet Plus */

        if (darkmode == 5) {
            textcolor = C2D_Color32(199, 255, 237, 200);
            themecolor = C2D_Color32(89, 101, 111, 255);
            textcolorb = C2D_Color32(199, 255, 237, 200);
            themecolorb = C2D_Color32(89, 101, 111, 255);
            logocolor = C2D_Color32(255, 159, 28, 200);
            strcpy(currenttheme, "Slate Plus");
        } /* Slate Plus */

        if (darkmode == 6) {
            textcolor = C2D_Color32(255, 56, 100, 200);
            themecolor = C2D_Color32(245, 215, 217, 255);
            textcolorb = C2D_Color32(255, 56, 100, 200);
            themecolorb = C2D_Color32(245, 215, 217, 255);
            logocolor = C2D_Color32(72, 172, 240, 200);
            strcpy(currenttheme, "Pearl Plus");
        } /* Pearl Plus */

        if (darkmode == 7) {
            textcolor = C2D_Color32(189, 228, 168, 200);
            themecolor = C2D_Color32(15, 82, 87, 255);
            textcolorb = C2D_Color32(189, 228, 168, 200);
            themecolorb = C2D_Color32(15, 82, 87, 255);
            logocolor = C2D_Color32(234, 53, 70, 200);
            strcpy(currenttheme, "Emerald Plus");
        } /* Emerald Plus */

        if (darkmode == 8) {
            if (!reversi) {
                textcolor = C2D_Color32(230, 57, 70, 200);
                themecolor = C2D_Color32(254, 240, 204, 255);
                textcolorb = C2D_Color32(16, 255, 203, 200);
                themecolorb = C2D_Color32(40, 196, 200, 255);
                logocolor = C2D_Color32(253, 26, 109, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(16, 255, 203, 200);
                themecolor = C2D_Color32(40, 196, 200, 255);
                textcolorb = C2D_Color32(230, 57, 70, 200);
                themecolorb = C2D_Color32(254, 240, 204, 255);
                logocolor = C2D_Color32(253, 26, 109, 200);
            }
            strcpy(currenttheme, "Lemon Soda");
        } /* Lemon Soda */

        if (darkmode == 9) {
            if (!reversi) {
                textcolor = C2D_Color32(235, 212, 203, 200);
                themecolor = C2D_Color32(182, 70, 95, 255);
                textcolorb = C2D_Color32(218, 159, 147, 200);
                themecolorb = C2D_Color32(137, 6, 32, 255);
                logocolor = C2D_Color32(44, 7, 3, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(218, 159, 147, 200);
                themecolor = C2D_Color32(137, 6, 32, 255);
                textcolorb = C2D_Color32(235, 212, 203, 200);
                themecolorb = C2D_Color32(182, 70, 95, 255);
                logocolor = C2D_Color32(44, 7, 3, 200);
            }
            strcpy(currenttheme, "Cinnamon");
        } /* Cinnamon */

        if (darkmode == 10) {
            if (!reversi) {
                textcolor = C2D_Color32(255, 217, 125, 200);
                themecolor = C2D_Color32(255, 108, 74, 255);
                textcolorb = C2D_Color32(170, 246, 131, 200);
                themecolorb = C2D_Color32(59, 201, 123, 255);
                logocolor = C2D_Color32(255, 255, 255, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(170, 246, 131, 200);
                themecolor = C2D_Color32(59, 201, 123, 255);
                textcolorb = C2D_Color32(255, 217, 125, 200);
                themecolorb = C2D_Color32(255, 108, 74, 255);
                logocolor = C2D_Color32(255, 255, 255, 200);
            }
            strcpy(currenttheme, "Tropical");
        } /* Tropical */

        if (darkmode == 11) {
            if (!reversi) {
                textcolor = C2D_Color32(19, 64, 116, 200);
                themecolor = C2D_Color32(238, 244, 237, 255);
                textcolorb = C2D_Color32(141, 169, 196, 200);
                themecolorb = C2D_Color32(11, 37, 69, 255);
                logocolor = C2D_Color32(19, 49, 92, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(141, 169, 196, 200);
                themecolor = C2D_Color32(11, 37, 69, 255);
                textcolorb = C2D_Color32(19, 64, 116, 200);
                themecolorb = C2D_Color32(238, 244, 237, 255);
                logocolor = C2D_Color32(19, 49, 92, 200);
            }
            strcpy(currenttheme, "Iceberg");
        } /* Iceberg */

        if (darkmode == 12) {
            if (!reversi) {
                textcolor = C2D_Color32(0, 255, 255, 200);
                themecolor = C2D_Color32(255, 0, 0, 255);
                textcolorb = C2D_Color32(255, 0, 255, 200);
                themecolorb = C2D_Color32(0, 255, 0, 255);
                logocolor = C2D_Color32(0, 0, 0, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(255, 0, 255, 200);
                themecolor = C2D_Color32(0, 255, 0, 255);
                textcolorb = C2D_Color32(0, 255, 255, 200);
                themecolorb = C2D_Color32(255, 0, 0, 255);
                logocolor = C2D_Color32(0, 0, 0, 200);
            }
            strcpy(currenttheme, "Eyestrain Deluxe");
        } /* Eyestrain Deluxe */

        // user reqs from here on out! thanks for the submissions!

        if (darkmode == 13) {
            textcolor = C2D_Color32(102, 216, 220, 200);
            themecolor = C2D_Color32(14, 53, 73, 255);
            textcolorb = C2D_Color32(102, 216, 220, 200);
            themecolorb = C2D_Color32(14, 53, 73, 255);
            logocolor = C2D_Color32(107, 214, 118, 200);
            strcpy(currenttheme, "Bread Guy's Theme");
        } /* Bread Guy's Theme */

        if (darkmode == 14) {
            textcolor = C2D_Color32(17, 255, 0, 200);
            themecolor = C2D_Color32(0, 0, 0, 255);
            textcolorb = C2D_Color32(17, 255, 0, 200);
            themecolorb = C2D_Color32(0, 0, 0, 255);
            logocolor = C2D_Color32(17, 255, 0, 200);
            strcpy(currenttheme, "hacker torn tyehme.");
        } /* hacker torn tyehme. */

        if (darkmode == 15) {
			textcolor = C2D_Color32(255, 176, 0, 255);
			themecolor = C2D_Color32(28, 28, 28, 255);
			textcolorb = C2D_Color32(255, 176, 0, 255);
			themecolorb = C2D_Color32(28, 28, 28, 255);
			logocolor = C2D_Color32(255, 176, 0, 255);
			strcpy(currenttheme, "P3 Phosphor");
		} /* P3 Phosphor */
		
		if (darkmode == 16) {
            textcolor = C2D_Color32(9, 44, 84, 200);
            themecolor = C2D_Color32(245, 120, 198, 255);
            textcolorb = C2D_Color32(9, 44, 84, 200);
            themecolorb = C2D_Color32(245, 120, 198, 255);
            logocolor = C2D_Color32(255, 237, 167, 200);
            strcpy(currenttheme, "Slowpoke Pink");
        } /* Slowpoke Pink */

        if (darkmode == 17) {
            textcolor = C2D_Color32(241, 241, 241, 200);
            themecolor = C2D_Color32(99, 183, 219, 255);
            textcolorb = C2D_Color32(241, 241, 241, 200);
            themecolorb = C2D_Color32(99, 183, 219, 255);
            logocolor = C2D_Color32(192, 255, 255, 200);
            strcpy(currenttheme, "Sky");
        } /* Sky */

        if (darkmode == 18) {
            if (!reversi) {
                textcolor = C2D_Color32(32, 91, 92, 200);
                themecolor = C2D_Color32(159, 228, 97, 255);
                textcolorb = C2D_Color32(159, 228, 97, 200);
                themecolorb = C2D_Color32(32, 91, 92, 255);
                logocolor = C2D_Color32(89, 145, 50, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(159, 228, 97, 200);
                themecolor = C2D_Color32(32, 91, 92, 255);
                textcolorb = C2D_Color32(32, 91, 92, 200);
                themecolorb = C2D_Color32(159, 228, 97, 255);
                logocolor = C2D_Color32(89, 145, 50, 200);
            }
            strcpy(currenttheme, "GB Green");
        } /* GB Green */

        if (darkmode == 19) {
            if (!reversi) {
                textcolor = C2D_Color32(203, 231, 255, 200);
                themecolor = C2D_Color32(0, 132, 255, 255);
                textcolorb = C2D_Color32(255, 255, 255, 200);
                themecolorb = C2D_Color32(68, 175, 255, 255);
                logocolor = C2D_Color32(98, 219, 252, 200);
            }
            if (reversi) {
                textcolor = C2D_Color32(255, 255, 255, 200);
                themecolor = C2D_Color32(68, 175, 255, 255);
                textcolorb = C2D_Color32(203, 231, 255, 200);
                themecolorb = C2D_Color32(0, 132, 255, 255);
                logocolor = C2D_Color32(98, 219, 252, 200);
            }

            strcpy(currenttheme, "Homebrew Blue");
        } /* Homebrew Blue */

        if (hidKeysDown() & KEY_Y) {
            if (rulesvisible < 3) {
                if (rulesvisible == 0) { //only roll for silly on the first press, not on subsequent ones
                    silly = rand() % 10; // all odds must be in multiples of 10%
                }
                rulesvisible += 1;
            }
            else {
                rulesvisible = 0;
            }
        } /* You wanna see the rules, huh? Huh? Punk? */ // moved to int so there can be rule pages


        if (scene == 10) {
            if (hidKeysDown() & KEY_A) {
                char msg[356];
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 356); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
                swkbdSetHintText(&swkbd, "Say something...");

                SwkbdButton button = swkbdInputText(&swkbd, msg, sizeof(msg));

                char sender[460];
                sprintf(sender, "{\"cmd\":\"CHAT\", \"content\":\"%s\", \"username\":\"%s\", \"password\":\"%s\"}", msg, username, password);
                http_post("http://104.236.25.60:3072/api", sender);
            }
            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                char msg[356];
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 356); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
                swkbdSetHintText(&swkbd, "Say something...");

                SwkbdButton button = swkbdInputText(&swkbd, msg, sizeof(msg));

                char sender[460];
                sprintf(sender, "{\"cmd\":\"CHAT\", \"content\":\"%s\", \"username\":\"%s\", \"password\":\"%s\"}", msg, username, password);
                http_post("http://104.236.25.60:3072/api", sender);
            }
        }





        if (scene == 1) {
            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                scene = 2;
            }
            if (isSpriteTapped(&button2, 0.8f, 0.8f)) {
                scene = 3;
            }
        }

        if (scene == 2) {
            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
                swkbdSetHintText(&swkbd, "Enter a username...");

                SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username)); 
            }

            if (isSpriteTapped(&button2, 0.8f, 0.8f)) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21); // made the username limit even longer because 21 ha ha funny
                swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY); // i added a cancel button, yippe
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0); // top ten stolen and EXPANDED from aurorachat
                swkbdSetHintText(&swkbd, "Enter a password...");
                if (password) {
                    swkbdSetInitialText(&swkbd, password);
                }
                if (!showpassword) {
                    swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
                }


                SwkbdButton button = swkbdInputText(&swkbd, password, sizeof(password)); 
            }
            if (isSpriteTapped(&button3, 0.8f, 0.8f)) {
                scene = 4;
            }

            if (hidKeysDown() & KEY_B) {
                scene = 1;
            }
        }

        if (scene == 4) {
            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                if (!showpassjustpressed) {
                    showpassword = !showpassword;
                    showpassjustpressed = true;
                }
            } else {
                showpassjustpressed = false;
            }
            if (isSpriteTapped(&button2, 0.8f, 0.8f)) {
                char signuppostbody[128];
                sprintf(signuppostbody, "{\"cmd\":\"MAKEACC\", \"username\":\"%s\", \"password\":\"%s\"}", username, password);
                http_post("http://104.236.25.60:3072/api", signuppostbody);
                sprintf(buftext, "%s", buf);
                if (strstr(buftext, "USR_CREATED") != 0) {
                    scene = 1;
                }
                if (strcmp(buftext, "(null)") == 0) {
                    scene = 61;
                }
                if (strstr(buftext, "USR_IN_USE") != 0) {
                    scene = 67;
                }
            }
        }

        if (scene == 3) {

            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21);
                swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
                swkbdSetHintText(&swkbd, "Enter your username...");

                SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username));
            }

            if (isSpriteTapped(&button2, 0.8f, 0.8f)) {
                SwkbdState swkbd;
                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21);
                swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
                swkbdSetHintText(&swkbd, "Enter your password...");
                if (password) {
                    swkbdSetInitialText(&swkbd, password);
                }
                if (!showpassword) { // I implemented this but I refuse to use it because IM EVIL.
                    swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
                }


                SwkbdButton button = swkbdInputText(&swkbd, password, sizeof(password)); 
            }
            if (isSpriteTapped(&button3, 0.8f, 0.8f)) {
                scene = 5;
            }

            if (hidKeysDown() & KEY_B) {
                scene = 1;
            }
        }

        if (scene == 5) {
            if (isSpriteTapped(&button, 0.8f, 0.8f)) {
                if (!showpassjustpressed) {
                    showpassword = !showpassword;
                    showpassjustpressed = true;
                }
            } else {
                showpassjustpressed = false;
            }
            if (isSpriteTapped(&button2, 0.8f, 0.8f)) {
                char signuppostbody[128];
                sprintf(signuppostbody, "{\"cmd\":\"LOGINACC\", \"username\":\"%s\", \"password\":\"%s\"}", username, password);
                http_post("http://104.236.25.60:3072/api", signuppostbody);
                sprintf(buftext, "%s", buf);
                if (strstr(buftext, "LOGIN_OK") != 0) {
                    scene = 10;
                    char welcome[100];
                    sprintf(welcome, "Welcome to aurorachat, %s!\n", username);
                    append_message(welcome);
                }
                if (strcmp(buftext, "(null)") == 0) {
                    scene = 61;
                }
                if (strstr(buftext, "LOGIN_WRONG_PASS") != 0) {
                    scene = 68;
                }
                if (strstr(buftext, "LOGIN_FAKE_ACC") != 0) {
                    scene = 68;
                }
            }
        }








        /*
        
            Begin the frame, nothing actually starts drawing when you do this, it is still required.
        
        */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);


        /*
        
            Clear the screens with a color. Think of it like sliding a blank piece of paper on top of one with drawings on it.
        
        */
        C2D_TargetClear(top, themecolor);
        C2D_TargetClear(bottom, themecolorb);

        C2D_SceneBegin(top);

        DrawText("Aurorachat", 290.0f, 5.0f, 0, 0.7f, 0.7f, logocolor, true);
        DrawText("version 4.3", 321.0f, 20.0f, 0, 0.5f, 0.5f, logocolor, true);


        if (scene == 10) {
            DrawText(chat, 10.0f, chatscroll, 0, 0.5f, 0.5f, textcolor, true);

            C2D_SceneBegin(bottom);
            if (rulesvisible == 0) {
                C2D_SpriteSetPos(&button, 200.0f, 200.0f);
                C2D_SpriteSetScale(&button, 0.4f, 0.4f);
                C2D_DrawSprite(&button);

                DrawText("Send", 235.0f, 215.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);

                DrawText(": Send Message", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                DrawText(", : Scroll Chat", 10.0f, 30.0f, 0, 0.5f, 0.5f, textcolorb, true);
                DrawText(": View Rules", 10.0f, 50.0f, 0, 0.5f, 0.5f, textcolorb, true);
                DrawText(", : Toggle Theme", 10.0f, 70.0f, 0, 0.5f, 0.5f, textcolorb, true);
                DrawText(": Reverse Theme", 10.0f, 90.0f, 0, 0.5f, 0.5f, textcolorb, true);
                char themealert[40];
                sprintf(themealert, "Current Theme:\n%s", currenttheme);
                DrawText(themealert, 10.0f, 200.0f, 0, 0.5f, 0.5f, textcolorb, true);
            }

            if (rulesvisible == 1) {
                if (silly > 1) { //as in, values of 0, 1, 2, or 3 will trigger the silly rules
                    DrawText(": Next Page", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("1. No racist, sexist, homophobic, or other", 10.0f, 30.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("prejudiced language or behavior, whether it's", 10.0f, 40.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("aimed at another user or not.", 10.0f, 50.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("2. No asking for or sharing personal info of", 10.0f, 80.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("yourself or anyone else. This includes name,", 10.0f, 90.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("age, gender, location, phone number, email", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("address, or any other personally identifiable", 10.0f, 110.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("information. Doxxing (or the threat of doing so)", 10.0f, 120.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("is grounds for a ban.", 10.0f, 130.0f, 0, 0.5f, 0.5f, textcolorb, true);
                }
                else {
                    DrawText(": Next Page", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("1. No saying oatmeal", 10.0f, 25.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("2. Eat burger or else", 10.0f, 40.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("2. No numbers containing 6 or 7", 10.0f, 55.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("3. 100% accurate grammar", 10.0f, 70.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("4. Crow like a rooster when you log in", 10.0f, 85.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("5. Ganondorf Suavemente gif", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("6. no more mr nice guy", 10.0f, 115.0f, 0, 0.5f, 0.5f, textcolorb, true);

                }
            }

            if (rulesvisible == 2) {
                if (silly > 1) {
                    DrawText(": Next Page", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("3. No overly violent or sexual behavior or", 10.0f, 30.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("language, including threats of violence or harm", 10.0f, 40.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("of ANY kind. This includes threats or", 10.0f, 50.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("discussion of harming yourself.", 10.0f, 60, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("4. No political discussion. Usernames of", 10.0f, 90.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("political figures are allowed (with some", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("exceptions), but any language that could incite", 10.0f, 110.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("arguments may get you banned.", 10.0f, 120.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("5. No impersonation of developers,", 10.0f, 150.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("moderators, admin, or any other Aurorachat", 10.0f, 160.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("staff. Additionally, ANY impersonation for the", 10.0f, 170.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("sake of harrassing another user is not allowed.", 10.0f, 180.0f, 0, 0.5f, 0.5f, textcolorb, true);
                }
                else {
                    DrawText(": Next Page", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("7. no hackertron", 10.0f, 25.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("8. eat burger", 10.0f, 40.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("9. Pigs must fly", 10.0f, 55.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("10. No more fortnite", 10.0f, 70.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("11. No battery", 10.0f, 85.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("12. No cats allowed", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("13. No dogs allowed either", 10.0f, 115.0f, 0, 0.5f, 0.5f, textcolorb, true);

                }
            }

            if (rulesvisible == 3) {
                if (silly > 1) {
                    DrawText(": Close Rules", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("6. No spamming.", 10.0f, 30.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("7. No hunting.", 10.0f, 45.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("8. No hackertron", 10.0f, 60.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("Friend code sharing is allowed, but please do", 10.0f, 90.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("not harrass or pressure other users for their", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("friend codes.", 10.0f, 110.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("Want access to the rest of Unitendo? Join our", 10.0f, 150.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("official Discord server here:", 10.0f, 160.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("discord.gg/dCSgz7KERv", 10.0f, 175.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("We are not accepting ban appeals at this time.", 10.0f, 220.0f, 0, 0.5f, 0.5f, textcolorb, true);
                }
                else {
                    DrawText(": Hide Rules", 10.0f, 10.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("Alright if I was an animal what animal do you", 10.0f, 30.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("think I would be? SERIOUS ANSWERS ONLY.", 10.0f, 40.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("A rooster. A rat. A rat. A rat. A rat. You’d be", 10.0f, 50.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("a rat. Jerma you’re a rat. You’d be a rat. I", 10.0f, 60.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("think you’d be a rat. I think i’d be a wolf.", 10.0f, 70.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("I think so too. I would be a wolf lion hybrid", 10.0f, 80.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("mix. King of the Junjile— Junjile, but still", 10.0f, 90.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("social and with it and ferocious.", 10.0f, 100.0f, 0, 0.5f, 0.5f, textcolorb, true);
                    DrawText("No ban appeals. Why can't you listen?", 10.0f, 220.0f, 0, 0.5f, 0.5f, textcolorb, true);
                }
            }

            if (hidKeysHeld() & KEY_UP) {
                    chatscroll = chatscroll + 5;
            }
            if (hidKeysHeld() & KEY_DOWN) {
                    chatscroll = chatscroll - 5;
            }
        }

        if (outdated) {
            DrawText("Outdated version. Update in Universal-Updater", 0.0f, 225.0f, 0, 0.5f, 0.5f, textcolor, false);
        }

        







        C2D_SceneBegin(bottom);

        if (scene == 1) {
            C2D_SceneBegin(top);
            DrawText("Sign Up or Sign In", 150.0f, 70.0f, 0, 0.5f, 0.5f, textcolor, true);
            C2D_SceneBegin(bottom);
            C2D_SpriteSetPos(&button, 75.0f, 50.0f);
            C2D_SpriteSetScale(&button, 0.4f, 0.4f);
            C2D_DrawSprite(&button);
            DrawText("Sign Up", 130.0f, 62.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button2, 75.0f, 130.0f);
            C2D_SpriteSetScale(&button2, 0.4f, 0.4f);
            C2D_DrawSprite(&button2);
            DrawText("Sign In", 132.0f, 143.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
        }

        if (scene == 2) {
            C2D_SceneBegin(top);
            DrawText("Sign Up", 175.0f, 70.0f, 0, 0.5f, 0.5f, textcolor, true);
            DrawText("Enter a username and password.", 100.0f, 90.0f, 0, 0.5f, 0.5f, textcolor, true);
            C2D_SceneBegin(bottom);
            C2D_SpriteSetPos(&button, 75.0f, 50.0f);
            C2D_SpriteSetScale(&button, 0.4f, 0.4f);
            C2D_DrawSprite(&button);
            DrawText("Username", 120.0f, 62.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button2, 75.0f, 130.0f);
            C2D_SpriteSetScale(&button2, 0.4f, 0.4f);
            C2D_DrawSprite(&button2);
            DrawText("Password", 120.0f, 143.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button3, 165.0f, 190.0f);
            C2D_SpriteSetScale(&button3, 0.4f, 0.4f);
            C2D_DrawSprite(&button3);
            DrawText("Continue", 210.0f, 205.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
        }

        if (scene == 3) {
            C2D_SceneBegin(top);
            DrawText("Sign In", 175.0f, 70.0f, 0, 0.5f, 0.5f, textcolor, true);
            DrawText("Enter your username and password.", 90.0f, 90.0f, 0, 0.5f, 0.5f, textcolor, true);
            C2D_SceneBegin(bottom);
            C2D_SpriteSetPos(&button, 75.0f, 50.0f);
            C2D_SpriteSetScale(&button, 0.4f, 0.4f);
            C2D_DrawSprite(&button);
            DrawText("Username", 120.0f, 62.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button2, 75.0f, 130.0f);
            C2D_SpriteSetScale(&button2, 0.4f, 0.4f);
            C2D_DrawSprite(&button2);
            DrawText("Password", 120.0f, 143.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button3, 165.0f, 190.0f);
            C2D_SpriteSetScale(&button3, 0.4f, 0.4f);
            C2D_DrawSprite(&button3);
            DrawText("Continue", 210.0f, 205.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
        }

        if (scene == 4) {
            C2D_SceneBegin(top);
            DrawText("Confirm", 175.0f, 70.0f, 0, 0.5f, 0.5f, textcolor, true);
            char confirmation[256];
            if (showpassword) {
                sprintf(confirmation, "Username: %s\nPassword: %s", username, password);
            }
            if (!showpassword) {
                sprintf(confirmation, "Username: %s\nPassword: (hidden)", username);
            }
            DrawText(confirmation, 140.0f, 90.0f, 0, 0.5f, 0.5f, textcolor, true);
            C2D_SceneBegin(bottom);
            C2D_SpriteSetPos(&button2, 75.0f, 50.0f);
            C2D_SpriteSetScale(&button2, 0.4f, 0.4f);
            C2D_DrawSprite(&button2);
            DrawText("Register", 130.0f, 62.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button, 75.0f, 130.0f);
            C2D_SpriteSetScale(&button, 0.4f, 0.4f);
            C2D_DrawSprite(&button);
            DrawText("Show Password", 100.0f, 143.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
        }

        if (scene == 5) {
            C2D_SceneBegin(top);
            DrawText("Confirm", 175.0f, 70.0f, 0, 0.5f, 0.5f, textcolor, true);
            char confirmation[256];
            if (showpassword) {
                sprintf(confirmation, "Username: %s\nPassword: %s", username, password);
            }
            if (!showpassword) {
                sprintf(confirmation, "Username: %s\nPassword: (hidden)", username);
            }
            DrawText(confirmation, 140.0f, 90.0f, 0, 0.5f, 0.5f, textcolor, true);
            C2D_SceneBegin(bottom);
            C2D_SpriteSetPos(&button2, 75.0f, 50.0f);
            C2D_SpriteSetScale(&button2, 0.4f, 0.4f);
            C2D_DrawSprite(&button2);
            DrawText("Log In", 130.0f, 62.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
            C2D_SpriteSetPos(&button, 75.0f, 130.0f);
            C2D_SpriteSetScale(&button, 0.4f, 0.4f);
            C2D_DrawSprite(&button);
            DrawText("Show Password", 100.0f, 143.0f, 0, 0.6f, 0.6f, C2D_Color32(0, 0, 0, 100), true);
        }


        if (scene == 61) {
            C2D_SceneBegin(top);
            DrawText("API is down. Restart to try again.", 100.0f, 100.0f, 0, 0.5f, 0.5f, textcolor, false);
            C2D_SceneBegin(bottom);
        }

        if (scene == 67) {
            C2D_SceneBegin(top);
            char debugger[280];
            DrawText("Account already exists.", 140.0f, 100.0f, 0, 0.5f, 0.5f, textcolor, false);
            DrawText("Press B to go back to the account screen.", 65.0f, 130.0f, 0, 0.5f, 0.5f, textcolor, true);
            sprintf(debugger, "Debug: %s", buftext);
            DrawText(debugger, 0.0f, 225.0f, 0, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 100), false);

            if (hidKeysDown() & KEY_B) {
                scene = 1;
            }
        }

        if (scene == 68) {
            C2D_SceneBegin(top);
            char debugger[280];
            DrawText("Invalid Credentials", 140.0f, 100.0f, 0, 0.5f, 0.5f, textcolor, false);
            DrawText("Press B to go back to the account screen.", 65.0f, 130.0f, 0, 0.5f, 0.5f, textcolor, true);
            sprintf(debugger, "Debug: %s", buftext);
            DrawText(debugger, 0.0f, 225.0f, 0, 0.5f, 0.5f, textcolor, false);

            if (hidKeysDown() & KEY_B) {
                scene = 1;
            }
        }







        C3D_FrameEnd(0);

    }


    quit = true;

//    if (sfx1) linearFree(sfx1);

    ndspSetCallback(NULL, NULL);
    ndspExit();

    httpcExit();

    C2D_SpriteSheetFree(spriteSheet);

    C2D_TextBufDelete(sbuffer);
    C2D_Fini();
    C3D_Fini();

    fsExit();
    romfsExit();
    gfxExit();
    return 0;


}
