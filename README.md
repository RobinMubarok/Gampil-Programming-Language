# Gampil Programming Language

  Bahasa pemrograman Gampil adalah bahasa yang berfokus sebagai media penyajian algoritma, bersifat komplementer dengan FlowChart. Tidak seperti bahasa lain yang pragmatis dan berfokus pada implementasi framework atau library, bahasa ini diharapkan mampu menyajikan alur komputasi dengan efektif dan sintaks yang familiar dengan bahasa manusia. Desain dari bahasa ini sendiri terinspirasi kuat dari Python dan Assembly. Rencananya, bahasa ini akan mendukung penggunaan library Python dan penerjemahan instruksi Assembly. Tujuannya, adalah memperluas ranah penyajian dari high-level sampai low-level.

### Operator

+, -, *, /, %, ^           : Operator Matematika
<, >, =, !, <=, >=, !=     : Operator Relasi
&, |, ||, ~, >>, <<        : Operator Bitwise
and, or, not               : Operator Boolean

### Hello World

```
  printf ["Hello World"]
```

### Variabel

Variabel dideklarasikan dengan menggunakan kata kunci 'be'.

```
  <tipe data> <nama variabel> be <nilai variabel>
```

Contoh:

```
  int x be 5
```

### Komentar

Komentar sebaris.

```
  \ ini adalah komentar sebaris
```

Komentar multi-baris.

```
`
  ini adalah komentar multi-baris
`
```

### Fungsi dsn Blok Kode

Tiap blok kode baik pada fungsi, percabangan, atau perulangan diawali titik dua (':') dan diakhiri dengan kata kunci 'ok'.

```
  <tipe keluaran> <nama fungsi> [<parameter formal>] :
      <baris kode>
    ok
```

Contoh:

```
  int add[int a, int b]:
    return a + b
  ok
```

### Percabangan

Percabangan menggunakan kata kunci 'if', 'but', dan 'else'. Tiap kondisi disebut dengan Guard. Apabila lebih dari satu Guard benar, maka kedua kondisi berjalan bersamaan.

```
  if <kondisi 1> :
      <instruksi>
    but
  <kondisi 2>:
      <instruksi>
    but
  else and <kondisi 2>:
      <instruksi>          \ tanpa 'else', bila lebih dari 1 kondisi benar maka dua-duanya berjalan bersamaan.
    but
  <kondisi n>:
      <instruksi>
    but
  ok
```

Contoh:

```
  int x be 5

  if x >= 5:
      printf ["Lebih dari 5"] \ x lebih dari sama dengan 5, output "Lebih dari 5".
    but
  x = 5:
      printf ["Sama dengan 5"] \ namun x juga sama dengan 5, oleh karena itu secara bersamaan juga outputnya "Sama dengan 5".
    but
  else and x <= 5:
      printf ["Kurang dari 5"] \ meskipun disini x juga kurang dari sama dengan 5, namun adanya 'else' membuatnya tidak dijalankan.
    but
  ok
```

### Array


