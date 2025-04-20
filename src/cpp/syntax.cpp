/** @file syntax.cpp

  @author  Melwin Svensson.
  
 */
#include "../include/prototypes.h"


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  vec4 color;
} SyntaxMapNode;


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


_UNUSED static HashMap *syntax_map = NULL;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Function to be called by `atexit()` to free `syntax_map`. */
static void syntax_map_free(void) {
  hashmap_free(syntax_map);
  syntax_map = NULL;
}

/* Add a `key` to the `syntax_map` with `color`. */
static void syntax_map_add(const char *const restrict key, vec4 color) {
  ASSERT(key);
  SyntaxMapNode *node;
  /* If the syntax map has not been initilized, then initialize it and setup it so its freed by 'atexit()'.  */
  if (!syntax_map) {
    syntax_map = hashmap_create_wfreefunc(free);
    atexit(syntax_map_free);
  }
  /* If the node already exists, just modify the color. */
  if ((node = (__TYPE(node))hashmap_get(syntax_map, key))) {
    node->color = color;
    return;
  }
  node = (__TYPE(node))xmalloc(sizeof(*node));
  node->color = color;
  hashmap_insert(syntax_map, key, node);
}

/* Configure color map with base c/cpp syntax. */
static void set_c_cpp_synx(openfilestruct *const file) {
  file->type.clear_and_set<C_CPP>();
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
  file->type.clear_and_set<ATNT_ASM>();
  /* Instruction's */
  syntax_map_add("movb", VEC4_VS_CODE_BLUE);
  syntax_map_add("movw", VEC4_VS_CODE_BLUE);
  syntax_map_add("movl", VEC4_VS_CODE_BLUE);
  syntax_map_add("movq", VEC4_VS_CODE_BLUE);
  syntax_map_add("cmpb", VEC4_VS_CODE_BLUE);
  syntax_map_add("cmpw", VEC4_VS_CODE_BLUE);
  syntax_map_add("cmpl", VEC4_VS_CODE_BLUE);
  syntax_map_add("cmpq", VEC4_VS_CODE_BLUE);
  syntax_map_add("subd", VEC4_VS_CODE_BLUE);
  syntax_map_add("subw", VEC4_VS_CODE_BLUE);
  syntax_map_add("subl", VEC4_VS_CODE_BLUE);
  syntax_map_add("subq", VEC4_VS_CODE_BLUE);
  syntax_map_add("incd", VEC4_VS_CODE_BLUE);
  syntax_map_add("incw", VEC4_VS_CODE_BLUE);
  syntax_map_add("incl", VEC4_VS_CODE_BLUE);
  syntax_map_add("incq", VEC4_VS_CODE_BLUE);
  syntax_map_add("leab", VEC4_VS_CODE_BLUE);
  syntax_map_add("leaw", VEC4_VS_CODE_BLUE);
  syntax_map_add("leal", VEC4_VS_CODE_BLUE);
  syntax_map_add("leaq", VEC4_VS_CODE_BLUE);
  syntax_map_add("xorb", VEC4_VS_CODE_BLUE);
  syntax_map_add("xorw", VEC4_VS_CODE_BLUE);
  syntax_map_add("xorl", VEC4_VS_CODE_BLUE);
  syntax_map_add("xorq", VEC4_VS_CODE_BLUE);
  syntax_map_add("jmp",  VEC4_VS_CODE_BLUE);
  syntax_map_add("je",   VEC4_VS_CODE_BLUE);  /* Jump when equal. */
  syntax_map_add("jne",  VEC4_VS_CODE_BLUE);  /* Jump when not equal. */
  syntax_map_add("jz",   VEC4_VS_CODE_BLUE);  /* Jump when last result was zero. */
  syntax_map_add("jg",   VEC4_VS_CODE_BLUE);  /* Jump when greater then. */
  syntax_map_add("jge",  VEC4_VS_CODE_BLUE);  /* Jump when greater then or equal to. */
  syntax_map_add("jl",   VEC4_VS_CODE_BLUE);  /* Jump when less then. */
  syntax_map_add("jle",  VEC4_VS_CODE_BLUE);  /* Jump when less then or equal to. */
  syntax_map_add("ret",  VEC4_VS_CODE_BLUE);
  syntax_map_add("syscall", VEC4_VS_CODE_BLUE);
  syntax_map_add("call", VEC4_VS_CODE_BLUE);
  /* 64-bit register's */
  syntax_map_add("rax", VEC4_VS_CODE_GREEN);  /* Return register. */
  syntax_map_add("rdi", VEC4_VS_CODE_GREEN);
  syntax_map_add("rsi", VEC4_VS_CODE_GREEN);
  syntax_map_add("rdx", VEC4_VS_CODE_GREEN);
  syntax_map_add("r10", VEC4_VS_CODE_GREEN);
  syntax_map_add("r8", VEC4_VS_CODE_GREEN);
  syntax_map_add("r9", VEC4_VS_CODE_GREEN);
  syntax_map_add("rbx", VEC4_VS_CODE_GREEN);
  syntax_map_add("rbp", VEC4_VS_CODE_GREEN);
  syntax_map_add("rsp", VEC4_VS_CODE_GREEN);
  syntax_map_add("rip", VEC4_VS_CODE_GREEN);  /* Instruction pointer. */
  /* 32-bit register's */
  syntax_map_add("eax", VEC4_VS_CODE_GREEN);
  syntax_map_add("ebx", VEC4_VS_CODE_GREEN);
  syntax_map_add("ecx", VEC4_VS_CODE_GREEN);
  syntax_map_add("edx", VEC4_VS_CODE_GREEN);
  syntax_map_add("esi", VEC4_VS_CODE_GREEN);
  syntax_map_add("edi", VEC4_VS_CODE_GREEN);
  syntax_map_add("esp", VEC4_VS_CODE_GREEN);  /* Stack pointer. */
  syntax_map_add("ebp", VEC4_VS_CODE_GREEN);  /* Base pointer. */
  /* 16-bit register's */
  syntax_map_add("ax", VEC4_VS_CODE_GREEN);
  syntax_map_add("bx", VEC4_VS_CODE_GREEN);
  syntax_map_add("cx", VEC4_VS_CODE_GREEN);
  syntax_map_add("dx", VEC4_VS_CODE_GREEN);
  /* High 8-bit register's */
  syntax_map_add("ah", VEC4_VS_CODE_GREEN);
  syntax_map_add("bh", VEC4_VS_CODE_GREEN);
  syntax_map_add("ch", VEC4_VS_CODE_GREEN);
  syntax_map_add("dh", VEC4_VS_CODE_GREEN);
  /* Low 8-bit register's */
  syntax_map_add("al", VEC4_VS_CODE_GREEN);
  syntax_map_add("bl", VEC4_VS_CODE_GREEN);
  syntax_map_add("cl", VEC4_VS_CODE_GREEN);
  syntax_map_add("dl", VEC4_VS_CODE_GREEN);
  /* Directive's */
  syntax_map_add("section", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("text", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("data", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("globl", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("extern", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("type", VEC4_VS_CODE_MAGENTA);
  syntax_map_add("asciz", VEC4_VS_CODE_MAGENTA);
}

/* Configure color map with base asm syntax. */
static void set_asm_synx(openfilestruct *const file) {
  file->type.clear_and_set<ASM>();
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
  file->type.clear_and_set<BASH>();
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
  get_env_path_binaries();
}

/* Configure color map with base systemd service syntax. */
static void set_systemd_service_synx(openfilestruct *file) {
  file->type.clear_and_set<SYSTEMD_SERVICE>();
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
  file->type.clear();
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
        file->type.clear_and_set<GLSL>();
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
        file->type.clear_and_set<NANOX_CONFIG>();
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

bool syntax_map_exists(const char *const restrict key, vec4 *const color) {
  ASSERT(key);
  ASSERT(color);
  SyntaxMapNode *node;
  if (!syntax_map || !(node = (__TYPE(node))hashmap_get(syntax_map, key))) {
    return FALSE;
  }
  *color = node->color;
  return TRUE;
}
