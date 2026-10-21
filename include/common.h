/*
 * common.h - Ortak tipler, sabitler ve hata kodlari
 * CLI Not ve Parola Yoneticisi - 2 Kademeli Guvenlik Mimarisi
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* -- Kriptografik Sabitler ----------------------------------------------- */
#define SALT_LEN        16      /* PBKDF2 salt uzunlugu (byte)            */
#define IV_LEN          12      /* AES-256-GCM nonce uzunlugu (byte)      */
#define TAG_LEN         16      /* GCM authentication tag uzunlugu (byte) */
#define KEY_LEN         32      /* AES-256 anahtar uzunlugu (byte)        */
#define KDF_ITERATIONS  100000  /* PBKDF2 iterasyon sayisi                */

/* -- 1. Asama: Ana Uygulama Kimlik Dogrulama Sabitleri ------------------ */
#define APP_AUTH_FILENAME   "app_auth.dat"
#define APP_AUTH_MAGIC      "APAU"
#define APP_AUTH_MAGIC_LEN  4

/* -- 2. Asama: Kasa Dosya Formati Sabitleri ------------------------------ */
#define VAULT_MAGIC         "SVLT"  /* Magic bytes (4 byte)                   */
#define VAULT_MAGIC_LEN     4
#define VAULT_VERSION       0x02    /* Dosya format versiyonu (v2)            */
#define VERIFY_BLOCK_LEN    32      /* Kasa parola dogrulama blogu uzunlugu   */
#define MAX_VAULT_NAME_LEN  64      /* Kasa adi maksimum uzunlugu             */
#define MAX_FILENAME_LEN    64      /* Dosya adi maksimum uzunlugu            */
#define MAX_VAULTS_LIMIT    64      /* Listelenebilir maksimum kasa sayisi    */

/* -- Uygulama Sabitleri -------------------------------------------------- */
#define MAX_INPUT_LEN       1024    /* Kullanici girisi maksimum uzunluk       */
#define MAX_TITLE_LEN       256     /* Kayit basligi maksimum uzunluk          */
#define MAX_CONTENT_LEN     4096    /* Kayit icerigi maksimum uzunluk          */
#define INITIAL_CAPACITY    8       /* Baslangic kayit dizisi kapasitesi       */

/* -- Hata Kodlari -------------------------------------------------------- */
typedef enum {
    VAULT_OK         =  0,  /* Basarili                     */
    VAULT_ERR_CRYPTO = -1,  /* Kriptografik hata            */
    VAULT_ERR_AUTH   = -2,  /* Kimlik dogrulama hatasi      */
    VAULT_ERR_IO     = -3,  /* Dosya I/O hatasi             */
    VAULT_ERR_MEMORY = -4,  /* Bellek tahsis hatasi         */
    VAULT_ERR_INPUT  = -5   /* Gecersiz kullanici girisi    */
} VaultError;

/* -- 1. Asama: Ana Uygulama Yetkilendirme Verisi ------------------------- */
typedef struct {
    char    magic[APP_AUTH_MAGIC_LEN]; /* "APAU" */
    uint8_t salt[SALT_LEN];            /* PBKDF2 tuzu */
    uint8_t hash[KEY_LEN];             /* Ana sifrenin PBKDF2 hash ozeti */
} AppAuth;

/* -- Sifreli Veri Blogu ------------------------------------------------- */
typedef struct {
    uint8_t  iv[IV_LEN];          /* Initialization Vector          */
    uint8_t  tag[TAG_LEN];        /* GCM Authentication Tag         */
    uint8_t *ciphertext;          /* Sifreli veri (heap allocated)  */
    uint32_t ciphertext_len;      /* Sifreli veri uzunlugu          */
} EncryptedBlob;

/* -- Vault Kayit Yapisi (sifreli, diskteki haliyle) --------------------- */
typedef struct {
    EncryptedBlob encrypted_title;    /* Sifreli baslik    */
    EncryptedBlob encrypted_content;  /* Sifreli icerik    */
} VaultRecord;

/* -- Vault Dosya Basligi (v2) -------------------------------------------- */
typedef struct {
    char     magic[VAULT_MAGIC_LEN];               /* "SVLT"                  */
    uint8_t  version;                              /* Format versiyonu (0x02) */
    char     vault_name[MAX_VAULT_NAME_LEN];       /* Kasa adi (orn: Kisisel) */
    uint8_t  salt[SALT_LEN];                       /* KDF salt                */
    uint8_t  verify_iv[IV_LEN];                    /* Dogrulama blogu IV     */
    uint8_t  verify_tag[TAG_LEN];                  /* Dogrulama blogu tag    */
    uint8_t  verify_ciphertext[VERIFY_BLOCK_LEN];  /* Sifreli dogrulama blogu*/
} VaultHeader;

/* -- Vault Ana Yapisi (RAM'de acik olan kasa) ---------------------------- */
typedef struct {
    char         filename[MAX_FILENAME_LEN]; /* orn: "vault_a1b2c3d4.dat"    */
    VaultHeader  header;                     /* Dosya basligi                 */
    VaultRecord *records;                    /* Kayit dizisi (dinamik)        */
    uint32_t     record_count;               /* Mevcut kayit sayisi           */
    uint32_t     capacity;                   /* Dizi kapasitesi               */
    uint8_t      master_key[KEY_LEN];        /* Kasa sifreleme anahtari       */
} Vault;

/* -- Kasa Listeleme Bilgisi --------------------------------------------- */
typedef struct {
    char     filename[MAX_FILENAME_LEN];
    char     vault_name[MAX_VAULT_NAME_LEN];
    uint32_t record_count;
} VaultInfo;

#endif /* COMMON_H */
