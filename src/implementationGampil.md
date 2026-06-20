Kembangkan Bahasa pemrograman gampil ini!

1. Python-like: Bahasa gampil mendukung sintaks penulisan data yang persis PEP, khususnya untuk nilai yang sifatnya non-sequenced, misal integer, float, complex, Boolean, atau string, dsb.. Untuk nilai yang sifatnya sequenced seperti set, frozen set, tuples dan list, pengguna harus menggunakan casting. 

2. Field type test: Pada file "D:\Gampil The Language\src\example.ga", terdapat tipe field. Tolong analisis dan realisasikan dalam Bahasa Gampil.

3. Malloc and pointer casting improvement: perbaiki error ini 

```"D:\Gampil The Language>gampil src/example.ga -o src/example.exe
Error: expected expression (got 'TOK_NUM32')
  --> line 16:21
   |
 16|     num32() num2 be num32()[malloc[2]]     \ if the size is empty, variable becomes pointer
   |                     ^ expected expression
   |
Compilation failed at parse stage."
```

4. Modularity: Pisahkan kompiler utama, python runtime dan assembler. Rancangan seharusnya, pengguna dapat mengganti jenis assembly dengan assembler yang kita sudah miliki, misal arm64, z80, x86, dan sebagainya. Dengan begitu pula, pengguna dapat mengganti jenis runtime dan versi python yang diinginkan, seperti cpython, atau python jenis lainnya.

5. Portability: Gampil asli harus dapat dijalankan di semua OS, seperti pada bahasa pemrograman ANSI C. Oleh karena itu, buat kompiler utamanya dengan ANSI C agar dapat berjalan di semua OS.

6. Usahakan kompiler dapat mengimplementasikan semua kode yang ada di file "D:\Gampil The Language\src\example.ga" dan "D:\Gampil The Language\README.md".

7. Implement "nil" keyword as pointer to nowhere (null pointer).

# Gampil Language Development Plan

This plan details the implementation of new features, semantic rules, syntax enhancements, and architectural modularity in the Gampil Programming Language.

## User Review Required

> [!IMPORTANT]
> **Modularity Defaults:** If no `gampil.cfg` exists, the default compiler behavior will be preserved (invoking `gcc` and standard `python` on `../runtime/gampil_runtime.py`). 
>
> **C99 `<complex.h>` Dependency:** Support for complex literals (e.g. `3j`) in static variables compiles using C99 complex numbers. The compiled target requires a compiler supporting `<complex.h>` (such as modern GCC/Clang).

---

## Proposed Changes

### Compiler Lexer and Tokens

#### [MODIFY] [token.h](file:///D:/Gampil%20The%20Language/compiler/include/token.h)
- Add `TOK_COMPLEX_LIT` to `TokenType` enum.

#### [MODIFY] [lexer.c](file:///D:/Gampil%20The%20Language/compiler/src/lexer.c)
- **Boolean literals**: Add `{"True", TOK_TRUE}` and `{"False", TOK_FALSE}` to `kw_table` for Python PEP-compliant booleans.
- **Float literals**: Update number lexing to support leading dot (e.g. `.001`) and trailing dot (e.g. `10.`).
- **Complex literals**: Scan complex literals ending with `j` or `J` (e.g. `3j`, `4.5j`) and return `TOK_COMPLEX_LIT`.
- **String literals**: 
  - Support Python prefixes (e.g. `r`, `b`, `f`, `u`, `rf`, `rb`, etc.) and record the prefix.
  - Scan Python raw strings (no escape sequence translation when `r` prefix is present).
  - Scan triple-quoted strings `"""..."""` and `'''...'''` that can span multiple lines.

---

### Compiler AST and Parser

#### [MODIFY] [ast.h](file:///D:/Gampil%20The%20Language/compiler/include/ast.h)
- Add `AST_COMPLEX_LIT` to `AstKind` enum.
- Add `prefix` string and `delim`/`is_triple` fields to `str_lit` union variant to preserve string literal metadata.
- Add `complex_lit` union variant to store the parsed complex number string representation.

#### [MODIFY] [ast.c](file:///D:/Gampil%20The%20Language/compiler/src/ast.c)
- Handle printing and freeing of `AST_COMPLEX_LIT` nodes and custom string literal metadata.

