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

#include <citro2d.h>
#include <3ds/applets/swkbd.h>
#include <3ds/services/sslc.h> // ensure header available

// --- Config ---
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8961

// --- Globals ---
int sockfd = -1;
sslcContext tlsCtx;

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

// Utility: print Result in hex
static void printRes(const char *prefix, Result r) {
    printf("%s: 0x%08lx\n", prefix, (unsigned long)r);
}

// TLS helpers
Result tls_init_service() {
    return sslcInit(0);
}

void tls_exit_service(void) {
    sslcExit();
}

Result tls_connect_socket(int sock, sslcContext *ctx) {
    Result ret;
    int internal_retval = 0;
    u32 out = 0;

    // Create context: Disable verification for development (SSLCOPT_DisableVerify)
    ret = sslcCreateContext(ctx, sock, SSLCOPT_DisableVerify, 0);
    if (R_FAILED(ret)) {
        printRes("sslcCreateContext failed", ret);
        return ret;
    }

    // Start TLS handshake: note sslcStartConnection takes 3 args
    ret = sslcStartConnection(ctx, &internal_retval, &out);
    if (R_FAILED(ret)) {
        printRes("sslcStartConnection failed", ret);
        return ret;
    }

    // At this point TLS handshake succeeded (or has internal retval; we assume success)
    return 0;
}

Result tls_send(sslcContext *ctx, const char *buf, size_t len) {
    // sslcWrite returns Result
    return sslcWrite(ctx, buf, len);
}

ssize_t tls_recv_text(sslcContext *ctx, char *buf, size_t bufsize) {
    // sslcRead returns Result; it writes up to bufsize bytes.
    // For text chat we assume data is textual and null-terminate after read.
    Result ret = sslcRead(ctx, buf, bufsize - 1, false);
    if (R_FAILED(ret)) return -1;

    // Many sslc examples return data as a null-terminated string; if not, we fallback to strlen.
    buf[bufsize - 1] = '\0';
    size_t len = strlen(buf);
    return (ssize_t)len;
}

void DrawText(char *text, float x, float y, int z, float scaleX, float scaleY, u32 color, bool wordwrap) {
    C2D_TextBufClear(sbuffer);
    C2D_TextParse(&stext, sbuffer, text);
    C2D_TextOptimize(&stext);

    if (!wordwrap) {
        C2D_DrawText(&stext, C2D_WithColor, x, y, z, scaleX, scaleY, color);
    } else {
        C2D_DrawText(&stext, C2D_WithColor | C2D_WordWrap, x, y, z, scaleX, scaleY, color, 290.0f);
    }
}

