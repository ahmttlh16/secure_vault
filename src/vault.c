/*
 * vault.c - Kasa (Vault) kayit yonetimi
 * Kayit Ekleme, Listeleme, Duzenleme, Silme ve bellek yonetimi
 */

#include "vault.h"
#include "crypto.h"
#include "storage.h"
#include <openssl/crypto.h>   /* OPENSSL_cleanse */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -- Vault Baslatma ------------------------------------------------------ */

VaultError vault_init(Vault *vault)
{
    if (!vault) return VAULT_ERR_MEMORY;

    memset(vault, 0, sizeof(Vault));

    vault->records = (VaultRecord *)calloc(INITIAL_CAPACITY, sizeof(VaultRecord));
    if (!vault->records) {
        return VAULT_ERR_MEMORY;
    }

    vault->capacity = INITIAL_CAPACITY;
    vault->record_count = 0;

    return VAULT_OK;
}

/* -- Yeni Kayit Ekleme --------------------------------------------------- */

VaultError vault_add_record_direct(Vault *vault, const char *title, const char *content)
{
    if (!vault || !title || !content) return VAULT_ERR_INPUT;
    size_t tlen = strlen(title);
    size_t clen = strlen(content);
    if (tlen == 0 || clen == 0) return VAULT_ERR_INPUT;

    /* Kapasite kontrolu */
    if (vault->record_count >= vault->capacity) {
        uint32_t new_cap = vault->capacity == 0 ? INITIAL_CAPACITY : vault->capacity * 2;
        VaultRecord *new_records = (VaultRecord *)realloc(
            vault->records, new_cap * sizeof(VaultRecord));
        if (!new_records) {
            return VAULT_ERR_MEMORY;
        }
        vault->records = new_records;
        vault->capacity = new_cap;
    }

    VaultRecord *rec = &vault->records[vault->record_count];
    memset(rec, 0, sizeof(VaultRecord));

    /* Basligi sifrele */
    VaultError err = crypto_encrypt(vault->master_key,
                                    (const uint8_t *)title,
                                    (uint32_t)tlen,
                                    &rec->encrypted_title);
    if (err != VAULT_OK) {
        return err;
    }

    /* Icerigi sifrele */
    err = crypto_encrypt(vault->master_key,
                         (const uint8_t *)content,
                         (uint32_t)clen,
                         &rec->encrypted_content);
    if (err != VAULT_OK) {
        encrypted_blob_free(&rec->encrypted_title);
        return err;
    }

    vault->record_count++;

    /* Dosyaya kaydet */
    err = storage_save_all(vault);
    if (err != VAULT_OK) {
        vault->record_count--;
        encrypted_blob_free(&rec->encrypted_title);
        encrypted_blob_free(&rec->encrypted_content);
        return err;
    }

    return VAULT_OK;
}

VaultError vault_get_record_decrypted(const Vault *vault, uint32_t index,
                                      char *out_title, size_t title_cap,
                                      char *out_content, size_t content_cap)
{
    if (!vault || index >= vault->record_count) return VAULT_ERR_INPUT;

    const VaultRecord *rec = &vault->records[index];

    if (out_title && title_cap > 0) {
        uint8_t *dec_title = NULL;
        uint32_t dec_title_len = 0;
        VaultError err = crypto_decrypt(vault->master_key,
                                        &rec->encrypted_title,
                                        &dec_title,
                                        &dec_title_len);
        if (err != VAULT_OK) return err;

        size_t copy_len = dec_title_len < title_cap - 1 ? dec_title_len : title_cap - 1;
        memcpy(out_title, dec_title, copy_len);
        out_title[copy_len] = '\0';

        OPENSSL_cleanse(dec_title, dec_title_len);
        free(dec_title);
    }

    if (out_content && content_cap > 0) {
        uint8_t *dec_content = NULL;
        uint32_t dec_content_len = 0;
        VaultError err = crypto_decrypt(vault->master_key,
                                        &rec->encrypted_content,
                                        &dec_content,
                                        &dec_content_len);
        if (err != VAULT_OK) return err;

        size_t copy_len = dec_content_len < content_cap - 1 ? dec_content_len : content_cap - 1;
        memcpy(out_content, dec_content, copy_len);
        out_content[copy_len] = '\0';

        OPENSSL_cleanse(dec_content, dec_content_len);
        free(dec_content);
    }

    return VAULT_OK;
}

