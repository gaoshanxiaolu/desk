/* Copyright (c) Qualcomm Atheros, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. No reverse engineering, decompilation, or disassembly of this software is permitted.
 * 
 * 2. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 3. Redistributions in binary form must reproduce the above copyright
 * notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 * 
 * 4. Neither the name of Qualcomm-Atheros Inc. nor the names of other
 * contributors to this software may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * 5. This software must only be used in a product manufactured by
 * Qualcomm-Atheros Inc, or in a product manufactured
 * by a third party that is used in combination with a product manufactured by
 * Qualcomm-Atheros Inc.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. NO LICENSES OR OTHER RIGHTS, WHETHER EXPRESS, IMPLIED, BASED ON
 * ESTOPPEL OR OTHERWISE, ARE GRANTED TO ANY PARTY'S PATENTS, PATENT 
 * APPLICATIONS, OR PATENTABLE INVENTIONS BY VIRTUE OF THIS LICENSE OR THE
 * DELIVERY OR PROVISION BY QUALCOMM ATHEROS, INC. OF THE SOFTWARE IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON * ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "qcom_common.h"
#include "swat_wmiconfig_common.h"

#define  CRYPTO_CONFIG_3DES 0
#define CRYPTO_DBG
#ifdef CRYPTO_DBG
#define CRYPTO_PRINTF     printf
#else
#define CRYPTO_PRINTF(x, ...)
#endif

#define ENABLE_DH_DEMO 1
#define ENABLE_ED25519_DEMO 1
#define SRP_MOD_SIZE_3072 1

#ifdef SWAT_CRYPTO
#define crypto_sign_ed25519_BYTES 64U
#define crypto_sign_ed25519_SEEDBYTES 32U
#define crypto_sign_ed25519_PUBLICKEYBYTES 32U
#define crypto_sign_ed25519_SECRETKEYBYTES (32U + 32U)

#define QCOM_CRYPTO_AE_AES_CCM_NONCEBYTES    12U
#define QCOM_CRYPTO_AE_AES_CCM_AADATA_MAXBYTES    14U
#define QCOM_CRYPTO_AE_AES_GCM_NONCEBYTES    12U

#if ENABLE_ED25519_DEMO
typedef struct TestData_ {
    unsigned char  sk[crypto_sign_ed25519_SEEDBYTES];
    unsigned char  pk[crypto_sign_ed25519_PUBLICKEYBYTES];
    unsigned char  sig[crypto_sign_ed25519_BYTES];
    char          *m;
} TestData;

typedef A_UINT32 size_t;

static TestData test_data[] = {
{{0x9d,0x61,0xb1,0x9d,0xef,0xfd,0x5a,0x60,0xba,0x84,0x4a,0xf4,0x92,0xec,0x2c,0xc4,0x44,0x49,0xc5,0x69,0x7b,0x32,0x69,0x19,0x70,0x3b,0xac,0x03,0x1c,0xae,0x7f,0x60,},{0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a,},{0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b,},""},
{{0x4c,0xcd,0x08,0x9b,0x28,0xff,0x96,0xda,0x9d,0xb6,0xc3,0x46,0xec,0x11,0x4e,0x0f,0x5b,0x8a,0x31,0x9f,0x35,0xab,0xa6,0x24,0xda,0x8c,0xf6,0xed,0x4f,0xb8,0xa6,0xfb,},{0x3d,0x40,0x17,0xc3,0xe8,0x43,0x89,0x5a,0x92,0xb7,0x0a,0xa7,0x4d,0x1b,0x7e,0xbc,0x9c,0x98,0x2c,0xcf,0x2e,0xc4,0x96,0x8c,0xc0,0xcd,0x55,0xf1,0x2a,0xf4,0x66,0x0c,},{0x92,0xa0,0x09,0xa9,0xf0,0xd4,0xca,0xb8,0x72,0x0e,0x82,0x0b,0x5f,0x64,0x25,0x40,0xa2,0xb2,0x7b,0x54,0x16,0x50,0x3f,0x8f,0xb3,0x76,0x22,0x23,0xeb,0xdb,0x69,0xda,0x08,0x5a,0xc1,0xe4,0x3e,0x15,0x99,0x6e,0x45,0x8f,0x36,0x13,0xd0,0xf1,0x1d,0x8c,0x38,0x7b,0x2e,0xae,0xb4,0x30,0x2a,0xee,0xb0,0x0d,0x29,0x16,0x12,0xbb,0x0c,0x00,},"\x72"},
{{0xc5,0xaa,0x8d,0xf4,0x3f,0x9f,0x83,0x7b,0xed,0xb7,0x44,0x2f,0x31,0xdc,0xb7,0xb1,0x66,0xd3,0x85,0x35,0x07,0x6f,0x09,0x4b,0x85,0xce,0x3a,0x2e,0x0b,0x44,0x58,0xf7,},{0xfc,0x51,0xcd,0x8e,0x62,0x18,0xa1,0xa3,0x8d,0xa4,0x7e,0xd0,0x02,0x30,0xf0,0x58,0x08,0x16,0xed,0x13,0xba,0x33,0x03,0xac,0x5d,0xeb,0x91,0x15,0x48,0x90,0x80,0x25,},{0x62,0x91,0xd6,0x57,0xde,0xec,0x24,0x02,0x48,0x27,0xe6,0x9c,0x3a,0xbe,0x01,0xa3,0x0c,0xe5,0x48,0xa2,0x84,0x74,0x3a,0x44,0x5e,0x36,0x80,0xd7,0xdb,0x5a,0xc3,0xac,0x18,0xff,0x9b,0x53,0x8d,0x16,0xf2,0x90,0xae,0x67,0xf7,0x60,0x98,0x4d,0xc6,0x59,0x4a,0x7c,0x15,0xe9,0x71,0x6e,0xd2,0x8d,0xc0,0x27,0xbe,0xce,0xea,0x1e,0xc4,0x0a,},"\xaf\x82"},
{{0x0d,0x4a,0x05,0xb0,0x73,0x52,0xa5,0x43,0x6e,0x18,0x03,0x56,0xda,0x0a,0xe6,0xef,0xa0,0x34,0x5f,0xf7,0xfb,0x15,0x72,0x57,0x57,0x72,0xe8,0x00,0x5e,0xd9,0x78,0xe9,},{0xe6,0x1a,0x18,0x5b,0xce,0xf2,0x61,0x3a,0x6c,0x7c,0xb7,0x97,0x63,0xce,0x94,0x5d,0x3b,0x24,0x5d,0x76,0x11,0x4d,0xd4,0x40,0xbc,0xf5,0xf2,0xdc,0x1a,0xa5,0x70,0x57,},{0xd9,0x86,0x8d,0x52,0xc2,0xbe,0xbc,0xe5,0xf3,0xfa,0x5a,0x79,0x89,0x19,0x70,0xf3,0x09,0xcb,0x65,0x91,0xe3,0xe1,0x70,0x2a,0x70,0x27,0x6f,0xa9,0x7c,0x24,0xb3,0xa8,0xe5,0x86,0x06,0xc3,0x8c,0x97,0x58,0x52,0x9d,0xa5,0x0e,0xe3,0x1b,0x82,0x19,0xcb,0xa4,0x52,0x71,0xc6,0x89,0xaf,0xa6,0x0b,0x0e,0xa2,0x6c,0x99,0xdb,0x19,0xb0,0x0c,},"\xcb\xc7\x7b"},
    };
#endif

//since the limitation of lit_reg, all the test data use this one.
A_UINT8 test_iv[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

void ref_attr_init(qcom_crypto_attrib_t* attr, A_UINT32 id,
        void* buffer, uint32_t length) 
{
    attr->attrib_id = id;
    attr->u.ref.len = length;
    attr->u.ref.buf = buffer;
}

void val_attr_init(qcom_crypto_attrib_t* attr, A_UINT32 id,
        A_UINT32 a, A_UINT32 b)
{
    attr->attrib_id = id;
    attr->u.val.a = a;
    attr->u.val.b = b;
}

#if ENABLE_DH_DEMO
/* DH Params */
#define PRIME_BYTES (128)
#define PRIME_BITS (PRIME_BYTES * 8)
static A_UINT8 p_1024[PRIME_BYTES] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
    0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
    0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
    0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
    0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
    0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
    0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
    0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
    0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
    0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static A_UINT8 g[4] = {0, 0, 0, 2};
/* End of DH Params */
#endif

#define ENABLE_RNG_DEMO
#ifdef ENABLE_RNG_DEMO
/* RNG Params */
#define crypto_rng_RANDOMBYTES 256
/* End of RNG Params */
#endif

#if 0
A_UINT8 rsa_mod[] = {0xed,0x41,0x2b,0xd3,0x57,0xb1,0xf1,0xff,0x76,0x31,0xaa,0x49,0x61,0x8b,0x8b,0x52,0xa0,0xab,0xf2,0x68,0x7c,0x4b,0xc4,0x37,0x18,0x55,0x09,0xf5,0x15,0xdc,0x0a,0x4e,0x15,0xa9,0x73,0xbb,0xfa,0xd8,0x14,0xa7,0x1e,0xac,0xa3,0x9f,0x65,0xb7,0x4e,0x33,0x8d,0x06,0xb4,0xfb,0x64,0xa7,0x46,0x5f,0xfe,0xc2,0x22,0x84,0xfa,0xd8,0x7d,0x84,0x2f,0xc8,0x39,0xc4,0xd0,0xa0,0x2f,0x33,0x56,0xf8,0xb9,0x49,0xdf,0x0b,0x62,0x4b,0x2d,0xac,0x5b,0xf0,0xe1,0x1a,0xf5,0x3c,0x0f,0x01,0xe9,0x24,0x6b,0xe8,0x04,0xc7,0xb3,0x47,0x18,0xc8,0x14,0x7b,0x56,0xb4,0xac,0xc5,0xd5,0x32,0xe4,0xb0,0x24,0x6e,0xbc,0x7d,0x88,0x71,0xcf,0xee,0x95,0xc6,0x60,0x9c,0x82,0xa0,0x43,0xe2,0x5a,0xf8,0x24,0x05,0xa5,0x9d,0xe2,0xd3,0x75,0x06,0x5c,0xdd,0x88,0x2e,0x03,0x39,0x2b,0x16,0xd5,0x03,0xba,0x2d,0xa4,0xe7,0x3b,0xf5,0xb7,0x7d,0x49,0xaf,0xc8,0x46,0x2a,0x1c,0x62,0xdc,0x98,0x6b,0xab,0xab,0xdd,0xa9,0x5d,0xac,0x10,0x2e,0x3e,0xb3,0xaa,0x9a,0x2c,0x57,0x1a,0x93,0xb1,0xa0,0x5a,0x12,0x60,0x67,0xca,0xe7,0xf1,0x55,0x85,0xf9,0xc9,0x51,0x58,0x21,0x05,0x27,0xc1,0x34,0x8d,0x3e,0x9a,0xb2,0xc4,0xd7,0x71,0xb8,0x46,0x7a,0xc9,0x67,0x77,0x1c,0x76,0x60,0x30,0x22,0xbb,0xad,0xa8,0x9c,0xc2,0xb3,0x89,0x9b,0xc8,0x80,0xf9,0xb8,0xcd,0x2f,0x09,0x62,0x2a,0x94,0x0d,0xb7,0xf5,0xc1,0x65,0xe7,0x4e,0x0f,0x49,0x5f,0x19,0x3e,0x8f,0xae,0x42,0xa3,0x1e,0xec,0x96,0x77};

A_UINT8 rsa_pvt_exp[] = {0xd6,0xab,0x1b,0x1e,0x54,0xc0,0xbb,0x37,0xec,0x17,0xaf,0xfe,0x49,0x76,0x5b,0x8f,0x5d,0xb5,0x76,0xd6,0x37,0x70,0xce,0x8f,0x13,0x43,0x0e,0x89,0x65,0x47,0xfd,0x42,0xfd,0xb2,0x9e,0xf7,0x3d,0x56,0x7a,0x09,0x64,0x65,0xcc,0x7e,0x93,0x28,0x32,0x67,0xce,0x78,0x7d,0x14,0xe1,0xd3,0xc0,0x87,0x67,0x18,0xfc,0xe6,0xd9,0x99,0x3c,0xa8,0x78,0x1b,0x70,0xb9,0xb6,0x12,0xd9,0xe2,0x58,0x15,0x20,0x81,0xc8,0x80,0xa2,0x65,0x67,0x64,0x06,0xa4,0x82,0xe9,0x43,0x6f,0x1e,0x1d,0x1a,0x78,0x4b,0xf2,0x59,0x30,0xdf,0xf3,0xba,0x66,0x7e,0xb1,0xc2,0x98,0x23,0xa3,0xb4,0xee,0x21,0xa1,0x86,0xb5,0x73,0x73,0x1a,0x1b,0xf0,0x89,0xed,0x96,0xdb,0x1f,0x81,0xc9,0xc4,0xe2,0x70,0x6f,0x4a,0xec,0x2c,0x59,0x6f,0x77,0x07,0xca,0x47,0xfe,0xb3,0xdd,0x75,0x1d,0xd2,0x86,0x62,0x75,0x5a,0x80,0xc9,0x25,0x71,0xf4,0x60,0xd4,0x0c,0x85,0xad,0x02,0x38,0x2a,0xab,0x26,0x6a,0x3e,0xc3,0x9b,0x4e,0xd7,0x36,0x40,0xd1,0xf0,0xfe,0x58,0x8d,0x4b,0xe5,0x82,0xa3,0xc1,0xda,0x1d,0x7a,0xfd,0xa6,0x20,0xd5,0xdc,0xbe,0x25,0xc3,0x90,0x9a,0x7a,0x04,0x28,0x49,0x2a,0x72,0x57,0x87,0xbf,0x63,0x3e,0x2a,0x6f,0x03,0xd2,0x0d,0x9a,0x5c,0x1c,0x55,0xf0,0x53,0x35,0x1a,0x4b,0xaa,0x3f,0x5a,0xee,0x13,0xbc,0x86,0x22,0x63,0xd7,0xd9,0x05,0x68,0x1e,0x1d,0xfb,0x94,0xe6,0xe6,0xee,0xdd,0xa8,0xaa,0x7b,0x63,0x5b,0x60,0x37,0xb6,0x58,0xe4,0x4e,0x14,0xaa,0x2d,0x53,0x50,0xe1};
#endif

A_UINT8 rsa_pub_exp[] = {0x01, 0x00, 0x01};

A_UINT8 rsa_mod[] = {0xa7,0x34,0x23,0xef,0xe9,0xcf,0xc5,0x25,0xed,0x78,0xe6,0x98,0xe9,0x63,0x37,0xc3,0xf5,0x1c,0xaf,0x39,0xb3,0xb1,0xb8,0x81,0x98,0x4f,0xcf,0x2f,0x95,0x2f,0x45,0x77,0x31,0xd5,0x4b,0x8e,0x86,0xc7,0x24,0x87,0xe8,0x46,0x18,0xa4,0xca,0x87,0x36,0x0b,0xaa,0x48,0x0b,0xd8,0xbd,0x31,0x2e,0xaf,0xd0,0xa0,0x0e,0xd6,0xe7,0x46,0x08,0x3a,0x03,0x90,0xdb,0x87,0x1c,0xcb,0x12,0x53,0x4a,0xaf,0x9e,0x9b,0x98,0xb0,0x66,0x78,0x43,0x61,0x37,0x23,0x9b,0x20,0x12,0xa3,0x55,0x63,0x47,0xe5,0x88,0xd8,0xf1,0x9c,0x1f,0x06,0x7b,0x9b,0xf3,0xfb,0xdb,0x76,0x86,0x97,0x67,0x11,0x9d,0x1f,0x02,0xda,0x15,0x2b,0x49,0x49,0x97,0xe6,0x92,0xbe,0x74,0xbb,0x8d,0x50,0xaa,0xed,0x1d,0x33};

A_UINT8 rsa_pvt_exp[] = {0xa2,0x2e,0xc8,0x21,0x01,0x49,0x40,0x70,0xde,0x9c,0x64,0x08,0xb4,0x71,0x41,0xf9,0x38,0x7b,0x1f,0x58,0x37,0xfd,0xcd,0xfb,0x1e,0x93,0x63,0x63,0x43,0x30,0xbc,0x0f,0xb6,0xed,0xae,0xc1,0x4b,0xe8,0x44,0x7d,0xf3,0x86,0x72,0x86,0xfd,0xd0,0x13,0x53,0x53,0x53,0x58,0xf2,0x5d,0xe9,0x23,0xe0,0xf3,0xdb,0x1d,0x29,0xc4,0xe1,0x40,0x3d,0xfc,0x15,0x98,0x0d,0x14,0x7f,0xcc,0xb3,0x4d,0xb9,0xeb,0xf5,0xf1,0xc7,0x9f,0xcd,0x9f,0xae,0x3c,0x8c,0x3b,0xc5,0x06,0x0a,0x59,0x11,0xac,0x44,0x9b,0xad,0xb9,0x1e,0x45,0x52,0xca,0x63,0xc2,0xea,0x67,0x0d,0x92,0x5c,0xa1,0x09,0x9a,0x9b,0xf3,0x79,0xf5,0x8b,0xcd,0x0d,0x8e,0x4f,0x9b,0x44,0xc1,0x36,0xda,0x01,0xd9,0x94,0xc3,0x61};

A_UINT8 rsa_prime1[] = {0xd4,0x03,0x3d,0x5d,0xa8,0xb4,0x42,0xb8,0x20,0x6a,0xb1,0x79,0x10,0xfc,0xfa,0xff,0xa6,0x5e,0xa3,0xa9,0xcd,0x7b,0x61,0x54,0x16,0x7d,0xdf,0xfb,0x90,0x1a,0xc6,0x6b,0x29,0x24,0x08,0xca,0x84,0xa6,0xc2,0xcf,0xf5,0xac,0xa8,0xdc,0xc9,0x10,0x7f,0xb7,0x84,0x12,0x2d,0xd7,0x90,0x1f,0xdb,0x27,0x01,0xfa,0x9f,0xed,0xf2,0xd8,0xcf,0xb1};

A_UINT8 rsa_prime2[] = {0xc9,0xe4,0xee,0xec,0x50,0xc7,0xa1,0x00,0xfd,0x37,0x89,0x92,0xab,0x99,0xb0,0x2d,0x45,0xa2,0x5a,0xec,0xd2,0x12,0x76,0xf1,0x68,0xc9,0x40,0x3e,0xe5,0x51,0x97,0x1f,0x43,0x77,0x7f,0x56,0x11,0x1e,0x05,0x82,0x85,0x8d,0x37,0x2e,0x1b,0xc7,0x07,0xd0,0x19,0x6d,0xdc,0x0c,0x8f,0xc4,0x74,0xa2,0x9f,0x1f,0x3a,0xbe,0x67,0xfa,0x38,0x23};

