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

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_internal.h>
#include <mbedtls/error.h>
#include <mbedtls/certs.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

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

void append_chat_message(const char *message) {
    char temp[6000];
    snprintf(temp, sizeof(temp), "%s\n%s", chatstring, message);
    strncpy(chatstring, temp, sizeof(chatstring));

    chatscroll -= 15; // move chat up for new message

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

bool connect_to_server(mbedtls_net_context *server_fd, mbedtls_ssl_context *ssl,
                       mbedtls_ssl_config *conf, mbedtls_ctr_drbg_context *ctr_drbg,
                       mbedtls_entropy_context *entropy, mbedtls_x509_crt *cacert,
                       char *chatstring) {
    const char *pers = "aurorachat";
    mbedtls_net_init(server_fd);
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_x509_crt_init(cacert);
    mbedtls_ctr_drbg_init(ctr_drbg);
    mbedtls_entropy_init(entropy);

    if (mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                              (const unsigned char *) pers, strlen(pers)) != 0) {
        append_chat_message("-Error: RNG Seed Failed-");
        return false;
    }

    if (mbedtls_net_connect(server_fd, "127.0.0.1", "8961", MBEDTLS_NET_PROTO_TCP) != 0) {
        append_chat_message("-Error: Connection Failed-");
        return false;
    }

    mbedtls_ssl_config_defaults(conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT);

    mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE); // don't verify server certificate
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
    mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);

    if (mbedtls_ssl_setup(ssl, conf) != 0) return false;
    mbedtls_ssl_set_bio(ssl, server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    int ret;
    while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

            char err[128];
            mbedtls_strerror(ret, err, sizeof(err));
            append_chat_message(err);
            return false;
        }
    }

    mbedtls_net_set_nonblock(server_fd);

    return true;
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

    // ---- mbedTLS objects ----
    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt_parse_file(&cacert, "romfs:/cert.pem");
    
    bool connected = connect_to_server(&server_fd, &ssl, &conf, &ctr_drbg, &entropy, &cacert, chatstring);

    char username[21];
    char buffer[512];
    u32 textcolor = C2D_Color32(0, 0, 0, 255);
    u32 themecolor = C2D_Color32(255, 255, 255, 255);

    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        // ---- Attempt to connect if not connected ----
        bool connection_error_shown = false;

        if (!connected) {
            if (!connected && !connection_error_shown) {
                connection_error_shown = true;
            } else if (connected) {
                connection_error_shown = false; // reset on successful connection
            }
        
            C2D_TextBufClear(chatbuffer);
            C2D_TextParse(&chat, chatbuffer, chatstring);
            C2D_TextOptimize(&chat);
        }

        // ---- Username input ----
        if (hidKeysDown() & KEY_A) {
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 21);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
            swkbdInputText(&swkbd, username, sizeof(username));
        }

        // ---- Send message ----
        if (hidKeysDown() & KEY_B && connected) {
            char message[80];
            char msg[128];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 80);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            if (swkbdInputText(&swkbd, message, sizeof(message)) == SWKBD_BUTTON_CONFIRM) {
                snprintf(msg, sizeof(msg), "<%s>: %s", username, message);
                mbedtls_ssl_write(&ssl,
                    (const unsigned char*)msg,
                    strlen(msg));
            }
        }

        // ---- Receive messages ----
        if (connected) {
            int len = mbedtls_ssl_read(&ssl,
                (unsigned char*)buffer,
                sizeof(buffer) - 1);
                        
            if (len > 0) {
                buffer[len] = 0;
                append_chat_message(buffer);
            }
            else if (len == 0) {
                // orderly shutdown from server
                connected = false;
                append_chat_message("-Server closed connection-");
            }
            else if (len == MBEDTLS_ERR_SSL_WANT_READ ||
                     len == MBEDTLS_ERR_SSL_WANT_WRITE) {
                // normal: no data yet
            }
            else {
                // real error
                char err[128];
                mbedtls_strerror(len, err, sizeof(err));
                append_chat_message(err);
            
                connected = false;
                mbedtls_ssl_close_notify(&ssl);
                mbedtls_net_free(&server_fd);
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

        C2D_DrawText(&chat, C2D_WithColor | C2D_WordWrap, 0.0f, chatscroll, 0, 0.5, 0.5, textcolor, 290.0f);



        C3D_FrameEnd(0);


        // svcSleepThread(1000000L); // required, otherwise audio can be glitchy, distorted, and chopped up.
        // audio is gone rn
        // will bring it back soon
    }

    // ---- Cleanup mbedTLS ----
    if (connected) mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&server_fd);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    // ---- Cleanup 3DS ----
    ndspExit();
    socExit();
    gfxExit();
    return 0;
}
