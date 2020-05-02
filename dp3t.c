/*
 * Copyright (C) 2020 Dyne.org foundation
 *
 * This file is subject to the terms and conditions of the GNU
 * General Public License (GPL) version 2. See the file LICENSE
 * for more details.
 *
 */
#include <Arduino.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
//#include <wolfssl/ssl.h>
//#include <wolfssl/wolfcrypt/sha256.h>
//#include <wolfssl/wolfcrypt/aes.h>
//#include <wolfssl/wolfcrypt/hmac.h>

//#include "nimble_scanner.h"
//#include "net/bluetil/ad.h"
//#include "nimble_scanlist.h"

#include "dp3t.h"

#define SK_LEN 32
#define SHA256_LEN 32
#define EPHID_LEN 16
#define EPOCH_LEN 15 // In minutes
#define EPOCHS_PER_DAY (((24 * 60) / EPOCH_LEN) + 1)
#define RETENTION_PERIOD (1) // In days
#define MAX_EPHIDS (RETENTION_PERIOD * EPOCHS_PER_DAY)

#define TRNG_BASE 0x4000D000
#define TRNG_TASKS_START (*(volatile uint32_t *)(TRNG_BASE + 0x000))
#define TRNG_TASKS_STOP  (*(volatile uint32_t *)(TRNG_BASE + 0x004))
#define TRNG_EV_VALRDY   (*(volatile uint32_t *)(TRNG_BASE + 0x100))
#define TRNG_VALUE       (*(volatile uint32_t *)(TRNG_BASE + 0x508))

static uint8_t SKT_0[SK_LEN] = {};
static int keystore_initialized = 0;
static int day_ephid_table = -1;

const uint8_t BROADCAST_KEY[32] = "Broadcast key";
const uint32_t BROADCAST_KEY_LEN = 13;

void dp3t_random(uint8_t *buf, int len)
{
    uint8_t val;
    int i = 0;

    /* Clear VALRDY */
    TRNG_EV_VALRDY = 0;
    /* Start TRNG */
    TRNG_TASKS_START = 1;
    for (i = 0; i < len; i++) {
        /* Wait until value ready */
        //while (TRNG_EV_VALRDY == 0)
        //    ;
        TRNG_VALUE = 0xFFFFFFFF; //random(0, 0xFFFFFFFF);
        buf[i] = (uint8_t)(TRNG_VALUE & 0x000000FF);
        TRNG_EV_VALRDY = 0;
    }
    TRNG_TASKS_STOP |= 1;
}



static uint8_t EPHIDS_LOCAL[EPOCHS_PER_DAY][EPHID_LEN];

/* 
 * SKT0 is random at every power-on now
 * (should it be created once then stored in flash?)
 * TODO
 *
 */
uint8_t *dp3t_get_skt_0(void)
{
    if (!keystore_initialized) {
        dp3t_random(SKT_0, SK_LEN);
        keystore_initialized = 1;
    }
    return SKT_0;
}

/*
 *   This function creates the next key in the chain of SK_t's.
 *   It is called either for the local rotation or when we
 *   recover the different SK_ts from an infected person.
*/
void dp3t_get_skt_1(const uint8_t *skt_0, uint8_t *skt_1)
{
    mbedtls_sha256_context sha;
    uint8_t digest[SHA256_LEN];
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_update_ret(&sha, skt_0, SK_LEN);
    mbedtls_sha256_finish_ret(&sha, skt_1);
    mbedtls_sha256_free(&sha);
}

static void print_hex(const uint8_t *x, int len)
{
    int i;
    for(i = 0; i < len; i++) {
        char str[3];
        sprintf(str, "%02x", (int)x[i]);
        //Serial.print(str);
    }
    //printf("\n");
    //Serial.printf("\n");
}

static void print_ephid(const uint8_t *x)
{
    print_hex(x, EPHID_LEN);
}

static void print_sk(const uint8_t *x)
{
    print_hex(x, SK_LEN);
}


void dp3t_print_ephids(void)
{
    int i;
    for (i = 0; i < EPOCHS_PER_DAY; i++) {
        //Serial.printf("[ %03d ] ", i);
        print_ephid(EPHIDS_LOCAL[i]);
    }
}

void dp3t_create_ephids(const uint8_t *skt_0)
{
    mbedtls_aes_context aes;
    mbedtls_md_context_t hmac;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    uint8_t prf[SK_LEN];
    int i;
    uint8_t zeroes[EPHID_LEN];
    memset(zeroes, 0, EPHID_LEN);
    //Serial.printf("SK0: ");
    print_sk(skt_0);

    /* PRF */
    mbedtls_md_init(&hmac);//, NULL, INVALID_DEVID);
    mbedtls_md_setup(&hmac, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&hmac, (const unsigned char *) skt_0, SK_LEN);
    mbedtls_md_hmac_update(&hmac, (const unsigned char *) BROADCAST_KEY, BROADCAST_KEY_LEN);
    mbedtls_md_hmac_finish(&hmac, prf);
    //Serial.printf("  PRF: ");
    print_sk(prf);

    char stream_block[16];
    /* PRG */
    mbedtls_aes_init(&aes);//, NULL, INVALID_DEVID);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*) prf, 32);//, zeroes, AES_ENCRYPTION);
    for(i = 0; i < EPOCHS_PER_DAY; i++)
        	mbedtls_aes_crypt_ctr(&aes, 16, i , i, stream_block, zeroes, EPHIDS_LOCAL[i]); 
    dp3t_print_ephids();
    mbedtls_md_free(&hmac);
    mbedtls_aes_free(&aes);
}


uint8_t *dp3t_get_ephid(int epoch)
{
    return EPHIDS_LOCAL[epoch];
}
