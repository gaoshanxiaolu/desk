/**
 * @file
 */
/******************************************************************************
* Copyright 2012-2015, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/
#define AJ_MODULE TARGET_CRYPTO

#include "aj_target.h"
#include "aj_util.h"
#include "aj_crypto.h"
#include "aj_crypto_sha2.h"
#include <qcom/qcom_crypto.h>
#include "aj_debug.h"

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgTARGET_CRYPTO = 0;
#endif

#define MAX_NONCE_LEN 13
#define MIN_NONCE_LEN 11

void AJ_RandBytes(uint8_t* rand, uint32_t len)
{
    A_UINT16 maxBytes = 0xfff0;

    while (len > maxBytes) {
        qcom_crypto_rng_get(rand, maxBytes);
        rand += maxBytes;
        len -= maxBytes;
    }
    qcom_crypto_rng_get(rand, (A_UINT16) len);
}

/*
 * Crypto utility functions
 */

static void set_ref_attribute(qcom_crypto_attrib_t* attr, A_UINT32 id, void* data, A_UINT32 len)
{
    attr->attrib_id = id;
    attr->u.ref.buf = data;
    attr->u.ref.len = len;
}

static void set_val_attribute(qcom_crypto_attrib_t* attr, A_UINT32 id, A_UINT32 a, A_UINT32 b)
{
    attr->attrib_id = id;
    attr->u.val.a = a;
    attr->u.val.b = b;
}

/*
 * AES
 */

/*
 * Common initialization for 128-bit AES-CCM encrypt/decrypt
 */
static A_CRYPTO_STATUS initAes(qcom_crypto_obj_hdl_t* obj, qcom_crypto_op_hdl_t* op, A_UINT32 mode, const uint8_t* key)
{
    qcom_crypto_attrib_t attribute;

    set_ref_attribute(&attribute, QCOM_CRYPTO_ATTR_SECRET_VALUE, (uint8_t*)key, QCOM_CRYPTO_AES128_KEY_BYTES);

    if (qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_AES, QCOM_CRYPTO_AES128_KEY_BITS, obj) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES obj failure\n"));
    } else if (qcom_crypto_transient_obj_populate(*obj, &attribute, 1) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES populate failure\n"));
        qcom_crypto_transient_obj_free(*obj);
    } else if (qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_AES_CCM, mode, QCOM_CRYPTO_AES128_KEY_BITS, op) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES op failure\n"));
        qcom_crypto_transient_obj_free(*obj);
    } else if (qcom_crypto_op_key_set(*op, *obj) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES key set failure\n"));
        qcom_crypto_op_free(*op);
        qcom_crypto_transient_obj_free(*obj);
    } else {
        /* All calls in the chain succeeded */
        return A_CRYPTO_OK;
    }

    return A_CRYPTO_ERROR;
}

/*
 * AES-CCM Encryption
 */
AJ_Status AJ_Encrypt_CCM(const uint8_t* key, uint8_t* msg, uint32_t msgLen, uint32_t hdrLen, uint8_t tagLen, const uint8_t* nonce, uint32_t nLen)
{
    qcom_crypto_obj_hdl_t obj;
    qcom_crypto_op_hdl_t op;
    uint8_t tag[16];
    uint8_t nonceBuf[MAX_NONCE_LEN] = { 0 };
    uint32_t tagOutputLen;
    uint8_t* src;
    uint32_t srcLen;
    uint8_t* dest;
    uint32_t destLen;
    uint32_t destOutputLen;
    AJ_Status status = AJ_ERR_SECURITY;

    src = msg + hdrLen;
    srcLen = msgLen - hdrLen;

    /*
     * Set up the nonce. AJ sometimes uses a nonstandard nLen == 5,
     * which needs to be padded up to a minimum of 11
     */
    memcpy(nonceBuf, nonce, nLen);
    nLen = max(nLen, MIN_NONCE_LEN);

    /* qcom_crypto_op_ae_encrypt_final() header documentation
     * describes the required destination buffer size */
    destLen = msgLen + tagLen + 17;
    dest = AJ_Malloc(destLen);

    if (!dest || initAes(&obj, &op, QCOM_CRYPTO_MODE_ENCRYPT, key) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES encrypt setup failure\n"));
        AJ_Free(dest);
        return AJ_ERR_SECURITY;
    } else if (qcom_crypto_op_ae_init(op, nonceBuf, nLen, tagLen * 8, hdrLen, srcLen) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES encrypt op init failure\n"));
    } else if (hdrLen != 0 && qcom_crypto_op_ae_update_aad(op, msg, hdrLen) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AAD encrypt failure\n"));
    } else if (qcom_crypto_op_ae_encrypt_final(op, src, srcLen, dest, &destOutputLen, &tag[0], &tagOutputLen) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES encrypt failure\n"));
    } else {
        /* All calls in the chain succeeded, assemble the final data buffer.
         * Header bytes are already in place, only copy encrypted data
         * and tag.
         */
        memcpy(msg + hdrLen, dest, destOutputLen);
        memcpy(msg + hdrLen + destOutputLen, tag, tagLen);
        status = AJ_OK;
    }

    qcom_crypto_transient_obj_free(obj);
    qcom_crypto_op_free(op);
    AJ_MemZeroSecure(tag, tagLen);
    AJ_MemZeroSecure(dest, destLen);
    AJ_MemZeroSecure(nonceBuf, MAX_NONCE_LEN);
    AJ_Free(dest);

    return status;
}