VaultError vault_add_record(Vault *vault)
{
    if (!vault) return VAULT_ERR_INPUT;

    char title[MAX_TITLE_LEN];
    char content[MAX_CONTENT_LEN];

    memset(title, 0, sizeof(title));
    memset(content, 0, sizeof(content));

    printf("\n  +-- Yeni Kayit Ekle -------------------------+\n");

    /* Baslik al */
    printf("  | Baslik: ");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        fprintf(stderr, "  | [!] Giris okunamadi.\n");
        return VAULT_ERR_INPUT;
    }

    size_t tlen = strlen(title);
    while (tlen > 0 && (title[tlen - 1] == '\n' || title[tlen - 1] == '\r')) {
        title[tlen - 1] = '\0';
        tlen--;
    }

    if (tlen == 0) {
        fprintf(stderr, "  | [!] Baslik bos olamaz.\n");
        return VAULT_ERR_INPUT;
    }

    /* Icerik al */
    printf("  | Icerik (not/parola): ");
    if (fgets(content, sizeof(content), stdin) == NULL) {
        fprintf(stderr, "  | [!] Giris okunamadi.\n");
        OPENSSL_cleanse(title, sizeof(title));
        return VAULT_ERR_INPUT;
    }

    size_t clen = strlen(content);
    while (clen > 0 && (content[clen - 1] == '\n' || content[clen - 1] == '\r')) {
        content[clen - 1] = '\0';
        clen--;
    }

    if (clen == 0) {
        fprintf(stderr, "  | [!] Icerik bos olamaz.\n");
        OPENSSL_cleanse(title, sizeof(title));
        return VAULT_ERR_INPUT;
    }

    VaultError err = vault_add_record_direct(vault, title, content);
    if (err != VAULT_OK) {
        fprintf(stderr, "  | [!] Kayit eklenemedi: %d\n", err);
    } else {
        printf("  | [OK] Kayit basariyla eklendi.\n");
    }

    printf("  +--------------------------------------------+\n");

    OPENSSL_cleanse(title, sizeof(title));
    OPENSSL_cleanse(content, sizeof(content));

    return err;
}

/* -- Kayitlari Listeleme ------------------------------------------------- */

VaultError vault_list_records(const Vault *vault)
{
    if (!vault) return VAULT_ERR_INPUT;

    if (vault->record_count == 0) {
        printf("\n  +-- Kayitlar --------------------------------+\n");
        printf("  | Henuz kayit bulunmuyor.                    |\n");
        printf("  +--------------------------------------------+\n");
        return VAULT_OK;
    }

    printf("\n  +-- Kayitlar (%u adet) -------------------------+\n",
           vault->record_count);

    for (uint32_t i = 0; i < vault->record_count; i++) {
        const VaultRecord *rec = &vault->records[i];

        uint8_t *dec_title = NULL;
        uint32_t dec_title_len = 0;

        VaultError err = crypto_decrypt(vault->master_key,
                                         &rec->encrypted_title,
                                         &dec_title,
                                         &dec_title_len);
        if (err != VAULT_OK) {
            printf("  | %u) [!] Baslik cozulemedi\n", i + 1);
            continue;
        }

        uint8_t *dec_content = NULL;
        uint32_t dec_content_len = 0;

        err = crypto_decrypt(vault->master_key,
                              &rec->encrypted_content,
                              &dec_content,
                              &dec_content_len);
        if (err != VAULT_OK) {
            printf("  | %u) %.*s -> [!] Icerik cozulemedi\n",
                   i + 1, (int)dec_title_len, dec_title);
            OPENSSL_cleanse(dec_title, dec_title_len);
            free(dec_title);
            continue;
        }

        printf("  |\n");
        printf("  |  [%u] Baslik : %.*s\n", i + 1,
               (int)dec_title_len, dec_title);
        printf("  |      Icerik : %.*s\n",
               (int)dec_content_len, dec_content);

        OPENSSL_cleanse(dec_title, dec_title_len);
        OPENSSL_cleanse(dec_content, dec_content_len);
        free(dec_title);
        free(dec_content);
    }

    printf("  |\n");
    printf("  +--------------------------------------------+\n");

    return VAULT_OK;
}

/* -- Kayit Duzenleme ----------------------------------------------------- */