A_UINT8 rsa_exp1[] = {0xc8,0x23,0x0e,0xc8,0xdd,0x3a,0xdd,0x48,0xc7,0x81,0x30,0x6b,0xa2,0xf7,0xcd,0x51,0x8c,0x12,0x06,0xd9,0x82,0x5a,0x18,0x34,0xb2,0xce,0xbc,0xa3,0xd3,0x13,0x13,0x7f,0x91,0x64,0xac,0xcf,0xd0,0x8d,0x43,0x95,0xe0,0xca,0xce,0xd5,0x2d,0x10,0xe0,0x1f,0xb3,0x13,0x1e,0x27,0x41,0xac,0x70,0xca,0xcf,0xf9,0x71,0x03,0xc4,0x9d,0x9a,0xe1};

A_UINT8 rsa_exp2[] = {0x0e,0x5a,0x43,0x0d,0xf3,0xb4,0x2d,0x62,0xf7,0x9d,0x62,0x1f,0x56,0x29,0xa7,0xd7,0xa0,0x12,0xa9,0xaa,0x1a,0x49,0x0b,0xc1,0x9f,0xb4,0x66,0xe7,0xd1,0xbf,0x9a,0x21,0xb3,0xd7,0x23,0xeb,0x47,0x6e,0x3d,0xf0,0x08,0x74,0x80,0x8e,0xbb,0x94,0xcb,0x9e,0x64,0xa0,0x65,0xbb,0x52,0xe1,0x21,0x75,0x8a,0x20,0x5b,0x39,0xbc,0x04,0x92,0xc7};

A_UINT8 rsa_coeff[] = {0xb0,0xe0,0x2c,0x43,0x47,0x11,0x38,0x5e,0xb9,0x26,0x74,0x39,0x73,0x96,0xc4,0x87,0x10,0xe4,0xb8,0x5c,0x2d,0x32,0x17,0x9b,0x5f,0x88,0xf7,0xf9,0xbd,0x09,0xac,0x48,0xf2,0x0e,0x96,0x8a,0xbc,0x97,0xfc,0xc7,0xa3,0x51,0x6f,0x84,0xfb,0x8f,0x0c,0x14,0x6a,0x26,0xec,0x08,0xe9,0xb2,0x43,0xde,0x23,0xc4,0x89,0x13,0x19,0x0a,0x8c,0xc1};

#ifdef SRP_MOD_SIZE_1024
/* Modulus size 1024 bit */
#define MOD_SIZE (1024/8)
A_UINT32 srp_mod_tmp[] = {
0xEEAF0AB9 , 0xADB38DD6 ,0x9C33F80A ,0xFA8FC5E8 ,0x60726187 ,0x75FF3C0B ,0x9EA2314C,
0x9C256576 , 0xD674DF74 ,0x96EA81D3 ,0x383B4813 ,0xD692C6E0 ,0xE0D5D8E2 ,0x50B98BE4,
0x8E495C1D , 0x6089DAD1 ,0x5DC7D7B4 ,0x6154D6B6 ,0xCE8EF4AD ,0x69B15D49 ,0x82559B29,
0x7BCF1885 , 0xC529F566 ,0x660E57EC ,0x68EDBC3C ,0x05726CC0 ,0x2FD4CBF4 ,0x976EAA9A,
0xFD5138FE , 0x8376435B ,0x9FC61D2F ,0xC0EB06E3
};

A_UINT8 srp_gen_sample[] = {2};

A_UINT32 srp_ver_tmp[] = {
0x7E273DE8 ,0x696FFC4F , 0x4E337D05, 0xB4B375BE , 0xB0DDE156 , 0x9E8FA00A , 0x9886D812,
0x9BADA1F1 ,0x822223CA , 0x1A605B53, 0x0E379BA4 , 0x729FDC59 , 0xF105B478 , 0x7E5186F5,
0xC671085A ,0x1447B52A , 0x48CF1970, 0xB4FB6F84 , 0x00BBF4CE , 0xBFBB1681 , 0x52E08AB5,
0xEA53D15C ,0x1AFF87B2 , 0xB9DA6E04, 0xE058AD51 , 0xCC72BFC9 , 0x033B564E , 0x26480D78,
0xE955A5E2 ,0x9E7AB245 , 0xDB2BE315, 0xE2099AFB
};

#endif

#ifdef SRP_MOD_SIZE_3072
/* Modulus size 3072 bit */
#define MOD_SIZE (3072/8)
#define ALG_TYPE (QCOM_CRYPTO_ALG_SHA512)

A_UINT32 srp_mod_tmp[] = {
0xFFFFFFFF ,0xFFFFFFFF ,0xC90FDAA2 ,0x2168C234 ,0xC4C6628B ,0x80DC1CD1 ,0x29024E08,
0x8A67CC74 ,0x020BBEA6 ,0x3B139B22 ,0x514A0879 ,0x8E3404DD ,0xEF9519B3 ,0xCD3A431B,
0x302B0A6D ,0xF25F1437 ,0x4FE1356D ,0x6D51C245 ,0xE485B576 ,0x625E7EC6 ,0xF44C42E9,
0xA637ED6B ,0x0BFF5CB6 ,0xF406B7ED ,0xEE386BFB ,0x5A899FA5 ,0xAE9F2411 ,0x7C4B1FE6,
0x49286651 ,0xECE45B3D ,0xC2007CB8 ,0xA163BF05 ,0x98DA4836 ,0x1C55D39A ,0x69163FA8,
0xFD24CF5F ,0x83655D23 ,0xDCA3AD96 ,0x1C62F356 ,0x208552BB ,0x9ED52907 ,0x7096966D,
0x670C354E ,0x4ABC9804 ,0xF1746C08 ,0xCA18217C ,0x32905E46 ,0x2E36CE3B ,0xE39E772C,
0x180E8603 ,0x9B2783A2 ,0xEC07A28F ,0xB5C55DF0 ,0x6F4C52C9 ,0xDE2BCBF6 ,0x95581718,
0x3995497C ,0xEA956AE5 ,0x15D22618 ,0x98FA0510 ,0x15728E5A ,0x8AAAC42D ,0xAD33170D,
0x04507A33 ,0xA85521AB ,0xDF1CBA64 ,0xECFB8504 ,0x58DBEF0A ,0x8AEA7157 ,0x5D060C7D,
0xB3970F85 ,0xA6E1E4C7 ,0xABF5AE8C ,0xDB0933D7 ,0x1E8C94E0 ,0x4A25619D ,0xCEE3D226,
0x1AD2EE6B ,0xF12FFA06 ,0xD98A0864 ,0xD8760273 ,0x3EC86A64 ,0x521F2B18 ,0x177B200C,
0xBBE11757 ,0x7A615D6C ,0x770988C0 ,0xBAD946E2 ,0x08E24FA0 ,0x74E5AB31 ,0x43DB5BFC,
0xE0FD108E ,0x4B82D120 ,0xA93AD2CA ,0xFFFFFFFF ,0xFFFFFFFF};
A_UINT8 srp_mod_sample[MOD_SIZE]; /* Hold SRP modulus as an array of srp_mod_tmp in big endian */

A_UINT8 srp_gen_sample[] = {5};
A_UINT32 srp_ver_tmp[] = 
   {0x9b5e0617, 0x01ea7aeb, 0X39CF6E35, 0X19655A85, 0X3CF94C75, 0XCAF2555E, 0XF1FAF759, 0XBB79CB47,
    0X7014E04A, 0X88D68FFC, 0X05323891, 0xD4C205B8, 0XDE81C2F2, 0X03D8FAD1, 0XB24D2C10, 0X9737F1BE,
    0xBBD71F91, 0x2447C4A0, 0x3C26B9FA, 0xD8EDB3E7, 0x80778e30, 0x2529ed1e, 0xe138ccfc, 0x36d4ba31,
    0x3CC48B14, 0xEA8C22A0, 0x186B222E, 0x655f2df5, 0x603fd75d, 0xf76b3b08, 0xff895006, 0x9add03a7,
    0x54EE4AE8, 0x8587CCE1, 0xBFDE3679, 0x4DBAE459, 0x2b7b904f, 0x442b041c, 0xb17aebad, 0x1e3aebe3, 
    0xCBE99DE6, 0x5F4BB1FA, 0x00B0E7AF, 0x06863DB5, 0x3B02254E, 0xC66E781e, 0x3b62a821, 0x2c86beb0,
    0xD50B5BA6, 0xD0B478D8, 0xC4E9BBCE, 0xC2176532, 0x6FBD1405, 0x8D2BBDE2, 0xC33045F0, 0x3873E539,
    0x48D78B79, 0x4F0790E4, 0x8C36AED6, 0xE880F557, 0x427B2FC0, 0x6DB5E1E2, 0xE1D7E661, 0xAC482D18,
    0xE528D729, 0x5EF74372, 0x95FF1A72, 0xD4027717, 0x13F16876, 0xDD050AE5, 0xB7AD53cc, 0xb90855c9, 
    0x39566483, 0x58ADFD96, 0x6422F524, 0x98732D68, 0xD1D7FBEF, 0x10D78034, 0xAB8DCB6F, 0x0FCF885C, 
    0xC2B2EA2C, 0x3E6AC866, 0x09EA058A, 0x9DA8CC63, 0x531DC915, 0x414df568, 0xb09482dd, 0xac1954de,
    0xC7EB714F, 0x6FF7D44C, 0xD5B86F6B, 0xD1158109, 0x30637C01, 0xD0F6013B, 0xC9740FA2, 0xC633BA89};
A_UINT8 srp_ver_sample[MOD_SIZE];
#endif

A_UINT8 srp_cli_pub_key[MOD_SIZE];
A_UINT8 srp_srv_pub_key[MOD_SIZE];
A_UINT8 srp_srv_shared_secret[MOD_SIZE];
A_UINT8 srp_cli_shared_secret[MOD_SIZE];
A_UINT32 salt_tmp[] = {0xBEB25379, 0xD1A8581E, 0xB5A72767, 0x3A2441EE};
#define SALT_LEN 16
A_UINT8 salt[SALT_LEN];
A_UINT8 uname[] = "alice";
A_UINT8 pwd[] = "password123";
A_UINT8 srp_mod_sample[MOD_SIZE]; /* Holds big endian array */
A_UINT8 srp_ver_sample[MOD_SIZE]; /* Holds big endian array */

/* To check for test vectors given in RFC 5054, remove RNG_getRNG from srp.c and replace with pvt keys below */
A_UINT8 client_pvt_key_sample[] = {0x60, 0x97, 0x55, 0x27, 0x03, 0x5c, 0xf2, 0xad, 0x19, 0x89, 0x80, 0x6f, 0x04, 0x07, 0x21, 0x0b, 0xc8, 0x1e, 0xdc, 0x04, 0xe2, 0x76, 0x2a, 0x56, 0xaf, 0xd5, 0x29, 0xdd, 0xda, 0x2d, 0x43, 0x93};
A_UINT8 server_pvt_key_sample[] = {0xE4, 0x87, 0xCB, 0x59, 0xD3, 0x1A, 0xC5, 0x50, 0x47, 0x1E, 0x81, 0xF0, 0x0F, 0x69, 0x28, 0xE0, 0x1D, 0xDA, 0x08, 0xE9, 0x74, 0xA0, 0x04, 0xF4, 0x9E, 0x61, 0xF5, 0xD1, 0x05, 0x28, 0x4D, 0x20};
A_UINT8 srp_new_ver[MOD_SIZE];

void _memmove_endianess(A_UINT8 *d , const A_UINT8 *s , A_UINT16 len );

A_INT32 srp_demo()
{
    qcom_crypto_obj_hdl_t cli_hdl, srv_hdl, srv_sec_hdl, cli_sec_hdl;
    qcom_crypto_op_hdl_t srv_op_hdl, cli_op_hdl;
    qcom_crypto_attrib_t attr[6];
    A_UINT32 srp_key_size = MOD_SIZE * 8;

    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_SRP_KEYPAIR, srp_key_size, 
                &cli_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
        return -1;
    }

    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_SRP_KEYPAIR, srp_key_size, 
                &srv_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
        return -1;
    }

    /* Input need to be array of bin endian integers */
    _memmove_endianess(srp_mod_sample, (A_UINT8 *)srp_mod_tmp, MOD_SIZE);
    _memmove_endianess(srp_ver_sample, (A_UINT8 *)srp_ver_tmp, MOD_SIZE);
    _memmove_endianess(salt, (A_UINT8 *)salt_tmp, SALT_LEN);

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PRIME, srp_mod_sample, MOD_SIZE);
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_SRP_GEN, srp_gen_sample, 1);
    val_attr_init(&attr[2], QCOM_CRYPTO_ATTR_SRP_TYPE, QCOM_CRYPTO_SRP_CLIENT, 0);
    if (qcom_crypto_transient_obj_keygen(cli_hdl, srp_key_size, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_SRP_CLIENT) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to gen client key\n");
        return -1;
    }

    if (qcom_crypto_obj_buf_attrib_get(cli_hdl, QCOM_CRYPTO_ATTR_SRP_PUBLIC_VALUE, srp_cli_pub_key, MOD_SIZE) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to get client public val\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PRIME, srp_mod_sample, MOD_SIZE);
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_SRP_GEN, srp_gen_sample, 1);
    ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_SRP_VERIFIER, srp_ver_sample, MOD_SIZE);
    val_attr_init(&attr[3], QCOM_CRYPTO_ATTR_SRP_TYPE, QCOM_CRYPTO_SRP_SERVER, 0);
    val_attr_init(&attr[4], QCOM_CRYPTO_ATTR_SRP_HASH, QCOM_CRYPTO_ALG_SHA512, 0);
    if (qcom_crypto_transient_obj_keygen(srv_hdl, srp_key_size, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_SRP_SERVER + 1) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to gen server key\n");
        return -1;
    }

    if (qcom_crypto_obj_buf_attrib_get(srv_hdl, QCOM_CRYPTO_ATTR_SRP_PUBLIC_VALUE, srp_srv_pub_key, MOD_SIZE) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to get server public val\n");
        return -1;
    }

    /* Derive server shared secret */
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, MOD_SIZE * 8, &srv_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc gen secret\n");
        return -1;
    }

    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SRP_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, MOD_SIZE*8, &srv_op_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }

    if (qcom_crypto_op_key_set(srv_op_hdl, srv_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set pvt key\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PUBLIC_VALUE, srp_cli_pub_key, MOD_SIZE);
    if (qcom_crypto_op_key_derive(srv_op_hdl, &attr[0], 1, srv_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to derive key 1\n");
        return -1;
    }

    if (qcom_crypto_obj_buf_attrib_get(srv_sec_hdl, QCOM_CRYPTO_ATTR_SECRET_VALUE, srp_srv_shared_secret, MOD_SIZE) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to get derived key val 1\n");
        return -1;
    }

    /* Derive client shared secret */
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, MOD_SIZE * 8, &cli_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc gen secret\n");
        return -1;
    }

    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SRP_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, MOD_SIZE*8, &cli_op_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }

    if (qcom_crypto_op_key_set(cli_op_hdl, cli_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set pvt key\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PUBLIC_VALUE, srp_srv_pub_key, MOD_SIZE);
    /* Note: Do not include NULL termination character in length of username and
     * pwd */
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_SRP_USERNAME, uname, sizeof(uname) - 1);
    ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_SRP_PASSWORD, pwd, sizeof(pwd) - 1);
    ref_attr_init(&attr[3], QCOM_CRYPTO_ATTR_SRP_SALT, salt, SALT_LEN);
    val_attr_init(&attr[4], QCOM_CRYPTO_ATTR_SRP_HASH, QCOM_CRYPTO_ALG_SHA512, 0);
    if (qcom_crypto_op_key_derive(cli_op_hdl, &attr[0], 5, cli_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to derive key 2\n");
        return -1;
    }

    if (qcom_crypto_obj_buf_attrib_get(cli_sec_hdl, QCOM_CRYPTO_ATTR_SECRET_VALUE, srp_cli_shared_secret, MOD_SIZE) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to get derived key val 2\n");
        return -1;
    }

    if (memcmp(srp_cli_shared_secret, srp_srv_shared_secret, MOD_SIZE) != 0) {
        CRYPTO_PRINTF("shared secret failure\n");
        return -1;
    }
    CRYPTO_PRINTF("SRP shared secret match!\n");


    /*Generate Verifier on Server side demo*/
    qcom_crypto_obj_hdl_t cli_ver_hdl, cli_ver_sec_hdl;
    qcom_crypto_op_hdl_t cli_ver_op_hdl;

    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_SRP_KEYPAIR, srp_key_size, 
                &cli_ver_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
        return -1;
    }
    
    /* Input need to be array of bin endian integers */
    _memmove_endianess(srp_mod_sample, (A_UINT8 *)srp_mod_tmp, MOD_SIZE);
    _memmove_endianess(srp_ver_sample, (A_UINT8 *)srp_ver_tmp, MOD_SIZE);
    _memmove_endianess(salt, (A_UINT8 *)salt_tmp, SALT_LEN);

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PRIME, srp_mod_sample, MOD_SIZE);
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_SRP_GEN, srp_gen_sample, 1);
    val_attr_init(&attr[2], QCOM_CRYPTO_ATTR_SRP_TYPE, QCOM_CRYPTO_SRP_CLIENT, 0);
    if (qcom_crypto_transient_obj_keygen(cli_ver_hdl, srp_key_size, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_SRP_CLIENT) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to gen client key\n");
        return -1;
    }
    /* Derive client shared secret */
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, MOD_SIZE * 8, &cli_ver_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc gen secret\n");
        return -1;
    }

    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SRP_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, MOD_SIZE*8, &cli_ver_op_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }

    if (qcom_crypto_op_key_set(cli_ver_op_hdl, cli_ver_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set pvt key\n");
        return -1;
    }
    memcpy(srp_srv_pub_key, srp_mod_sample, MOD_SIZE); //use fake server pub key since it could not be get
    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SRP_PUBLIC_VALUE, srp_srv_pub_key, MOD_SIZE);   //
    /* Note: Do not include NULL termination character in length of username and
     * pwd */
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_SRP_USERNAME, uname, sizeof(uname) - 1);
    ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_SRP_PASSWORD, pwd, sizeof(pwd) - 1);
    ref_attr_init(&attr[3], QCOM_CRYPTO_ATTR_SRP_SALT, salt, SALT_LEN);
    val_attr_init(&attr[4], QCOM_CRYPTO_ATTR_SRP_HASH, QCOM_CRYPTO_ALG_SHA512, 0);
    ref_attr_init(&attr[5], QCOM_CRYPTO_ATTR_SRP_VERIFIER, srp_new_ver, MOD_SIZE);
    if (qcom_crypto_op_key_derive(cli_ver_op_hdl, &attr[0], 6, cli_ver_sec_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to derive key 2\n");
        return -1;
    }

    if (memcmp(srp_ver_tmp, srp_new_ver, MOD_SIZE) == 0){
        CRYPTO_PRINTF("Generate Verifier Success!\n");
    }
    
//QUIT_VER:
    qcom_crypto_transient_obj_free(cli_ver_hdl); 
    qcom_crypto_transient_obj_free(cli_ver_sec_hdl); 
    qcom_crypto_op_free(cli_ver_op_hdl);
    
    /*End Verrifier Demo*/

//QUIT_SRP:
    qcom_crypto_transient_obj_free(cli_hdl); 
    qcom_crypto_transient_obj_free(srv_hdl); 
    qcom_crypto_transient_obj_free(cli_sec_hdl); 
    qcom_crypto_transient_obj_free(srv_sec_hdl); 
    qcom_crypto_op_free(srv_op_hdl); 
    qcom_crypto_op_free(cli_op_hdl); 
}

