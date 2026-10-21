/*
 * storage.c - Binary vault dosyalari I/O implementasyonu
 * Coklu Kasa (Multi-Vault) ve platform-bagimsiz atomik dosya islemleri
 */

#include "storage.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifdef _WIN32
  #include <windows.h>
#endif

/* -- Yardimci: Big-endian yazma/okuma ------------------------------------ */

static void write_uint32_be(uint8_t buf[4], uint32_t val)
{
    buf[0] = (uint8_t)((val >> 24) & 0xFF);
    buf[1] = (uint8_t)((val >> 16) & 0xFF);
    buf[2] = (uint8_t)((val >>  8) & 0xFF);
    buf[3] = (uint8_t)((val      ) & 0xFF);
}

static uint32_t read_uint32_be(const uint8_t buf[4])
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] <<  8) |
           ((uint32_t)buf[3]);
}

static void write_uint16_be(uint8_t buf[2], uint16_t val)
{
    buf[0] = (uint8_t)((val >> 8) & 0xFF);
    buf[1] = (uint8_t)((val     ) & 0xFF);
}

static uint16_t read_uint16_be(const uint8_t buf[2])
{
    return ((uint16_t)buf[0] << 8) |
           ((uint16_t)buf[1]);
}

/* -- Dosya Varlik Kontrolu ----------------------------------------------- */

