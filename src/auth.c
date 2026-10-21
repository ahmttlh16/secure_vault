/*
 * auth.c - 2 Kademeli Kimlik Dogrulama Implementasyonu
 * 1. Asama: Ana Uygulama Giris Korumasi (app_auth.dat)
 * 2. Asama: Kasa Bazli Bagimsiz Sifre Dogrulama
 */

#include "auth.h"
#include "crypto.h"
#include <openssl/crypto.h>   /* OPENSSL_cleanse, CRYPTO_memcmp */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -- Platform-bagimsiz terminal kontrolu --------------------------------- */
#ifdef _WIN32
  #include <windows.h>
#else
  #include <termios.h>
  #include <unistd.h>
#endif

/* Sabit dogrulama blogu - kasa parolasi kontrolu icin sifrelenir */
static const uint8_t VERIFY_PLAINTEXT[VERIFY_BLOCK_LEN] = {
    'V','A','U','L','T','_','V','E','R','I','F','I','C','A','T','I',
    'O','N','_','B','L','O','C','K','_','v','0','2','_','O','K','!'
};

/* -- Terminalden Parola Okuma (echo kapali) ----------------------------- */

VaultError auth_read_password(const char *prompt, char *buf, size_t buf_len)
{
    if (!prompt || !buf || buf_len < 2) {
        return VAULT_ERR_INPUT;
    }

    fprintf(stderr, "%s", prompt);

#ifdef _WIN32
    /* Windows: Console mode ile echo kapatma */
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD old_mode;
    GetConsoleMode(hStdin, &old_mode);
    SetConsoleMode(hStdin, old_mode & ~ENABLE_ECHO_INPUT);

    if (fgets(buf, (int)buf_len, stdin) == NULL) {
        SetConsoleMode(hStdin, old_mode);
        fprintf(stderr, "\n");
        return VAULT_ERR_INPUT;
    }

    SetConsoleMode(hStdin, old_mode);
#else
    /* Linux/macOS: termios ile echo kapatma */
    struct termios old_term, new_term;

    if (tcgetattr(STDIN_FILENO, &old_term) != 0) {
        return VAULT_ERR_INPUT;
    }

    new_term = old_term;
    new_term.c_lflag &= ~(ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) != 0) {
        return VAULT_ERR_INPUT;
    }

    if (fgets(buf, (int)buf_len, stdin) == NULL) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
        fprintf(stderr, "\n");
        return VAULT_ERR_INPUT;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
#endif

    fprintf(stderr, "\n");

    /* Satir sonu karakterlerini temizle */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[len - 1] = '\0';
        len--;
    }

    /* Bos parola kontrolu */
    if (len == 0) {
        return VAULT_ERR_INPUT;
    }

    return VAULT_OK;
}

/* ========================================================================= */
/* 1. ASAMA: ANA UYGULAMA KORUMASI (MASTER AUTH)                             */
/* ========================================================================= */

bool auth_app_is_initialized(void)
{
    FILE *f = fopen(APP_AUTH_FILENAME, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

VaultError auth_app_setup_direct(const char *password)
{
    if (!password || strlen(password) < 4) {
        return VAULT_ERR_INPUT;
    }

    AppAuth auth_data;
    memset(&auth_data, 0, sizeof(auth_data));
    memcpy(auth_data.magic, APP_AUTH_MAGIC, APP_AUTH_MAGIC_LEN);

    VaultError err = crypto_random_bytes(auth_data.salt, SALT_LEN);
    if (err != VAULT_OK) {
        return err;
    }

    err = crypto_derive_key(password, auth_data.salt, auth_data.hash);
    if (err != VAULT_OK) {
        return err;
    }

    FILE *f = fopen(APP_AUTH_FILENAME, "wb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    if (fwrite(&auth_data, sizeof(auth_data), 1, f) != 1) {
        fclose(f);
        return VAULT_ERR_IO;
    }
    fclose(f);

    return VAULT_OK;
}

VaultError auth_app_verify_password_direct(const char *password)
{
    if (!password) {
        return VAULT_ERR_INPUT;
    }

    FILE *f = fopen(APP_AUTH_FILENAME, "rb");
    if (!f) {
        return VAULT_ERR_IO;
    }

    AppAuth stored_auth;
    if (fread(&stored_auth, sizeof(stored_auth), 1, f) != 1) {
        fclose(f);
        return VAULT_ERR_IO;
    }
    fclose(f);

    if (memcmp(stored_auth.magic, APP_AUTH_MAGIC, APP_AUTH_MAGIC_LEN) != 0) {
        return VAULT_ERR_IO;
    }

    uint8_t computed_hash[KEY_LEN];
    VaultError err = crypto_derive_key(password, stored_auth.salt, computed_hash);
    if (err != VAULT_OK) {
        return err;
    }

    if (CRYPTO_memcmp(computed_hash, stored_auth.hash, KEY_LEN) != 0) {
        OPENSSL_cleanse(computed_hash, sizeof(computed_hash));
        return VAULT_ERR_AUTH;
    }

    OPENSSL_cleanse(computed_hash, sizeof(computed_hash));
    return VAULT_OK;
}

VaultError auth_app_setup(void)
{
    char pwd1[MAX_INPUT_LEN];
    char pwd2[MAX_INPUT_LEN];
    VaultError err = VAULT_OK;

    memset(pwd1, 0, sizeof(pwd1));
    memset(pwd2, 0, sizeof(pwd2));

    printf("\n+==============================================+\n");
    printf("|   1. ASAMA: ANA UYGULAMA GIRIS KURULUMU      |\n");
    printf("|   Lutfen ana uygulama sifrenizi belirleyin.  |\n");
    printf("+==============================================+\n\n");

    err = auth_read_password("  Ana Uygulama Sifresi       : ", pwd1, sizeof(pwd1));
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Sifre alinamadi.\n");
        goto cleanup;
    }

    err = auth_read_password("  Ana Uygulama Sifresi Tekrar: ", pwd2, sizeof(pwd2));
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Sifre alinamadi.\n");
        goto cleanup;
    }

    if (strcmp(pwd1, pwd2) != 0) {
        fprintf(stderr, "\n[X] Sifreler eslesmiyor! Kurulum tamamlanamadi.\n");
        err = VAULT_ERR_AUTH;
        goto cleanup;
    }

    if (strlen(pwd1) < 4) {
        fprintf(stderr, "\n[!] Ana sifre en az 4 karakter olmalidir.\n");
        err = VAULT_ERR_INPUT;
        goto cleanup;
    }

    err = auth_app_setup_direct(pwd1);
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Ana uygulama kurulumu basarisiz: %d\n", err);
        goto cleanup;
    }

    printf("\n  [OK] Ana uygulama sifresi basariyla olusturuldu.\n");