A_INT32 ecdsa_demo()
{
        qcom_crypto_obj_hdl_t obj_hdl;
        qcom_crypto_op_hdl_t sign_hdl, verify_hdl;
        qcom_crypto_attrib_t attr[4];
        unsigned char sm[QCOM_CRYPTO_ECC_P521_PUBLIC_KEY_BITS/8];
        A_UINT32 smlen;
        int i;

        A_UINT8 pub_key_X[QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES];
        A_UINT8 pub_key_Y[QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES];
        A_UINT8 pvt_key[QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BYTES];
#define DIGEST_LEN (256/8)
        A_UINT8 digest[DIGEST_LEN];
        for (i = 0; i < DIGEST_LEN; i++) {
            digest[i] = i;
        }


        {
            /* ECDSA demo 1 - sign/verify using key_gen */

            /* Allocate ECDSA keypair object */
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDSA_KEYPAIR, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, 
                        &obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
                return -1;
            }

            /* Generate ECDSA keypair for a given curve id */
            val_attr_init(attr, QCOM_CRYPTO_ATTR_ECC_CURVE, QCOM_CRYPTO_ECC_CURVE_NIST_P256, 0);
            if (qcom_crypto_transient_obj_keygen(obj_hdl, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, attr, 1) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to gen key\n");
                return -1;
            }

            /* Allocate ECDSA sign operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDSA_P256, QCOM_CRYPTO_MODE_SIGN, QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BITS, 
                        &sign_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                return -1;
            }

            /* Copy private key from keypair object for sign operation */
            if (qcom_crypto_op_key_set(sign_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }

            /* Allocate ECDSA verify operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDSA_P256, QCOM_CRYPTO_MODE_VERIFY, QCOM_CRYPTO_ECC_P256_PUBLIC_KEY_BITS, &verify_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc verify op\n");
                return -1;
            }


            /* Copy public key from keypair object for verify operation */
            if (qcom_crypto_op_key_set(verify_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }

            /* Generate signature */
            if (qcom_crypto_op_sign_digest(sign_hdl, NULL, 0, digest, DIGEST_LEN, sm, &smlen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to sign\n");
                return -1;
            }


            /* Verify signature */
            if (qcom_crypto_op_verify_digest(verify_hdl, NULL, 0, digest, DIGEST_LEN, sm, smlen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed verify\n");
                return -1;
            }

            CRYPTO_PRINTF("\nDemo 1 successful\n");

            qcom_crypto_transient_obj_free(obj_hdl);
            qcom_crypto_op_free(sign_hdl);
            qcom_crypto_op_free(verify_hdl);
        }

        {
            /* DEMO 2 - sign/verify using populate QAPI 
             *
             * In the real world, the public key would be received over the
             * air and private key may be provisioned to the device. In
             * those cases, the populate API should be used 
             *
             * Here, for demo purposes, keygen API is used to generate the key;
             * buf_attr_get API to extract the keys and populate API to copy the
             * keys into the object
             *
             * */

            qcom_crypto_obj_hdl_t keygen_hdl, keypair_hdl, public_key_hdl;
            /* Allocate ECDSA keypair object */
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDSA_KEYPAIR, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, &keygen_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
                return -1;
            }

            /* Generate ECDSA keypair for a given curve id */
            val_attr_init(attr, QCOM_CRYPTO_ATTR_ECC_CURVE, QCOM_CRYPTO_ECC_CURVE_NIST_P256, 0);
            if (qcom_crypto_transient_obj_keygen(keygen_hdl, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, attr, 1) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to gen key\n");
                return -1;
            }

            /* Get the attributes from the generated keypair object */
            if (qcom_crypto_obj_buf_attrib_get(keygen_hdl, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X, QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to get public val X\n");
                return -1;
            }
            if (qcom_crypto_obj_buf_attrib_get(keygen_hdl, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y, QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to get public val Y\n");
                return -1;
            }

            if (qcom_crypto_obj_buf_attrib_get(keygen_hdl, QCOM_CRYPTO_ATTR_ECC_PRIVATE_VALUE, pvt_key, QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BYTES) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to get public val X\n");
                return -1;
            }

            /* Allocate the keypair object that will be populated using
             * the attributes obtained from above. This object will be
             * used for signing
             */
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDSA_KEYPAIR, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, &keypair_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
                return -1;
            }

            /* Populate attributes */
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_ECC_PRIVATE_VALUE, pvt_key,QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BYTES);
            ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X, QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BYTES);
            ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y, QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BYTES);
            val_attr_init(&attr[3], QCOM_CRYPTO_ATTR_ECC_CURVE, QCOM_CRYPTO_ECC_CURVE_NIST_P256, 0);
            if (qcom_crypto_transient_obj_populate(keypair_hdl, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_ECC_KEYPAIR) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate ecc keypair obj\n");
                return -1;
            }

            /* Allocate public key object for use in verify operation */
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDSA_PUBLIC_KEY, QCOM_CRYPTO_ECC_P256_PUBLIC_KEY_BITS, &public_key_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
                return -1;
            }

            /* Populate public key object */
            if (qcom_crypto_transient_obj_populate(public_key_hdl, &attr[1], QCOM_CRYPTO_OBJ_ATTRIB_COUNT_ECC_PUBLIC_KEY) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate ecc public obj\n");
                return -1;
            }

            /* Allocate sign operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDSA_P256, QCOM_CRYPTO_MODE_SIGN, QCOM_CRYPTO_ECC_P256_PRIVATE_KEY_BITS, &sign_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                return -1;
            }

            /* Copy private key from populated keypair object */
            if (qcom_crypto_op_key_set(sign_hdl, keypair_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }

            /* Allocate verify operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDSA_P256, QCOM_CRYPTO_MODE_VERIFY, QCOM_CRYPTO_ECC_P256_PUBLIC_KEY_BITS, &verify_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc verify op\n");
                return -1;
            }

            /* Copy public key from populated public key object */
            if (qcom_crypto_op_key_set(verify_hdl, public_key_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }

            /* Generate signature */
            if (qcom_crypto_op_sign_digest(sign_hdl, NULL, 0, digest, DIGEST_LEN, sm, &smlen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to sign\n");
                return -1;
            }


            /* Verify signature */
            if (qcom_crypto_op_verify_digest(verify_hdl, NULL, 0, digest, DIGEST_LEN, sm, smlen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed verify\n");
                return -1;
            }

            CRYPTO_PRINTF("\nDemo 2 successful\n");

            qcom_crypto_transient_obj_free(keygen_hdl);
            qcom_crypto_transient_obj_free(keypair_hdl);
            qcom_crypto_transient_obj_free(public_key_hdl);
            qcom_crypto_op_free(sign_hdl);
            qcom_crypto_op_free(verify_hdl);
        }
} 
#if ENABLE_DH_DEMO
A_INT32 dh_demo()
{
        /* 1. Allocate two dh key pair objects 
         * 2. Generate keys
         * 3. Extract public key from each object
         * 4. Allocate two derived key operations
         * 4. Derive shared keys using secret key from obj1 and public key from
         * obj2 and vice versa
         * 5. Verify that shared keys match
         */
        qcom_crypto_obj_hdl_t dh_hdl_1, dh_hdl_2, derived_key_hdl_1, derived_key_hdl_2;
        qcom_crypto_op_hdl_t op_hdl_1, op_hdl_2;
        qcom_crypto_attrib_t attr[QCOM_CRYPTO_OBJ_ATTRIB_COUNT_DH_KEYGEN];
        A_UINT8 pub_key_1[PRIME_BYTES];
        A_UINT8 pub_key_2[PRIME_BYTES];
        A_UINT8 derived_key_1[PRIME_BYTES];
        A_UINT8 derived_key_2[PRIME_BYTES];


        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_DH_KEYPAIR, PRIME_BITS, &dh_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_DH_PRIME, p_1024, PRIME_BYTES);
        ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_DH_BASE, g, 4);
        if (qcom_crypto_transient_obj_keygen(dh_hdl_1, PRIME_BITS, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_DH_KEYGEN) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(dh_hdl_1, QCOM_CRYPTO_ATTR_DH_PUBLIC_VALUE, pub_key_1, PRIME_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_DH_KEYPAIR, PRIME_BITS, &dh_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_keygen(dh_hdl_2, PRIME_BITS, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_DH_KEYGEN) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(dh_hdl_2, QCOM_CRYPTO_ATTR_DH_PUBLIC_VALUE, pub_key_2, PRIME_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val\n");
            return -1;
        }


        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DH_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, PRIME_BITS, &op_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_1, dh_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }


        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DH_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, PRIME_BITS, &op_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_2, dh_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, PRIME_BITS, &derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, PRIME_BITS, &derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }


        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_DH_PUBLIC_VALUE, pub_key_2, PRIME_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_1, &attr[0], 1, derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key 1\n");
            return -1;
        }

        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_DH_PUBLIC_VALUE, pub_key_1, PRIME_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_2, &attr[0], 1, derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key 2\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_1, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_1, PRIME_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val 1\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_2, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_2, PRIME_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val 2\n");
            return -1;
        }

        if (A_MEMCMP(derived_key_1, derived_key_2, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BYTES) == 0) {
            CRYPTO_PRINTF("\nDerived keys match!\n");

        }
        else {
            CRYPTO_PRINTF("\nDerived keys DONOT match!\n");
        }

        qcom_crypto_transient_obj_free(dh_hdl_1);
        qcom_crypto_transient_obj_free(dh_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_1);
        qcom_crypto_op_free(op_hdl_1);
        qcom_crypto_op_free(op_hdl_2);
}
#endif

A_INT32 ecdh_demo()
{
        /* 1. Allocate two ecdh key pair objects 
         * 2. Generate keys
         * 3. Extract public key from each object
         * 4. Allocate two derived key operations
         * 4. Derive shared keys using secret key from obj1 and public key from
         * obj2 and vice versa
         * 5. Verify that shared keys match
         */
        qcom_crypto_obj_hdl_t ecc_hdl_1, ecc_hdl_2, derived_key_hdl_1, derived_key_hdl_2;
        qcom_crypto_op_hdl_t op_hdl_1, op_hdl_2;
        qcom_crypto_attrib_t attr[2];
        A_UINT8 pub_key_X_1[QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES];
        A_UINT8 pub_key_X_2[QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES];
        A_UINT8 pub_key_Y_1[QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES];
        A_UINT8 pub_key_Y_2[QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES];
        A_UINT8 derived_key_1[QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES];
        A_UINT8 derived_key_2[QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES];


        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDH_KEYPAIR, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, &ecc_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        val_attr_init(attr, QCOM_CRYPTO_ATTR_ECC_CURVE, QCOM_CRYPTO_ECC_CURVE_NIST_P256, 0);
        if (qcom_crypto_transient_obj_keygen(ecc_hdl_1, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, attr, 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(ecc_hdl_1, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X_1, QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val X\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(ecc_hdl_1, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y_1, QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val X\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ECDH_KEYPAIR, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, &ecc_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        val_attr_init(attr, QCOM_CRYPTO_ATTR_ECC_CURVE, QCOM_CRYPTO_ECC_CURVE_NIST_P256, 0);
        if (qcom_crypto_transient_obj_keygen(ecc_hdl_2, QCOM_CRYPTO_ECC_P256_KEYPAIR_BITS, attr, 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(ecc_hdl_2, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X_2, QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val X\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(ecc_hdl_2, QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y_2, QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val Y\n");
            return -1;
        }


        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDH_P256, QCOM_CRYPTO_MODE_DERIVE, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BITS, &op_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_1, ecc_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }


        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ECDH_P256, QCOM_CRYPTO_MODE_DERIVE, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BITS, &op_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_2, ecc_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BITS, &derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BITS, &derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }


        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X_2, QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES);
        ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y_2, QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_1, &attr[0], 2, derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key 1\n");
            return -1;
        }

        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_X, pub_key_X_1, QCOM_CRYPTO_ECC_P256_PUB_VAL_X_BYTES);
        ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_ECC_PUBLIC_VALUE_Y, pub_key_Y_1, QCOM_CRYPTO_ECC_P256_PUB_VAL_Y_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_2, &attr[0], 2, derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key 2\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_1, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_1, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val 1\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_2, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_2, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val 2\n");
            return -1;
        }

        if (A_MEMCMP(derived_key_1, derived_key_2, QCOM_CRYPTO_ECC_P256_SHARED_SECRET_BYTES) == 0) {
            CRYPTO_PRINTF("\nDerived keys match!\n");

        }
        else {
            CRYPTO_PRINTF("\nDerived keys DONOT match!\n");
        }

        qcom_crypto_transient_obj_free(ecc_hdl_1);
        qcom_crypto_transient_obj_free(ecc_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_1);
        qcom_crypto_op_free(op_hdl_1);
        qcom_crypto_op_free(op_hdl_2);
}

A_INT32 curve_demo()
{
        /* 1. Allocate two curve25519 key pair objects 
         * 2. Generate keys
         * 3. Extract public key from each object
         * 4. Allocate two derived key operations
         * 4. Derive shared keys using secret key from obj1 and public key from
         * obj2 and vice versa
         * 5. Verify that shared keys match
         */
        qcom_crypto_obj_hdl_t curve_hdl_1, curve_hdl_2, derived_key_hdl_1, derived_key_hdl_2;
        qcom_crypto_op_hdl_t op_hdl_1, op_hdl_2;
        A_UINT8 pub_key_1[QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES];
        A_UINT8 pub_key_2[QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES];
        qcom_crypto_attrib_t attr[2];
        A_UINT8 derived_key_1[QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BYTES];
        A_UINT8 derived_key_2[QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BYTES];


        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_CURVE25519_KEYPAIR, QCOM_CRYPTO_CURVE25519_KEYPAIR_BITS, &curve_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_keygen(curve_hdl_1, QCOM_CRYPTO_CURVE25519_KEYPAIR_BITS, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(curve_hdl_1, QCOM_CRYPTO_ATTR_CURVE25519_PUBLIC_VALUE, pub_key_1, QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val X\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_CURVE25519_KEYPAIR, QCOM_CRYPTO_CURVE25519_KEYPAIR_BITS, &curve_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_keygen(curve_hdl_2, QCOM_CRYPTO_CURVE25519_KEYPAIR_BITS, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to gen key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(curve_hdl_2, QCOM_CRYPTO_ATTR_CURVE25519_PUBLIC_VALUE, pub_key_2, QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get public val X\n");
            return -1;
        }
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_CURVE25519_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BITS, &op_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_1, curve_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_CURVE25519_DERIVE_SHARED_SECRET, QCOM_CRYPTO_MODE_DERIVE, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BITS, &op_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }

        if (qcom_crypto_op_key_set(op_hdl_2, curve_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BITS, &derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }

        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BITS, &derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            return -1;
        }


        ref_attr_init(attr, QCOM_CRYPTO_ATTR_CURVE25519_PUBLIC_VALUE, pub_key_2, QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_1, attr, 1, derived_key_hdl_1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key\n");
            return -1;
        }

        ref_attr_init(attr, QCOM_CRYPTO_ATTR_CURVE25519_PUBLIC_VALUE, pub_key_1, QCOM_CRYPTO_CURVE25519_PUBLIC_KEY_BYTES);
        if (qcom_crypto_op_key_derive(op_hdl_2, attr, 1, derived_key_hdl_2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to derive key\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_1, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_1, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val\n");
            return -1;
        }

        if (qcom_crypto_obj_buf_attrib_get(derived_key_hdl_2, QCOM_CRYPTO_ATTR_SECRET_VALUE, derived_key_2, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BYTES) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to get derived key val\n");
            return -1;
        }

        if (A_MEMCMP(derived_key_1, derived_key_2, QCOM_CRYPTO_CURVE25519_SHARED_SECRET_BYTES) == 0) {
            CRYPTO_PRINTF("\nDerived keys match!\n");

        }
        else {
            CRYPTO_PRINTF("\nDerived keys DONOT match!\n");
        }

        qcom_crypto_transient_obj_free(curve_hdl_1);
        qcom_crypto_transient_obj_free(curve_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_2);
        qcom_crypto_transient_obj_free(derived_key_hdl_1);
        qcom_crypto_op_free(op_hdl_1);
        qcom_crypto_op_free(op_hdl_2);
}

A_INT32 ed25519_demo()
{
#if ENABLE_ED25519_DEMO
        A_UINT32 klen;
        unsigned char skpk[crypto_sign_ed25519_SECRETKEYBYTES];
        qcom_crypto_obj_hdl_t obj_hdl;
        qcom_crypto_op_hdl_t sign_hdl, verify_hdl;
        qcom_crypto_attrib_t attr[2];
        unsigned char sm[crypto_sign_ed25519_BYTES];
        A_UINT32 smlen;
        A_UINT32 i;
        
        for (i = 0; i < (sizeof test_data) / (sizeof test_data[0]); i++) {
        //for (i = 0; i < 1; i++) {
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_ED25519_KEYPAIR, QCOM_CRYPTO_ED25519_PRIVATE_KEY_BITS, &obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
                continue;
            }
           
#if 0
            if (qcom_crypto_transient_obj_keygen(hdl, 64*8, NULL, 0) != A_CRYPTO_OK) {
                printf("\nFailed to gen key\n");
            }
#endif
            klen = i;
            memset(sm, 0, sizeof sm);
            memcpy(skpk, test_data[i].sk, crypto_sign_ed25519_SEEDBYTES);
            memcpy(skpk + crypto_sign_ed25519_SEEDBYTES, test_data[i].pk,
                    crypto_sign_ed25519_PUBLICKEYBYTES);
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_ED25519_PRIVATE_VALUE, skpk, QCOM_CRYPTO_ED25519_PRIVATE_KEY_BYTES);
            ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_ED25519_PUBLIC_VALUE, test_data[i].pk, QCOM_CRYPTO_ED25519_PUBLIC_KEY_BYTES);

            if (qcom_crypto_transient_obj_populate(obj_hdl, attr, 2) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate obj\n");
                continue;
            }

            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ED25519, QCOM_CRYPTO_MODE_SIGN, QCOM_CRYPTO_ED25519_PRIVATE_KEY_BITS, &sign_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                continue;
            }

            if (qcom_crypto_op_key_set(sign_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                continue;
            }

            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_ED25519, QCOM_CRYPTO_MODE_VERIFY, QCOM_CRYPTO_ED25519_PUBLIC_KEY_BITS, &verify_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                continue;
            }

            if (qcom_crypto_op_key_set(verify_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                continue;
            }

            if (qcom_crypto_op_sign_digest(sign_hdl, NULL, 0, test_data[i].m, klen, sm, &smlen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to sign\n");
                continue;
            }

            if (memcmp(test_data[i].sig, sm, crypto_sign_ed25519_BYTES) != 0) {
                CRYPTO_PRINTF("signature failure: [%u]\n", i);
                continue;
            }

            if (qcom_crypto_op_verify_digest(verify_hdl, NULL, 0, test_data[i].m, klen, sm, smlen) != A_CRYPTO_OK) {
                printf("crypto_sign_open() failure: [%u]\n", i);
                continue;
            }

            CRYPTO_PRINTF("ed25519 Case [%u] pass\n", i);
            qcom_crypto_transient_obj_free(obj_hdl);
            qcom_crypto_op_free(sign_hdl);
            qcom_crypto_op_free(verify_hdl);

        }
#endif
}

A_INT32 rsa_demo()
{
    qcom_crypto_obj_hdl_t obj_hdl, obj_pub_hdl;
    qcom_crypto_op_hdl_t sign_hdl, verify_hdl;
    qcom_crypto_attrib_t attr[QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR];
    A_UINT32 rsa_key_size = 1024;
    
#define RSA_DIGEST_LEN (256/8)
    A_UINT8 digest[RSA_DIGEST_LEN];
    A_UINT8 sm[1024/8];
    A_UINT32 smlen;

#if 1
    if (qcom_crypto_rng_get(digest, RSA_DIGEST_LEN) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to get rng\n");
        return -1;
    }
#endif


    /* key pair generate the signature*/
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_RSA_KEYPAIR, rsa_key_size, 
                &obj_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_RSA_MODULUS, rsa_mod, sizeof(rsa_mod));
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_RSA_PUBLIC_EXPONENT, rsa_pub_exp, sizeof(rsa_pub_exp));
    ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_RSA_PRIVATE_EXPONENT, rsa_pvt_exp, sizeof(rsa_pvt_exp));
    ref_attr_init(&attr[3], QCOM_CRYPTO_ATTR_RSA_PRIME1, rsa_prime1, sizeof(rsa_prime1));
    ref_attr_init(&attr[4], QCOM_CRYPTO_ATTR_RSA_PRIME2, rsa_prime2, sizeof(rsa_prime2));
    ref_attr_init(&attr[5], QCOM_CRYPTO_ATTR_RSA_EXPONENT1, rsa_exp1, sizeof(rsa_exp1));
    ref_attr_init(&attr[6], QCOM_CRYPTO_ATTR_RSA_EXPONENT2, rsa_exp2, sizeof(rsa_exp2));
    ref_attr_init(&attr[7], QCOM_CRYPTO_ATTR_RSA_COEFFICIENT, rsa_coeff, sizeof(rsa_coeff));

    if (qcom_crypto_transient_obj_populate(obj_hdl, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to populate rsa keypair obj\n");
        return -1;
    }
    /* Allocate sign operation */
    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSASSA_PKCS1_V1_5_SHA256, QCOM_CRYPTO_MODE_SIGN, rsa_key_size, 
                &sign_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }

    /* Copy private key from keypair object for sign operation */
    if (qcom_crypto_op_key_set(sign_hdl, obj_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set pvt key\n");
        return -1;
    }

    /* Generate signature */
    if (qcom_crypto_op_sign_digest(sign_hdl, NULL, 0, digest, RSA_DIGEST_LEN, sm, &smlen) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to sign\n");
        return -1;
    }

    CRYPTO_PRINTF("\nSign Successful\n");

    /* public key verify the signature*/
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_RSA_PUBLIC_KEY, rsa_key_size, 
                &obj_pub_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_RSA_MODULUS, rsa_mod, sizeof(rsa_mod));
    ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_RSA_PUBLIC_EXPONENT, rsa_pub_exp, sizeof(rsa_pub_exp));

    if (qcom_crypto_transient_obj_populate(obj_pub_hdl, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_PUBLIC_KEY) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to populate rsa public key obj\n");
        return -1;
    }

    /* Allocate verify operation */
    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSASSA_PKCS1_V1_5_SHA256, QCOM_CRYPTO_MODE_VERIFY, rsa_key_size, &verify_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc verify op\n");
        return -1;
    }

    /* Copy public key from public key object for verify operation */
    if (qcom_crypto_op_key_set(verify_hdl, obj_pub_hdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set public key\n");
        return -1;
    }

    /* Verify signature */
    if (qcom_crypto_op_verify_digest(verify_hdl, NULL, 0, digest, RSA_DIGEST_LEN, sm, smlen) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed verify\n");
        return -1;
    }

    CRYPTO_PRINTF("\nVerify Successful\n");

    qcom_crypto_transient_obj_free(obj_hdl);
    qcom_crypto_transient_obj_free(obj_pub_hdl);
    qcom_crypto_op_free(sign_hdl);
    qcom_crypto_op_free(verify_hdl);
    return 0;
}

