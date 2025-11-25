#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/debug.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT "8961"

#define DEBUG_LEVEL 0   // 1–4 for debugging

#define MAX_MSG_LEN 128

static void wait_for_exit() {
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
            break;
        gspWaitForVBlank();
    }
}

void send_message(mbedtls_ssl_context *ssl, const char *msg)
{
    size_t msg_len = strlen(msg);

    if (msg_len > MAX_MSG_LEN) {
        sprintf(MAX_MSG_LEN, sizeof(MAX_MSG_LEN) "Message larger than %d\n");
        return;
    }

    unsigned char frame[2 + MAX_MSG_LEN]; // 2 bytes for length + message
    frame[0] = (msg_len >> 8) & 0xFF; // high byte
    frame[1] = msg_len & 0xFF;        // low byte
    memcpy(frame + 2, msg, msg_len);

    size_t total_len = msg_len + 2;
    size_t written = 0;

    while (written < total_len) {
        int ret = mbedtls_ssl_write(ssl, frame + written, total_len - written);
        if (ret > 0) {
            written += ret;
            continue;
        }
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue; // retry
        }
        // unrecoverable error
        char errbuf[200];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("mbedtls_ssl_write failed: -0x%x (%s)\n", -ret, errbuf);
        return;
    }

    printf("Sent %zu bytes (length-prefixed frame)\n", written);
}

int main(int argc, char **argv)
{
    // -------------------------
    // 3DS initialization
    // -------------------------
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    if (socInit(NULL, 0x100000) != 0) {
        printf("socInit failed!\n");
        wait_for_exit();
        gfxExit();
        return 0;
    }

    printf("3DS TLS Test using mbedTLS\n");

    // -------------------------
    // mbedTLS structures
    // -------------------------
    mbedtls_net_context     net;
    mbedtls_ssl_context     ssl;
    mbedtls_ssl_config      conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_net_init(&net);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    const char *pers = "3ds_tls_client";

    // -------------------------
    // Entropy + RNG
    // -------------------------
    int ret = mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char *)pers,
        strlen(pers)
    );

    if (ret != 0) {
        printf("DRBG seed failed: -0x%x\n", -ret);
        goto exit;
    }

    printf("Connecting to %s...\n", SERVER_HOST);

    // -------------------------
    // TCP Connection
    // -------------------------
    ret = mbedtls_net_connect(
        &net,
        SERVER_HOST,
        SERVER_PORT,
        MBEDTLS_NET_PROTO_TCP
    );

    if (ret != 0) {
        printf("TCP connect failed: -0x%x\n", -ret);
        goto exit;
    }

    // -------------------------
    // Setup SSL config
    // -------------------------
    mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

#if DEBUG_LEVEL > 0
    mbedtls_ssl_conf_dbg(&conf, mbedtls_debug, NULL);
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    // -------------------------
    // Apply config to context
    // -------------------------
    ret = mbedtls_ssl_setup(&ssl, &conf);
    if (ret != 0) {
        printf("ssl_setup failed: -0x%x\n", -ret);
        goto exit;
    }

    mbedtls_ssl_set_hostname(&ssl, SERVER_HOST);
    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, NULL);

    // -------------------------
    // TLS Handshake (TLS 1.3)
    // -------------------------
    printf("Performing handshake...\n");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

            char errbuf[200];
            mbedtls_strerror(ret, errbuf, sizeof(errbuf));
            printf("Handshake failed: -0x%x (%s)\n", -ret, errbuf);

            goto exit;
        }
    }

    if (ret == 0) {
        printf("Handshake OK!\n\n");
    }

    // -------------------------
    // Send Message
    // -------------------------
    unsigned char msg[] = "test message, gdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnjgdfjgkdfhgdhkjjdfgjfugejgkdkgdnkbnkjnbdnj";
    send_message(&ssl, (const char*)msg);

    // -------------------------
    // Read Response
    // -------------------------
    char buffer[1024];

    printf("Response:\n\n");

    for (;;) {
        int ret = mbedtls_ssl_read(&ssl, (unsigned char*)buffer, sizeof(buffer)-1);

        if (ret > 0) {
            buffer[ret] = '\0';
            printf("%s", buffer);
            continue;
        }

        if (ret == 0) {
            /* Connection was closed cleanly by peer */
            printf("\n[peer closed connection]\n");
            break;
        }

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            /* retry read */
            continue;
        }

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            printf("\n[peer sent close_notify]\n");
            break;
        }

        /* other error */
        char errbuf[200];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("\nmbedtls_ssl_read failed: -0x%x (%s)\n", -ret, errbuf);
        break;
    }

    // -------------------------
    // Cleanup
    // -------------------------
exit:
    mbedtls_ssl_close_notify(&ssl);

    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    printf("\n\nPress START to exit.\n");
    wait_for_exit();

    socExit();
    gfxExit();
    return 0;
}