bool storage_file_exists(const char *filename)
{
    if (!filename) return false;
    FILE *f = fopen(filename, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* -- Header Boyutu ------------------------------------------------------- */

static size_t get_header_size(void)
{
    return VAULT_MAGIC_LEN + 1 + MAX_VAULT_NAME_LEN +
           SALT_LEN + IV_LEN + TAG_LEN + VERIFY_BLOCK_LEN;
}

/* -- Header Yazma -------------------------------------------------------- */

VaultError storage_write_header(const char *filename, const VaultHeader *header)
{
    if (!filename || !header) return VAULT_ERR_IO;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    if (fwrite(header->magic, 1, VAULT_MAGIC_LEN, f) != VAULT_MAGIC_LEN) goto fail;
    if (fwrite(&header->version, 1, 1, f) != 1) goto fail;
    if (fwrite(header->vault_name, 1, MAX_VAULT_NAME_LEN, f) != MAX_VAULT_NAME_LEN) goto fail;
    if (fwrite(header->salt, 1, SALT_LEN, f) != SALT_LEN) goto fail;
    if (fwrite(header->verify_iv, 1, IV_LEN, f) != IV_LEN) goto fail;
    if (fwrite(header->verify_tag, 1, TAG_LEN, f) != TAG_LEN) goto fail;
    if (fwrite(header->verify_ciphertext, 1, VERIFY_BLOCK_LEN, f) != VERIFY_BLOCK_LEN) goto fail;

    /* Kayit sayisi baslangicta 0 */
    uint8_t count_buf[4];
    write_uint32_be(count_buf, 0);
    if (fwrite(count_buf, 1, 4, f) != 4) goto fail;

    fclose(f);
    return VAULT_OK;

fail:
    fclose(f);
    return VAULT_ERR_IO;
}

/* -- Header Okuma -------------------------------------------------------- */

VaultError storage_read_header(const char *filename, VaultHeader *header)
{
    if (!filename || !header) return VAULT_ERR_IO;

    FILE *f = fopen(filename, "rb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    if (fread(header->magic, 1, VAULT_MAGIC_LEN, f) != VAULT_MAGIC_LEN) goto fail;
    if (fread(&header->version, 1, 1, f) != 1) goto fail;
    if (fread(header->vault_name, 1, MAX_VAULT_NAME_LEN, f) != MAX_VAULT_NAME_LEN) goto fail;
    if (fread(header->salt, 1, SALT_LEN, f) != SALT_LEN) goto fail;
    if (fread(header->verify_iv, 1, IV_LEN, f) != IV_LEN) goto fail;
    if (fread(header->verify_tag, 1, TAG_LEN, f) != TAG_LEN) goto fail;
    if (fread(header->verify_ciphertext, 1, VERIFY_BLOCK_LEN, f) != VERIFY_BLOCK_LEN) goto fail;

    fclose(f);

    /* Magic bytes kontrolu */
    if (memcmp(header->magic, VAULT_MAGIC, VAULT_MAGIC_LEN) != 0) {
        return VAULT_ERR_IO;
    }

    return VAULT_OK;

fail:
    fclose(f);
    return VAULT_ERR_IO;
}

/* -- EncryptedBlob Yazma/Okuma ------------------------------------------- */

static VaultError write_blob(FILE *f, const EncryptedBlob *blob)
{
    uint8_t len_buf[2];
    write_uint16_be(len_buf, (uint16_t)blob->ciphertext_len);

    if (fwrite(len_buf, 1, 2, f) != 2) return VAULT_ERR_IO;
    if (fwrite(blob->iv, 1, IV_LEN, f) != IV_LEN) return VAULT_ERR_IO;
    if (fwrite(blob->tag, 1, TAG_LEN, f) != TAG_LEN) return VAULT_ERR_IO;
    if (fwrite(blob->ciphertext, 1, blob->ciphertext_len, f) != blob->ciphertext_len) {
        return VAULT_ERR_IO;
    }

    return VAULT_OK;
}

static VaultError read_blob(FILE *f, EncryptedBlob *blob)
{
    uint8_t len_buf[2];

    if (fread(len_buf, 1, 2, f) != 2) return VAULT_ERR_IO;
    blob->ciphertext_len = read_uint16_be(len_buf);

    if (fread(blob->iv, 1, IV_LEN, f) != IV_LEN) return VAULT_ERR_IO;
    if (fread(blob->tag, 1, TAG_LEN, f) != TAG_LEN) return VAULT_ERR_IO;

    blob->ciphertext = (uint8_t *)malloc(blob->ciphertext_len);
    if (!blob->ciphertext) return VAULT_ERR_MEMORY;

    if (fread(blob->ciphertext, 1, blob->ciphertext_len, f) != blob->ciphertext_len) {
        free(blob->ciphertext);
        blob->ciphertext = NULL;
        return VAULT_ERR_IO;
    }

    return VAULT_OK;
}

/* -- Tum Kasayi Kaydetme (Atomik Yazma) ---------------------------------- */

VaultError storage_save_all(const Vault *vault)
{
    if (!vault || strlen(vault->filename) == 0) return VAULT_ERR_IO;

    char tmp_name[MAX_FILENAME_LEN + 8];
    snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", vault->filename);

    FILE *f = fopen(tmp_name, "wb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    /* Header yaz */
    const VaultHeader *h = &vault->header;
    if (fwrite(h->magic, 1, VAULT_MAGIC_LEN, f) != VAULT_MAGIC_LEN) goto fail;
    if (fwrite(&h->version, 1, 1, f) != 1) goto fail;
    if (fwrite(h->vault_name, 1, MAX_VAULT_NAME_LEN, f) != MAX_VAULT_NAME_LEN) goto fail;
    if (fwrite(h->salt, 1, SALT_LEN, f) != SALT_LEN) goto fail;
    if (fwrite(h->verify_iv, 1, IV_LEN, f) != IV_LEN) goto fail;
    if (fwrite(h->verify_tag, 1, TAG_LEN, f) != TAG_LEN) goto fail;
    if (fwrite(h->verify_ciphertext, 1, VERIFY_BLOCK_LEN, f) != VERIFY_BLOCK_LEN) goto fail;

    /* Kayit sayisi */
    uint8_t count_buf[4];
    write_uint32_be(count_buf, vault->record_count);
    if (fwrite(count_buf, 1, 4, f) != 4) goto fail;

    /* Kayitlari yaz */
    for (uint32_t i = 0; i < vault->record_count; i++) {
        VaultError err = write_blob(f, &vault->records[i].encrypted_title);
        if (err != VAULT_OK) goto fail;

        err = write_blob(f, &vault->records[i].encrypted_content);
        if (err != VAULT_OK) goto fail;
    }

    fclose(f);

    /* Atomik degistirme */
#ifdef _WIN32
    if (!MoveFileExA(tmp_name, vault->filename, MOVEFILE_REPLACE_EXISTING)) {
        remove(tmp_name);
        return VAULT_ERR_IO;
    }
#else
    if (rename(tmp_name, vault->filename) != 0) {
        remove(tmp_name);
        return VAULT_ERR_IO;
    }
#endif

    return VAULT_OK;

fail:
    fclose(f);
    remove(tmp_name);
    return VAULT_ERR_IO;
}

/* -- Tum Kayitlari Okuma ------------------------------------------------ */

VaultError storage_read_all_records(Vault *vault)
{
    if (!vault || strlen(vault->filename) == 0) return VAULT_ERR_IO;

    FILE *f = fopen(vault->filename, "rb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    /* Header'i atla */
    size_t header_size = get_header_size();
    if (fseek(f, (long)header_size, SEEK_SET) != 0) {
        fclose(f);
        return VAULT_ERR_IO;
    }

    /* Kayit sayisini oku */
    uint8_t count_buf[4];
    if (fread(count_buf, 1, 4, f) != 4) {
        fclose(f);
        return VAULT_ERR_IO;
    }

    uint32_t count = read_uint32_be(count_buf);
    vault->record_count = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (vault->record_count >= vault->capacity) {
            uint32_t new_cap = vault->capacity * 2;
            VaultRecord *new_records = (VaultRecord *)realloc(
                vault->records, new_cap * sizeof(VaultRecord));
            if (!new_records) {
                fclose(f);
                return VAULT_ERR_MEMORY;
            }
            vault->records = new_records;
            vault->capacity = new_cap;
        }

        VaultRecord *rec = &vault->records[vault->record_count];
        memset(rec, 0, sizeof(VaultRecord));

        VaultError err = read_blob(f, &rec->encrypted_title);
        if (err != VAULT_OK) {
            fclose(f);
            return err;
        }

        err = read_blob(f, &rec->encrypted_content);
        if (err != VAULT_OK) {
            encrypted_blob_free(&rec->encrypted_title);
            fclose(f);
            return err;
        }

        vault->record_count++;
    }

    fclose(f);
    return VAULT_OK;
}

/* -- Mevcut Kasalari Listeleme ------------------------------------------ */

uint32_t storage_list_vaults(VaultInfo out_vaults[], uint32_t max_count)
{
    if (!out_vaults || max_count == 0) return 0;

    DIR *d = opendir(".");
    if (!d) return 0;

    struct dirent *dir;
    uint32_t count = 0;

    while ((dir = readdir(d)) != NULL && count < max_count) {
        size_t len = strlen(dir->d_name);

        /* "vault_*.dat" formatina uyuyor mu? (tmp haric) */
        if (len >= 14 &&
            strncmp(dir->d_name, "vault_", 6) == 0 &&
            strcmp(dir->d_name + len - 4, ".dat") == 0)
        {
            VaultHeader h;
            if (storage_read_header(dir->d_name, &h) == VAULT_OK) {
                strncpy(out_vaults[count].filename, dir->d_name, MAX_FILENAME_LEN - 1);
                out_vaults[count].filename[MAX_FILENAME_LEN - 1] = '\0';

                strncpy(out_vaults[count].vault_name, h.vault_name, MAX_VAULT_NAME_LEN - 1);
                out_vaults[count].vault_name[MAX_VAULT_NAME_LEN - 1] = '\0';

                out_vaults[count].record_count = 0;

                /* Kayit sayisini oku */
                FILE *f = fopen(dir->d_name, "rb");
                if (f) {
                    size_t header_size = get_header_size();
                    if (fseek(f, (long)header_size, SEEK_SET) == 0) {
                        uint8_t count_buf[4];
                        if (fread(count_buf, 1, 4, f) == 4) {
                            out_vaults[count].record_count = read_uint32_be(count_buf);
                        }
                    }
                    fclose(f);
                }

                count++;
            }
        }
    }

    closedir(d);
    return count;
}