A_INT32 crypto_aescbc(A_INT32 argc, char* argv[])
{
    //"wmiconfig --cryptotest aescbc 128/256"
        char srcData[32] = {0};
        char encData[32] = {0};    
        char destData[32] = {0};    
        qcom_crypto_attrib_t attr[2];
        A_UINT32 keyLen = atoi(argv[3]);
        A_UINT32 encLen, destLen, srcLen;

        srcLen = 32;
        memcpy(srcData, test_iv, srcLen);
      
        qcom_crypto_obj_hdl_t aes_objHdl;
        qcom_crypto_op_hdl_t aes_encHdl, aes_decHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES256_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
 #if 0       
        if (qcom_crypto_transient_obj_keygen(aes_objHdl, keyLen, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
 #endif
        if (keyLen == QCOM_CRYPTO_AES128_KEY_BITS){        
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES128_KEY_BYTES);
        }    
        else if (keyLen == QCOM_CRYPTO_AES256_KEY_BITS){
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES256_KEY_BYTES);
        }
        else {
            return -1;
        }

         if (qcom_crypto_transient_obj_populate(aes_objHdl, attr, 1) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to populate obj\n");
             return -1;
         }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CBC_NOPAD, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_encHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;            
        }
        
        if (qcom_crypto_op_key_set(aes_encHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CBC_NOPAD, QCOM_CRYPTO_MODE_DECRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_decHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_decHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_init(aes_encHdl, (void*)test_iv, 16) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }

        if (qcom_crypto_op_cipher_update(aes_encHdl, (void*)srcData, srcLen/2, encData, &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 

        if (qcom_crypto_op_cipher_dofinal(aes_encHdl, (void*)&srcData[16], srcLen/2, &encData[16], &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        
#if 0        
        printf("encrypt encData len %d :\n", encLen);
        int i;
        for (i=0; i<encLen; i++)
            printf("%02x, ", encData[i]);
        printf("\n ");   
#endif        
        if (qcom_crypto_op_cipher_init(aes_decHdl, (void*)test_iv, 16) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_update(aes_decHdl, (void*)encData, encLen, destData, &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 

        if (qcom_crypto_op_cipher_dofinal(aes_decHdl, (void*)&encData[16], encLen, &destData[16], &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        //printf("Decrypt destData len %d %s\n", destLen, destData);
        if (memcmp(destData, srcData, destLen)==0){
            CRYPTO_PRINTF("aescbc encryption and decryption success\n");
        }
        else {
            CRYPTO_PRINTF("aescbc encryption and decryption failed\n");
        }

        qcom_crypto_transient_obj_free(aes_objHdl);
        qcom_crypto_op_free(aes_encHdl);
        qcom_crypto_op_free(aes_decHdl);
}

A_INT32 crypto_aesccm()
{
    //"wmiconfig --cryptotest aesccm" 
        qcom_crypto_attrib_t attr[2];
#if 1    
        const A_UINT8 *key = (A_UINT8[]){0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
        A_UINT32 keyLen = 128;
        const A_UINT8 *nonce = (A_UINT8[]){0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b};
        A_UINT32 nonceLen = 12;
        const A_UINT8 *adata = (A_UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13};
        A_UINT32 adataLen = 20;
        A_UINT8 tag[16] = {};
        A_UINT32 tagLen = 0;
        const A_UINT8 *srcData = (A_UINT8[]){0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
        A_UINT32 srcLen = 24;
        A_UINT8 destData[128] = {0};
        A_UINT32 destLen = 0;
         A_UINT8 decData[128] = {0};
        A_UINT32 decLen = 0;    
        
        A_UINT8 goldTag[] = {0x48, 0x43, 0x92, 0xfb, 0xc1, 0xb0, 0x99, 0x51};
        A_UINT32 goldTagLen = 64;
#else
        A_UINT8 key[] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
        A_UINT32 keyLen = 128;
        A_UINT8 nonce[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
        A_UINT32 nonceLen = 8;
        A_UINT8 adata[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
        A_UINT32 adataLen = 16;
        A_UINT8 tag[16] = {};
        A_UINT32 tagLen = 0;
        A_UINT8 srcData[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
        A_UINT32 srcLen = 16;
        A_UINT8 destData[256] = {0};
        A_UINT32 destLen = 0;
         A_UINT8 decData[256] = {0};
        A_UINT32 decLen = 0;    
        
        A_UINT8 goldTag[] = {0x1f, 0xc6, 0x4f, 0xbf,0xac, 0xcd};
        A_UINT32 goldTagLen = 48;
#endif
        qcom_crypto_obj_hdl_t aes_objHdl;
        qcom_crypto_op_hdl_t aes_encHdl, aes_decHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES256_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
#if 0        
        if (qcom_crypto_transient_obj_keygen(aes_objHdl, keyLen, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif       
        if (keyLen == QCOM_CRYPTO_AES128_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, QCOM_CRYPTO_AES128_KEY_BYTES);
        }    
        else if (keyLen == QCOM_CRYPTO_AES256_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES256_KEY_BYTES);
        }
        else {
            return -1;
        }

        if (qcom_crypto_transient_obj_populate(aes_objHdl, attr, 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate obj\n");
            return -1;
        }
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CCM, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_encHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_encHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CCM, QCOM_CRYPTO_MODE_DECRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_decHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_decHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_init(aes_encHdl, (void*)nonce, nonceLen, goldTagLen, adataLen, srcLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae init\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_update_aad(aes_encHdl, (void*)adata, adataLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae update aad\n");
            return -1;
        } 

        if (qcom_crypto_op_ae_encrypt_final(aes_encHdl, (void*)srcData, srcLen, destData, &destLen, tag, &tagLen) != A_CRYPTO_OK){
            CRYPTO_PRINTF("\nFailed to ae encrypto final\n");
            return -1;
        }

#if 0       
        printf("encrypt destData len %d tagLen %d:\n", destLen, tagLen);
        int i;
        for (i=0; i<destLen; i++)
            printf("%02x, ", destData[i]);
        printf("\n ");   

        for (i=0; i<tagLen/8; i++)
            printf("%02x, ", tag[i]);
        printf("\n ");
#endif   

        if (qcom_crypto_op_ae_init(aes_decHdl, (void*)nonce, nonceLen, goldTagLen, adataLen, srcLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae init\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_update_aad(aes_decHdl, (void*)adata, adataLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae update aad\n");
            return -1;
        } 
        
        if (qcom_crypto_op_ae_decrypt_final(aes_decHdl, destData, destLen, decData, &decLen, goldTag, goldTagLen) != A_CRYPTO_OK){
            CRYPTO_PRINTF("\nFailed to ae decrypto final\n");
            return -1;
        }
        else{
#if 0   
            printf("decrypt destData len %d\n", decLen);
            for (i=0; i<decLen; i++)
                printf("%02x, ", decData[i]);
            printf("\n ");
#endif    
             if ((decLen==srcLen) && (memcmp(decData, srcData, decLen)==0))
                CRYPTO_PRINTF("aesccm decryption and decryption success\n");
             else
                CRYPTO_PRINTF("aesccm decryption and decryption fail\n");
        }

        qcom_crypto_transient_obj_free(aes_objHdl);
        qcom_crypto_op_free(aes_encHdl);
        qcom_crypto_op_free(aes_decHdl);
}

A_INT32 crypto_aesgcm(A_INT32 argc, char* argv[])
{
    //"wmiconfig --cryptotest aesgcm  <keyLen>  <xxx> "  keyLen: 128/256,  xxx:test data
        char srcData[128] = {0};
        char originalData[128] = {0};    
        qcom_crypto_attrib_t attr[2];
        A_UINT32 nonceLen = QCOM_CRYPTO_AE_AES_GCM_NONCEBYTES; //fixed in SharkSSL.
        A_UINT32 adataLen=16;  
        A_UINT8 tag[16] = {0};
  
        A_UINT32 keyLen = atoi(argv[3]);
        A_UINT32 tagLen = 128;   //just support 128          
        A_UINT32 srcLen = strlen(argv[4]);
        A_UINT32 destLen = 0;

        if (srcLen>128){
            CRYPTO_PRINTF("\n The max length of data is 128\n");
            return -1;
        }
        
        memcpy(srcData, argv[4], srcLen);
        memcpy(originalData, argv[4], srcLen);
         
        qcom_crypto_obj_hdl_t aes_objHdl;
        qcom_crypto_op_hdl_t aes_encHdl, aes_decHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES256_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
#if 0        
        if (qcom_crypto_transient_obj_keygen(aes_objHdl, keyLen, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif
        if (keyLen == QCOM_CRYPTO_AES128_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES128_KEY_BYTES);
        }    
        else if (keyLen == QCOM_CRYPTO_AES256_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES256_KEY_BYTES);
        }

        if (qcom_crypto_transient_obj_populate(aes_objHdl, attr, 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate obj\n");
            return -1;
        }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_GCM, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_encHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_encHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_GCM, QCOM_CRYPTO_MODE_DECRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_decHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_decHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_init(aes_encHdl, (void*)test_iv, nonceLen, 128, 0, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae init\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_update_aad(aes_encHdl, (void*)test_iv, adataLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae update aad\n");
            return -1;
        } 

        if (qcom_crypto_op_ae_encrypt_final(aes_encHdl, srcData, srcLen, srcData, &destLen, tag, &tagLen) != A_CRYPTO_OK){
            CRYPTO_PRINTF("\nFailed to ae encrypto final\n");
            return -1;
        }
        
#if 0        
        printf("encrypt srcData len %d tagLen %d:\n", destLen, tagLen);
        int i;
        for (i=0; i<destLen; i++)
            printf("%02x, ", srcData[i]);
        printf("\n ");   

        for (i=0; i<tagLen/8; i++)
            printf("%02x, ", tag[i]);
        printf("\n ");
#endif        
        if (qcom_crypto_op_ae_init(aes_decHdl, (void*)test_iv, nonceLen, tagLen, adataLen, srcLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae init\n");
            return -1;
        }
        
        if (qcom_crypto_op_ae_update_aad(aes_decHdl, (void*)test_iv, adataLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to ae update aad\n");
            return -1;
        } 

        if (qcom_crypto_op_ae_decrypt_final(aes_decHdl, srcData, srcLen, srcData, &destLen, tag, tagLen) != A_CRYPTO_OK){
            CRYPTO_PRINTF("\nFailed to ae decrypto final\n");
            return -1;
        }
        else{
            //printf("Decrypt srcData len %d %s\n", destLen, srcData);
            if (memcmp(originalData, srcData, destLen)==0){
                CRYPTO_PRINTF("aesgcm decryption and decryption success\n");
            }
        } 
       
        qcom_crypto_transient_obj_free(aes_objHdl);
        qcom_crypto_op_free(aes_encHdl);
        qcom_crypto_op_free(aes_decHdl);
}


#define QCOM_CRYPTO_CHACHA20_POLY1305_ENABLE_RANDOM_SIZE_UPDATES 1

#if QCOM_CRYPTO_CHACHA20_POLY1305_ENABLE_RANDOM_SIZE_UPDATES 

    typedef struct _chunk_info {
        unsigned int start_index;
        unsigned int length;
    } chunk_info_t;

    unsigned int get_pseudo_random_number(unsigned int lower_bound, unsigned int upper_bound)
    {
        if ( upper_bound < lower_bound ) {
            CRYPTO_PRINTF("\nget_pseudo_random_number: upper bound must be at least equal to lower bound\n");
            return lower_bound;
        }
        if ( upper_bound == lower_bound ) {
            return lower_bound;
        }
        unsigned int temp_number;
        if (qcom_crypto_rng_get((void *) &temp_number, sizeof(temp_number)) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nget_pseudo_random_number: failed on a call to qcom_crypto_rng_get\n");
            return lower_bound;
        }
        return ( (temp_number % (upper_bound-lower_bound)) + lower_bound );
    }

    void print_chunks_info(chunk_info_t * chunks, unsigned int num_chunks)
    {
        int chunk_index;
        CRYPTO_PRINTF("Chunks: ");
        for ( chunk_index = 0; chunk_index < num_chunks; chunk_index++ ) {
            CRYPTO_PRINTF("(%d, %d) ", chunks[chunk_index].start_index, chunks[chunk_index].length);
        }
        CRYPTO_PRINTF("\n");
    }

    void randomize_chunks(chunk_info_t * p_chunks, unsigned int chunks_count, unsigned int total_length)
    {
        if ( chunks_count == 0 ) {
            CRYPTO_PRINTF("\nrandomize_chunks(): chunks_count must be greater than 0\n");
            return;
        }
        int i;
        unsigned int start_index = 0;
        unsigned int remaining_length = total_length;
        for ( i = 0; i < chunks_count-1; i++ ) {
            unsigned int chunk_length = get_pseudo_random_number(0, remaining_length);
            p_chunks[i].start_index = start_index;
            p_chunks[i].length = chunk_length;
            remaining_length -= chunk_length;
            start_index += chunk_length;
        }
        p_chunks[chunks_count-1].start_index = start_index;
        p_chunks[chunks_count-1].length = remaining_length;
        start_index += remaining_length;
        if ( total_length != start_index ) {
            CRYPTO_PRINTF("\nrandomize_chunks(): internal error\n");
        }
    }
#else
    #define print_chunks_info(a, b) void(a), void(b)
#endif

#define A_COMPILE_TIME_ASSERT(assertion_name, predicate) \
    typedef char assertion_name[(predicate) ? 1 : -1];


A_INT32 crypto_chacha20_poly1305(A_INT32 argc, char* argv[])
{
    const unsigned int key_length = 32;
    static unsigned char key[] = { 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f };
    A_COMPILE_TIME_ASSERT(_key_length_check, (sizeof(key) == (key_length+1)))

    const unsigned int nonce_length = 12;
    static unsigned char nonce[] = { 0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 };
    A_COMPILE_TIME_ASSERT(_nonce_length_check, (sizeof(nonce) == (nonce_length+1)))

    const unsigned int aad_length = 12;
    static unsigned char aad[] = { 0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7 };
    A_COMPILE_TIME_ASSERT(_aad_length_check, (sizeof(aad) == (aad_length+1)))

    const unsigned int plaintext_length = 114;
    static unsigned char plaintext[] = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    A_COMPILE_TIME_ASSERT(_plaintext_length_check, (sizeof(plaintext) == (plaintext_length+1)))

    const unsigned int expected_ciphertext_length = 114;
    static unsigned char expected_ciphertext[] = { 0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2, 0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6, 0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b, 0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc, 0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16 };
    A_COMPILE_TIME_ASSERT(_expected_ciphertext_length_check, (sizeof(expected_ciphertext) == (expected_ciphertext_length+1)))

    const unsigned int expected_tag_length = 16;
    static unsigned char expected_tag[] = { 0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a, 0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91 };
    A_COMPILE_TIME_ASSERT(_expected_tag_length_check, (sizeof(expected_tag) == (expected_tag_length+1)))

    unsigned char ciphertext[expected_ciphertext_length];
    unsigned int ciphertext_length = expected_ciphertext_length;


    unsigned char tag[expected_tag_length];
    unsigned int tag_length = expected_tag_length;

    qcom_crypto_attrib_t attr[2];
    qcom_crypto_obj_hdl_t aes_objHdl;
    qcom_crypto_op_hdl_t aes_encHdl, aes_decHdl;

    //printf("srcData len %d %s\n", srcLen, srcData);
    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_CHACHA20, QCOM_CRYPTO_CHACHA20_POLY1305_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
    //if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_GENERIC_SECRET, QCOM_CRYPTO_CHACHA20_POLY1305_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\n chacha20_poly1305 Failed to alloc transient obj\n");
        return -1;
    }

    ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, key, QCOM_CRYPTO_CHACHA20_POLY1305_KEY_BYTES);

    if (qcom_crypto_transient_obj_populate(aes_objHdl, attr, 1) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to populate obj\n");
        return -1;
    }

    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_CHACHA20_POLY1305, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_CHACHA20_POLY1305_KEY_BITS, &aes_encHdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }
    
    if (qcom_crypto_op_key_set(aes_encHdl, aes_objHdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set key\n");
        return -1;
    }
    

    if (qcom_crypto_op_ae_init(aes_encHdl, (void*)nonce, nonce_length, tag_length, aad_length, plaintext_length) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to ae init\n");
        return -1;
    }
    
    A_CRYPTO_STATUS status_code;

#if QCOM_CRYPTO_CHACHA20_POLY1305_ENABLE_RANDOM_SIZE_UPDATES
    const unsigned int num_aad_chunks = 4;
    chunk_info_t aad_chunks[num_aad_chunks];
    randomize_chunks(aad_chunks, num_aad_chunks, aad_length);
    int aad_chunk_index;
    for ( aad_chunk_index = 0; aad_chunk_index < num_aad_chunks; aad_chunk_index++ ) {
        status_code =
            qcom_crypto_op_ae_update_aad(
                aes_encHdl,
                (void*) &aad[aad_chunks[aad_chunk_index].start_index],
                aad_chunks[aad_chunk_index].length
                );

        if ( status_code != A_CRYPTO_OK ) {
            CRYPTO_PRINTF("\ncrypto_poly1305: failed on aad_update()\n");
            print_chunks_info(aad_chunks, num_aad_chunks);
            return -1;
        }
    }

#else
    status_code = qcom_crypto_op_ae_update_aad(aes_encHdl, (void*)aad, aad_length);
#endif

    if ( status_code != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to ae update aad\n");
        return -1;
    } 


#if QCOM_CRYPTO_CHACHA20_POLY1305_ENABLE_RANDOM_SIZE_UPDATES
    const unsigned int num_chunks = 3;
    chunk_info_t chunks[num_chunks];
    randomize_chunks(chunks, num_chunks, plaintext_length);
    int chunk_index;
    ciphertext_length = 0;
    unsigned char buffer[plaintext_length];
    unsigned int buffer_length = plaintext_length;
    for ( chunk_index = 0; chunk_index < num_chunks-1; chunk_index++ ) {
        buffer_length = plaintext_length;
        status_code =
            qcom_crypto_op_ae_update(
                aes_encHdl, 
                &plaintext[chunks[chunk_index].start_index],
                chunks[chunk_index].length,
                buffer,
                &buffer_length
                );
        if ( status_code != A_CRYPTO_OK ) {
            CRYPTO_PRINTF("\ncrypto_poly1305: failed on update()\n");
            print_chunks_info(chunks, num_chunks);
            return -1;
        }
        if ( buffer_length > 0 ) {
            memcpy(&ciphertext[ciphertext_length], buffer, buffer_length);
            ciphertext_length += buffer_length;
        }
    }

    buffer_length = plaintext_length;
    status_code =
        qcom_crypto_op_ae_encrypt_final(
            aes_encHdl,
            &plaintext[chunks[num_chunks-1].start_index],
            chunks[num_chunks-1].length,
            buffer,
            &buffer_length,
            tag,
            &tag_length
            );
    if ( buffer_length > 0 ) {
        memcpy(&ciphertext[ciphertext_length], buffer, buffer_length);
        ciphertext_length += buffer_length;
    }
#else
    status_code = qcom_crypto_op_ae_encrypt_final(aes_encHdl, plaintext, plaintext_length, ciphertext, &ciphertext_length, tag, &tag_length);
#endif

    if ( status_code != A_CRYPTO_OK){
        CRYPTO_PRINTF("\nFailed to ae encrypto final\n");
        print_chunks_info(chunks, num_chunks);
        return -1;
    }
    else
    {
        if ( 1 &&
            (tag_length == expected_tag_length) &&
            (0 == memcmp(expected_tag, tag, tag_length))
           )
        {
            CRYPTO_PRINTF("\nEncryption tag is CORRECT\n");
        }
        else
        {
            CRYPTO_PRINTF("\nEncryption tag is WRONG\n");
            print_chunks_info(chunks, num_chunks);
           return -1;
        }
        if ( 1 &&
            (ciphertext_length == expected_ciphertext_length) &&
            (0 == memcmp(expected_ciphertext, ciphertext, ciphertext_length))
           )
        {
            CRYPTO_PRINTF("\nEncryption ciphertext is CORRECT\n");
        }
        else
        {
            CRYPTO_PRINTF("\nEncryption ciphertext is WRONG\n");
            print_chunks_info(chunks, num_chunks);
            return -1;
        }
    }

    unsigned char decrypted_plaintext[plaintext_length];
    unsigned int decrypted_plaintext_length = plaintext_length;

    // decryption
    if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_CHACHA20_POLY1305, QCOM_CRYPTO_MODE_DECRYPT, QCOM_CRYPTO_CHACHA20_POLY1305_KEY_BITS, &aes_decHdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to alloc op\n");
        return -1;
    }
    
    if (qcom_crypto_op_key_set(aes_decHdl, aes_objHdl) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to set key\n");
        return -1;
    }
    
    
    if (qcom_crypto_op_ae_init(aes_decHdl, (void*)nonce, nonce_length, tag_length, aad_length, ciphertext_length) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to ae init\n");
        return -1;
    }
    
    if (qcom_crypto_op_ae_update_aad(aes_decHdl, (void*)aad, aad_length) != A_CRYPTO_OK) {
        CRYPTO_PRINTF("\nFailed to ae update aad\n");
        return -1;
    } 

#if QCOM_CRYPTO_CHACHA20_POLY1305_ENABLE_RANDOM_SIZE_UPDATES
    randomize_chunks(chunks, num_chunks, plaintext_length);
    decrypted_plaintext_length = 0;
    buffer_length = plaintext_length;
    for ( chunk_index = 0; chunk_index < num_chunks-1; chunk_index++ ) {
        buffer_length = plaintext_length;
        status_code =
            qcom_crypto_op_ae_update(
                aes_decHdl, 
                &ciphertext[chunks[chunk_index].start_index],
                chunks[chunk_index].length,
                buffer,
                &buffer_length
                );
        if ( status_code != A_CRYPTO_OK ) {
            CRYPTO_PRINTF("\ncrypto_poly1305: decryption failed on update()\n");
            print_chunks_info(chunks, num_chunks);
            return -1;
        }
        if ( buffer_length > 0 ) {
            memcpy(&decrypted_plaintext[decrypted_plaintext_length], buffer, buffer_length);
            decrypted_plaintext_length += buffer_length;
        }
    }

    buffer_length = plaintext_length;
    status_code =
        qcom_crypto_op_ae_decrypt_final(
            aes_decHdl,
            &ciphertext[chunks[num_chunks-1].start_index],
            chunks[num_chunks-1].length,
            buffer,
            &buffer_length,
            tag,
            tag_length
            );
    if ( buffer_length > 0 ) {
        memcpy(&decrypted_plaintext[decrypted_plaintext_length], buffer, buffer_length);
        decrypted_plaintext_length += buffer_length;
    }
#else
    status_code = qcom_crypto_op_ae_decrypt_final(aes_decHdl, ciphertext, ciphertext_length, decrypted_plaintext, &decrypted_plaintext_length, tag, tag_length);
#endif

    if ( status_code != A_CRYPTO_OK){
        CRYPTO_PRINTF("\nFailed to ae decrypto final\n");
        print_chunks_info(chunks, num_chunks);
        return -1;
    }
    else
    {
        if (1 &&
            (decrypted_plaintext_length == plaintext_length) &&
            (0 == memcmp(plaintext, decrypted_plaintext, plaintext_length))
           )
        {
            CRYPTO_PRINTF("\nDecryption SUCCEEDED\n");
        }
        else
        {
            CRYPTO_PRINTF("\nDecryption FAILED\n");
            print_chunks_info(chunks, num_chunks);
            return -1;
        }
    }

    qcom_crypto_transient_obj_free(aes_objHdl);
    qcom_crypto_op_free(aes_encHdl);
    qcom_crypto_op_free(aes_decHdl);

    return 0;
}


A_INT32 crypto_aesctr(A_INT32 argc, char* argv[])
{
    //"wmiconfig --cryptotest aesctr 128/256 "
        char srcData[64] = {0};
        char encData[64] = {0};       
        char destData[64] = {0};
        
        qcom_crypto_attrib_t attr[2];
        
        A_UINT32 keyLen = atoi(argv[3]);
        A_UINT32 srcLen = 60;
        A_UINT32 destLen, encLen, t_enLen, t_destLen;
        
       
        memcpy(srcData, test_iv, 32);
        memcpy(&srcData[32], test_iv, 32);
        
        qcom_crypto_obj_hdl_t aes_objHdl;
        qcom_crypto_op_hdl_t aes_encHdl, aes_decHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES256_KEY_BITS, &aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
 #if 0       
        if (qcom_crypto_transient_obj_keygen(aes_objHdl, keyLen, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
 #endif
        if (keyLen == QCOM_CRYPTO_AES128_KEY_BITS){        
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES128_KEY_BYTES);
        }    
        else if (keyLen == QCOM_CRYPTO_AES256_KEY_BITS){
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_AES256_KEY_BYTES);
        }
        else {
            return -1;
        }

         if (qcom_crypto_transient_obj_populate(aes_objHdl, attr, 1) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to populate obj\n");
             return -1;
         }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CTR, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_encHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;            
        }
        
        if (qcom_crypto_op_key_set(aes_encHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CTR, QCOM_CRYPTO_MODE_DECRYPT, QCOM_CRYPTO_AES256_KEY_BITS, &aes_decHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
        
        if (qcom_crypto_op_key_set(aes_decHdl, aes_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_init(aes_encHdl, (void*)test_iv, 16) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_update(aes_encHdl, (void*)srcData, 20, encData, &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        t_enLen = encLen;
        if (qcom_crypto_op_cipher_update(aes_encHdl, (void*)&srcData[20], 20, &encData[16], &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        t_enLen+=encLen;
        if (qcom_crypto_op_cipher_dofinal(aes_encHdl, (void*)&srcData[40], srcLen-40, &encData[32], &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        t_enLen+=encLen;
#if 0       
        printf("encrypt encData len %d :\n", t_enLen);
        int i;
        for (i=0; i<t_enLen; i++)
            printf("%02x, ", encData[i]);
        printf("\n ");   
#endif        
        if (qcom_crypto_op_cipher_init(aes_decHdl, (void*)test_iv, 16) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_update(aes_decHdl, (void*)encData, 30, destData, &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        t_destLen = destLen;
        if (qcom_crypto_op_cipher_update(aes_decHdl, (void*)&encData[30], 30, &destData[16], &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        t_destLen += destLen;
        if (qcom_crypto_op_cipher_dofinal(aes_decHdl, encData, 0, &destData[48], &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        t_destLen += destLen;
#if 0       
        printf("encrypt decData len %d :\n", t_destLen );
        for (i=0; i<t_destLen ; i++)
            printf("%02x, ", destData[i]);
        printf("\n ");   
#endif        
        if (memcmp(srcData, destData, srcLen)==0){
            CRYPTO_PRINTF("aesctr encryption and decryption success\n");
        }
        else {
            CRYPTO_PRINTF("aesctr encryption and decryption failed\n");
        }

        qcom_crypto_transient_obj_free(aes_objHdl);
        qcom_crypto_op_free(aes_encHdl);
        qcom_crypto_op_free(aes_decHdl);
}

#if CRYPTO_CONFIG_3DES
A_INT32 crypto_descbc(A_INT32 argc, char* argv[])
{ 
    //"wmiconfig --cryptotest des/3des"
        char srcData[16] = {0};
        char encData[16] = {0};    
        char destData[16] = {0};    
        
        qcom_crypto_attrib_t attr[2];
        A_UINT32 keyLen;
        A_UINT32 srcLen = 16;
        A_UINT32 encLen, destLen;
        
        if (strcmp(argv[2], "des") == 0)
            keyLen = QCOM_CRYPTO_DES_KEY_BITS;
        else if (strcmp(argv[2], "3des") == 0)
            keyLen = QCOM_CRYPTO_DES3_KEY_BITS;
        
        memcpy(srcData, test_iv, srcLen);
        
        qcom_crypto_obj_hdl_t des_objHdl;
        qcom_crypto_op_hdl_t des_encHdl, des_decHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (keyLen == QCOM_CRYPTO_DES_KEY_BITS)
            qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_DES, QCOM_CRYPTO_DES_KEY_BITS, &des_objHdl);
        else if (keyLen == QCOM_CRYPTO_DES3_KEY_BITS)
            qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_DES3, QCOM_CRYPTO_DES3_KEY_BITS, &des_objHdl);

#if 0
        if (qcom_crypto_transient_obj_keygen(des_objHdl, keyLen, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif

        if (keyLen == QCOM_CRYPTO_DES_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_DES_KEY_BYTES);
        }    
        else if (keyLen == QCOM_CRYPTO_DES3_KEY_BITS){            
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, test_iv, QCOM_CRYPTO_DES3_KEY_BYTES);
        }else {
            return -1;
        }

        if (qcom_crypto_transient_obj_populate(des_objHdl, attr, 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate obj\n");
            return -1;
        }

        if (keyLen == QCOM_CRYPTO_DES_KEY_BITS)
            qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DES_CBC_NOPAD, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_DES_KEY_BITS, &des_encHdl);
        else if (keyLen == QCOM_CRYPTO_DES3_KEY_BITS)
            qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DES3_CBC_NOPAD, QCOM_CRYPTO_MODE_ENCRYPT, QCOM_CRYPTO_DES3_KEY_BITS, &des_encHdl);
                
        if (qcom_crypto_op_key_set(des_encHdl, des_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (keyLen == QCOM_CRYPTO_DES_KEY_BITS)
            qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DES_CBC_NOPAD, QCOM_CRYPTO_MODE_DECRYPT, keyLen, &des_decHdl);
        else if (keyLen == QCOM_CRYPTO_DES3_KEY_BITS)
            qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_DES3_CBC_NOPAD, QCOM_CRYPTO_MODE_DECRYPT, keyLen, &des_decHdl);
        
        if (qcom_crypto_op_key_set(des_decHdl, des_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_init(des_encHdl, (void*)test_iv, 8) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_update(des_encHdl, (void*)srcData, srcLen/2, encData, &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        
        if (qcom_crypto_op_cipher_dofinal(des_encHdl, (void*)&srcData[8], srcLen/2, &encData[8], &encLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        
#if 0   
        printf("encrypt encData len %d :\n", encLen);
        int i;
        for (i=0; i<encLen; i++)
            printf("%02x, ", encData[i]);
        printf("\n ");   
#endif        
        if (qcom_crypto_op_cipher_init(des_decHdl, (void*)test_iv, 8) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher init\n");
            return -1;
        }
        
        if (qcom_crypto_op_cipher_update(des_decHdl, (void*)encData, encLen, destData, &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher update\n");
            return -1;
        } 
        
        if (qcom_crypto_op_cipher_dofinal(des_decHdl, (void*)&encData[8], encLen, &destData[8], &destLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to cipher doFinal\n");
            return -1;
        } 
        else{
            //printf("Decrypt destData len %d %s\n", destLen, destData);
            if (memcmp(destData, srcData, destLen)==0){
                CRYPTO_PRINTF("des/3des encryption and decryption success\n");
            }
            else {
                CRYPTO_PRINTF("des/3des encryption and decryption fail\n");
            }
        }       

        qcom_crypto_transient_obj_free(des_objHdl);
        qcom_crypto_op_free(des_encHdl);
        qcom_crypto_op_free(des_decHdl);
}
#endif
A_INT32 crypto_hmacsha256()
{
    //"wmiconfig --cryptotest hmacsha256 " 
    int ret = A_CRYPTO_OK;
#if 0    
        const  A_UINT8 *msg = (A_UINT8[]){0x54, 0x65, 0x73, 0x74, 0x20, 0x55, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x4c, 0x61, 0x72, 0x67, 0x65, 0x72, 0x20, 0x54, 0x68, 0x61, 0x6e, 0x20, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x53, 0x69, 0x7a, 0x65, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x2d, 0x20, 0x48, 0x61, 0x73, 0x68, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x46, 0x69, 0x72, 0x73, 0x74};
        const  A_UINT8 *key = (A_UINT8[]){0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
        const  A_UINT8 *hmac = (A_UINT8[]){0x17, 0xc7, 0x97, 0xd4, 0xdc, 0xa7, 0x71, 0xbb, 0x78, 0x27, 0x8b, 0x21, 0xef, 0x40, 0x67, 0x51, 0xb8, 0x93, 0x17, 0x9f, 0xdf, 0x3a, 0xeb, 0xb7, 0xdb, 0x0c, 0x04, 0x81, 0xfa, 0x6f, 0x2c, 0x37};
        A_UINT32 msgLen = 54;
        A_UINT32 keyLen = 36*8;

          const A_UINT8 *msg = (A_UINT8[]){0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74,\
                                             0x20, 0x75, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x61, 0x20, 0x6c, 0x61, 0x72, 0x67, 0x65, 0x72,\
                                             0x20, 0x74, 0x68, 0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x73, 0x69, 0x7a, 0x65, \
                                             0x20, 0x6b, 0x65, 0x79, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x61, 0x20, 0x6c, 0x61, 0x72, 0x67, 0x65, 0x72, \
                                             0x20, 0x74, 0x68, 0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x64, \
                                             0x61, 0x74, 0x61, 0x2e, 0x20, 0x54, 0x68, 0x65, 0x20, 0x6b, 0x65, 0x79, 0x20, 0x6e, 0x65, 0x65, 0x64, 0x73, \
                                             0x20, 0x74, 0x6f, 0x20, 0x62, 0x65, 0x20, 0x68, 0x61, 0x73, 0x68, 0x65, 0x64, 0x20, 0x62, 0x65, 0x66, 0x6f, \
                                             0x72, 0x65, 0x20, 0x62, 0x65, 0x69, 0x6e, 0x67, 0x20, 0x75, 0x73, 0x65, 0x64, 0x20, 0x62, 0x79, 0x20, 0x74,\
                                             0x68, 0x65, 0x20, 0x48, 0x4d, 0x41, 0x43, 0x20, 0x61, 0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x2e};

           const A_UINT8 *key = (A_UINT8[]){0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, \
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,\
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,\
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, \
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,\
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, \
                                        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
            const A_UINT8 *hmac = (A_UINT8[]){0x18, 0xe6, 0xdf, 0x57, 0x25, 0xd4, 0x21, 0x7b, 0x30, 0xff, 0x15, 0x86, 0x0c, 0x1e, 0x32, 0xf4, 0x60, 0x1b, 0x03, 0xca, 0xe3, 0x65, 0x9e, 0x48, 0xef, 0x93, 0x36, 0x6f, 0xc9, 0x19, 0x00, 0x6c};
            // A_UINT8 hkey[] = {0x6a, 0x27, 0x5e, 0x92, 0xd3, 0x56, 0x58, 0x78, 0x21, 0xfd, 0x00, 0x00, 0x56, 0x50, 0x5f, 0x73, 0x33, 0x87, 0x2a, 0xc1, 0x37, 0x8d, 0x82, 0xc3, 0x3c, 0x49, 0xcd, 0x9b, 0x1f, 0x67, 0x77, 0x68};
             A_UINT32 msgLen = 152;
             A_UINT32 keyLen = 122*8;
#endif
        A_UINT8* msg = swat_mem_malloc(384);
        A_UINT8 *key = msg;
        //A_UINT8 hmac[32] = (A_UINT8[]){0x17, 0xc7, 0x97, 0xd4, 0xdc, 0xa7, 0x71, 0xbb, 0x78, 0x27, 0x8b, 0x21, 0xef, 0x40, 0x67, 0x51, 0xb8, 0x93, 0x17, 0x9f, 0xdf, 0x3a, 0xeb, 0xb7, 0xdb, 0x0c, 0x04, 0x81, 0xfa, 0x6f, 0x2c, 0x37};
        A_UINT32 keyLen = 0;
        char mac[32] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];

        A_UINT8 i;
        A_UINT32 testLen[4][5] = {{70, 90, 160, 60, 24*8},{30, 32, 1, 1,46*8},{120,72, 64, 128, 64*8}, {64,64, 64, 128,128*8}};
        for (i=0;  i<384/32; i++){
            memcpy(&msg[i*32], test_iv, 32);
        }
        
        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        for (i=0; i<4; i++){
            keyLen = testLen[i][4];
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_SHA256, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            }
#if 0        
            if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\naes Failed to gen key\n");
            }
#endif
            if (keyLen<QCOM_CRYPTO_HMAC_SHA256_MIN_KEY_BITS || keyLen>QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS){
                CRYPTO_PRINTF("\nNot valid key length\n");
            }
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);
          
            if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
            }
         
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA256, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
           
            if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }
     
            if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
            }
                        
            int len = testLen[i][0]+testLen[i][1] +testLen[i][2] + testLen[i][3];
            if (i<2){
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)msg, testLen[i][0]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0]+testLen[i][1]], testLen[i][2]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                            
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)&msg[len - testLen[i][3]], testLen[i][3], mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                } 
            }
            else {
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg, len, mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                } 
            }
#if 0        
            printf("hmacsha256 mac len %d :\n", macLen);
            int j;
            for (j=0; j<macLen; j++)
                printf("%02x, ", mac[j]);
            printf("\n ");   
#endif   
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA256, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
           
            if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
            }
            if (i>=2){  
                if (qcom_crypto_op_mac_update(compare_opHdl, (void*)msg, testLen[i][0]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0]+testLen[i][1]], testLen[i][2]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                            
                ret = qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)&msg[len - testLen[i][3]], testLen[i][3], mac, macLen);
            }
            else {
                ret = qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, len, (void*)mac, macLen);
            }
            CRYPTO_PRINTF("Case %d: ", i);
            if (ret == A_CRYPTO_ERROR){
                 CRYPTO_PRINTF("Failed to mac compare final\n");
             } 
             else if (ret == A_CRYPTO_ERROR_MAC_INVALID){
                 CRYPTO_PRINTF("Invalid mac to mac compare final\n");
             }
             else {
                 CRYPTO_PRINTF("hmacsha256 compare success\n");
             }
            qcom_crypto_transient_obj_free(mac_objHdl);
            qcom_crypto_op_free(compute_opHdl);
            qcom_crypto_op_free(compare_opHdl);
        }
        qcom_mem_free(msg);
        return 0;
}

A_INT32 crypto_hmacsha1(A_INT32 argc, char* argv[])
{
    //"wmiconfig --cryptotest hmacsha1 " 
    int ret = A_CRYPTO_OK;
#if 0    
        const A_UINT8 *msg = (A_UINT8[]){0x53, 0x61, 0x6D, 0x70, 0x6C, 0x65 , 0x20, 0x6D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x6B, 0x65, 0x79, 0x6C, 0x65, 0x6E, 0x3C, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x6C, 0x65, 0x6E, 0x2C, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x74, 0x72, 0x75, 0x6E, 0x63, 0x61, 0x74, 0x65, 0x64, 0x20, 0x74, 0x61, 0x67};
        const A_UINT8 *key = (A_UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30};
        const A_UINT8 *hmac = (A_UINT8[]){0xFE, 0x35, 0x29, 0x56, 0x5C, 0xD8, 0xE2, 0x8C, 0x5F, 0xA7, 0x9E, 0xAC, 0x9D, 0x80, 0x23, 0xB5, 0x3B, 0x28, 0x9D, 0x96};
        A_UINT32 msgLen = 54;
        A_UINT32 keyLen =49*8;

        const char *msg = "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data";
        A_UINT8 key[80] = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};
        //const A_UINT8 *hmac = (A_UINT8[]){0xe8, 0xe9, 0x9d, 0x0f, 0x45, 0x23, 0x7d, 0x78, 0x6d, 0x6b, 0xba, 0xa7, 0x96, 0x5c, 0x78, 0x08, 0xbb, 0xff, 0x1a, 0x91};
        A_UINT32 msgLen = 73;
        A_UINT32 keyLen =50*8;
#endif
        A_UINT8* msg = swat_mem_malloc(384);

        A_UINT8 *key = msg;
        A_UINT32 keyLen = 0;
        char mac[32] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];
        
        A_UINT8 i;
        A_UINT32 testLen[4][5] = {{70, 90, 160, 60, 10*8},{30, 32, 1, 1, 30*8},{120,72, 64, 128, 48*8}, {64,64, 64, 128,64*8}};
        for (i=0;  i<384/32; i++){
            memcpy(&msg[i*32], test_iv, 32);
        }
        
        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        for (i=0; i<4; i++){
            keyLen = testLen[i][4];
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_SHA1, QCOM_CRYPTO_HMAC_SHA1_MAX_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            }
#if 0        
            if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\naes Failed to gen key\n");
            }
#endif
            if (keyLen<QCOM_CRYPTO_HMAC_SHA1_MIN_KEY_BITS || keyLen>QCOM_CRYPTO_HMAC_SHA1_MAX_KEY_BITS){
                CRYPTO_PRINTF("\nUnvalid key length\n");
            }
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);

            if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
            }

            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA1, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA1_MAX_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
           
            if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }
     
            if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
            }
            
            int len = testLen[i][0]+testLen[i][1] +testLen[i][2] + testLen[i][3];
            if (i<2){
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)msg, testLen[i][0]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                
                if (qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0]+testLen[i][1]], testLen[i][2]) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                }
                            
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)&msg[len - testLen[i][3]], testLen[i][3], mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                } 
            }   
            else {
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg, len, mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                } 
            }
#if 0        
            printf("hmacsha1 mac len %d :\n", macLen);
            int j;
            for (j=0; j<macLen; j++)
                printf("%02x, ", mac[j]);
            printf("\n ");   
#endif   
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA1, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA1_MAX_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
           
            if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
            }

            if (i>=2){
               if (qcom_crypto_op_mac_update(compare_opHdl, (void*)msg, testLen[i][0]) != A_CRYPTO_OK) {
                   CRYPTO_PRINTF("\nFailed to mac compute final\n");
               }
               
               if (qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]) != A_CRYPTO_OK) {
                   CRYPTO_PRINTF("\nFailed to mac compute final\n");
               }
               
               if (qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0]+testLen[i][1]], testLen[i][2]) != A_CRYPTO_OK) {
                   CRYPTO_PRINTF("\nFailed to mac compute final\n");
               }
                           
              ret = qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)&msg[len - testLen[i][3]], testLen[i][3], mac, macLen);
           }
           else { 
               ret = qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, len, (void*)mac, macLen);
           } 
           CRYPTO_PRINTF("Case %d: ", i);
           if (ret == A_CRYPTO_ERROR){
                CRYPTO_PRINTF("\nFailed to mac compare final\n");
            } 
            else if (ret == A_CRYPTO_ERROR_MAC_INVALID){
                CRYPTO_PRINTF("\nInvalid mac to mac compare final\n");
            }
            else {
                CRYPTO_PRINTF("hmacsha1 compare success\n");
            }

            qcom_crypto_transient_obj_free(mac_objHdl);
            qcom_crypto_op_free(compute_opHdl);
            qcom_crypto_op_free(compare_opHdl);
        }
        qcom_mem_free(msg);
        return 0;
}