#### [MODIFY] [parser.c](file:///D:/Gampil%20The%20Language/compiler/src/parser.c)
- **Complex literals**: In `parse_primary`, parse `TOK_COMPLEX_LIT` to `AST_COMPLEX_LIT`.
- **Field types**: 
  - Implement full parsing of `field({type1, type2, ...}) var` inside statement dispatcher.
  - Parse and populate `field_params` list inside the variable declaration node.

---

### Compiler Code Generator

#### [MODIFY] [codegen.c](file:///D:/Gampil%20The%20Language/compiler/src/codegen.c)
- **Complex Literals**: Add support for `#include <complex.h>` at top of generated C source, and generate `(value * _Complex_I)` for `AST_COMPLEX_LIT` nodes.
- **Field Structs**:
  - Implement struct generation for `GTYPE_FIELD` variables.
  - Support explicit layouts (from `field_params` to `_0`, `_1`, etc.).
  - Support inferred layouts from `AST_TABLE_LIT` initializers (either mapping name-value initializers like `num16 id be 2` to struct field `id`, or indexing layout type inference to `_0`, `_1`, etc.).
- **Field Indexing**:
  - In `AST_INDEX_EXPR`, if the target array name maps to a `GTYPE_FIELD` variable, translate the static index `(i)` access to struct member field `._i`.
  - Handle indexing in assignments `user(0) be 10`.
- **Dynamic Python AST translator**:
  - Implement `ast_to_python()` to recursively compile Gampil expressions to Python syntax.
  - Support Python operators and syntax mapping (e.g. `^` -> `**`, `=` -> `==`, boolean casing, nil -> `None`).
  - Translate any dynamic assignment or expression statement involving `let` variables to `_gampil_pysnip.py` and run it via the custom python path.
- **Modularity Configuration**:
  - Dynamically load and escape the configured `python` executable and `runtime` path in C code generation, instead of hardcoding.

---

### Compiler Entry and CLI

#### [MODIFY] [main.c](file:///D:/Gampil%20The%20Language/compiler/src/main.c)
- **Configuration loading**:
  - Read `gampil.cfg` configuration file in the working directory or in the executable's directory.
  - Support key-value properties: `assembler`, `python`, `runtime`.
- **Custom environment variables**:
  - Check and override configuration with environment variables `GAMPIL_ASSEMBLER`, `GAMPIL_PYTHON`, `GAMPIL_RUNTIME`.
- **CLI parameters**:
  - Implement `--assembler "<cmd>"`, `--python "<bin>"`, and `--runtime "<path>"` flags.
- **Custom Assembler Command**:
  - Format the assembler command pattern replacing `{src}` and `{out}` placeholders, and run the assembler.

---

### Runtime Bridge

#### [MODIFY] [runtime/gampil_runtime.py](file:///D:/Gampil%20The%20Language/runtime/gampil_runtime.py)
- **Iterability**: Implement `__iter__` in `GampilTable` to allow python sequenced constructors (`set()`, `list()`, `tuple()`) to consume Gampil tables.
- **Hashed Fields**: Extend `GampilTable.__init__` to support keyword arguments `**kwargs` and set them as attributes on the table instance.
- **Casting support**: Export Python's `set`, `tuple`, and `frozenset` types in the runtime's global context.
- **Jython/MicroPython Compatibility**: Protect `builtins` import using fallback checking for Jython/MicroPython environments.

---

### Modularity Settings File

#### [NEW] [gampil.cfg](file:///D:/Gampil%20The%20Language/gampil.cfg)
- The settings file defining compiler and runtime binaries.

---

## Verification Plan

### Automated Tests
- Run `make clean` and `make all` inside `compiler/` to build the updated `gampil` executable.
- Run `make test` to verify that existing test suites (`arithmetic.ga`, `conditionals.ga`, `loops.ga`, `arrays.ga`, `dynamic.ga`) compile and pass.
- Write a new regression and feature test `tests/field_pep_test.ga` covering:
  - PEP-compliant literals (hex, bin with underscores, leading/trailing dot floats, complex imaginary numbers).
  - Python sequenced casting: `set[{1, 2}]`, `list[{3, 4}]`, `tuple[{5, 6}]`.
  - Field type layouts (inferred, named/hashed, explicit layout, indexed reading/writing).
- Compile and run `tests/field_pep_test.ga`.

### Manual Verification
- Test different Python configurations by changing `python` to `python3` or `micropython` in `gampil.cfg` or env vars, and confirm successful execution.
- Test changing assembly compilation toolchain (e.g. custom compiler flags or commands) and verify the program compiles correctly.
