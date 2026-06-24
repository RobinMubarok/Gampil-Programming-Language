# Gampil Language Support for VS Code

Ekstensi ini menyediakan *Syntax Highlighting* (pewarnaan kode) dan konfigurasi bahasa dasar untuk bahasa pemrograman **Gampil** (`.ga`).

---

## 🚀 Cara Menjalankan & Mengetes Ekstensi

Jika kamu sedang mengembangkan atau ingin melihat hasilnya secara langsung:

1. **Buka Project di VS Code:**
   Buka folder `ide/vscode-gampil` ini langsung di VS Code.
   
2. **Jalankan Extension Development Host:**
   - Tekan tombol **`F5`** pada keyboard kamu (atau masuk ke menu *Run and Debug* lalu klik tombol *Start Debugging*).
   - VS Code akan membuka jendela baru bernama **[Extension Development Host]**.

3. **Coba File `.ga`:**
   - Di dalam jendela baru tersebut, buka folder project Gampil kamu (misalnya folder `tests/`).
   - Buka salah satu file test seperti `arithmetic.ga` atau `arrays.ga`.
   - Kode bahasa Gampil sekarang sudah memiliki pewarnaan syntax!

---

## 📦 Cara Install Permanen di VS Code Lokal

Agar ekstensi ini aktif secara otomatis setiap kali kamu membuka VS Code (tanpa harus menekan `F5`), ikuti langkah berikut:

### Metode 1: Copy-Paste Folder (Paling Mudah)
1. Salin/Copy seluruh folder `vscode-gampil` ini.
2. Buka folder ekstensi VS Code di komputermu:
   - **Windows:** Tekan `Win + R`, ketik `%USERPROFILE%\.vscode\extensions`, lalu tekan Enter.
   - **Mac/Linux:** Buka path `~/.vscode/extensions`.
3. Tempel/Paste folder `vscode-gampil` ke dalam folder `extensions` tersebut.
4. Restart/buka ulang VS Code kamu.

### Metode 2: Menggunakan File Installer `.vsix`
Jika kamu ingin membagikan ekstensi ini ke teman atau menginstallnya secara rapi:
1. Pastikan kamu memiliki [Node.js](https://nodejs.org/) terinstall.
2. Install tools compiler ekstensi bernama `vsce` secara global:
   ```bash
   npm install -g @vscode/vsce
   ```
3. Buka terminal di folder `ide/vscode-gampil` ini, lalu ketik perintah:
   ```bash
   vsce package
   ```
4. Perintah di atas akan menghasilkan sebuah file bernama `gampil-1.0.0.vsix` di folder ini.
5. Di VS Code, buka menu *Extensions* (`Ctrl+Shift+X`), klik ikon titik tiga `...` di pojok kanan atas panel ekstensi, pilih **Install from VSIX...**, lalu pilih file `.vsix` yang baru saja kamu buat.

---

## 🛠️ Cara Mengubah Kode Syntax Highlighting

Konfigurasi ekstensi ini berada di file-file berikut:

- **`syntaxes/gampil.tmLanguage.json`**: Tempat mendefinisikan aturan deteksi warna menggunakan regex (TextMate Grammar). Jika kamu menambah kata kunci baru (seperti tipe data baru, kontrol keyword, dll.), edit bagian `"keywords"` atau `"types"` di file ini.
- **`language-configuration.json`**: Mengatur penutup kurung otomatis (`{}`, `[]`, `()`), penutup kutip otomatis (`""`, `''`), dan karakter komentar (menggunakan backslash `\`).
- **`package.json`**: Menghubungkan ekstensi dengan jenis ekstensi file `.ga`.

---

Selamat mencoba dan mengembangkan bahasa pemrograman **Gampil**! 🚀
