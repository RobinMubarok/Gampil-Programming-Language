# Gampil Programming Language

Gampil is a programming language designed primarily as a medium for presenting algorithms. It serves as a complementary alternative to flowcharts while remaining fully compilable. The name *Gampil* originates from the Javanese language and means *"very easy"*.

Unlike many modern programming languages that focus heavily on practical ecosystems with extensive frameworks and libraries, Gampil aims to represent computational workflows clearly and effectively through syntax that closely resembles human language.

The language design is strongly inspired by both Python and Assembly. Future development plans include support for Python libraries as well as direct Assembly instructions, while maintaining a balanced integration between the two. The ultimate goal is to broaden the language's expressive range from high-level abstractions to low-level system programming.

## Operators

### Arithmetic Operators

* plus `+`, minus `-`, times `*`, divide `/`, modulo `%`, power `^`

### Relational Operators

* less than `<`, greater than `>`, equal `=`, not equal `!`, less than or equal `<=`, greater than or equal `>=`, not equal `!=`

### Bitwise Operators

* bitwise AND `&`, bitwise OR `|`, bitwise XOR `||`, bitwise NOT `~`, bitwise right shift `>>`, bitwise left shift `<<`

### Boolean Operators

* boolean AND `and`, boolean OR `or`, boolean NOT `not`

## Hello World

```gampil
printf ["Hello World"]
```

## Variables

Variables are declared using the `be` keyword.

```gampil
<data type> <variable name> be <value>
```

Example:

```gampil
num16 x be 5
```

Gampil provides four built-in data categories:

* **Bit**

  * `bitOff` and `bitOn`
  * Intended as replacements for `void` and `boolean`

* **ASCII (Character Types)**

  * `asc8`, `asc16`, `asc32`, and `asc64`
  * Equivalent to raw bytes depending on its size

* **Number (Integer Types)**

  * `num8`, `num16`, `num32`, and `num64`
  * Equivalent to raw bytes (signed) depending on its size

* **Rational (Floating-Point Types)**

  * `rat32`, `rat64`, and `rat128`
  * Equivalent to `float`, `double`, and `long double`

Gampil also supports Python-style dynamic typing through the `let` keyword:

```gampil
let varString be "20"
```

In addition, variables can represent processor registers, similar to Assembly language:

```gampil
eax varReg be 20
```

## New Lines

Like Python, Gampil does not require semicolons (`;`) to terminate statements. However, semicolons may still be used to place multiple statements on the same line.

```gampil
num16 var be 5
printf ["%d", var]
```

The following is also valid:

```gampil
num16 var2 be 7; printf ["%d", var2]
```

## Comments

### Single-Line Comments

```gampil
\ this is a single-line comment
```

### Multi-Line Comments

```gampil
`
this is a multi-line comment
`
```

## Functions and Code Blocks

Every code block—whether a function, conditional statement, or loop—begins with a colon (`:`) and ends with the `ok` keyword.

```gampil
<return type> <function name> [<formal parameters>] :
    <statements>
ok
```

Example:

```gampil
num16 add[int a, int b]:
    return a + b
ok
```

Functions are invoked using square brackets `[]`.

```gampil
printf ["%d", add[2, 3]]
```

## Conditional Statements

Conditional branching uses the keywords `if`, `but`, and `else`.

Each condition is referred to as a **Guard**. If multiple Guards evaluate to true, all corresponding branches are executed unless restricted by `else`.

```gampil
if <condition 1>:
    <statements>
but
<condition 2>:
    <statements>
but
else and <condition 3>:
    <statements>
but
<condition n>:
    <statements>
but
ok
```

Example:

```gampil
num16 x be 5

if x >= 5:
    printf ["Greater than or equal to 5"]
but
x = 5:
    printf ["Equal to 5"]
but
else and x <= 5:
    printf ["Less than or equal to 5"]
but
ok
```

In this example:

* `x >= 5` evaluates to true.
* `x = 5` also evaluates to true.
* Both branches are executed.
* Although `x <= 5` is also true, the presence of `else` prevents that branch from running.

## Arrays

Arrays are declared using curly braces `{}` and may have an explicitly defined size.

```gampil
num16(3) varArray be {1, 2, 3}
```

To access an element, use parentheses `()`.

```gampil
printf ["%d", varArray(0)]
```

## Loops

Gampil introduces the **redo loop**, a versatile looping construct capable of emulating `for`, `while`, and other looping patterns.

### For-Style Iteration

```gampil
redo <array> as <iterator>:
    <statements>
ok
```

### Infinite Loop

```gampil
redo:
    <statements>
ok
```

Example:

```gampil
num16(5) arrayNum be {1, 3, 5, 7, 9}

redo arrayNum as int i:
    printf ["%d", i]
ok
```

Output:

```text
13579
```

## File Extension

Gampil source files use the `.ga` extension.

An example can be found in:

```text
src/example.ga
```

# Gampil Programming Language

  Bahasa pemrograman Gampil adalah bahasa yang berfokus sebagai media penyajian algoritma, bersifat komplementer dengan FlowChart namun dapat dikompilasi. Gampil berasal dari bahasa Jawa, yang berarti 'sangat mudah'. Tidak seperti bahasa lain yang pragmatis dengan penekanan pada banyaknya implementasi framework dan library, bahasa ini diharapkan mampu menyajikan alur komputasi secara efektif dengan sintaks yang familiar dengan bahasa manusia. Desain dari bahasa ini sendiri terinspirasi kuat dari Python dan Assembly. Bahkan rencananya, bahasa ini juga akan dibuat agar mendukung penggunaan library Python dan penggunaan instruksi Assembly secara penuh namun seimbang. Tujuannya, adalah memperluas ranah penyajian dari high-level sampai low-level.

### Operator

- tambah ```+```, kurang ```-```, kali ```*```, bagi ```/```, modulo ```%```, pangkat ```^``` Operator Matematika;
- kurang dari ```<```, lebih dari ```>```, sama dengan ```=```, tidak sama dengan ```!```, kurang dari atau sama dengan ```<=```, lebih dari atau sama dengan ```>=```, tidak sama dengan ```!=``` Operator Relasi;
- bitwise and ```&```, bitwise or ```|```, bitwise xor ```||```, bitwise not ```~```, bitwise right shift ```>>```, bitwise left shift ```<<``` Operator Bitwise;
- boolean and ```and```, boolean or ```or```, boolean not ```not``` Operator Boolean.

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
- ASCII (karakter), ada ```asc8```, ```asc16```, ```asc32```, dan ```asc64``` yang mengalokasikan byte tidak bertanda sesuai jumlah yang ditentukan;
- Number (angka), ada ```num8```, ```num16```, ```num32```, dan ```num64``` yang mengalokasikan byte bertanda sesuai jumlah yang ditentukan;
- Rational (desimal), ada ```rat32```, ```rat64```, dan ```rat128``` menggantikan ```float```, ```double```, dan ```long double```.

Namun, Gampil juga mendukung tipe pada Python, gunakan kata kunci 'let'.

```
    let varString be "20"
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
  num16 x be 5

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

Array menggunakan kurawal ```{}``` dan dapat ditentukan ukurannya.

```
  num16(3) varArray be {1, 2, 3} \ berukuran 3
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
  num16(5) arrayNum be {1, 3, 5, 7, 9}
  redo arrayNum as int i:
    printf ["%d", i]
  ok

  \ output 13579
```

### Ekstensi

Gampil menggunakan ekstensi ```.ga```. Bisa dilihat di file ```src/example.ga```.


