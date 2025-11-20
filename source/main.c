// aurorachat
// Author: mii-man
// Description: A chatting application for 3DS

// yes I did copy basically the entire hbchat codebase here and call it a day because ain't no way your original base was ever gonna work :sob:
// as mii man, yes my base would never work ;(

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <errno.h>

#include <citro2d.h>
#include <3ds/applets/swkbd.h>

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/x509.h"
#include "mbedtls/net_sockets.h"

// --- Config ---
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8961

// --- Globals ---
int sockfd = -1;

// mbedTLS contexts
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static bool tls_initialized = false;

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
    } else {
        C2D_DrawText(&stext, C2D_WithColor | C2D_WordWrap, x, y, z, scaleX, scaleY, color, 290.0f);
    }
}

// Print mbedTLS errors in a compact form
static void print_mbed_error(const char *prefix, int ret) {
    char errbuf[200];
    mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    printf("%s: -0x%04x (%s)\n", prefix, -ret, errbuf);
}

// ---------- mbedTLS socket wrappers ----------
// mbedTLS expects send/recv callbacks with this prototype:
// int (*f_send)(void *ctx, const unsigned char *buf, size_t len)
// int (*f_recv)(void *ctx, unsigned char *buf, size_t len)

static int tls_socket_send(void *ctx, const unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    ssize_t ret = send(fd, buf, len, 0);
    if (ret >= 0) return (int)ret;

    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_WANT_WRITE;
    if (errno == EINTR) return MBEDTLS_ERR_SSL_WANT_WRITE;
    // Map other socket errors to a generic mbedTLS network error
    return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int tls_socket_recv(void *ctx, unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    ssize_t ret = recv(fd, buf, len, 0);
    if (ret > 0) return (int)ret;
    if (ret == 0) return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY; // connection closed

    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_WANT_READ;
    if (errno == EINTR) return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_NET_RECV_FAILED;
}

// Initialize mbedTLS contexts and configure a client for TLS 1.3
static int tls_init_mbed(int fd) {
    int ret;
    const char *pers = "aurorachat_3ds";

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char *)pers, strlen(pers))) != 0) {
        print_mbed_error("mbedtls_ctr_drbg_seed failed", ret);
        return ret;
    }

    // Defaults: client, stream transport
    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        print_mbed_error("mbedtls_ssl_config_defaults failed", ret);
        return ret;
    }

    // For development/testing: do not verify server cert. Change to MBEDTLS_SSL_VERIFY_REQUIRED to enable.
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    // Apply the config
    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        print_mbed_error("mbedtls_ssl_setup failed", ret);
        return ret;
    }

    // Set SNI / hostname for certificate validation (if you enable verify later)
    mbedtls_ssl_set_hostname(&ssl, SERVER_IP); // can be host name if available

    // Set BIO callbacks to use our socket
    mbedtls_ssl_set_bio(&ssl, &fd, tls_socket_send, tls_socket_recv, NULL);

    tls_initialized = true;
    return 0;
}

static void tls_free_mbed(void) {
    if (!tls_initialized) return;

    mbedtls_ssl_close_notify(&ssl);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    tls_initialized = false;
}

// Wrapper to perform handshake (non-blocking-friendly)
static int tls_perform_handshake(void) {
    int ret;
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            // Caller should poll the socket; for simplicity we return WANT_READ/WRITE so caller can select.
            return ret;
        }
        print_mbed_error("mbedtls_ssl_handshake failed", ret);
        return ret;
    }
    return 0; // success
}

// Send via mbedTLS
static int tls_send_mbed(const char *buf, size_t len) {
    if (!tls_initialized) return -1;
    int ret;
    size_t written = 0;

    while (written < len) {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf + written, len - written);
        if (ret >= 0) {
            written += ret;
            continue;
        }
        if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ) {
            // Would block; caller may retry later.
            return 0; // indicate "no error, but nothing sent yet" to keep original code style
        }
        print_mbed_error("mbedtls_ssl_write failed", ret);
        return -1;
    }
    return (int)written;
}

// Receive text data (null-terminated), return bytes read or -1 on error
static ssize_t tls_recv_text_mbed(char *buf, size_t bufsize) {
    if (!tls_initialized) return -1;
    int ret;
    ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, bufsize - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        return (ssize_t)ret;
    }
    if (ret == 0) {
        // Connection closed cleanly
        return 0;
    }
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return -2; // indicate "try again"
    }
    // Other error
    print_mbed_error("mbedtls_ssl_read failed", ret);
    return -1;
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

    // Create and connect TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket() failed\n");
        socExit();
        return 1;
    }

    // Make socket non-blocking to interop nicely with mbedTLS
    uint32_t enable = 1;
    ioctl(sockfd, FIONBIO, &enable);

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Attempt connect (non-blocking)
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0 && errno != EINPROGRESS) {
        printf("TCP connect failed (errno=%d)\n", errno);
        closesocket(sockfd);
        socExit();
        return 1;
    }

    // Wait for connect to complete via select with timeout
    fd_set wfds;
    struct timeval tv;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (select(sockfd + 1, NULL, &wfds, NULL, &tv) <= 0) {
        printf("TCP connect/select timeout or error\n");
        closesocket(sockfd);
        socExit();
        return 1;
    }

    // Check for socket error
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        printf("connect error: %d\n", so_error);
        closesocket(sockfd);
        socExit();
        return 1;
    }

    // Initialize mbedTLS on this connected socket
    int ret = tls_init_mbed(sockfd);
    if (ret != 0) {
        printf("tls_init_mbed failed\n");
        closesocket(sockfd);
        tls_free_mbed();
        socExit();
        return 1;
    }

    // Perform handshake — this may return WANT_READ/WANT_WRITE initially because socket is non-blocking.
    while (true) {
        ret = tls_perform_handshake();
        if (ret == 0) break;

        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            // wait until socket readable
            fd_set rf;
            FD_ZERO(&rf);
            FD_SET(sockfd, &rf);
            tv.tv_sec = 5; tv.tv_usec = 0;
            select(sockfd + 1, &rf, NULL, NULL, &tv);
            continue;
        } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            fd_set wf;
            FD_ZERO(&wf);
            FD_SET(sockfd, &wf);
            tv.tv_sec = 5; tv.tv_usec = 0;
            select(sockfd + 1, NULL, &wf, NULL, &tv);
            continue;
        } else {
            printf("Handshake failed\n");
            closesocket(sockfd);
            tls_free_mbed();
            socExit();
            return 1;
        }
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
                int sret = tls_send_mbed(sendbuf, strlen(sendbuf));
                if (sret < 0) {
                    printf("tls_send_mbed failed\n");
                }
            }
        }

        // Non-blocking poll for incoming TLS data using select on the raw socket
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            // Data likely available; read via mbedTLS
            memset(inbuf, 0, sizeof(inbuf));
            ssize_t got = tls_recv_text_mbed(inbuf, sizeof(inbuf));
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
            } else if (got == 0) {
                // connection closed by peer
                printf("TLS connection closed by peer\n");
                break;
            } else if (got == -2) {
                // WANT_READ/WANT_WRITE -> nothing to do right now
            } else {
                // error
                printf("tls_recv_text_mbed error\n");
                break;
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

    // Cleanup
    tls_free_mbed();
    closesocket(sockfd);
    socExit();
    gfxExit();
    return 0;
}
