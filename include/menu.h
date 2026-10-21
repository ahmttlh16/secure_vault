/*
 * menu.h - CLI menu fonksiyon prototipleri
 * Kasa Secim Menusu ve Kasa Ici Kayit Yonetim Menusu
 */

#ifndef MENU_H
#define MENU_H

#include "common.h"

/*
 * menu_vault_selection - Ana kasa secim ve yeni kasa olusturma dongusu.
 * Kullanici 'C' secene kadar dongude kalir.
 */
void menu_vault_selection(void);

/*
 * menu_vault_records - Acik olan bir kasanin ic kayit yonetim dongusu.
 * Secenekler: Ekle, Goruntule, Duzenle, Sil, Kasayi Kapat.
 *
 * @param vault  Acilmis ve master_key'i yuklenmis vault yapisi
 */
void menu_vault_records(Vault *vault);

#endif /* MENU_H */