/*
 * AES-CCM Decryption
 */
AJ_Status AJ_Decrypt_CCM(const uint8_t* key, uint8_t* msg, uint32_t msgLen, uint32_t hdrLen, uint8_t tagLen, const uint8_t* nonce, uint32_t nLen)
{
    qcom_crypto_obj_hdl_t obj;
    qcom_crypto_op_hdl_t op;
    uint8_t nonceBuf[MAX_NONCE_LEN] = { 0 };
    uint8_t* tag;
    uint8_t* src;
    uint32_t srcLen;
    uint8_t* dest;
    uint32_t destLen;
    uint32_t destOutputLen;
    AJ_Status status = AJ_ERR_SECURITY;

    src = msg + hdrLen;
    srcLen = msgLen - hdrLen;
    tag = msg + msgLen;

    /*
     * Set up the nonce. AJ sometimes uses a nonstandard nLen == 5,
     * which needs to be padded up to a minimum of 11
     */
    memcpy(nonceBuf, nonce, nLen);
    nLen = max(nLen, MIN_NONCE_LEN);

    /* qcom_crypto_op_ae_decrypt_final() header documentation
     * describes the required destination buffer size */
    destLen = msgLen + tagLen + 17;
    dest = AJ_Malloc(destLen);

    if (!dest || initAes(&obj, &op, QCOM_CRYPTO_MODE_DECRYPT, key) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES decrypt setup failure\n"));
        AJ_Free(dest);
        return AJ_ERR_SECURITY;
    } else if (qcom_crypto_op_ae_init(op, nonceBuf, nLen, tagLen * 8, hdrLen, srcLen) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES decrypt op init failure\n"));
    } else if (hdrLen != 0 && qcom_crypto_op_ae_update_aad(op, msg, hdrLen) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AAD decrypt failure\n"));
    } else if (qcom_crypto_op_ae_decrypt_final(op, src, srcLen, dest, &destOutputLen, tag, tagLen * 8) != A_CRYPTO_OK) {
        AJ_ErrPrintf(("AES decrypt failure\n"));
    } else {
        /* All calls in the chain succeeded, assemble the final data buffer */
        memcpy(msg + hdrLen, dest, destOutputLen);
        status = AJ_OK;
    }

    if (status != AJ_OK) {
        AJ_MemZeroSecure(msg, msgLen);
    }

    AJ_MemZeroSecure(tag, tagLen);
    AJ_MemZeroSecure(nonceBuf, MAX_NONCE_LEN);
    qcom_crypto_transient_obj_free(obj);
    qcom_crypto_op_free(op);
    AJ_Free(dest);

    return status;
}

/*
 * SHA2
 */

/*
 * Context is retrofitted in to the AJ_SHA256_Context struct's 'state' array.
 */
static const int CTX_STATE_VALID_IDX = 0;
static const int CTX_STATE_HANDLE_IDX = 1;
static const uint32_t CTX_VALID_MAGIC = 0x00c0ffee;

static uint32_t activeShaHandles = 0;

/* Confirm that a SHA context is marked as valid and is associated with
 * a crypto op in use by this module
 */
static uint32_t validateShaContext(AJ_SHA256_Context* context)
{
    uint32_t valid = 0;

    if (context->state[CTX_STATE_VALID_IDX] == CTX_VALID_MAGIC) {
        qcom_crypto_op_hdl_t op;
        op = context->state[CTX_STATE_HANDLE_IDX];
        if (activeShaHandles & (1 << op)) {
            valid = 1;
        }
    }

    return valid;
}

/* Free resources associated with a valid SHA context */
static void freeShaContext(AJ_SHA256_Context* context)
{
    /* See if this context refers to a crypto op handle already in use, free if it does. */
    if (validateShaContext(context)) {
        qcom_crypto_op_hdl_t op;
        op = context->state[CTX_STATE_HANDLE_IDX];
        activeShaHandles &= ~(1 << op);
        qcom_crypto_op_free(op);
    }
    memset(context, 0, sizeof(*context));
}

