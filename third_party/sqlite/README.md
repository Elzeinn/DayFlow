# SQLite (vendored)

Amalgamation resmi SQLite, disalin apa adanya. Tidak dimodifikasi.

| Field        | Nilai                                                              |
| ------------ | ------------------------------------------------------------------ |
| Versi        | 3.54.1                                                             |
| Arsip        | `sqlite-amalgamation-3540100.zip`                                   |
| Sumber       | https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip         |
| Ukuran arsip | 2.990.966 byte                                                     |
| SHA3-256     | `92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30`  |
| Lisensi      | Public domain (https://www.sqlite.org/copyright.html)              |

File yang dipakai: `sqlite3.c`, `sqlite3.h`, `sqlite3ext.h`. `shell.c` (CLI `sqlite3.exe`)
tidak disertakan.

Divalidasi dengan SHA3-256 dari halaman resmi
<https://www.sqlite.org/download.html> saat diunduh.

## Cara update

1. Ambil nama arsip + hash dari baris `PRODUCT,` di halaman download SQLite.
2. Unduh, verifikasi SHA3-256, ekstrak, salin ketiga file di atas.
3. Perbarui tabel di README ini dan jalankan ulang CMake (`cmake --preset dev`).
