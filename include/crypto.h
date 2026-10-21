/*
 * crypto.h - Kriptografik fonksiyon prototipleri
 * PBKDF2-HMAC-SHA256 anahtar turetme, AES-256-GCM sifreleme/cozme, SHA-256 hash
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include "common.h"

/*
 * crypto_derive_key - Paroladan PBKDF2 ile sifreleme anahtari turetir.
 *
 * @param password      Kullanicinin parolasi (null-terminated)
 * @param salt          16-byte salt degeri
 * @param out_key       Turetilen 32-byte anahtar
 * @return              VAULT_OK veya VAULT_ERR_CRYPTO
 */
VaultError crypto_derive_key(const char *password,
                             const uint8_t salt[SALT_LEN],
                             uint8_t out_key[KEY_LEN]);

/*
 * crypto_sha256 - Girdinin 32-byte SHA-256 ozetini hesaplar.
 *
 * @param data          Girdi verisi
 * @param len           Girdi uzunlugu
 * @param out_hash      32-byte cikti tamponu
 * @return              VAULT_OK veya VAULT_ERR_CRYPTO
 */
VaultError crypto_sha256(const uint8_t *data, size_t len, uint8_t out_hash[32]);

/*
 * crypto_vault_filename - Kasa sifresinin SHA-256 ozetinin ilk 8 hex
 *                         karakterinden vault_<hash8>.dat dosya adini uretir.
 *
 * @param vault_password  Kullanicinin belirledigi kasa sifresi
 * @param out_filename    Dosya adinin yazilacagi tampon
 * @param max_len         Tampon boyutu
 * @return                VAULT_OK veya VAULT_ERR_CRYPTO
 */
VaultError crypto_vault_filename(const char *vault_password,
                                 char *out_filename,
                                 size_t max_len);

/*
 * crypto_encrypt - Plaintext veriyi AES-256-GCM ile sifreler.
 *
 * @param key           32-byte sifreleme anahtari
 * @param plaintext     Sifrelenecek veri
 * @param plaintext_len Veri uzunlugu
 * @param out_blob      Cikti: IV, tag ve ciphertext doldurulur
 * @return              VAULT_OK veya VAULT_ERR_CRYPTO / VAULT_ERR_MEMORY
 */
VaultError crypto_encrypt(const uint8_t key[KEY_LEN],
                          const uint8_t *plaintext,
                          uint32_t plaintext_len,
                          EncryptedBlob *out_blob);

/*
 * crypto_decrypt - AES-256-GCM ile sifrelenmis veriyi cozer.
 *
 * @param key            32-byte sifreleme anahtari
 * @param blob           Sifreli veri blogu (IV, tag, ciphertext)
 * @param out_plaintext  Cikti: cozulmus veri (heap'te tahsis edilir)
 * @param out_len        Cikti: cozulmus veri uzunlugu
 * @return               VAULT_OK, VAULT_ERR_AUTH (tag eslesmezse),
 *                       VAULT_ERR_CRYPTO veya VAULT_ERR_MEMORY
 */
VaultError crypto_decrypt(const uint8_t key[KEY_LEN],
                          const EncryptedBlob *blob,
                          uint8_t **out_plaintext,
                          uint32_t *out_len);

/*
 * crypto_random_bytes - Kriptografik olarak guvenli rasgele byte uretir.
 *
 * @param buf   Hedef tampon
 * @param len   Uretilecek byte sayisi
 * @return      VAULT_OK veya VAULT_ERR_CRYPTO
 */
VaultError crypto_random_bytes(uint8_t *buf, size_t len);

/*
 * encrypted_blob_free - EncryptedBlob icindeki ciphertext bellegini serbest birakir.
 *
 * @param blob  Temizlenecek blob
 */
void encrypted_blob_free(EncryptedBlob *blob);

#endif /* CRYPTO_H */