/*
 * Initialize SHA256 by allocating digest operation
 */
void AJ_SHA256_Init(AJ_SHA256_Context* context)
{
    qcom_crypto_op_hdl_t op;
    A_CRYPTO_STATUS status;

    AJ_InfoPrintf(("AJ_SHA256_Init(%p)\n", context));

    freeShaContext(context);

    status = qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &op);

    if (status == A_CRYPTO_OK) {
        context->state[CTX_STATE_VALID_IDX] = CTX_VALID_MAGIC;
        context->state[CTX_STATE_HANDLE_IDX] = op;
        activeShaHandles |= (1 << op);
    }
}

/*
 * Update digest with new data
 */
void AJ_SHA256_Update(AJ_SHA256_Context* context, const uint8_t* buf, size_t bufSize)
{
    qcom_crypto_op_hdl_t op;
    A_CRYPTO_STATUS status;

    AJ_InfoPrintf(("AJ_SHA256_Update(%p, %p, %d)\n", context, buf, bufSize));

    if (!validateShaContext(context)) {
        AJ_ErrPrintf(("SHA256 update not initialized\n"));
        return;
    }

    op = context->state[CTX_STATE_HANDLE_IDX];
    status = qcom_crypto_op_digest_update(op, (uint8_t*)buf, bufSize);

    if (status != A_CRYPTO_OK) {
        AJ_ErrPrintf(("SHA256 update error\n"));
        freeShaContext(context);
    }
}

/*
 * Retrieve the current digest value, optionally keeping the digest operation active.
 */
void AJ_SHA256_GetDigest(AJ_SHA256_Context* context, uint8_t* digest, const uint8_t keepAlive)
{
    qcom_crypto_op_hdl_t op;
    qcom_crypto_op_hdl_t finalOp;
    A_CRYPTO_STATUS status;
    A_UINT32 digestLen;

    AJ_InfoPrintf(("AJ_SHA256_GetDigest(%p, %p, %u)\n", context, digest, (uint32_t)keepAlive));

    if (!validateShaContext(context)) {
        AJ_ErrPrintf(("SHA256 get not initialized\n"));
        return;
    }

    op = context->state[CTX_STATE_HANDLE_IDX];

    if (keepAlive) {
        /* Retrieving the digest from qcom_crypto closes the digest
         * operation.  To support 'keepAlive', the operation must be
         * copied. This temporary copy is used to calculate the
         * current digest value, and the original is unchanged and may
         * continue to be updated.
         */
        status = qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_SHA256, QCOM_CRYPTO_MODE_DIGEST, 0, &finalOp);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("SHA256 get alloc failure\n"));
            return;
        }
        status = qcom_crypto_op_copy(finalOp, op);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("SHA256 get copy failure\n"));
            qcom_crypto_op_free(finalOp);
            return;
        }
    } else {
        /* Not keeping alive, so finalize the existing op */
        finalOp = op;
    }

    status = qcom_crypto_op_digest_dofinal(finalOp, (void*)NULL, 0, digest, &digestLen);
    if (status != A_CRYPTO_OK || digestLen != SHA256_DIGEST_LENGTH) {
        AJ_ErrPrintf(("SHA256 final failure\n"));
    }

    if (keepAlive) {
        /* Free the copied op, not tracked as part of context */
        qcom_crypto_op_free(finalOp);
    } else {
        /* Done with context if not keeping hash alive */
        freeShaContext(context);
    }
}

/*
 * Get the digest and release resources
 */
void AJ_SHA256_Final(AJ_SHA256_Context* context, uint8_t* digest)
{
    AJ_InfoPrintf(("AJ_SHA256_Final(%p, %p)\n", context, digest));

    if (!digest) {
        freeShaContext(context);
    } else {
        AJ_SHA256_GetDigest(context, digest, 0);
    }
}

/*
 * AllJoyn SHA256 Pseudo-Random Function
 */