A_INT32 crypto_hmacsha512()
{
    //"wmiconfig --cryptotest hmacsha512 " 
        char *str = "Sample message for keylen<blocklen";
        A_UINT8 msg[48] = {0};
       const  A_UINT8 *key = (A_UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,\
                                  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, \
                                  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,\
                                  0x3C, 0x3D, 0x3E, 0x3F};
       const  A_UINT8 *hmac = (A_UINT8[]){0xFD, 0x44, 0xC1, 0x8B, 0xDA, 0x0B, 0xB0, 0xA6, 0xCE, 0x0E, 0x82, 0xB0, 0x31, 0xBF, 0x28, 0x18, 0xF6, 0x53, 0x9B, 0xD5, 
                                     0x6E, 0xC0, 0x0B, 0xDC, 0x10, 0xA8, 0xA2, 0xD7, 0x30, 0xB3, 0x63, 0x4D, 0xE2, 0x54, 0x5D, 0x63, 0x9B, 0x0F, 0x2C, 0xF7,
                                     0x10, 0xD0, 0x69, 0x2C, 0x72, 0xA1, 0x89, 0x6F, 0x1F, 0x21, 0x1C, 0x2B, 0x92, 0x2D, 0x1A, 0x96, 0xC3, 0x92, 0xE0, 0x7E, 
                                     0x7E, 0xA9, 0xFE, 0xDC};
        A_UINT32 msgLen = 34;
        A_UINT32 keyLen =64*8;
        memcpy(msg, str,msgLen);
        char mac[64] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];

        //memcpy(msg, argv[4], msgLen);
        
        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_SHA512, QCOM_CRYPTO_HMAC_SHA512_MAX_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
