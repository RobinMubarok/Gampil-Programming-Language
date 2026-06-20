# ============================================================
#  Gampil Programming Language Compiler — Makefile
#  Builds the `gampil` compiler from compiler/src/*.c
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11
INCDIR  = include
SRCDIR  = src
OBJDIR  = build

SRCS    = $(SRCDIR)/lexer.c    \
          $(SRCDIR)/ast.c      \
          $(SRCDIR)/parser.c   \
          $(SRCDIR)/symtable.c \
          $(SRCDIR)/semantic.c \
          $(SRCDIR)/codegen.c  \
          $(SRCDIR)/main.c

OBJS    = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
TARGET  = gampil.exe

.PHONY: all clean test

# ── Default target ───────────────────────────────────────────
all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm
	@echo "Built: $(TARGET)"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

# ── Cleaning ─────────────────────────────────────────────────
clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "Cleaned."

# ── Quick test ───────────────────────────────────────────────
test: all
	@echo "=== Test: Hello World ==="
	./$(TARGET) ../tests/hello.ga -o ../tests/hello_out
	../tests/hello_out
	@echo ""
	@echo "=== Test: Arithmetic ==="
	./$(TARGET) ../tests/arithmetic.ga -o ../tests/arith_out
	../tests/arith_out
	@echo ""
	@echo "=== Test: Conditionals ==="
	./$(TARGET) ../tests/conditionals.ga -o ../tests/cond_out
	../tests/cond_out
	@echo ""
	@echo "=== Test: Loops ==="
	./$(TARGET) ../tests/loops.ga -o ../tests/loops_out
	../tests/loops_out
	@echo ""
	@echo "=== Test: Arrays ==="
	./$(TARGET) ../tests/arrays.ga -o ../tests/arrays_out
	../tests/arrays_out

# ── Debug: print tokens ──────────────────────────────────────
tokens: all
	./$(TARGET) ../src/example.ga --tokens

# ── Debug: print AST ─────────────────────────────────────────
ast: all
	./$(TARGET) ../src/example.ga --ast

# ── Generate C source only (no gcc) ─────────────────────────
csource: all
	./$(TARGET) ../src/example.ga -S
	@echo "C source: ../src/example.ga.out.c"
