/** @file syntax.cpp

  @author  Melwin Svensson.
  
 */
#include "../include/prototypes.h"


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  vec4 color;
} SyntaxMapNode;


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


_UNUSED static HashMap *atnt_asm_syntax_map = NULL;
_UNUSED static HashMap *bash_syntax_map = NULL;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Function to be called by `atexit()` to free `syntax_map`. */
static void syntax_map_free(void) {
  if (atnt_asm_syntax_map) {
    hashmap_free(atnt_asm_syntax_map);
    atnt_asm_syntax_map = NULL;
  }
  if (bash_syntax_map) {
    hashmap_free(bash_syntax_map);
    bash_syntax_map = NULL;
  }
}

static void syntax_map_set_free(void) {
  static bool done = FALSE;
  if (!done) {
    atexit(syntax_map_free);
    done = TRUE;
  }
}

/* Add a `key` to the `syntax_map` with `color`. */
static void syntax_map_add(file_type type, const char *const restrict key, vec4 color) {
  ASSERT(key);
  SyntaxMapNode *node;
  HashMap *map;
  switch (type) {
    case ATNT_ASM: {
      /* If the syntax map has not been initilized, then initialize it and setup it so its freed by 'atexit()'.  */
      if (!atnt_asm_syntax_map) {
        atnt_asm_syntax_map = hashmap_create_wfreefunc(free);
        syntax_map_set_free();
      }
      map = atnt_asm_syntax_map;
      break;
    }
    default: {
      return;
    }
  }
  /* If the node already exists, just modify the color. */
  if ((node = (__TYPE(node))hashmap_get(map, key))) {
    node->color = color;
    return;
  }
  MALLOC_STRUCT(node);
  node->color = color;
  hashmap_insert(map, key, node);
}