VaultError vault_edit_record(Vault *vault)
{
    if (!vault) return VAULT_ERR_INPUT;

    if (vault->record_count == 0) {
        printf("\n  +-- Kayit Duzenle ---------------------------+\n");
        printf("  | Duzenlenecek kayit bulunmuyor.             |\n");
        printf("  +--------------------------------------------+\n");
        return VAULT_OK;
    }

    /* Once basliklari kisaca listele */
    printf("\n  +-- Duzenlenebilecek Kayitlar ---------------+\n");
    for (uint32_t i = 0; i < vault->record_count; i++) {
        uint8_t *dec_title = NULL;
        uint32_t dec_title_len = 0;
        if (crypto_decrypt(vault->master_key, &vault->records[i].encrypted_title,
                           &dec_title, &dec_title_len) == VAULT_OK) {
            printf("  |  [%u] %.*s\n", i + 1, (int)dec_title_len, dec_title);
            OPENSSL_cleanse(dec_title, dec_title_len);
            free(dec_title);
        } else {
            printf("  |  [%u] (Cozulemedi)\n", i + 1);
        }
    }
    printf("  +--------------------------------------------+\n");

    char input[32];
    printf("  Duzenlenecek kayit no [1-%u] (Iptal icin 0): ", vault->record_count);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return VAULT_ERR_INPUT;
    }

    int choice = atoi(input);
    if (choice <= 0 || (uint32_t)choice > vault->record_count) {
        printf("  [!] Islem iptal edildi.\n");
        return VAULT_OK;
    }

    uint32_t idx = (uint32_t)(choice - 1);
    VaultRecord *rec = &vault->records[idx];

    /* Mevcut baslik ve icerigi coz */
    uint8_t *cur_title = NULL;
    uint32_t cur_title_len = 0;
    uint8_t *cur_content = NULL;
    uint32_t cur_content_len = 0;

    if (crypto_decrypt(vault->master_key, &rec->encrypted_title, &cur_title, &cur_title_len) != VAULT_OK ||
        crypto_decrypt(vault->master_key, &rec->encrypted_content, &cur_content, &cur_content_len) != VAULT_OK) {
        fprintf(stderr, "  [!] Kayit verisi cozulemedi.\n");
        if (cur_title) { OPENSSL_cleanse(cur_title, cur_title_len); free(cur_title); }
        if (cur_content) { OPENSSL_cleanse(cur_content, cur_content_len); free(cur_content); }
        return VAULT_ERR_CRYPTO;
    }

    printf("\n  Secilen Kayit [%u]:\n", choice);
    printf("  Mevcut Baslik : %.*s\n", (int)cur_title_len, cur_title);
    printf("  Mevcut Icerik : %.*s\n\n", (int)cur_content_len, cur_content);

    char new_title[MAX_TITLE_LEN];
    char new_content[MAX_CONTENT_LEN];
    memset(new_title, 0, sizeof(new_title));
    memset(new_content, 0, sizeof(new_content));

    printf("  Yeni Baslik (Degistirmemek icin Enter): ");
    if (fgets(new_title, sizeof(new_title), stdin) != NULL) {
        size_t len = strlen(new_title);
        while (len > 0 && (new_title[len - 1] == '\n' || new_title[len - 1] == '\r')) {
            new_title[len - 1] = '\0';
            len--;
        }
    }

    printf("  Yeni Icerik (Degistirmemek icin Enter): ");
    if (fgets(new_content, sizeof(new_content), stdin) != NULL) {
        size_t len = strlen(new_content);
        while (len > 0 && (new_content[len - 1] == '\n' || new_content[len - 1] == '\r')) {
            new_content[len - 1] = '\0';
            len--;
        }
    }

    bool title_changed = (strlen(new_title) > 0);
    bool content_changed = (strlen(new_content) > 0);

    if (!title_changed && !content_changed) {
        printf("  [!] Hicbir alan degistirilmedi.\n");
        OPENSSL_cleanse(cur_title, cur_title_len);
        OPENSSL_cleanse(cur_content, cur_content_len);
        free(cur_title);
        free(cur_content);
        return VAULT_OK;
    }

    /* Yeni basligi sifrele */
    if (title_changed) {
        EncryptedBlob new_title_blob;
        memset(&new_title_blob, 0, sizeof(new_title_blob));
        if (crypto_encrypt(vault->master_key, (const uint8_t *)new_title, (uint32_t)strlen(new_title), &new_title_blob) == VAULT_OK) {
            encrypted_blob_free(&rec->encrypted_title);
            rec->encrypted_title = new_title_blob;
        } else {
            fprintf(stderr, "  [!] Yeni baslik sifrelenemedi.\n");
        }
    }

    /* Yeni icerigi sifrele */
    if (content_changed) {
        EncryptedBlob new_content_blob;
        memset(&new_content_blob, 0, sizeof(new_content_blob));
        if (crypto_encrypt(vault->master_key, (const uint8_t *)new_content, (uint32_t)strlen(new_content), &new_content_blob) == VAULT_OK) {
            encrypted_blob_free(&rec->encrypted_content);
            rec->encrypted_content = new_content_blob;
        } else {
            fprintf(stderr, "  [!] Yeni icerik sifrelenemedi.\n");
        }
    }

    /* Dosyaya kaydet */
    VaultError err = storage_save_all(vault);
    if (err == VAULT_OK) {
        printf("\n  [OK] Kayit basariyla guncellendi ve dosyaya kaydedildi.\n");
    } else {
        fprintf(stderr, "\n  [!] Dosya guncellenirken hata olustu!\n");
    }

    /* Bellek temizligi */
    OPENSSL_cleanse(cur_title, cur_title_len);
    OPENSSL_cleanse(cur_content, cur_content_len);
    free(cur_title);
    free(cur_content);
    OPENSSL_cleanse(new_title, sizeof(new_title));
    OPENSSL_cleanse(new_content, sizeof(new_content));

    return err;
}