cleanup:
    OPENSSL_cleanse(pwd1, sizeof(pwd1));
    OPENSSL_cleanse(pwd2, sizeof(pwd2));
    return err;
}

VaultError auth_app_login(void)
{
    char entered_pwd[MAX_INPUT_LEN];
    memset(entered_pwd, 0, sizeof(entered_pwd));

    printf("\n+==============================================+\n");
    printf("|   1. ASAMA: ANA UYGULAMA GIRIS KORUMASI      |\n");
    printf("+==============================================+\n\n");

    VaultError err = auth_read_password("  Ana Uygulama Sifresi: ", entered_pwd, sizeof(entered_pwd));
    if (err != VAULT_OK) {
        fprintf(stderr, "\n  [X] Hatali giris! Uygulama derhal kapatiliyor...\n\n");
        exit(EXIT_FAILURE);
    }

    err = auth_app_verify_password_direct(entered_pwd);
    OPENSSL_cleanse(entered_pwd, sizeof(entered_pwd));

    if (err != VAULT_OK) {
        fprintf(stderr, "\n  [X] Hatali ana sifre! Uygulama derhal kapatiliyor...\n\n");
        exit(EXIT_FAILURE);
    }

    printf("\n  [OK] Ana sifre dogrulandi. Kasalar yukleniyor...\n");

    return VAULT_OK;
}

/* ========================================================================= */
/* 2. ASAMA: KASA BAZLI BAGIMSIZ SIFRELEME                                  */
/* ========================================================================= */

VaultError auth_create_vault_direct(Vault *vault, const char *vault_name, const char *password)
{
    if (!vault || !vault_name || !password) return VAULT_ERR_INPUT;
    if (strlen(password) < 4) return VAULT_ERR_INPUT;

    /* Kasa dosya adini turet: vault_<hash_ilk8karakter>.dat */
    VaultError err = crypto_vault_filename(password, vault->filename, sizeof(vault->filename));
    if (err != VAULT_OK) return err;

    /* Header hazirla */
    memset(&vault->header, 0, sizeof(VaultHeader));
    memcpy(vault->header.magic, VAULT_MAGIC, VAULT_MAGIC_LEN);
    vault->header.version = VAULT_VERSION;
    strncpy(vault->header.vault_name, vault_name, MAX_VAULT_NAME_LEN - 1);

    /* Kasa icin ozgun salt uret */
    err = crypto_random_bytes(vault->header.salt, SALT_LEN);
    if (err != VAULT_OK) return err;

    /* Kasa anahtarini turet */
    err = crypto_derive_key(password, vault->header.salt, vault->master_key);
    if (err != VAULT_OK) return err;

    /* Dogrulama blogunu sifrele */
    EncryptedBlob verify_blob;
    memset(&verify_blob, 0, sizeof(verify_blob));

    err = crypto_encrypt(vault->master_key,
                         VERIFY_PLAINTEXT,
                         VERIFY_BLOCK_LEN,
                         &verify_blob);
    if (err != VAULT_OK) return err;

    memcpy(vault->header.verify_iv, verify_blob.iv, IV_LEN);
    memcpy(vault->header.verify_tag, verify_blob.tag, TAG_LEN);
    memcpy(vault->header.verify_ciphertext, verify_blob.ciphertext, VERIFY_BLOCK_LEN);
    encrypted_blob_free(&verify_blob);

    return VAULT_OK;
}