/* Configure color map with base c/cpp syntax. */
static void set_c_cpp_synx(openfilestruct *const file) {
  // file->type.clear_and_set<C_CPP>();
  file->is_c_file        = TRUE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = FALSE;
  file->is_atnt_asm_file = FALSE;
  file->is_bash_file     = FALSE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  /* Types. */
  test_map["bool"]      = {FG_VS_CODE_BLUE};
  test_map["char"]      = {FG_VS_CODE_BLUE};
  test_map["short"]     = {FG_VS_CODE_BLUE};
  test_map["int"]       = {FG_VS_CODE_BLUE, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["long"]      = {FG_VS_CODE_BLUE};
  test_map["unsigned"]  = {FG_VS_CODE_BLUE};
  test_map["float"]     = {FG_VS_CODE_BLUE};
  test_map["double"]    = {FG_VS_CODE_BLUE};
  test_map["void"]      = {FG_VS_CODE_BLUE};
  test_map["static"]    = {FG_VS_CODE_BLUE};
  test_map["extern"]    = {FG_VS_CODE_BLUE};
  test_map["constexpr"] = {FG_VS_CODE_BLUE};
  test_map["const"]     = {FG_VS_CODE_BLUE};
  test_map["true"]      = {FG_VS_CODE_BLUE};
  test_map["false"]     = {FG_VS_CODE_BLUE};
  test_map["true"]      = {FG_VS_CODE_BLUE};
  test_map["false"]     = {FG_VS_CODE_BLUE};
  test_map["NULL"]      = {FG_VS_CODE_BLUE};
  test_map["typedef"]   = {FG_VS_CODE_BLUE};
  test_map["sizeof"]    = {FG_VS_CODE_BLUE};
  test_map["struct"]    = {FG_VS_CODE_BLUE, -1, -1, IS_WORD_STRUCT};
  test_map["class"]     = {FG_VS_CODE_BLUE, -1, -1, IS_WORD_CLASS};
  test_map["enum"]      = {FG_VS_CODE_BLUE};
  test_map["namespace"] = {FG_VS_CODE_BLUE};
  test_map["inline"]    = {FG_VS_CODE_BLUE};
  test_map["typename"]  = {FG_VS_CODE_BLUE};
  test_map["template"]  = {FG_VS_CODE_BLUE};
  test_map["volatile"]  = {FG_VS_CODE_BLUE};
  test_map["public"]    = {FG_VS_CODE_BLUE};
  test_map["private"]   = {FG_VS_CODE_BLUE};
  test_map["explicit"]  = {FG_VS_CODE_BLUE};
  test_map["this"]      = {FG_VS_CODE_BLUE};
  test_map["union"]     = {FG_VS_CODE_BLUE};
  test_map["auto"]      = {FG_VS_CODE_BLUE};
  test_map["noexcept"]  = {FG_VS_CODE_BLUE, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["protected"] = {FG_VS_CODE_BLUE};
  /* Control statements. */
  test_map["if"]       = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["else"]     = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["case"]     = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["switch"]   = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["default"]  = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["for"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["while"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["return"]   = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["break"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["do"]       = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["continue"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["using"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["operator"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["try"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
  test_map["catch"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
}

/* Set the `AT&T asm` syntax in `syntax_map` as well as setting the type flag in `file` to `ATNT_ASM`. */
static void set_atnt_asm_syntax(openfilestruct *const file) {
  ASSERT(file);
  // file->type.clear_and_set<ATNT_ASM>();
  file->is_c_file        = FALSE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = FALSE;
  file->is_atnt_asm_file = TRUE;
  file->is_bash_file     = FALSE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  /* Instruction's */
  syntax_map_add(ATNT_ASM, "movb", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "movw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "movl", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "movq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "cmpb", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "cmpw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "cmpl", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "cmpq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "subd", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "subw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "subl", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "subq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "incd", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "incw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "incl", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "incq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "leab", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "leaw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "leal", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "leaq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "xorb", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "xorw", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "xorl", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "xorq", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "jmp",  VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "je",   VEC4_VS_CODE_BLUE);  /* Jump when equal. */
  syntax_map_add(ATNT_ASM, "jne",  VEC4_VS_CODE_BLUE);  /* Jump when not equal. */
  syntax_map_add(ATNT_ASM, "jz",   VEC4_VS_CODE_BLUE);  /* Jump when last result was zero. */
  syntax_map_add(ATNT_ASM, "jg",   VEC4_VS_CODE_BLUE);  /* Jump when greater then. */
  syntax_map_add(ATNT_ASM, "jge",  VEC4_VS_CODE_BLUE);  /* Jump when greater then or equal to. */
  syntax_map_add(ATNT_ASM, "jl",   VEC4_VS_CODE_BLUE);  /* Jump when less then. */
  syntax_map_add(ATNT_ASM, "jle",  VEC4_VS_CODE_BLUE);  /* Jump when less then or equal to. */
  syntax_map_add(ATNT_ASM, "ret",  VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "syscall", VEC4_VS_CODE_BLUE);
  syntax_map_add(ATNT_ASM, "call", VEC4_VS_CODE_BLUE);
  /* 64-bit register's */
  syntax_map_add(ATNT_ASM, "rax", VEC4_VS_CODE_GREEN);  /* Return register. */
  syntax_map_add(ATNT_ASM, "rdi", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rsi", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rdx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r8",  VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r9",  VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r10", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r11", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r12", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r13", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r14", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "r15", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rbx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rbp", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rsp", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "rip", VEC4_VS_CODE_GREEN);  /* Instruction pointer. */
  /* 32-bit register's */
  syntax_map_add(ATNT_ASM, "eax", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "ebx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "ecx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "edx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "esi", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "edi", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "esp", VEC4_VS_CODE_GREEN);  /* Stack pointer. */
  syntax_map_add(ATNT_ASM, "ebp", VEC4_VS_CODE_GREEN);  /* Base pointer. */
  /* 16-bit register's */
  syntax_map_add(ATNT_ASM, "ax", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "bx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "cx", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "dx", VEC4_VS_CODE_GREEN);
  /* High 8-bit register's */
  syntax_map_add(ATNT_ASM, "ah", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "bh", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "ch", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "dh", VEC4_VS_CODE_GREEN);
  /* Low 8-bit register's */
  syntax_map_add(ATNT_ASM, "al", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "bl", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "cl", VEC4_VS_CODE_GREEN);
  syntax_map_add(ATNT_ASM, "dl", VEC4_VS_CODE_GREEN);
  /* Directive's */
  syntax_map_add(ATNT_ASM, "section", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "text", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "data", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "globl", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "extern", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "type", VEC4_VS_CODE_MAGENTA);
  syntax_map_add(ATNT_ASM, "asciz", VEC4_VS_CODE_MAGENTA);
}

/* Configure color map with base asm syntax. */
static void set_asm_synx(openfilestruct *const file) {
  // file->type.clear_and_set<ASM>();
  file->is_c_file        = FALSE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = TRUE;
  file->is_atnt_asm_file = FALSE;
  file->is_bash_file     = FALSE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  /* 64-bit registers. */
  test_map["rax"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rbx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rcx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rdx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rsi"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rdi"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rbp"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rsp"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["rip"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r8"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r9"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r10"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r11"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r12"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r13"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r14"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["r15"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* 32-bit registers. */
  test_map["edx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* 16-bit registers. */
  test_map["dx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* Lover 8-bit of 16-bit registers. */
  test_map["dl"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["al"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* Higher 8-bit of 16-bit registers. */
  test_map["dh"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* Simd 'sse' registers. */
  test_map["xmm0"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm1"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm2"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm3"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm4"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm5"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm6"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["xmm7"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* Simd 'avx' registers. */
  test_map["ymm0"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm1"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm2"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm3"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm4"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm5"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm6"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  test_map["ymm7"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
  /* 'avx' instructions. */
  test_map["vmovdqa"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["vmovaps"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["vaddps"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["vsubps"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  /* Instructions. */
  test_map["syscall"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["mov"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["xor"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["pxor"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["cmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["cmpb"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["je"]       = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["inc"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["int"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["jmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["ret"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["add"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["sub"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["mul"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["and"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["test"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["jz"]       = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["jnz"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["jmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["bsf"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["equ"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["lea"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["movdqa"]   = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["movdqu"]   = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["pcmpeqb"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  test_map["pmovmskb"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
  /* Control words. */
  test_map["section"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
  test_map["global"]  = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
  test_map["db"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
  test_map["byte"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
}

/* Configure color map with base bash syntax. */
static void set_bash_synx(openfilestruct *const file) {
  ASSERT(file);
  // file->type.clear_and_set<BASH>();
  file->is_c_file        = FALSE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = FALSE;
  file->is_atnt_asm_file = FALSE;
  file->is_bash_file     = TRUE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  test_map["if"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["elif"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["else"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["fi"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["then"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["case"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["in"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["esac"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["for"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["while"] = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["do"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["done"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  // get_env_path_binaries();
}

/* Configure color map with base systemd service syntax. */
static void set_systemd_service_synx(openfilestruct *file) {
  // file->type.clear_and_set<SYSTEMD_SERVICE>();
  file->is_c_file        = FALSE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = FALSE;
  file->is_atnt_asm_file = FALSE;
  file->is_bash_file     = FALSE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  /* Groups. */
  test_map["Unit"]    = {FG_VS_CODE_BRIGHT_CYAN};
  test_map["Service"] = {FG_VS_CODE_BRIGHT_CYAN};
  test_map["Install"] = {FG_VS_CODE_BRIGHT_CYAN};
  /* Unit control statements. */
  test_map["Description"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Documentation"] = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Requires"]      = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Wants"]         = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Before"]        = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["After"]         = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Conflicts"]     = {FG_VS_CODE_BRIGHT_MAGENTA};
  /* Service control statements. */
  test_map["Type"]             = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["ExecStart"]        = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["ExecStop"]         = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["ExecReload"]       = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Restart"]          = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["RestartSec"]       = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["User"]             = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Environment"]      = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["EnvironmentFile"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["PIDFile"]          = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["WorkingDirectory"] = {FG_VS_CODE_BRIGHT_MAGENTA};
  /* Install control statements. */
  test_map["WantedBy"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["RequiredBy"] = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Also"]       = {FG_VS_CODE_BRIGHT_MAGENTA};
  test_map["Alias"]      = {FG_VS_CODE_BRIGHT_MAGENTA};
}

/* Function to check syntax for a open buffer. */
void syntax_check_file(openfilestruct *file) {
  // file->type.clear();
  file->is_c_file        = FALSE;
  file->is_cxx_file      = FALSE;
  file->is_nasm_file     = FALSE;
  file->is_atnt_asm_file = FALSE;
  file->is_glsl_file     = FALSE;
  file->is_systemd_file  = FALSE;
  file->is_nanox_file    = FALSE;
  const char *file_ext;
  if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
    file_ext = ext(file->filename);
    if (file_ext && *(++file_ext)) {
      if (strcmp(file_ext, "cpp") == 0 || strcmp(file_ext, "c") == 0 || strcmp(file_ext, "cc") == 0 || strcmp(file_ext, "h") == 0 || strcmp(file_ext, "hpp") == 0) {
        set_c_cpp_synx(file);
      }
      /* AT&T asm syntax. */
      else if (strcmp(file_ext, "S") == 0 || strcmp(file_ext, "s") == 0) {
        set_atnt_asm_syntax(file);
      }
      /* NASM syntax. */
      else if (strcmp(file_ext, "asm") == 0) {
        set_asm_synx(file);
      }
      else if (strcmp(file_ext, "sh") == 0) {
        set_bash_synx(file);
      }
      else if (strcmp(file_ext, "glsl") == 0) {
        // file->type.clear_and_set<GLSL>();
        file->is_glsl_file     = TRUE;
        /* Standard types. */
        test_map["mat4"]  = {FG_VS_CODE_BLUE};
        test_map["mat3"]  = {FG_VS_CODE_BLUE};
        test_map["vec4"]  = {FG_VS_CODE_BLUE};
        test_map["vec3"]  = {FG_VS_CODE_BLUE};
        test_map["vec2"]  = {FG_VS_CODE_BLUE};
        test_map["int"]   = {FG_VS_CODE_BLUE};
        test_map["float"] = {FG_VS_CODE_BLUE};
        /* Control types. */
        test_map["return"] = {FG_VS_CODE_BRIGHT_MAGENTA};
        test_map["if"]     = {FG_VS_CODE_BRIGHT_MAGENTA};
        test_map["else"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
        // LSP->index_file(file->filename);
      }
      else if (strcmp(file_ext, "service") == 0) {
        set_systemd_service_synx(file);
      }
      else if (strcmp(file_ext, "nxcfg") == 0) {
        // file->type.clear_and_set<NANOX_CONFIG>();
        file->is_nanox_file = TRUE;
      }
    }
    /* TODO: Check that this is fully safe. */
    else {
      linestruct *line = file->filetop;
      while (line && !line->data[0]) {
        line = line->next;
      }
      if (line && strstr(line->data, "#!")) {
        set_bash_synx(file);
      }
    }
  }
}

bool syntax_map_exists(file_type type, const char *const restrict key, vec4 *const color) {
  ASSERT(key);
  ASSERT(color);
  SyntaxMapNode *node;
  HashMap *map;
  switch (type) {
    case ATNT_ASM: {
      map = atnt_asm_syntax_map;
      break;
    }
    default: {
      return FALSE;
    }
  }
  if (!map || !(node = (__TYPE(node))hashmap_get(map, key))) {
    return FALSE;
  }
  *color = node->color;
  return TRUE;
}