#if 0        
        if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA512_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif
        if (keyLen<QCOM_CRYPTO_HMAC_SHA512_MIN_KEY_BITS || keyLen>QCOM_CRYPTO_HMAC_SHA512_MAX_KEY_BITS){
            CRYPTO_PRINTF("\nUnvalid key length\n");
            return -1;
        }
        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);

        if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
            return -1;
        }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA512, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA512_MAX_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
 
        if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }

        if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg, msgLen, mac, &macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compute final\n");
            return -1;
        } 
#if 0        
        printf("hmacsha512 mac len %d :\n", macLen);
        int i;
        for (i=0; i<macLen; i++)
            printf("%02x, ", mac[i]);
        printf("\n ");   
#endif   
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA512, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA512_MAX_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }
        if (qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, msgLen, (void*)hmac, macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compare final\n");
            return -1;
        } 
        else {
            CRYPTO_PRINTF("hmacsha512 compare success\n");
        }
        
        qcom_crypto_transient_obj_free(mac_objHdl);
        qcom_crypto_op_free(compute_opHdl);
        qcom_crypto_op_free(compare_opHdl);
}

A_INT32 crypto_hmacsha384()
{
    //"wmiconfig --cryptotest hmacsha384 " 
        char *str = "Sample message for keylen<blocklen";
        A_UINT8 msg[48] = {0};
        const A_UINT8 *key = (A_UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,\
                                  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, \
                                  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F};
        const A_UINT8 *hmac = (A_UINT8[]){0x6E, 0xB2, 0x42, 0xBD, 0xBB, 0x58, 0x2C, 0xA1, 0x7B, 0xEB, 0xFA, 0x48, 0x1B, 0x1E, 0x23, 0x21, 0x14, 0x64, 0xD2, 0xB7, 0xF8, 0xC2, 0x0B, 0x9F, 0xF2, 0x20, 0x16, 0x37, 0xB9, 0x36, 0x46, 0xAF, 0x5A, 0xE9, 0xAC, 0x31, 0x6E, 0x98, 0xDB, 0x45, 0xD9, 0xCA, 0xE7, 0x73, 0x67, 0x5E, 0xEE, 0xD0};
        A_UINT32 msgLen = 34;
        A_UINT32 keyLen =48*8;
        memcpy(msg, str,msgLen);
        char mac[64] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];

        //memcpy(msg, argv[4], msgLen);
        
        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_SHA384, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
#if 0        
        if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif
        if (keyLen<QCOM_CRYPTO_HMAC_SHA384_MIN_KEY_BITS || keyLen>QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS){
            CRYPTO_PRINTF("\nUnvalid key length\n");
            return -1;
        }
        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);

        if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
            return -1;
        }

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA384, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
 
        if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }

        if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg, msgLen, mac, &macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compute final\n");
            return -1;
        } 
#if 0        
        printf("hmacsha384 mac len %d :\n", macLen);
        int i;
        for (i=0; i<macLen; i++)
            printf("%02x, ", mac[i]);
        printf("\n ");   
#endif   
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA384, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }
        
        if (qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, msgLen, (void*)hmac, macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compare final\n");
            return -1;
        } 
        else {
            CRYPTO_PRINTF("hmacsha384 compare success\n");
        }

        qcom_crypto_transient_obj_free(mac_objHdl);
        qcom_crypto_op_free(compute_opHdl);
        qcom_crypto_op_free(compare_opHdl);
}

A_INT32 crypto_hmacmd5()
{
    //"wmiconfig --cryptotest hmacmd5 " 
        A_UINT8 msg[256] = {0};
        char *str = "Hi There";
        const A_UINT8 *key = (A_UINT8[]){0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
        const A_UINT8 *hmac = (A_UINT8[]){0x92, 0x94, 0x72, 0x7a, 0x36, 0x38, 0xbb, 0x1c, 0x13, 0xf4, 0x8e, 0xf8, 0x15, 0x8b, 0xfc, 0x9d};
        A_UINT32 msgLen = 8;
        A_UINT32 keyLen =16*8;
        memcpy(msg, str,msgLen);
        char mac[16] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];

        //memcpy(msg, argv[4], msgLen);
        
        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        //printf("srcData len %d %s\n", srcLen, srcData);
        if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_MD5, QCOM_CRYPTO_HMAC_MD5_MAX_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
            return -1;
        }
#if 0        
        if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\naes Failed to gen key\n");
        }
#endif
        if (keyLen<QCOM_CRYPTO_HMAC_MD5_MIN_KEY_BITS || keyLen>QCOM_CRYPTO_HMAC_MD5_MAX_KEY_BITS){
            CRYPTO_PRINTF("\nUnvalid key length\n");
            return -1;
        }
        
        ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);
        if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
            return -1;
        }
     
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_MD5, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_MD5_MAX_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }
 
        if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }

        if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg, msgLen, mac, &macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compute final\n");
            return -1;
        } 
#if 0        
        printf("hmacMD5 mac len %d :\n", macLen);
        int i;
        for (i=0; i<macLen; i++)
            printf("%02x, ", mac[i]);
        printf("\n ");   
#endif   
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_MD5, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_MD5_MAX_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
            return -1;
        }
       
        if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to set key\n");
            return -1;
        }

        if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac init\n");
            return -1;
        }
        if (qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, msgLen, (void*)hmac, macLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to mac compare final\n");
            return -1;
        } 
        else {
            CRYPTO_PRINTF("hmacmd5 compare success\n");
        }
        
        qcom_crypto_transient_obj_free(mac_objHdl);
        qcom_crypto_op_free(compute_opHdl);
        qcom_crypto_op_free(compare_opHdl);
}