int main(int argc, char **argv) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    sbuffer = C2D_TextBufNew(4096);
    chatbuffer = C2D_TextBufNew(8192);
    C2D_TextParse(&chat, chatbuffer, chatstring);
    C2D_TextOptimize(&chat);

    // --- Networking init ---
    u32 *soc_buffer = memalign(0x1000, 0x100000);
    if (!soc_buffer) {
        printf("soc memalign failed\n");
        return 1;
    }

    if (socInit(soc_buffer, 0x100000) != 0) {
        printf("socInit failed\n");
        return 1;
    }

    // Initialize SSL service (pass process handle)
    Result r = tls_init_service();
    if (R_FAILED(r)) {
        printRes("sslcInit failed", r);
        socExit();
        return 1;
    }

    // Create and connect TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket() failed\n");
        tls_exit_service();
        socExit();
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) != 0) {
        printf("TCP connect failed\n");
        closesocket(sockfd);
        tls_exit_service();
        socExit();
        return 1;
    }

    // Wrap the TCP socket with SSL/TLS
    r = tls_connect_socket(sockfd, &tlsCtx);
    if (R_FAILED(r)) {
        printRes("tls_connect_socket failed", r);
        closesocket(sockfd);
        tls_exit_service();
        socExit();
        return 1;
    }

    printf("Connected to TLS server %s:%d\n", SERVER_IP, SERVER_PORT);

    char username[21]; //swkbd registers name, but client doesnt, now it does
    // okay
    // was i the one who made that comment???

    u32 textcolor;
    u32 themecolor;

    // --- Main loop (UI + chat) ---
    char inbuf[512];
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        if (hidKeysDown() & KEY_A) {
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
            SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username));
            if (button == SWKBD_BUTTON_CONFIRM) {
                snprintf(usernameholder, sizeof(usernameholder), "Username: %s", username);
            }
        }

        if (hidKeysDown() & KEY_B) {
            char message[256];
            char sendbuf[512];

            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 128);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
            SwkbdButton button = swkbdInputText(&swkbd, message, sizeof(message));
            if (button == SWKBD_BUTTON_CONFIRM) {
                snprintf(sendbuf, sizeof(sendbuf), "<%s>: %s", username[0] ? username : "guest", message);
                Result mr = tls_send(&tlsCtx, sendbuf, strlen(sendbuf));
                if (R_FAILED(mr)) {
                    printRes("sslcWrite failed", mr);
                }
            }
        }

        // Non-blocking poll for incoming TLS data using select on the raw socket
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        if (select(sockfd + 1, &readfds, NULL, NULL, &tv) > 0) {
            // Data likely available; read via sslcRead (decrypted)
            memset(inbuf, 0, sizeof(inbuf));
            ssize_t got = tls_recv_text(&tlsCtx, inbuf, sizeof(inbuf));
            if (got > 0) {
                char tmp[6000];
                snprintf(tmp, sizeof(tmp), "%s\n%s", chatstring, inbuf);
                strncpy(chatstring, tmp, sizeof(chatstring));
                chatscroll -= 15;

                const char *parseResult = C2D_TextParse(&chat, chatbuffer, chatstring);
                if (parseResult != NULL && *parseResult != '\0') {
                    chatbuffer = C2D_TextBufResize(chatbuffer, 16384);
                    if (chatbuffer) {
                        C2D_TextBufClear(chatbuffer);
                        C2D_TextParse(&chat, chatbuffer, chatstring);
                    }
                }
                C2D_TextOptimize(&chat);
            }
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
            if (theme > 8) {
                theme = 1;
            }
        }
        if (hidKeysDown() & KEY_DDOWN) {
            if (!switched) {
                theme--;
                switched = true;
            }
            if (theme < 1) {
                theme = 8;
            }
        }

        switched = false;

        // Rendering
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
            themename = "True Dark Mode";
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
        


        if (scene == 1) {
            DrawText("aurorachat", 260.0f, 0.0f, 0, 1.0f, 1.0f, textcolor, false);
            

            sprintf(usernameholder, "%s %s", "Username:", username);

            DrawText(usernameholder, 0.0f, 200.0f, 0, 1.0f, 1.0f, textcolor, false);



            
            DrawText("v0.0.3.2", 335.0f, 25.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            
            




            DrawText("A: Change Username\nB: Send Message\nL: Rules\nD-PAD: Change Theme", 0.0f, 0.0f, 0, 0.6f, 0.6f, textcolor, false);
            
            


            DrawText(themename, 170.0f, 0.0f, 0, 0.4f, 0.4f, textcolor, false);

        }


        if (scene == 2) {
            DrawText("(Press X to Go Back)\n\nRule 1: No Spamming\n\nRule 2: No Swearing\n\nRule 3: No Impersonating\n\nRule 4: No Politics\n\nAll of these could result in a ban.", 0.0f, 0.0f, 0, 0.5f, 0.6f, textcolor, false);
        }

        C2D_TargetClear(bottom, themecolor);
        
        C2D_SceneBegin(bottom);

        DrawText(chatstring, 0.0f, chatscroll, 0, 0.5f, 0.5f, textcolor, true);
        C3D_FrameEnd(0);
    }




    closesocket(sockfd);
    tls_exit_service();
    socExit();
    gfxExit();
    return 0;
}
