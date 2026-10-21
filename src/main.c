/*
 * main.c - Uygulama giris noktasi
 * CLI Not ve Parola Yoneticisi - 2 Kademeli Guvenlik Mimarisi
 *
 * Akis:
 *   1. 1. Asama: Ana Uygulama Giris Korumasi
 *      - Ilk acilista ana uygulama sifresi belirle (app_auth.dat)
 *      - Mevcut ise dogrula (yanlis sifrede ANINDA sonlandir)
 *   2. 2. Asama: Bagimsiz Sifreli Coklu Kasa Yonetimi
 *      - Kasa Secim Menusu (vault_<hash8>.dat)
 *      - Kasa ici kayit yonetimi (Ekle, Goruntule, Duzenle, Sil)
 */

#include "common.h"
#include "auth.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* -- 1. ASAMA: ANA UYGULAMA GIRIS KORUMASI --------------------------- */
    if (!auth_app_is_initialized()) {
        VaultError err = auth_app_setup();
        if (err != VAULT_OK) {
            fprintf(stderr, "[!] Ana uygulama kurulumu tamamlanamadi.\n");
            return EXIT_FAILURE;
        }
    } else {
        /*
         * Giris dogrulamasi: yanlis sifrede auth_app_login icinde
         * program aninda exit(EXIT_FAILURE) ile sonlandirilir.
         */
        auth_app_login();
    }

    /* -- 2. ASAMA: KASA SECIM VE YONETIM MENUSU ------------------------- */
    menu_vault_selection();

    return EXIT_SUCCESS;
}
