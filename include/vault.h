/*
 * vault.h - Kasa (Vault) CRUD islemleri prototipleri
 * Kayit Ekleme, Listeleme, Duzenleme ve Silme
 */

#ifndef VAULT_H
#define VAULT_H

#include "common.h"

/*
 * vault_init - Vault yapisini baslatir, bellek tahsis eder.
 *
 * @param vault  Baslatilacak vault yapisi
 * @return       VAULT_OK veya VAULT_ERR_MEMORY
 */
VaultError vault_init(Vault *vault);

/*
 * vault_add_record - Yeni bir not/parola kaydi ekler.
 *
 * @param vault  Vault yapisi
 * @return       VAULT_OK veya hata kodu
 */
VaultError vault_add_record(Vault *vault);

/*
 * vault_list_records - Tum kayitlari cozer ve ekrana basar.
 *
 * @param vault  Vault yapisi
 * @return       VAULT_OK veya hata kodu
 */
VaultError vault_list_records(const Vault *vault);

/*
 * vault_edit_record - Mevcut bir kaydin baslik veya icerigini duzenler.
 *
 * @param vault  Vault yapisi
 * @return       VAULT_OK veya hata kodu
 */
VaultError vault_edit_record(Vault *vault);

/*
 * vault_delete_record - Secilen kaydi bellekten ve dosyadan kalici olarak siler.
 *
 * @param vault  Vault yapisi
 * @return       VAULT_OK veya hata kodu
 */
VaultError vault_delete_record(Vault *vault);

/* -- Dogrudan (GUI / Non-Interactive) CRUD Fonksiyonlari ------------------ */

/*
 * vault_add_record_direct - Bellekteki kasaya yeni baslik ve icerik kaydeder ve dosyayi gunceller.
 *
 * @param vault    Vault yapisi
 * @param title    Kayit basligi
 * @param content  Kayit icerigi (not veya parola)
 * @return         VAULT_OK veya hata kodu
 */
VaultError vault_add_record_direct(Vault *vault, const char *title, const char *content);

/*
 * vault_get_record_decrypted - Belirtilen indisteki kaydin baslik ve icerigini cozer.
 *
 * @param vault        Vault yapisi
 * @param index        Kayit indisi (0-tabanli)
 * @param out_title    Cozulen basligin yazilacagi tampon
 * @param title_cap    Baslik tampon boyutu
 * @param out_content  Cozulen icerigin yazilacagi tampon
 * @param content_cap  Icerik tampon boyutu
 * @return             VAULT_OK veya hata kodu
 */
VaultError vault_get_record_decrypted(const Vault *vault, uint32_t index,
                                      char *out_title, size_t title_cap,
                                      char *out_content, size_t content_cap);

/*
 * vault_edit_record_direct - Belirtilen indisteki kaydi yeni baslik/icerik ile gunceller ve kaydeder.
 *
 * @param vault        Vault yapisi
 * @param index        Kayit indisi (0-tabanli)
 * @param new_title    Yeni baslik (NULL ise degistirilmez)
 * @param new_content  Yeni icerik (NULL ise degistirilmez)
 * @return             VAULT_OK veya hata kodu
 */
VaultError vault_edit_record_direct(Vault *vault, uint32_t index,
                                    const char *new_title, const char *new_content);

/*
 * vault_delete_record_direct - Belirtilen indisteki kaydi siler ve dosyayi atomik olarak gunceller.
 *
 * @param vault  Vault yapisi
 * @param index  Silinecek kayit indisi (0-tabanli)
 * @return       VAULT_OK veya hata kodu
 */
VaultError vault_delete_record_direct(Vault *vault, uint32_t index);

/*
 * vault_free - Vault yapisindaki tum kaynaklari serbest birakir.
 *
 * @param vault  Temizlenecek vault yapisi
 */
void vault_free(Vault *vault);

#endif /* VAULT_H */