A_INT32 crypto_aescmac(A_INT32 argc, char* argv[])
{
    //"wmiconfig --cryptotest aescmac 128/256" 
#if 0    
        const A_UINT8 *msg = (A_UINT8[]){0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a, \
                                                            0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, \
                                                            0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11};
        const A_UINT8 *key = (A_UINT8[]){0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
        const A_UINT8 *cmac = (A_UINT8[]){0xff, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30, 0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27};
        //const A_UINT8 *cmac = (A_UINT8[]){0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30, 0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27};
        A_UINT32 msgLen = 40;

        const A_UINT8 *msg1 = (A_UINT8[]){0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
        const A_UINT8 *key1 = (A_UINT8[]){0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,\
                                        0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
        const A_UINT8 *cmac1 = (A_UINT8[]){0x28, 0xa7, 0x02, 0x3f, 0x45, 0x2e, 0x8f, 0x82, 0xbd, 0x4b, 0xf2, 0x8d, 0x8c, 0x37, 0xc3, 0x5c};
        A_UINT32 msgLen1 = 16;

        char mac[16] = {0};
        A_UINT32 macLen = 0;
        A_UINT32 keyLen =atoi(argv[3]);

        qcom_crypto_attrib_t attr[1];
#endif

        qcom_crypto_obj_hdl_t mac_objHdl;
        qcom_crypto_op_hdl_t compute_opHdl;
        qcom_crypto_op_hdl_t compare_opHdl;

        A_UINT8 msg[128] = {0};
        A_UINT8 *key = msg;
        A_UINT32 keyLen =atoi(argv[3]);
        char mac[16] = {0};
        A_UINT32 macLen = 0;
        qcom_crypto_attrib_t attr[1];
        int ret = 0;
        
        A_UINT8 i;
        A_UINT32 testLen[4][4] = {{7, 3, 2, 4},{20, 35, 32, 40},{30,18, 16, 64}, {64,32, 16, 16}};
        for (i=0;  i<128/32; i++){
            memcpy(&msg[i*32], test_iv, 32);
        }
        for (i=0; i<4; i++){
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES256_KEY_BITS, &mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\n aes Failed to alloc transient obj\n");
                return -1;
            }
#if 0        
            if (qcom_crypto_transient_obj_keygen(mac_objHdl, QCOM_CRYPTO_HMAC_SHA384_MAX_KEY_BITS, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\naes Failed to gen key\n");
            }
#endif
            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_SECRET_VALUE, (void*)key, keyLen/8);
            
            if (qcom_crypto_transient_obj_populate(mac_objHdl, &attr[0], 1) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate mac public obj\n");
                return -1;
            }
         
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CMAC, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_AES256_KEY_BITS, &compute_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                return -1;
            }
           
            if (qcom_crypto_op_key_set(compute_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }
     
            if (qcom_crypto_op_mac_init(compute_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
                return -1;
            }
            
            int len = testLen[i][0]+testLen[i][1] +testLen[i][2] + testLen[i][3];
            if (i<2){
                qcom_crypto_op_mac_update(compute_opHdl, (void*)msg, testLen[i][0]);
                qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]);
                qcom_crypto_op_mac_update(compute_opHdl, (void*)&msg[testLen[i][0] + testLen[i][1]], testLen[i][2]);
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)&msg[len -testLen[i][3]],testLen[i][3] , mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                    return -1;
                }
            }
            else {
                if (qcom_crypto_op_mac_compute_final(compute_opHdl, (void*)msg,len , mac, &macLen) != A_CRYPTO_OK) {
                    CRYPTO_PRINTF("\nFailed to mac compute final\n");
                    return -1;
                }
            }
#if 0       
            printf("aescmac mac len %d :\n", macLen);
            int j;
            for (j=0; j<macLen; j++)
                printf("%02x, ", mac[j]);
            printf("\n ");   
#endif   
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CMAC, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_AES256_KEY_BITS, &compare_opHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
                return -1;
            }
           
            if (qcom_crypto_op_key_set(compare_opHdl, mac_objHdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
                return -1;
            }

            if (qcom_crypto_op_mac_init(compare_opHdl, NULL, 0) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to mac init\n");
                return -1;
            }
            if (i>=2){
                qcom_crypto_op_mac_update(compare_opHdl, (void*)msg, testLen[i][0]);
                qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0]], testLen[i][1]);
                qcom_crypto_op_mac_update(compare_opHdl, (void*)&msg[testLen[i][0] + testLen[i][1]], testLen[i][2]);
                ret =qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)&msg[len -testLen[i][3]],testLen[i][3] , mac, macLen);
            }
            else {
                ret = qcom_crypto_op_mac_compare_final(compare_opHdl, (void*)msg, len, (void*)mac, macLen);
            }
            CRYPTO_PRINTF("Case %d: ", i);
            if (ret == A_CRYPTO_ERROR){
                CRYPTO_PRINTF("\nFailed to mac compare final\n");
            } 
            else if (ret == A_CRYPTO_ERROR_MAC_INVALID){
                CRYPTO_PRINTF("\nInvalid mac to mac compare final\n");
            }
            else {
                CRYPTO_PRINTF("aescmac compare success\n");
            }
            
            qcom_crypto_transient_obj_free(mac_objHdl);
            qcom_crypto_op_free(compute_opHdl);
            qcom_crypto_op_free(compare_opHdl);
       }       
        return 0;
}

A_INT32 crypto_sha1()
{
    //"wmiconfig --cryptotest sha1 " 
        A_UINT8 msg[128] = {0};
        const A_UINT8 *digest = (A_UINT8[]){0x60, 0x53, 0xD7, 0x61, 0x08, 0x4E, 0x9E, 0xB4, 0xEC, 0x12, 0x81, 0x01, 0x10, 0xDE, 0x07, 0xE7, 0x32, 0x07, 0x87, 0xB6};
        A_UINT32 msgLen = 127;
        char hash[32] = {0};
        A_UINT32 hashLen = 0;
       
        qcom_crypto_op_hdl_t opHdl;

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA1, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_digest_update(opHdl, msg, 32) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
          if (qcom_crypto_op_digest_update(opHdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
        
        if (qcom_crypto_op_digest_dofinal(opHdl, (void*)msg, msgLen - 32-64, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 

#if 0        
        printf("sha1 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        if (memcmp(hash, digest, QCOM_CRYPTO_SHA1_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha1 op sucess\n");
         qcom_crypto_op_free(opHdl);
}

A_INT32 crypto_opcopy_sha1()
{
    //"wmiconfig --cryptotest opcopy_sha1 " 
        A_UINT8 msg[128] = {0};
        const A_UINT8 *digest = (A_UINT8[]){0x60, 0x53, 0xD7, 0x61, 0x08, 0x4E, 0x9E, 0xB4, 0xEC, 0x12, 0x81, 0x01, 0x10, 0xDE, 0x07, 0xE7, 0x32, 0x07, 0x87, 0xB6};
        const A_UINT8 *digest2= (A_UINT8[]){0xC8, 0xD7, 0xD0, 0xEF, 0x0E, 0xED, 0xFA, 0x82, 0xD2, 0xEA, 0x1A, 0xA5, 0x92, 0x84, 0x5B, 0x9A, 0x6D, 0x4B, 0x02, 0xB7};
        A_UINT32 msgLen = 127;
        char hash[32] = {0};
        A_UINT32 hashLen = 0;
        
        qcom_crypto_op_hdl_t src_Hdl;
        qcom_crypto_op_hdl_t dst_Hdl;

//CASE1: the total data size of qcom_crypto_op_digest_update is not a multiple of block size.
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA1, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA1, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

         if (qcom_crypto_op_digest_update(src_Hdl, msg, 32) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    

        if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
        
        if (qcom_crypto_op_digest_update(src_Hdl, msg, msgLen - 32-64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
        
        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 
#if 0        
        printf("sha1 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(hash, digest, QCOM_CRYPTO_SHA1_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha1 op copy case 1 sucess\n");
        
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
         
//CASE2:the total data size of qcom_crypto_op_digest_update is a multiple of block size.
        memset(hash, 0, 32);
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA1, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA1, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

         if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    

        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 
#if 0       
        printf("sha1 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(hash, digest2, QCOM_CRYPTO_SHA1_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha1 op copy case 2 sucess\n");
        
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
}

A_INT32 crypto_sha256()
{
    //"wmiconfig --cryptotest sha256 " 
        A_UINT8 msg[128] = {0};
        const A_UINT8 *digest = (A_UINT8[]){0x15, 0xDA, 0xE5, 0x97, 0x90, 0x58, 0xBF, 0xBF, 0x4F, 0x91, 0x66, 0x02, 0x9B, 0x6E, 0x34, 0x0E, 0xA3, 0xCA, 0x37, 0x4F, 0xEF, 0x57, 0x8A, 0x11, 0xDC, 0x9E, 0x6E, 0x92, 0x38, 0x60, 0xD7, 0xAE};
        A_UINT32 msgLen = 127;
        char hash[32] = {0};
        A_UINT32 hashLen = 0;
       
        qcom_crypto_op_hdl_t opHdl;

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_digest_update(opHdl, msg, 32) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
          if (qcom_crypto_op_digest_update(opHdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
        
        if (qcom_crypto_op_digest_dofinal(opHdl, (void*)msg, msgLen-32-64, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 

#if 0       
        printf("sha256 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif           
        if (memcmp(hash, digest, QCOM_CRYPTO_SHA256_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha256 op sucess\n");

         qcom_crypto_op_free(opHdl);
}

A_INT32 crypto_opcopy_sha256()
{
    //"wmiconfig --cryptotest opcopy_sha256 " 
         A_UINT8 msg[128] = {0};
         const A_UINT8 *digest= (A_UINT8[]){0x15, 0xDA, 0xE5, 0x97, 0x90, 0x58, 0xBF, 0xBF, 0x4F, 0x91, 0x66, 0x02, 0x9B, 0x6E, 0x34, 0x0E, 0xA3, 0xCA, 0x37, 0x4F, 0xEF, 0x57, 0x8A, 0x11, 0xDC, 0x9E, 0x6E, 0x92, 0x38, 0x60, 0xD7, 0xAE};
         const A_UINT8 *digest2= (A_UINT8[]){0xF5, 0xA5, 0xFD, 0x42, 0xD1, 0x6A, 0x20, 0x30, 0x27, 0x98, 0xEF, 0x6E, 0xD3, 0x09, 0x97, 0x9B, 0x43, 0x00, 0x3D, 0x23, 0x20, 0xD9, 0xF0, 0xE8, 0xEA, 0x98, 0x31, 0xA9, 0x27, 0x59, 0xFB, 0x4B};
         A_UINT32 msgLen = 127;
        char hash[32] = {0};
        A_UINT32 hashLen = 0;
        
        qcom_crypto_op_hdl_t src_Hdl;
        qcom_crypto_op_hdl_t dst_Hdl;

//CASE1:the total data size of qcom_crypto_op_digest_update is not a multiple of block size.
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

         if (qcom_crypto_op_digest_update(src_Hdl, msg, 32) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    

        if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
        
        if (qcom_crypto_op_digest_update(src_Hdl, msg, msgLen - 32-64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
        
        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 
#if 0        
        printf("sha256 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(hash, digest, QCOM_CRYPTO_SHA256_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha256 op copy case 1 sucess\n");
        
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
//CASE2:the total data size of qcom_crypto_op_digest_update is a multiple of block size.
        memset(hash, 0, 32);
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

        if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
                
        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, hash, &hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 
#if 0        
        printf("sha256 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(hash, digest2, QCOM_CRYPTO_SHA256_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha256 op copy case 2 sucess\n");
        
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
}

A_INT32 crypto_sha384()
{
    //"wmiconfig --cryptotest sha384 " 
        A_UINT8 msg[256] = {0};
#if 0    
        A_UINT8 digest[48] = {0x48, 0x57, 0x09, 0xA9, 0x1F, 0x98, 0x46, 0xD3, 0xEB, 0x98, 0x24, 0x33, 0x51, 0x06, 0x53, 0x2F, 
                                          0xDE, 0x8B, 0xFD, 0x5D, 0x77, 0x04, 0x08, 0xE0, 0x89, 0x9D, 0x99, 0xDB, 0x5B, 0xF8, 0x5B, 0x79,
                                          0xBA, 0x04, 0x63, 0x0D, 0x1E, 0x6D, 0x13, 0xF9, 0xF3, 0x36, 0x91, 0x5D, 0xAF, 0x7C, 0x1D, 0x1D};
#endif
        A_UINT8 hash1[48] = {0};
        A_UINT8 hash2[48] = {0};
        A_UINT32 hashLen1 = 0;
        A_UINT32 hashLen2 = 0;

        A_UINT32 msgLen = 256;
        memset(msg, 0xa,msgLen);

       
        qcom_crypto_op_hdl_t opHdl1;
        qcom_crypto_op_hdl_t opHdl2;

        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA384, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }
         
        if (qcom_crypto_op_digest_dofinal(opHdl1, (void*)&msg, msgLen, hash1, &hashLen1) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA384, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }
   
         if (qcom_crypto_op_digest_update(opHdl2, msg, 32) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    

         if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest update\n");
        }    
         
         if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
        
        if (qcom_crypto_op_digest_dofinal(opHdl2, (void*)msg, msgLen - 64*2 -32, hash2, &hashLen2) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        } 

#if 0        
        printf("sha384 hash1 len %d :\n", hashLen1);
        int i;
        for (i=0; i<hashLen1; i++)
            printf("%02x, ", hash1[i]);
        printf("\n ");  

        printf("sha384 hash2 len %d :\n", hashLen2);
        
        for (i=0; i<hashLen2; i++)
            printf("%02x, ", hash2[i]);
        printf("\n ");  
#endif        
        if (memcmp(hash1, hash2, QCOM_CRYPTO_SHA384_DIGEST_BYTES) == 0)
            CRYPTO_PRINTF("sha384 op sucess\n");
        
         qcom_crypto_op_free(opHdl1);
         qcom_crypto_op_free(opHdl2);
}

A_INT32 crypto_opcopy_sha384()
{
    //"wmiconfig --cryptotest opcopy_384 " 
        A_UINT8 msg[256] = {0};
        A_UINT8 dst_hash[48] = {0};
        A_UINT8 tmp_hash[48] = {0};
        A_UINT32 dst_hashLen = 0;
        A_UINT32 tmp_hashLen = 0;
        
        A_UINT32 msgLen = 256;
        memset(msg, 0xa,msgLen);
        
        qcom_crypto_op_hdl_t src_Hdl;
        qcom_crypto_op_hdl_t dst_Hdl;
        qcom_crypto_op_hdl_t tmp_Hdl;
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA384, QCOM_CRYPTO_MODE_DIGEST, 0, &tmp_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }
        
        if (qcom_crypto_op_digest_dofinal(tmp_Hdl, msg, msgLen, tmp_hash, &tmp_hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        }
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA384, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA384, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

          if (qcom_crypto_op_digest_update(src_Hdl, msg, 32) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
         
          if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
          
          if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
              CRYPTO_PRINTF("\nFailed to digest update\n");
          }    
         
         if (qcom_crypto_op_digest_update(src_Hdl, msg, msgLen - 64*2 -32) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         } 
         
        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, dst_hash, &dst_hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        }
#if 0
        printf("sha384 digest len %d :\n", dst_hashLen);
        int i;
        for (i=0; i<dst_hashLen; i++)
            printf("%02x, ", dst_hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(dst_hash, tmp_hash, QCOM_CRYPTO_SHA384_DIGEST_BYTES) == 0){
            CRYPTO_PRINTF("sha384 op copy case 1 sucess\n");
        }
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
         qcom_crypto_op_free(tmp_Hdl);
}

A_INT32 crypto_sha512()
{
       //"wmiconfig --cryptotest sha512 " 
           A_UINT8 msg[256] = {0};
#if 0    
           A_UINT8 digest[48] = {0x48, 0x57, 0x09, 0xA9, 0x1F, 0x98, 0x46, 0xD3, 0xEB, 0x98, 0x24, 0x33, 0x51, 0x06, 0x53, 0x2F, 
                                             0xDE, 0x8B, 0xFD, 0x5D, 0x77, 0x04, 0x08, 0xE0, 0x89, 0x9D, 0x99, 0xDB, 0x5B, 0xF8, 0x5B, 0x79,
                                             0xBA, 0x04, 0x63, 0x0D, 0x1E, 0x6D, 0x13, 0xF9, 0xF3, 0x36, 0x91, 0x5D, 0xAF, 0x7C, 0x1D, 0x1D};
#endif
           A_UINT8 hash1[64] = {0};
           A_UINT8 hash2[64] = {0};
           A_UINT32 hashLen1 = 0;
           A_UINT32 hashLen2 = 0;
   
           A_UINT32 msgLen = 256;
           memset(msg, 0xa,msgLen);
   
          
           qcom_crypto_op_hdl_t opHdl1;
           qcom_crypto_op_hdl_t opHdl2;
   
           if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA512, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl1) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to alloc op\n");
           }
            
           if (qcom_crypto_op_digest_dofinal(opHdl1, (void*)&msg, msgLen, hash1, &hashLen1) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest dofinal\n");
           } 
           
           if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA512, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl2) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to alloc op\n");
           }
      
            if (qcom_crypto_op_digest_update(opHdl2, msg, 32) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest update\n");
           }    
   
            if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest update\n");
           }    
            
            if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to digest update\n");
            }    
           
           if (qcom_crypto_op_digest_dofinal(opHdl2, (void*)msg, msgLen - 64*2 -32, hash2, &hashLen2) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest dofinal\n");
           } 
   
#if 0        
           printf("sha512 hash1 len %d :\n", hashLen1);
           int i;
           for (i=0; i<hashLen1; i++)
               printf("%02x, ", hash1[i]);
           printf("\n ");  
   
           printf("sha512 hash2 len %d :\n", hashLen2);
           for (i=0; i<hashLen2; i++)
               printf("%02x, ", hash2[i]);
           printf("\n ");  
#endif        
           if (memcmp(hash1, hash2, QCOM_CRYPTO_SHA512_DIGEST_BYTES) == 0)
               CRYPTO_PRINTF("sha512 op sucess\n");
           
            qcom_crypto_op_free(opHdl1);
            qcom_crypto_op_free(opHdl2);
}

A_INT32 crypto_opcopy_sha512()
{
    //"wmiconfig --cryptotest opcopy_sha512" 
           A_UINT8 msg[256] = {0};
            A_UINT8 dst_hash[64] = {0};
            A_UINT8 tmp_hash[64] = {0};
            A_UINT32 dst_hashLen = 0;
            A_UINT32 tmp_hashLen = 0;
            
            A_UINT32 msgLen = 256;
            memset(msg, 0xa,msgLen);
            
            qcom_crypto_op_hdl_t src_Hdl;
            qcom_crypto_op_hdl_t dst_Hdl;
            qcom_crypto_op_hdl_t tmp_Hdl;
            
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA512, QCOM_CRYPTO_MODE_DIGEST, 0, &tmp_Hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
            
            if (qcom_crypto_op_digest_dofinal(tmp_Hdl, msg, msgLen, tmp_hash, &tmp_hashLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to digest dofinal\n");
            }
            
    //CASE1:
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA512, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc op\n");
            }
    
             if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA512, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
                 CRYPTO_PRINTF("\nFailed to alloc op\n");
             }
    
              if (qcom_crypto_op_digest_update(src_Hdl, msg, 32) != A_CRYPTO_OK) {
                 CRYPTO_PRINTF("\nFailed to digest update\n");
             }    
             
              if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
                 CRYPTO_PRINTF("\nFailed to digest update\n");
             }    
              
              if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
                  CRYPTO_PRINTF("\nFailed to digest update\n");
              }    
             
             if (qcom_crypto_op_digest_update(src_Hdl, msg, msgLen - 64*2 -32) != A_CRYPTO_OK) {
                 CRYPTO_PRINTF("\nFailed to digest update\n");
             } 
             
            if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
                 CRYPTO_PRINTF("\nFailed to op copy\n");
            }    
            
            if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, dst_hash, &dst_hashLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to digest dofinal\n");
            }
#if 0        
            printf("sha512 digest len %d :\n", hashLen);
            int i;
            for (i=0; i<hashLen; i++)
                printf("%02x, ", hash[i]);
            printf("\n ");   
#endif        
            
            if (memcmp(dst_hash, tmp_hash, QCOM_CRYPTO_SHA512_DIGEST_BYTES) == 0){
                CRYPTO_PRINTF("sha512 op copy case 1 sucess\n");
            }
             qcom_crypto_op_free(src_Hdl);
             qcom_crypto_op_free(dst_Hdl);
             qcom_crypto_op_free(tmp_Hdl);
}

A_INT32 crypto_md5()
{
       //"wmiconfig --cryptotest md5 " 
           A_UINT8 msg[256] = {0};
           A_UINT8 hash1[16] = {0};
           A_UINT8 hash2[16] = {0};
           A_UINT32 hashLen1 = 0;
           A_UINT32 hashLen2 = 0;
   
           A_UINT32 msgLen = 256;
           memset(msg, 0xa,msgLen);
   
          
           qcom_crypto_op_hdl_t opHdl1;
           qcom_crypto_op_hdl_t opHdl2;
   
           if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_MD5, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl1) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to alloc op\n");
           }
            
           if (qcom_crypto_op_digest_dofinal(opHdl1, (void*)&msg, msgLen, hash1, &hashLen1) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest dofinal\n");
           } 
           
           if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_MD5, QCOM_CRYPTO_MODE_DIGEST, 0, &opHdl2) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to alloc op\n");
           }
      
            if (qcom_crypto_op_digest_update(opHdl2, msg, 32) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest update\n");
           }    
   
            if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest update\n");
           }    
            
            if (qcom_crypto_op_digest_update(opHdl2, msg, 64) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to digest update\n");
            }    
           
           if (qcom_crypto_op_digest_dofinal(opHdl2, (void*)msg, msgLen - 64*2 -32, hash2, &hashLen2) != A_CRYPTO_OK) {
               CRYPTO_PRINTF("\nFailed to digest dofinal\n");
           } 
   