AJ_Status AJ_Crypto_PRF_SHA256(const uint8_t** inputs, const uint8_t* lengths, uint32_t count, uint8_t* out, uint32_t outLen)
{
    uint8_t digest[SHA256_DIGEST_LENGTH];
    A_UINT32 digestLen = 0;
    A_UINT32 digestOffset;
    qcom_crypto_obj_hdl_t obj;
    qcom_crypto_attrib_t attribute;
    A_CRYPTO_STATUS status = A_CRYPTO_OK;
    uint8_t* message;
    uint8_t* insert;
    A_UINT32 messageLen;
    uint32_t i;
    uint8_t firstPass;

    AJ_InfoPrintf(("AJ_Crypto_PRF_SHA256(%p, %p, %u, %p, %u)\n", inputs, lengths, count, out, outLen));

    /*
     * The HMAC API only gives one pass at calculating the digest, so
     * the inputs must be combined before calculating the HMAC digest.
     * Set up a combined message buffer with the concatenation of
     * everything in the 'inputs' array (except for the first value in
     * the array, which is the key). There is extra space at the
     * beginning of the message buffer for the intermediate digest.
     */
    if (outLen <= SHA256_DIGEST_LENGTH) {
        /* Only one HMAC pass needed, shorter buffer works */
        digestOffset = 0;
    } else {
        /* Multiple HMAC passes needed, buffer needs room for digest */
        digestOffset = SHA256_DIGEST_LENGTH;
    }
    messageLen = digestOffset;
    for (i = 1; i < count; i++) {
        messageLen += lengths[i];
    }

    if (messageLen == SHA256_DIGEST_LENGTH) {
        AJ_ErrPrintf(("No input data for SHA256 PRF\n"));
        return AJ_ERR_INVALID;
    }

    message = AJ_Malloc(messageLen);
    if (!message) {
        return AJ_ERR_RESOURCES;
    }

    insert = message + digestOffset;
    for (i = 1; i < count; i++) {
        memcpy(insert, inputs[i], lengths[i]);
        insert += lengths[i];
    }

    /*
     * Create the overall HMAC object which holds the key. This is
     * reused across multiple digest iterations.
     */
    status = qcom_crypto_transient_obj_alloc(QCOM_CRYPTO_OBJ_TYPE_HMAC_SHA256, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, &obj);
    if (status != A_CRYPTO_OK) {
        AJ_ErrPrintf(("PRF obj failure\n"));
        return AJ_ERR_SECURITY;
    }

    set_ref_attribute(&attribute, QCOM_CRYPTO_ATTR_SECRET_VALUE, (uint8_t*)inputs[0], lengths[0]);
    status = qcom_crypto_transient_obj_populate(obj, &attribute, 1);
    if (status != A_CRYPTO_OK) {
        AJ_ErrPrintf(("PRF populate failure\n"));
        qcom_crypto_transient_obj_free(obj);
        return AJ_ERR_SECURITY;
    }

    /*
     * Generate pseudorandom bytes.  The first pass is an HMAC hash of
     * the inputs.  Subsequent passes prepend the hash of the previous
     * pass to the inputs.
     */
    firstPass = 1;
    while (outLen) {
        qcom_crypto_op_hdl_t op;
        uint32_t copyBytes = 0;

        /* After the first pass, include the digest from the previous pass */
        if (!firstPass) {
            memcpy(message, digest, SHA256_DIGEST_LENGTH);
        }

        /* Need a fresh HMAC op for each pass */
        status = qcom_crypto_op_alloc(QCOM_CRYPTO_ALG_HMAC_SHA256, QCOM_CRYPTO_MODE_MAC, QCOM_CRYPTO_HMAC_SHA256_MAX_KEY_BITS, &op);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("PRF op failure\n"));
            break;
        }

        status = qcom_crypto_op_key_set(op, obj);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("PRF key failure\n"));
            qcom_crypto_op_free(op);
            break;
        }

        status = qcom_crypto_op_mac_init(op, NULL, 0);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("PRF mac init failure\n"));
            qcom_crypto_op_free(op);
            break;
        }

        status = qcom_crypto_op_mac_compute_final(op, (void*)(message + digestOffset), messageLen - digestOffset, digest, &digestLen);
        if (status != A_CRYPTO_OK) {
            AJ_ErrPrintf(("PRF mac compute failure\n"));
            qcom_crypto_op_free(op);
            break;
        }

        qcom_crypto_op_free(op);

        if (outLen < SHA256_DIGEST_LENGTH) {
            copyBytes = outLen;
        } else {
            copyBytes = SHA256_DIGEST_LENGTH;
        }
        memcpy(out, digest, copyBytes);
        outLen -= copyBytes;
        out += copyBytes;
        digestOffset = 0;
        firstPass = 0;
    }

    qcom_crypto_transient_obj_free(obj);
    AJ_MemZeroSecure(digest, SHA256_DIGEST_LENGTH);
    AJ_MemZeroSecure(message, messageLen);
    AJ_Free(message);

    AJ_InfoPrintf(("AJ_Crypto_PRF_SHA256() -> %u\n", status));

    return status == A_CRYPTO_OK ? AJ_OK : AJ_ERR_SECURITY;
}

