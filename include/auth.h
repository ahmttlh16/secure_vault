/*
 * auth.h - 2 Kademeli Kimlik Dogrulama Prototipleri
 * 1. Asama: Ana Uygulama Giris Korumasi (app_auth.dat)
 * 2. Asama: Kasa Bazli Bagimsiz Sifre Dogrulama
 */

#ifndef AUTH_H
#define AUTH_H

#include "common.h"
#include <stdbool.h>

/* -- 1. Asama: Ana Uygulama Korumasi ------------------------------------- */

/*
 * auth_app_is_initialized - Ana uygulama sifresinin tanimli olup olmadigini kontrol eder.
 *
 * @return  true: app_auth.dat mevcut, false: henuz tanimlanmamis
 */
bool auth_app_is_initialized(void);

/*
 * auth_app_setup - Ilk calistirmada ana uygulama sifresi belirler.
 *
 * @return  VAULT_OK veya hata kodu
 */
VaultError auth_app_setup(void);

/*
 * auth_app_login - Ana uygulama sifresini sorar ve dogrular.
 * Yanlis girildiginde programi ANINDA sonlandirir (exit).
 *
 * @return  VAULT_OK
 */
VaultError auth_app_login(void);

/* -- 2. Asama: Kasa Bazli Kimlik Dogrulama ------------------------------- */

/*
 * auth_create_vault - Yeni bir kasa icin ozel sifre alir ve header'i hazirlar.
 *
 * @param vault       Olusturulacak vault yapisi
 * @param vault_name  Kullanicinin belirledigi kasa adi (orn: "Kisisel")
 * @return            VAULT_OK veya hata kodu
 */
VaultError auth_create_vault(Vault *vault, const char *vault_name);

/*
 * auth_verify_vault - Secilen kasanin ozel sifresini dogrular.
 *
 * @param vault  Vault yapisi (header ve filename onceden doldurulmus olmali)
 * @return       VAULT_OK (basarili) veya VAULT_ERR_AUTH (hatali sifre)
 */
VaultError auth_verify_vault(Vault *vault);

/* -- Dogrudan (GUI / Non-Interactive) Fonksiyonlar ------------------------ */

/*
 * auth_app_verify_password_direct - Verilen parolayi app_auth.dat ile dogrular.
 *
 * @param password  Dogrulanacak parola
 * @return          VAULT_OK veya VAULT_ERR_AUTH / VAULT_ERR_IO
 */
VaultError auth_app_verify_password_direct(const char *password);

/*
 * auth_app_setup_direct - Verilen parola ile app_auth.dat olusturur.
 *
 * @param password  Tanimlanacak ana parola
 * @return          VAULT_OK veya hata kodu
 */
VaultError auth_app_setup_direct(const char *password);

/*
 * auth_create_vault_direct - Verilen kasa adi ve parola ile yeni Vault header'i olusturur.
 *
 * @param vault       Olusturulacak vault yapisi
 * @param vault_name  Kasa adi
 * @param password    Kasa parolasi
 * @return            VAULT_OK veya hata kodu
 */
VaultError auth_create_vault_direct(Vault *vault, const char *vault_name, const char *password);

/*
 * auth_verify_vault_direct - Verilen parola ile vault kilit dogrulamasini yapar.
 *
 * @param vault     Vault yapisi
 * @param password  Denenen kasa parolasi
 * @return          VAULT_OK veya VAULT_ERR_AUTH / hata kodu
 */
VaultError auth_verify_vault_direct(Vault *vault, const char *password);

/* -- Yardimci: Terminalden Gizli Parola Okuma --------------------------- */
VaultError auth_read_password(const char *prompt, char *buf, size_t buf_len);

#endif /* AUTH_H */

