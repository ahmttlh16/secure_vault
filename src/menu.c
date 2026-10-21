/*
 * menu.c - CLI menu dongusu implementasyonu
 * 1. Kademe sonrasi Kasa Secim Menusu ve Kasa Ici Kayit Menusu (Duzenle & Sil dahil)
 */

#include "menu.h"
#include "vault.h"
#include "auth.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -- Kasa Secim Menusu --------------------------------------------------- */

void menu_vault_selection(void)
{
    char input[64];
    int running = 1;

    while (running) {
        VaultInfo vaults[MAX_VAULTS_LIMIT];
        uint32_t vault_count = storage_list_vaults(vaults, MAX_VAULTS_LIMIT);

        printf("\n");
        printf("  +==============================================+\n");
        printf("  |              KASA SECIM MENUSU               |\n");
        printf("  +==============================================+\n");

        if (vault_count == 0) {
            printf("  |  (Mevcut kayitli kasa bulunmuyor)            |\n");
        } else {
            printf("  |  Mevcut Kasalar:                             |\n");
            for (uint32_t i = 0; i < vault_count; i++) {
                printf("  |   [%u] %-14s (%s) - %u kayit\n",
                       i + 1,
                       vaults[i].vault_name,
                       vaults[i].filename,
                       vaults[i].record_count);
            }
        }

        printf("  +----------------------------------------------+\n");
        printf("  |  Secenekler:                                 |\n");
        if (vault_count > 0) {
            printf("  |   [1-%u] Kasayi Ac                            |\n", vault_count);
        }
        printf("  |   [N]   Yeni Kasa Olustur                    |\n");
        printf("  |   [C]   Uygulamadan Cikis                    |\n");
        printf("  +==============================================+\n");
        printf("\n  Seciminiz: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        /* Satir sonu temizle */
        size_t len = strlen(input);
        while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r')) {
            input[len - 1] = '\0';
            len--;
        }

        if (len == 0) continue;

        if (input[0] == 'C' || input[0] == 'c') {
            printf("\n  Gule gule! Uygulama guvenli sekilde kapatiliyor...\n\n");
            running = 0;

        } else if (input[0] == 'N' || input[0] == 'n') {
            /* Yeni Kasa Olustur */
            char vault_name[MAX_VAULT_NAME_LEN];
            printf("\n  Yeni Kasa Adi (orn: Kisisel, Finans, Is): ");
            if (fgets(vault_name, sizeof(vault_name), stdin) == NULL) {
                continue;
            }

            size_t nlen = strlen(vault_name);
            while (nlen > 0 && (vault_name[nlen - 1] == '\n' || vault_name[nlen - 1] == '\r')) {
                vault_name[nlen - 1] = '\0';
                nlen--;
            }

            if (nlen == 0) {
                printf("  [!] Kasa adi bos olamaz. Islem iptal edildi.\n");
                continue;
            }

            Vault new_vault;
            if (vault_init(&new_vault) != VAULT_OK) {
                fprintf(stderr, "  [!] Bellek hatasi olustu.\n");
                continue;
            }

            VaultError err = auth_create_vault(&new_vault, vault_name);
            if (err == VAULT_OK) {
                /* Header'i diske yaz */
                err = storage_write_header(new_vault.filename, &new_vault.header);
                if (err == VAULT_OK) {
                    printf("  [OK] Kasa dosyasi olusturuldu: %s\n", new_vault.filename);
                    /* Kasa ici menuye gir */
                    menu_vault_records(&new_vault);
                } else {
                    fprintf(stderr, "  [!] Kasa dosyasi diske yazilamadi!\n");
                }
            }
            vault_free(&new_vault);

        } else {
            /* Mevcut bir kasa numarasi mi secildi? */
            int choice = atoi(input);
            if (choice >= 1 && (uint32_t)choice <= vault_count) {
                uint32_t idx = (uint32_t)(choice - 1);

                Vault open_vault;
                if (vault_init(&open_vault) != VAULT_OK) {
                    fprintf(stderr, "  [!] Bellek hatasi olustu.\n");
                    continue;
                }

                strncpy(open_vault.filename, vaults[idx].filename, MAX_FILENAME_LEN - 1);
                open_vault.filename[MAX_FILENAME_LEN - 1] = '\0';

                VaultError err = storage_read_header(open_vault.filename, &open_vault.header);
                if (err != VAULT_OK) {
                    fprintf(stderr, "  [!] Kasa basligi okunamadi.\n");
                    vault_free(&open_vault);
                    continue;
                }

                /* Kasa ozel sifresini dogrula */
                err = auth_verify_vault(&open_vault);
                if (err == VAULT_OK) {
                    /* Kayitlari oku */
                    err = storage_read_all_records(&open_vault);
                    if (err == VAULT_OK) {
                        menu_vault_records(&open_vault);
                    } else {
                        fprintf(stderr, "  [!] Kasa kayitlari okunamadi.\n");
                    }
                }
                vault_free(&open_vault);

            } else {
                printf("  [!] Gecersiz secim. Lutfen gecerli bir kasa numarasi, 'N' veya 'C' girin.\n");
            }
        }
    }
}

/* -- Kasa Ici Kayit Yonetim Menusu --------------------------------------- */

void menu_vault_records(Vault *vault)
{
    if (!vault) return;

    char input[16];
    int in_vault = 1;

    while (in_vault) {
        printf("\n");
        printf("  +==============================================+\n");
        printf("  | KASA : %-37s |\n", vault->header.vault_name);
        printf("  | DOSYA: %-37s |\n", vault->filename);
        printf("  +==============================================+\n");
        printf("  |                                              |\n");
        printf("  |   [1]  Yeni Not / Parola Ekle                |\n");
        printf("  |   [2]  Kayitlari Goruntule                   |\n");
        printf("  |   [3]  Kayit Duzenle                         |\n");
        printf("  |   [4]  Kayit Sil                             |\n");
        printf("  |   [5]  Kasayi Kapat (Ana Menuye Don)         |\n");
        printf("  |                                              |\n");
        printf("  +==============================================+\n");
        printf("\n  Seciminiz [1-5]: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        size_t len = strlen(input);
        while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r')) {
            input[len - 1] = '\0';
            len--;
        }

        if (len == 0) continue;

        if (strcmp(input, "1") == 0) {
            vault_add_record(vault);

        } else if (strcmp(input, "2") == 0) {
            vault_list_records(vault);

        } else if (strcmp(input, "3") == 0) {
            vault_edit_record(vault);

        } else if (strcmp(input, "4") == 0) {
            vault_delete_record(vault);

        } else if (strcmp(input, "5") == 0) {
            printf("\n  Kasa guvenli sekilde kapatildi. Kasa secim menusune donuluyor...\n");
            in_vault = 0;

        } else {
            printf("  [!] Gecersiz secim. Lutfen 1 ile 5 arasinda bir deger girin.\n");
        }
    }
}
