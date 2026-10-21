/*
 * crypto.c - Kriptografik islemler
 * PBKDF2-HMAC-SHA256 anahtar turetme, AES-256-GCM sifreleme/cozme, SHA-256 ozet
 *
 * OpenSSL EVP API kullanir.
 */

#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -- Anahtar Turetme ----------------------------------------------------- */

VaultError crypto_derive_key(const char *password,
                             const uint8_t salt[SALT_LEN],
                             uint8_t out_key[KEY_LEN])
{
    if (!password || !salt || !out_key) {
        return VAULT_ERR_CRYPTO;
    }

    int ret = PKCS5_PBKDF2_HMAC(password,
                                 (int)strlen(password),
                                 salt,
                                 SALT_LEN,
                                 KDF_ITERATIONS,
                                 EVP_sha256(),
                                 KEY_LEN,
                                 out_key);
    if (ret != 1) {
        return VAULT_ERR_CRYPTO;
    }

    return VAULT_OK;
}

/* -- SHA-256 Ozet Hesaplama --------------------------------------------- */

VaultError crypto_sha256(const uint8_t *data, size_t len, uint8_t out_hash[32])
{
    if (!data || !out_hash) {
        return VAULT_ERR_CRYPTO;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return VAULT_ERR_CRYPTO;
    }

    VaultError result = VAULT_OK;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    if (EVP_DigestUpdate(ctx, data, len) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    unsigned int out_len = 0;
    if (EVP_DigestFinal_ex(ctx, out_hash, &out_len) != 1 || out_len != 32) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

cleanup:
    EVP_MD_CTX_free(ctx);
    return result;
}

/* -- Kasa Dosya Adi Uretimi (vault_<hash_ilk8karakter>.dat) ------------- */

VaultError crypto_vault_filename(const char *vault_password,
                                 char *out_filename,
                                 size_t max_len)
{
    if (!vault_password || !out_filename || max_len < 24) {
        return VAULT_ERR_INPUT;
    }

    uint8_t hash[32];
    VaultError err = crypto_sha256((const uint8_t *)vault_password,
                                   strlen(vault_password),
                                   hash);
    if (err != VAULT_OK) {
        return err;
    }

    /* Ilk 4 byte -> 8 hex karakter */
    snprintf(out_filename, max_len, "vault_%02x%02x%02x%02x.dat",
             hash[0], hash[1], hash[2], hash[3]);

    return VAULT_OK;
}

/* -- AES-256-GCM Sifreleme ---------------------------------------------- */

VaultError crypto_encrypt(const uint8_t key[KEY_LEN],
                          const uint8_t *plaintext,
                          uint32_t plaintext_len,
                          EncryptedBlob *out_blob)
{
    if (!key || !plaintext || !out_blob) {
        return VAULT_ERR_CRYPTO;
    }

    EVP_CIPHER_CTX *ctx = NULL;
    VaultError result = VAULT_OK;

    /* Rasgele IV uret */
    result = crypto_random_bytes(out_blob->iv, IV_LEN);
    if (result != VAULT_OK) {
        return result;
    }

    /* Ciphertext icin bellek tahsis et */
    out_blob->ciphertext = (uint8_t *)malloc(plaintext_len);
    if (!out_blob->ciphertext) {
        return VAULT_ERR_MEMORY;
    }
    out_blob->ciphertext_len = plaintext_len;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(out_blob->ciphertext);
        out_blob->ciphertext = NULL;
        return VAULT_ERR_CRYPTO;
    }

    /* Sifreleme baslat */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* IV uzunlugunu ayarla */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Key ve IV'yi set et */
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, out_blob->iv) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Plaintext'i sifrele */
    int out_len = 0;
    if (EVP_EncryptUpdate(ctx, out_blob->ciphertext, &out_len,
                          plaintext, (int)plaintext_len) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Finalize */
    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, out_blob->ciphertext + out_len, &final_len) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Authentication tag'i al */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, out_blob->tag) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

cleanup:
    EVP_CIPHER_CTX_free(ctx);

    if (result != VAULT_OK) {
        free(out_blob->ciphertext);
        out_blob->ciphertext = NULL;
        out_blob->ciphertext_len = 0;
    }

    return result;
}

/* -- AES-256-GCM Cozme --------------------------------------------------- */

VaultError crypto_decrypt(const uint8_t key[KEY_LEN],
                          const EncryptedBlob *blob,
                          uint8_t **out_plaintext,
                          uint32_t *out_len)
{
    if (!key || !blob || !out_plaintext || !out_len) {
        return VAULT_ERR_CRYPTO;
    }

    EVP_CIPHER_CTX *ctx = NULL;
    VaultError result = VAULT_OK;

    /* Plaintext icin bellek tahsis et */
    *out_plaintext = (uint8_t *)malloc(blob->ciphertext_len);
    if (!*out_plaintext) {
        return VAULT_ERR_MEMORY;
    }

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(*out_plaintext);
        *out_plaintext = NULL;
        return VAULT_ERR_CRYPTO;
    }

    /* Cozme baslat */
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* IV uzunlugunu ayarla */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Key ve IV'yi set et */
    if (EVP_DecryptInit_ex(ctx, NULL, NULL, key, blob->iv) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Ciphertext'i coz */
    int out_length = 0;
    if (EVP_DecryptUpdate(ctx, *out_plaintext, &out_length,
                          blob->ciphertext, (int)blob->ciphertext_len) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Expected tag'i set et */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN,
                            (void *)blob->tag) != 1) {
        result = VAULT_ERR_CRYPTO;
        goto cleanup;
    }

    /* Finalize - tag dogrulamasi burada yapilir */
    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx, *out_plaintext + out_length, &final_len) != 1) {
        /* Tag eslesmedi -> yanlis anahtar veya veri bozuk */
        result = VAULT_ERR_AUTH;
        goto cleanup;
    }

    *out_len = (uint32_t)(out_length + final_len);

cleanup:
    EVP_CIPHER_CTX_free(ctx);

    if (result != VAULT_OK) {
        if (*out_plaintext) {
            /* Hassas veriyi sifirla */
            OPENSSL_cleanse(*out_plaintext, blob->ciphertext_len);
            free(*out_plaintext);
            *out_plaintext = NULL;
        }
        *out_len = 0;
    }

    return result;
}

/* -- Rasgele Byte Uretimi ------------------------------------------------ */

VaultError crypto_random_bytes(uint8_t *buf, size_t len)
{
    if (!buf || len == 0) {
        return VAULT_ERR_CRYPTO;
    }

    if (RAND_bytes(buf, (int)len) != 1) {
        return VAULT_ERR_CRYPTO;
    }

    return VAULT_OK;
}

/* -- EncryptedBlob Temizleme --------------------------------------------- */

void encrypted_blob_free(EncryptedBlob *blob)
{
    if (!blob) return;

    if (blob->ciphertext) {
        free(blob->ciphertext);
        blob->ciphertext = NULL;
    }
    blob->ciphertext_len = 0;
}
