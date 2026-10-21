/*
 * storage.h - Dosya I/O prototipleri
 * Coklu Kasa (Multi-Vault) binary dosya okuma/yazma islemleri
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"
#include <stdbool.h>

/*
 * storage_file_exists - Belirtilen dosyanin varligini kontrol eder.
 *
 * @param filename  Dosya adi
 * @return          true: dosya mevcut, false: dosya yok
 */
bool storage_file_exists(const char *filename);

/*
 * storage_write_header - Kasa basligini belirtilen dosyaya yazar.
 *
 * @param filename  Hedef dosya adi
 * @param header    Yazilacak baslik yapisi
 * @return          VAULT_OK veya VAULT_ERR_IO
 */
VaultError storage_write_header(const char *filename, const VaultHeader *header);

/*
 * storage_read_header - Kasa basligini belirtilen dosyadan okur.
 *
 * @param filename  Okunacak dosya adi
 * @param header    Okunan basligin yazilacagi yapi
 * @return          VAULT_OK veya VAULT_ERR_IO
 */
VaultError storage_read_header(const char *filename, VaultHeader *header);

/*
 * storage_save_all - Tum kasayi (header + kayitlar) vault->filename dosyasina yazar.
 * Atomik yazma: once gecici dosyaya yazar, sonra atomik degistirir.
 *
 * @param vault  Yazilacak vault yapisi
 * @return       VAULT_OK veya VAULT_ERR_IO
 */
VaultError storage_save_all(const Vault *vault);

/*
 * storage_read_all_records - Tum sifreli kayitlari vault->filename dosyasindan okur.
 *
 * @param vault  Vault yapisi
 * @return       VAULT_OK veya VAULT_ERR_IO / VAULT_ERR_MEMORY
 */
VaultError storage_read_all_records(Vault *vault);

/*
 * storage_list_vaults - Calisma dizinindeki tum vault_*.dat dosyalarini tarar.
 *
 * @param out_vaults  Bulunan kasa bilgilerinin yazilacagi dizi
 * @param max_count   Dizinin maksimum eleman kapasitesi
 * @return            Bulunan gecerli kasa sayisi
 */
uint32_t storage_list_vaults(VaultInfo out_vaults[], uint32_t max_count);

#endif /* STORAGE_H */