#if 0        
           printf("md5 hash1 len %d :\n", hashLen1);
           int i;
           for (i=0; i<hashLen1; i++)
               printf("%02x, ", hash1[i]);
           printf("\n ");  
   
           printf("md5 hash2 len %d :\n", hashLen2);
           for (i=0; i<hashLen2; i++)
               printf("%02x, ", hash2[i]);
           printf("\n ");  
#endif        
           if (memcmp(hash1, hash2, QCOM_CRYPTO_MD5_DIGEST_BYTES) == 0)
               CRYPTO_PRINTF("md5 op sucess\n");
           
            qcom_crypto_op_free(opHdl1);
            qcom_crypto_op_free(opHdl2);
}

A_INT32 crypto_opcopy_md5()
{
    //"wmiconfig --cryptotest opcopy_md5 " 
        A_UINT8 msg[256] = {0};
        A_UINT8 dst_hash[16] = {0};
        A_UINT8 tmp_hash[16] = {0};
        A_UINT32 dst_hashLen = 0;
        A_UINT32 tmp_hashLen = 0;
        
        A_UINT32 msgLen = 256;
        memset(msg, 0xa,msgLen);
        
        qcom_crypto_op_hdl_t src_Hdl;
        qcom_crypto_op_hdl_t dst_Hdl;
        qcom_crypto_op_hdl_t tmp_Hdl;
        
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_MD5, QCOM_CRYPTO_MODE_DIGEST, 0, &tmp_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }
        
        if (qcom_crypto_op_digest_dofinal(tmp_Hdl, msg, msgLen, tmp_hash, &tmp_hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        }
        
//CASE1:
        if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_MD5, QCOM_CRYPTO_MODE_DIGEST, 0, &src_Hdl) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to alloc op\n");
        }

         if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_MD5, QCOM_CRYPTO_MODE_DIGEST, 0, &dst_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to alloc op\n");
         }

          if (qcom_crypto_op_digest_update(src_Hdl, msg, 32) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
         
          if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         }    
          
          if (qcom_crypto_op_digest_update(src_Hdl, msg, 64) != A_CRYPTO_OK) {
              CRYPTO_PRINTF("\nFailed to digest update\n");
          }    
         
         if (qcom_crypto_op_digest_update(src_Hdl, msg, msgLen - 64*2 -32) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to digest update\n");
         } 
         
        if (qcom_crypto_op_copy(dst_Hdl, src_Hdl) != A_CRYPTO_OK) {
             CRYPTO_PRINTF("\nFailed to op copy\n");
        }    
        
        if (qcom_crypto_op_digest_dofinal(dst_Hdl, NULL, 0, dst_hash, &dst_hashLen) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to digest dofinal\n");
        }
#if 0        
        printf("md5 digest len %d :\n", hashLen);
        int i;
        for (i=0; i<hashLen; i++)
            printf("%02x, ", hash[i]);
        printf("\n ");   
#endif        
        
        if (memcmp(dst_hash, tmp_hash, QCOM_CRYPTO_MD5_DIGEST_BYTES) == 0){
            CRYPTO_PRINTF("md5 op copy case 1 sucess\n");
        }
         qcom_crypto_op_free(src_Hdl);
         qcom_crypto_op_free(dst_Hdl);
         qcom_crypto_op_free(tmp_Hdl);
}

A_INT32 crypto_rsav15(A_INT32 argc, char* argv[])
{
            //wmiconfig --cryptotest rsav15 xxx    xxx:input data
            qcom_crypto_obj_hdl_t obj_hdl;
            qcom_crypto_op_hdl_t enc_hdl, dec_hdl;
            qcom_crypto_attrib_t attr[QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR];
            A_UINT32 rsa_key_size = 1024;
            A_UINT8* encData;;
            A_UINT8* decData;;
            A_UINT8* originalData;;
            A_UINT8* temp;;            
            A_UINT32 originalLen, encLen, decLen;
            
            A_UINT8* data = swat_mem_malloc(512*3 +256);
            if (data == NULL){
                CRYPTO_PRINTF("\nFailed to alloc memory\n");
                return -1;
            }
            encData = data;
            decData = encData +512;
            originalData = decData +512;
            temp = originalData +512;

            originalLen = strlen(argv[3]);
            A_MEMCPY(temp, argv[3], originalLen);
            A_MEMCPY(originalData, argv[3], originalLen);
            
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_RSA_KEYPAIR, rsa_key_size, 
                        &obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            }

            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_RSA_MODULUS, rsa_mod, sizeof(rsa_mod));
            ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_RSA_PUBLIC_EXPONENT, rsa_pub_exp, sizeof(rsa_pub_exp));
            ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_RSA_PRIVATE_EXPONENT, rsa_pvt_exp, sizeof(rsa_pvt_exp));
            ref_attr_init(&attr[3], QCOM_CRYPTO_ATTR_RSA_PRIME1, rsa_prime1, sizeof(rsa_prime1));
            ref_attr_init(&attr[4], QCOM_CRYPTO_ATTR_RSA_PRIME2, rsa_prime2, sizeof(rsa_prime2));
            ref_attr_init(&attr[5], QCOM_CRYPTO_ATTR_RSA_EXPONENT1, rsa_exp1, sizeof(rsa_exp1));
            ref_attr_init(&attr[6], QCOM_CRYPTO_ATTR_RSA_EXPONENT2, rsa_exp2, sizeof(rsa_exp2));
            ref_attr_init(&attr[7], QCOM_CRYPTO_ATTR_RSA_COEFFICIENT, rsa_coeff, sizeof(rsa_coeff));

            if (qcom_crypto_transient_obj_populate(obj_hdl, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate ecc keypair obj\n");
            }
            /* Allocate encrypt operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSAES_PKCS1_V1_5, QCOM_CRYPTO_MODE_ENCRYPT, rsa_key_size, 
                        &enc_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc encrypt op\n");
            }

            /* Copy public key from keypair object for encrypt operation */
            if (qcom_crypto_op_key_set(enc_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            /* Allocate decrypt operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSAES_PKCS1_V1_5, QCOM_CRYPTO_MODE_DECRYPT, rsa_key_size, &dec_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc decrypt op\n");
            }

            /* Copy private key from keypair object for decrypt operation */
            if (qcom_crypto_op_key_set(dec_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            /*encrypt */
            if (qcom_crypto_op_asym_encrypt(enc_hdl, NULL, 0, originalData, originalLen, encData, &encLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to encrypt\n");
            }

            /* decrypt */
            if (qcom_crypto_op_asym_decrypt(dec_hdl, NULL, 0, encData, encLen, decData, &decLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed decrypt\n");
            }

            if ((originalLen == decLen) && (A_MEMCMP(temp, decData, decLen) == 0)){
                CRYPTO_PRINTF("\nQCOM_CRYPTO_ALG_RSAES_PKCS1_V1_5 encrypt/decrypt Successful\n");
            }
            qcom_mem_free(data);
            qcom_crypto_op_free(enc_hdl);
            qcom_crypto_op_free(dec_hdl);
            qcom_crypto_transient_obj_free(obj_hdl);            
 }

A_INT32 crypto_rsanopad()
{
            //wmiconfig --cryptotest rsanopad
            qcom_crypto_obj_hdl_t obj_hdl;
            qcom_crypto_op_hdl_t enc_hdl, dec_hdl;
            qcom_crypto_attrib_t attr[QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR];
            A_UINT32 rsa_key_size = 1024;
            A_UINT8* encData;
            A_UINT8* decData;
            A_UINT8* originalData;
            A_UINT8* temp;            
            A_UINT32 originalLen, encLen, decLen;

            A_UINT8* data = swat_mem_malloc(512*3 +256);
            if (data == NULL){
                CRYPTO_PRINTF("\nFailed to alloc memory\n");
                return -1;
            }
            encData = data;
            decData = encData +512;
            originalData = decData +512;
            temp = originalData +512;
            
            originalLen = rsa_key_size/8;
            if (qcom_crypto_rng_get(temp, originalLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to get rng\n");
            }
            
            if (temp[0] >= 0xa7) { //the plain text must be less than rsa_mod, this change just for test.
                temp[0] = 0xa6;
            }
            
            A_MEMCPY(originalData, temp, originalLen);
            if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_RSA_KEYPAIR, rsa_key_size, 
                        &obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc transient obj\n");
            }

            ref_attr_init(&attr[0], QCOM_CRYPTO_ATTR_RSA_MODULUS, rsa_mod, sizeof(rsa_mod));
            ref_attr_init(&attr[1], QCOM_CRYPTO_ATTR_RSA_PUBLIC_EXPONENT, rsa_pub_exp, sizeof(rsa_pub_exp));
            ref_attr_init(&attr[2], QCOM_CRYPTO_ATTR_RSA_PRIVATE_EXPONENT, rsa_pvt_exp, sizeof(rsa_pvt_exp));
            ref_attr_init(&attr[3], QCOM_CRYPTO_ATTR_RSA_PRIME1, rsa_prime1, sizeof(rsa_prime1));
            ref_attr_init(&attr[4], QCOM_CRYPTO_ATTR_RSA_PRIME2, rsa_prime2, sizeof(rsa_prime2));
            ref_attr_init(&attr[5], QCOM_CRYPTO_ATTR_RSA_EXPONENT1, rsa_exp1, sizeof(rsa_exp1));
            ref_attr_init(&attr[6], QCOM_CRYPTO_ATTR_RSA_EXPONENT2, rsa_exp2, sizeof(rsa_exp2));
            ref_attr_init(&attr[7], QCOM_CRYPTO_ATTR_RSA_COEFFICIENT, rsa_coeff, sizeof(rsa_coeff));

            if (qcom_crypto_transient_obj_populate(obj_hdl, attr, QCOM_CRYPTO_OBJ_ATTRIB_COUNT_RSA_KEYPAIR) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to populate ecc keypair obj\n");
            }
            /* Allocate encrypt operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSA_NOPAD, QCOM_CRYPTO_MODE_ENCRYPT, rsa_key_size, 
                        &enc_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc encrypt op\n");
            }

            /* Copy public key from keypair object for encrypt operation */
            if (qcom_crypto_op_key_set(enc_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            /* Allocate decrypt operation */
            if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_RSA_NOPAD, QCOM_CRYPTO_MODE_DECRYPT, rsa_key_size, &dec_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to alloc decrypt op\n");
            }

            /* Copy private key from keypair object for decrypt operation */
            if (qcom_crypto_op_key_set(dec_hdl, obj_hdl) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to set key\n");
            }

            /*encrypt */
            if (qcom_crypto_op_asym_encrypt(enc_hdl, NULL, 0, originalData, originalLen, encData, &encLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed to encrypt\n");
            }

            /* decrypt */
            if (qcom_crypto_op_asym_decrypt(dec_hdl, NULL, 0, encData, encLen, decData, &decLen) != A_CRYPTO_OK) {
                CRYPTO_PRINTF("\nFailed decrypt\n");
            }

            if ((originalLen == decLen) && (A_MEMCMP(temp, decData, decLen) == 0)){
                CRYPTO_PRINTF("\nQCOM_CRYPTO_ALG_RSA_NOPAD encrypt/decrypt Successful\n");
            }

            qcom_mem_free(data);
            qcom_crypto_op_free(enc_hdl);
            qcom_crypto_op_free(dec_hdl);
            qcom_crypto_transient_obj_free(obj_hdl);            
 }

A_INT32 getrng_demo()
{
#ifdef ENABLE_RNG_DEMO
        A_UINT32 rng_length;
        unsigned char rng_buffer[crypto_rng_RANDOMBYTES];
        int i;
        
        rng_length = sizeof(rng_buffer);
        memset(rng_buffer, 0, rng_length);

        if (qcom_crypto_rng_get(rng_buffer, rng_length) != A_CRYPTO_OK) {
            CRYPTO_PRINTF("\nFailed to retrieve %d random bytes\n", rng_length);
        } 
        else {
            CRYPTO_PRINTF("Successfully retrieved %d random bytes\n", rng_length);
        }

        CRYPTO_PRINTF("[");
        for (i = 0; i < rng_length; i++) {
            if (i%10 == 0) 
                CRYPTO_PRINTF("\n");
            CRYPTO_PRINTF("%d, ", rng_buffer[i]);
        }
        CRYPTO_PRINTF("]\n");
#endif //ENABLE_RNG_DEMO
}

A_INT32 swat_crypto_test(A_INT32 argc, char* argv[])
{
	if (argc < 3) {
		return SWAT_ERROR;
	}
	
    if(strcmp(argv[2], "rsa") == 0) {
        rsa_demo();
    }

    if(strcmp(argv[2], "srp") == 0) {
        srp_demo();
    }

    if(strcmp(argv[2], "ecdsa") == 0) {
       ecdsa_demo();
    }

#if ENABLE_DH_DEMO
    if(strcmp(argv[2], "dh") == 0) {
      dh_demo();
    }
#endif

    if(strcmp(argv[2], "ecdh") == 0) {
      ecdh_demo();
    }

    if(strcmp(argv[2], "curve") == 0) {
      curve_demo();
    }

    if(strcmp(argv[2], "ed25519") == 0) {
      ed25519_demo();
    }
    else if(strcmp(argv[2], "aescbc") == 0) {
        crypto_aescbc(argc, argv);
    }
    else if(strcmp(argv[2], "aesccm") == 0) {
        crypto_aesccm();
    }
    else if (strcmp(argv[2], "aesgcm") == 0){
        crypto_aesgcm(argc, argv);
    }
    else if (strcmp(argv[2], "chacha20_poly1305") == 0) {
        crypto_chacha20_poly1305(argc, argv);
    }
    else if(strcmp(argv[2], "aesctr") == 0){
        crypto_aesctr(argc, argv);
    } 
#if CRYPTO_CONFIG_3DES
    else if((strcmp(argv[2], "des") == 0) || (strcmp(argv[2], "3des") == 0)){
        crypto_descbc(argc, argv);
    }
#endif    
    else if (strcmp(argv[2], "hmacsha256")==0){
        crypto_hmacsha256();
    }
    else if (strcmp(argv[2], "hmacsha1")==0){
        crypto_hmacsha1(argc, argv);
    }
    else if (strcmp(argv[2], "hmacsha512")==0){
        crypto_hmacsha512();
    }
    else if (strcmp(argv[2], "hmacsha384")==0){
        crypto_hmacsha384();
    }
    else if (strcmp(argv[2], "hmacmd5")==0){
        crypto_hmacmd5();
    }
    else if (strcmp(argv[2], "aescmac")==0){
        crypto_aescmac(argc, argv);
    }
    else if (strcmp(argv[2], "sha1")==0){
        crypto_sha1();
    }
    else if (strcmp(argv[2], "opcopy_sha1")==0){
        crypto_opcopy_sha1();
    }    
    else if (strcmp(argv[2], "sha256")==0){
        crypto_sha256();
    }
    else if (strcmp(argv[2], "opcopy_sha256")==0){
        crypto_opcopy_sha256();
    }    
    else if (strcmp(argv[2], "sha384")==0){
        crypto_sha384();
    }
    else if (strcmp(argv[2], "opcopy_sha384")==0){
        crypto_opcopy_sha384();
    }    
    else if (strcmp(argv[2], "sha512")==0){
        crypto_sha512();
    }
    else if (strcmp(argv[2], "opcopy_sha512")==0){
        crypto_opcopy_sha512();
    }    
    else if (strcmp(argv[2], "md5")==0){
        crypto_md5();
    }
    else if (strcmp(argv[2], "opcopy_md5")==0){
        crypto_opcopy_md5();
    }
    else if (strcmp(argv[2], "rsav15") == 0){
        crypto_rsav15(argc, argv);
    }
    else if (strcmp(argv[2], "rsanopad") == 0){
        crypto_rsanopad();
    }

    if(strcmp(argv[2], "getrng") == 0) {
      getrng_demo();
    }
    
    return SWAT_OK;

}

#endif