VaultError auth_verify_vault_direct(Vault *vault, const char *password)
{
    if (!vault || !password) return VAULT_ERR_INPUT;

    /* Anahtar turet */
    VaultError err = crypto_derive_key(password, vault->header.salt, vault->master_key);
    if (err != VAULT_OK) return err;

    /* Dogrulama blogunu coz (AES-GCM tag kontrolu) */
    EncryptedBlob verify_blob;
    memset(&verify_blob, 0, sizeof(verify_blob));

    memcpy(verify_blob.iv, vault->header.verify_iv, IV_LEN);
    memcpy(verify_blob.tag, vault->header.verify_tag, TAG_LEN);

    verify_blob.ciphertext = (uint8_t *)malloc(VERIFY_BLOCK_LEN);
    if (!verify_blob.ciphertext) {
        OPENSSL_cleanse(vault->master_key, KEY_LEN);
        return VAULT_ERR_MEMORY;
    }
    memcpy(verify_blob.ciphertext, vault->header.verify_ciphertext, VERIFY_BLOCK_LEN);
    verify_blob.ciphertext_len = VERIFY_BLOCK_LEN;

    uint8_t *decrypted = NULL;
    uint32_t decrypted_len = 0;

    err = crypto_decrypt(vault->master_key, &verify_blob, &decrypted, &decrypted_len);
    encrypted_blob_free(&verify_blob);

    if (err != VAULT_OK) {
        OPENSSL_cleanse(vault->master_key, KEY_LEN);
        return err;
    }

    if (decrypted) {
        OPENSSL_cleanse(decrypted, decrypted_len);
        free(decrypted);
    }

    return VAULT_OK;
}

VaultError auth_create_vault(Vault *vault, const char *vault_name)
{
    if (!vault || !vault_name) return VAULT_ERR_INPUT;

    char pwd1[MAX_INPUT_LEN];
    char pwd2[MAX_INPUT_LEN];
    VaultError err = VAULT_OK;

    memset(pwd1, 0, sizeof(pwd1));
    memset(pwd2, 0, sizeof(pwd2));

    printf("\n+==============================================+\n");
    printf("|   YENI KASA OLUSTURULUYOR                    |\n");
    printf("|   Kasa Adi: %-32s |\n", vault_name);
    printf("|   Lutfen BU KASAYA OZEL sifre belirleyin.    |\n");
    printf("+==============================================+\n\n");

    err = auth_read_password("  Kasa Sifresi       : ", pwd1, sizeof(pwd1));
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Sifre alinamadi.\n");
        goto cleanup;
    }

    err = auth_read_password("  Kasa Sifresi Tekrar: ", pwd2, sizeof(pwd2));
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Sifre alinamadi.\n");
        goto cleanup;
    }

    if (strcmp(pwd1, pwd2) != 0) {
        fprintf(stderr, "\n[X] Sifreler eslesmiyor! Kasa olusturulamadi.\n");
        err = VAULT_ERR_AUTH;
        goto cleanup;
    }

    if (strlen(pwd1) < 4) {
        fprintf(stderr, "\n[!] Kasa sifresi en az 4 karakter olmalidir.\n");
        err = VAULT_ERR_INPUT;
        goto cleanup;
    }

    err = auth_create_vault_direct(vault, vault_name, pwd1);
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Kasa olusturulamadi: %d\n", err);
        goto cleanup;
    }

    printf("\n  [OK] Kasa sifresi basariyla belirlendi.\n");
    printf("  [OK] Kasa dosyasi: %s\n", vault->filename);

cleanup:
    OPENSSL_cleanse(pwd1, sizeof(pwd1));
    OPENSSL_cleanse(pwd2, sizeof(pwd2));
    return err;
}

VaultError auth_verify_vault(Vault *vault)
{
    if (!vault) return VAULT_ERR_INPUT;

    char pwd[MAX_INPUT_LEN];
    VaultError err = VAULT_OK;

    memset(pwd, 0, sizeof(pwd));

    printf("\n+==============================================+\n");
    printf("|   KASA KILIDI ACILIYOR                       |\n");
    printf("|   Kasa Adi: %-32s |\n", vault->header.vault_name);
    printf("+==============================================+\n\n");

    err = auth_read_password("  Kasa Sifresi: ", pwd, sizeof(pwd));
    if (err != VAULT_OK) {
        fprintf(stderr, "[!] Sifre okunamadi.\n");
        goto cleanup;
    }

    err = auth_verify_vault_direct(vault, pwd);
    if (err == VAULT_ERR_AUTH) {
        fprintf(stderr, "\n  [X] Hatali kasa sifresi! Kasa acilamadi.\n");
        goto cleanup;
    } else if (err != VAULT_OK) {
        fprintf(stderr, "[!] Kasa dogrulanirken hata olustu.\n");
        goto cleanup;
    }

    printf("\n  [OK] Kasa kilidi basariyla acildi: %s\n", vault->header.vault_name);

cleanup:
    OPENSSL_cleanse(pwd, sizeof(pwd));
    return err;
}

