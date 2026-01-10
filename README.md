# Gampil Programming Language

  Bahasa pemrograman Gampil adalah bahasa yang berfokus sebagai media penyajian algoritma, bersifat komplementer dengan FlowChart. Gampil berasal dari bahasa Jawa, yang berarti 'sangat mudah'. Tidak seperti bahasa lain yang pragmatis dan berfokus pada implementasi framework atau library, bahasa ini diharapkan mampu menyajikan alur komputasi dengan efektif dan sintaks yang familiar dengan bahasa manusia. Desain dari bahasa ini sendiri terinspirasi kuat dari Python dan Assembly. Rencananya, bahasa ini juga akan mendukung penggunaan library Python dan penerjemahan instruksi Assembly. Tujuannya, adalah memperluas ranah penyajian dari high-level sampai low-level.

### Operator

- ```+```, ```-```, ```*```, ```/```, ```%```, ```^``` Operator Matematika;
- ```<```, ```>```, ```=```, ```!```, ```<=```, ```>=```, ```!=``` Operator Relasi;
- ```&```, ```|```, ```||```, ```~```, ```>>```, ```<<``` Operator Bitwise;
- ```and```, ```or```, ```not``` Operator Boolean.

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
  num16 x be 5
```

Tipe data bawaan di bahasa Gampil ada empat jenis:

- Bit, ada ```bitOff``` dan ```bitOn``` menggantikan ```void``` dan ```boolean```;
- ASCII (karakter), ada ```asc8```, ```asc16```, dan ```asc32``` menggantikan ```char```, ```unsigned short```, dan ```unsigned int```;
- Number (angka), ada ```num16```, ```num32```, dan ```num64``` menggantikan ```signed int```, ```long```, dan ```long long```;
- Rational (desimal), ada ```rat32```, ```rat64```, dan ```rat128``` menggantikan ```float```, ```double```, dan ```long double```.

Namun, Gampil juga mendukung tipe pada Python.

```
    Any varString be 20
```

Selain itu, juga mendukung variabel untuk mewakili register seperti pada Assembly.

```
    eax varReg be 20
```

### Baris baru

Sama seperti Python, tidak perlu titik dua (';') sebagai pengganti baris. Meski begitu, masih bisa digunakan apabila memang mau sebaris.

```
  num16 var be 5
  printf ["%d", var]
```

Ini juga valid,

```
    num16 var2 be 7; printf ["%d", var2]
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

### Fungsi dan Blok Kode

Tiap blok kode baik pada fungsi, percabangan, atau perulangan diawali titik dua (':') dan diakhiri dengan kata kunci 'ok'.

```
  <tipe keluaran> <nama fungsi> [<parameter formal>] :
      <baris kode>
    ok
```

Contoh:

```
  num16 add[int a, int b]:
    return a + b
  ok
```

Untuk memanggil, gunakan kurung siku ```[]```.

```
  printf ["%d", add[2, 3]]
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

Array menggunakan kurawal ```{}``` dan dideklarasikan dengan tanda bintang ```*```.

```
  num16* varArray be {1, 2, 3}
```

Untuk memperoleh nilai, dapat menggunakan gunakan kurung biasa ```()```.

```
  printf ["%d", varArray(0)]
```

### Loop

Gampil memperkenalkan 'redo' loop! Loop yang bisa meniru 'while', 'for', dan loop lain sekaligus. Untuk for-loop:

```
  redo <array> as <iterator>:
    <instruksi>
  ok
```

Untuk looping tak berujung:

```
  redo:
    <instruksi>
  ok
```

Contoh:

```
  num16* arrayNum be {1, 3, 5, 7, 9}
  redo arrayNum as i:
    printf ["%d", i]
  ok

  \ output 13579
```

### Ekstensi

Gampil menggunakan ekstensi ```.ga```. Bisa dilihat di file ```example.ga```.


