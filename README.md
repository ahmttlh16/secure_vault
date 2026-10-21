<div align="center">

# 🔐 Vault Manager
### 2 Kademeli Güvenlik Mimarisine Sahip CLI Parola & Gizli Not Yöneticisi

![C11](https://img.shields.io/badge/Language-C11-00599C?style=for-the-badge&logo=c&logoColor=white)
![OpenSSL](https://img.shields.io/badge/Crypto-OpenSSL%203.x-721412?style=for-the-badge&logo=openssl&logoColor=white)
![Security](https://img.shields.io/badge/Cipher-AES--256--GCM-success?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)

<p align="center">
  <b>Askeri düzeyde şifreleme (AES-256-GCM + PBKDF2) ile korunan, bağımsız çoklu kasa (multi-vault) mimarisine sahip, yerel ve modern C ile geliştirilmiş güvenli komut satırı kasası.</b>
</p>

[Özellikler](#-öne-çıkan-özellikler) •
[Güvenlik Mimarisi](#-güvenlik-ve-kriptografi-mimarisi) •
[Kurulum & Derleme](#-kurulum-ve-derleme) •
[Kullanım](#-kullanım-rehberi) •
[Dosya Yapısı](#-proje-dizin-yapısı) •
[GitHub Notları](#-githuba-yüklemeden-önce-önemli)

</div>

---

## 📖 Genel Bakış

**Vault Manager**, hassas kimlik bilgilerinizi, API anahtarlarınızı ve özel notlarınızı harici bir bulut sağlayıcısına bağımlı olmadan, tamamen yerel diskinizde ve **Zero-Knowledge** (Sıfır Bilgi) ilkesiyle güvenle saklamanız için tasarlanmış profesyonel bir C projesidir.

Proje, geleneksel tek şifreli yöneticilerin ötesine geçerek **2 Aşamalı Güvenlik Katmanı** sunar:
1. **Uygulama Giriş Kalkanı:** Yanlış ana parola girildiğinde sistem bellek ve dosyalara hiçbir erişim izni vermeden süreci anında sonlandırır.
2. **İzole Çoklu Kasalar:** İş, Kişisel veya Finans gibi farklı kasalar birbirinden bağımsız anahtarlarla şifrelenir; bir kasanın anahtarı diğerini açamaz.

---

## ✨ Öne Çıkan Özellikler

- 🛡️ **2 Kademeli Doğrulama (Two-Tier Authentication)**:
  - **Kademe 1 (Master App Auth):** Uygulama düzeyinde genel kilit (`app_auth.dat`).
  - **Kademe 2 (Vault Auth):** Her kasa için ayrı parola ile korunan bağımsız veri konteynerleri (`vault_<hash8>.dat`).
- 🔒 **Kimlik Doğrulamalı Şifreleme (AES-256-GCM)**:
  - Yalnızca gizlilik değil, aynı zamanda verinin bütünlüğü (integrity) de 16-byte kimlik doğrulama etiketi (`Auth Tag`) ile garanti altına alınır.
  - Dosya üzerinde bayt düzeyinde yapılacak herhangi bir kurcalama (tampering) anında tespit edilir.
- 🔑 **Güçlü Anahtar Türetme (PBKDF2-HMAC-SHA256)**:
  - Kaba kuvvet (brute-force) ve Rainbow Table saldırılarına karşı her kasa için kriptografik olarak rastgele üretilen 16-byte tuz (`salt`) ve **100.000 iterasyon**.
- 🧠 **Zero-Knowledge & RAM Temizliği**:
  - Parolalar ve çözülmüş hassas metinler bellekte asla gereksiz yere tutulmaz.
  - İşlem bittiğinde veya kasa kapatıldığında RAM blokları `OPENSSL_cleanse` ile güvenli şekilde sıfırlanır (Wipe/Zeroization).
- 💾 **Atomik Dosya Güvenliği (Atomic File Write)**:
  - Veri kaybını ve elektrik/süreç kesintilerinde dosya bozulmasını önlemek için veriler önce geçici bir `.tmp` dosyasına yazılır, ardından işletim sistemi düzeyinde atomik olarak asıl dosyayla değiştirilir (`MoveFileExA` / `rename`).
- 📂 **Dinamik Kasa Keşfi (Dynamic Discovery)**:
  - Diskteki mevcut kasalar otomatik taranır, adları ve kayıt sayıları güvenli başlıktan dinamik olarak okunup menüde listelenir.
- 📝 **Eksiksiz CRUD Yetenekleri**:
  - Yeni kayıt ekleme, yalnızca RAM'de çözerek güvenli listeleme, dinamik güncelleme ve kalıcı silme.

---

## 🏛️ Güvenlik ve Kriptografi Mimarisi

### 1. Kimlik Doğrulama ve Kasa Akış Şeması

```text
                  +--------------------------------+
                  |   vault_manager Başlatıldı     |
                  +--------------------------------+
                                  |
                      [ app_auth.dat Mevcut mu? ]
                               /      \
                             Hayır    Evet
                             /          \
                +-----------------+   +-------------------------+
                | İlk Kurulum:    |   | Ana Uygulama Parolası   |
                | Master Parola   |   | Doğrulaması (PBKDF2)    |
                | Belirle         |   +-------------------------+
                +-----------------+          /           \
                         \             Geçersiz        Başarılı
                          \               /                \
                    +--------------------+           +----------------------+
                    | EXIT (Anında Kapat)|           |  Kasa Seçim Menüsü   |
                    +--------------------+           +----------------------+
                                                                |
                                             +------------------+------------------+
                                             |                                     |
                                    [ Yeni Kasa Oluştur ]                 [ Mevcut Kasa Seç ]
                                             |                                     |
                                  Kasa Adı & Parolası Belirle             Kasa Parolası Doğrula
                                             \                                     /
                                              +-----------------+-----------------+
                                                                |
                                                    +-----------------------+
                                                    |  Kasa Yönetim Menüsü  |
                                                    |  - [1] Yeni Kayıt     |
                                                    |  - [2] Görüntüle      |
                                                    |  - [3] Düzenle        |
                                                    |  - [4] Sil            |
                                                    |  - [5] Kasayı Kapat   |
                                                    +-----------------------+
```

### 2. Kriptografik Parametreler

| Parametre | Değer | Açıklama |
| :--- | :--- | :--- |
| **Simetrik Şifreleme** | `AES-256-GCM` | Galois/Counter Mode (AEAD) ile şifreleme ve bütünlük doğrulaması |
| **Anahtar Türetme (KDF)** | `PBKDF2-HMAC-SHA256` | Kaba kuvvet saldırılarına karşı yavaşlatılmış anahtar türetimi |
| **KDF İterasyon Sayısı** | `100,000` | Güvenlik / performans dengeli endüstri standardı |
| **KDF Tuz (Salt)** | `16 Byte (128-bit)` | `RAND_bytes` ile kriptografik olarak rastgele üretilir |
| **Nonce / IV** | `12 Byte (96-bit)` | Her şifreleme işlemi için tekil ve rastgele üretilir |
| **Kimlik Doğrulama Etiketi**| `16 Byte (128-bit)` | Verinin değiştirilmediğini doğrulayan GCM Authentication Tag |
| **Dosya Adı Türetimi** | `SHA-256 (İlk 8 Hex)`| Kasa parolası sızdırılmadan dosya adına `vault_<hash8>.dat` verilir |

### 3. İkili (Binary) Dosya Formatları

#### `app_auth.dat` (Uygulama Giriş Başlığı)
```text
[ 4B: Magic "APAU" ] [ 16B: PBKDF2 Salt ] [ 32B: Parola Hash Özeti ]
```

#### `vault_<hash8>.dat` (Kasa Dosyası)
```text
┌────────────────────────────────────────────────────────────────────────┐
│ VaultHeader (149 Byte)                                                 │
│ ├─ Magic (4B): "SVLT"                                                  │
│ ├─ Versiyon (1B): 0x02                                                 │
│ ├─ Kasa Adı (64B): "Kisisel", "Finans" vb.                             │
│ ├─ Salt (16B): KDF tuzu                                                │
│ ├─ Verify IV (12B): Parola doğrulama IV                                │
│ ├─ Verify Tag (16B): Parola doğrulama GCM Tag                          │
│ └─ Verify Ciphertext (32B): "VAULT_VERIFY_PAYLOAD_VALID_2024"          │
├────────────────────────────────────────────────────────────────────────┤
│ Record Count (4 Byte, uint32_t)                                        │
├────────────────────────────────────────────────────────────────────────┤
│ Kayıt 1: [Encrypted Title Blob] + [Encrypted Content Blob]             │
│ Kayıt 2: [Encrypted Title Blob] + [Encrypted Content Blob]             │
│ ...                                                                    │
└────────────────────────────────────────────────────────────────────────┘
```

> [!NOTE]
> Her `Encrypted Blob` yapısı; **12B IV**, **16B Tag**, **4B Veri Uzunluğu** ve ardından **Şifreli Veri** içerecek şekilde paketlenir.

---

## 🛠️ Kurulum ve Derleme

### Ön Koşullar

- **C Derleyici**: `gcc` veya `clang` (C11 desteği)
- **OpenSSL Kütüphanesi**: `OpenSSL 1.1.1` veya `OpenSSL 3.x` geliştirici paketleri (`libssl`, `libcrypto`)
- **Make Aracı**: `make` (Linux) veya `mingw32-make` (Windows)

---

### 🪟 Windows (MinGW-w64)

1. **OpenSSL'i Hazırlayın**:
   - [Win64 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) (Full versiyon) indirip varsayılan dizine (`C:\Program Files\OpenSSL-Win64`) kurun.
   - Alternatif bir dizine kurduysanız `Makefile` içerisindeki `OPENSSL_DIR` yolunu güncelleyebilirsiniz.

2. **Projeyi Derleyin**:
   ```cmd
   make
   ```
   *(veya `make.bat` dosyasını çalıştırabilirsiniz)*

3. **Çalıştırın**:
   ```cmd
   .\vault_manager.exe
   ```

---

### 🐧 Linux (Ubuntu / Debian)

1. **Gerekli Paketleri Yükleyin**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential libssl-dev
   ```

2. **Derleyin**:
   ```bash
   make
   ```

3. **Çalıştırın**:
   ```bash
   ./vault_manager
   ```

---

### 🧹 Temizlik ve Debug Modu

```bash
# Obje dosyalarını ve çalıştırılabilir dosyayı temizlemek için:
make clean

# AddressSanitizer ve hata ayıklama sembolleri ile derlemek için:
make DEBUG=1
```

---

## 🚀 Kullanım Rehberi

### Adım 1: İlk Kurulum (Ana Kilit Belirleme)
Uygulamayı ilk kez açtığınızda bir **Master Parola** belirlemeniz istenir. Bu parola tüm kasalarınıza erişim sağlayan ilk güvenlik duvarıdır.

```text
========================================
    ANA UYGULAMA KURULUMU (1. ASAMA)
========================================
Ilk calistirma tespit edildi.
Lutfen uygulamayi korumak icin bir ANA SIFRE belirleyin.
Ana sifre: ********
Ana sifreyi tekrar girin: ********
[+] Ana uygulama sifresi basariyla kaydedildi.
```

### Adım 2: Kasa Seçim Menüsü
Ana parola doğrulandıktan sonra mevcut kasalar listelenir ya da yeni kasa oluşturulabilir.

```text
============================================================
              KASA SECIM MENUSU (2. Asama)
============================================================
[1] Yeni Kasa Olustur
[2] Mevcut Kasayi Ac
    --> Kayitli Kasalar:
        [1] Kisisel Notlar (vault_a9f1b2c4.dat) - 3 kayit
        [2] Finans ve Bankacilik (vault_83d2e1f0.dat) - 5 kayit
[0] Cikis
============================================================
```

### Adım 3: Kasa İçi Kayıt Yönetimi (CRUD)
Seçtiğiniz kasanın şifresini girdiğinizde, kasanın içeriği çözülür ve işlem menüsü açılır:

```text
============================================================
           KASA: Kisisel Notlar (vault_a9f1b2c4.dat)
============================================================
[1] Yeni Not / Parola Ekle
[2] Kayitlari Goruntule
[3] Kayit Duzenle
[4] Kayit Sil
[5] Kasayi Kapat (Kasa Menusune Don)
[0] Programdan Cikis
============================================================
```

---

## 📁 Proje Dizin Yapısı

```text
.
├── Makefile                  # Çapraz platform derleme yapılandırması
├── make.bat                  # Windows kullanıcıları için pratik derleme scripti
├── README.md                 # Kapsamlı proje dokümantasyonu
├── .gitignore                # Git versiyon kontrolü kural dosyası
├── include/                  # C Başlık Dosyaları (Headers)
│   ├── common.h              # Ortak veri modelleri, sabitler ve hata kodları
│   ├── crypto.h              # OpenSSL EVP AES-GCM, PBKDF2 & SHA-256 tanımları
│   ├── auth.h                # 1. & 2. kademe kimlik doğrulama motoru
│   ├── vault.h               # Kayıt yönetimi ve bellek manipülasyonu
│   ├── storage.h             # İkili dosya I/O ve kasa tespiti
│   └── menu.h                # Güvenli CLI kullanıcı arayüzü fonksiyonları
└── src/                      # C Kaynak Kodları (Implementations)
    ├── main.c                # Uygulama giriş noktası ve güvenlik akış kontrolü
    ├── crypto.c              # Kriptografik işlemler ve OpenSSL entegrasyonu
    ├── auth.c                # Parola kontrol, salt yönetimi ve anında exit koruması
    ├── vault.c               # RAM üzerinde kayıt ekleme, listeleme, güncelleme, silme
    ├── storage.c             # Atomik dosya yazma ve diskten kasa tarama
    └── menu.c                # CLI menü ekranları ve güvenli girdi alma
```

---

## ⚠️ GitHub'a Yüklemeden Önce Önemli!

> [!WARNING]
> Bu projeyi GitHub'a veya halka açık bir ortama yüklemeden önce kişisel verilerinizi barındıran dosyaları kesinlikle commitlemeyin!

Aşağıdaki dosyalar yerel çalışma sırasında üretilir ve `.gitignore` ile yoksayılmalıdır:
- `*.dat` (`app_auth.dat`, `vault_*.dat` — sizin gerçek şifreli verileriniz)
- `*.exe`, `*.o`, `obj/` (Derleme çıktıları)
- `*.dll` (Dağıtım dosyaları)

Proje dizinine eklenmiş olan `.gitignore` dosyasını kullanarak bu dosyaların yanlışlıkla depoya yüklenmesini engelleyebilirsiniz.

---

## 🔒 Güvenlik Notları ve En İyi Uygulamalar

- **Parola Kurtarma Yoktur**: Bu uygulama **Zero-Knowledge** mimarisiyle çalışır. Ana parolanızı veya kasa parolanızı unutursanız, kriptografik olarak verilerinizi kurtarmanın **hiçbir yolu yoktur**.
- **Yedekleme**: Kasanızı yedeklemek isterseniz, `app_auth.dat` ve ilgili `vault_<hash>.dat` dosyalarını güvenli bir harici belleğe kopyalamanız yeterlidir.
- **Bellek Dökümü Koruması**: Uygulama açıkken parola bellekte tutulmaz, sadece oturum boyunca kasanın türetilmiş simetrik anahtarı RAM'de tutulur ve kasa kapatıldığı anda `OPENSSL_cleanse` ile tamamen sıfırlanır.

---

## 📜 Lisans

Bu proje [MIT Lisansı](LICENSE) kapsamında lisanslanmıştır. Detaylar için `LICENSE` dosyasına göz atabilirsiniz.