/* -- Kayit Silme --------------------------------------------------------- */

VaultError vault_delete_record(Vault *vault)
{
    if (!vault) return VAULT_ERR_INPUT;

    if (vault->record_count == 0) {
        printf("\n  +-- Kayit Sil -------------------------------+\n");
        printf("  | Silinecek kayit bulunmuyor.                |\n");
        printf("  +--------------------------------------------+\n");
        return VAULT_OK;
    }

    /* Basliklari listele */
    printf("\n  +-- Silinebilecek Kayitlar ------------------+\n");
    for (uint32_t i = 0; i < vault->record_count; i++) {
        uint8_t *dec_title = NULL;
        uint32_t dec_title_len = 0;
        if (crypto_decrypt(vault->master_key, &vault->records[i].encrypted_title,
                           &dec_title, &dec_title_len) == VAULT_OK) {
            printf("  |  [%u] %.*s\n", i + 1, (int)dec_title_len, dec_title);
            OPENSSL_cleanse(dec_title, dec_title_len);
            free(dec_title);
        } else {
            printf("  |  [%u] (Cozulemedi)\n", i + 1);
        }
    }
    printf("  +--------------------------------------------+\n");

    char input[32];
    printf("  Silinecek kayit no [1-%u] (Iptal icin 0): ", vault->record_count);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return VAULT_ERR_INPUT;
    }

    int choice = atoi(input);
    if (choice <= 0 || (uint32_t)choice > vault->record_count) {
        printf("  [!] Islem iptal edildi.\n");
        return VAULT_OK;
    }

    uint32_t idx = (uint32_t)(choice - 1);

    /* Silinecek kaydin basligini cozelim ve onay isteyelim */
    uint8_t *title_to_del = NULL;
    uint32_t title_len = 0;
    crypto_decrypt(vault->master_key, &vault->records[idx].encrypted_title, &title_to_del, &title_len);

    printf("\n  Silinecek: [%u] %.*s\n", choice,
           title_to_del ? (int)title_len : 11,
           title_to_del ? (char *)title_to_del : "(Cozulemedi)");
    printf("  Bu kaydi KALICI olarak silmek istediginize emin misiniz? [E/H]: ");

    char confirm[16];
    if (fgets(confirm, sizeof(confirm), stdin) == NULL || (confirm[0] != 'E' && confirm[0] != 'e')) {
        printf("  [!] Silme islemi iptal edildi.\n");
        if (title_to_del) { OPENSSL_cleanse(title_to_del, title_len); free(title_to_del); }
        return VAULT_OK;
    }

    if (title_to_del) {
        OPENSSL_cleanse(title_to_del, title_len);
        free(title_to_del);
    }

    /* Silinecek kaydin blob belleklerini serbest birak */
    encrypted_blob_free(&vault->records[idx].encrypted_title);
    encrypted_blob_free(&vault->records[idx].encrypted_content);

    /* Diger kayitlari sola kaydir */
    if (idx < vault->record_count - 1) {
        memmove(&vault->records[idx],
                &vault->records[idx + 1],
                (vault->record_count - idx - 1) * sizeof(VaultRecord));
    }

    vault->record_count--;

    /* Son elemani sifirla */
    memset(&vault->records[vault->record_count], 0, sizeof(VaultRecord));

    /* Dosyaya kaydet */
    VaultError err = storage_save_all(vault);
    if (err == VAULT_OK) {
        printf("  [OK] Kayit basariyla silindi ve dosya guncellendi.\n");
    } else {
        fprintf(stderr, "  [!] Kayit silindi ancak dosya kaydedilemedi!\n");
    }

    return err;
}

/* -- Vault Bellek Temizleme ---------------------------------------------- */

void vault_free(Vault *vault)
{
    if (!vault) return;

    OPENSSL_cleanse(vault->master_key, KEY_LEN);

    if (vault->records) {
        for (uint32_t i = 0; i < vault->record_count; i++) {
            encrypted_blob_free(&vault->records[i].encrypted_title);
            encrypted_blob_free(&vault->records[i].encrypted_content);
        }
        free(vault->records);
        vault->records = NULL;
    }

    vault->record_count = 0;
    vault->capacity = 0;
}
