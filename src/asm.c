#include "asm.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#define MAX_TOKENS 16
#define MAX_LABELS 256
#define MAX_CODE   65536

typedef struct {
    char name[64];
    int addr;
} Label;

typedef struct {
    int count;
    Label labels[MAX_LABELS];
} SymTable;

static int sym_find(SymTable *st, const char *name) {
    for (int i = 0; i < st->count; i++)
        if (strcmp(st->labels[i].name, name) == 0)
            return st->labels[i].addr;
    return -1;
}

static void sym_add(SymTable *st, const char *name, int addr) {
    if (st->count >= MAX_LABELS) return;
    strncpy(st->labels[st->count].name, name, 63);
    st->labels[st->count].name[63] = '\0';
    st->labels[st->count].addr = addr;
    st->count++;
}

static int reg_from_name(const char *s) {
    if (strcasecmp(s, "ACC") == 0) return REG_ACC;
    if (strcasecmp(s, "B") == 0)   return REG_B;
    if (strcasecmp(s, "SP") == 0)  return REG_SP;
    if (strcasecmp(s, "FL") == 0 || strcasecmp(s, "FLAGS") == 0) return REG_FL;
    return -1;
}

static int is_register(const char *s) {
    return reg_from_name(s) >= 0;
}

static int parse_int(const char *s, int64_t *val) {
    char *end;
    errno = 0;
    *val = strtoll(s, &end, 10);
    return errno != ERANGE && *end == '\0';
}

static int is_ternary_literal(const char *s) {
    if (*s == '\0') return 0;
    for (; *s; s++)
        if (*s != '-' && *s != '0' && *s != '+')
            return 0;
    return 1;
}

static int64_t ternary_literal_val(const char *s) {
    Tryte t;
    tryte_from_str(s, &t);
    return tryte_to_int64(t);
}

static int is_string_literal(const char *s) {
    return *s == '"';
}

static int64_t string_first_char_val(const char *s) {
    size_t len = strlen(s);
    if (len < 3 || s[0] != '"' || s[len - 1] != '"') return -1;
    return (int64_t)(unsigned char)s[1];
}

static int is_valid_label(const char *s) {
    if (*s == '\0') return 0;
    if (*s >= '0' && *s <= '9') return 0;
    for (; *s; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')
              || (*s >= '0' && *s <= '9') || *s == '_'))
            return 0;
    return 1;
}

static int is_label_def(const char *s) {
    size_t len = strlen(s);
    if (len < 2) return 0;
    return s[len - 1] == ':';
}

static char **tokenize_line(const char *line, int *ntok) {
    static char *tokens[MAX_TOKENS];
    static char buf[1024];
    int n = 0;

    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *p = buf;
    while (*p && (unsigned char)*p <= ' ') p++;

    char *token = p;
    int in_token = 0;

    for (; *p; p++) {
        if (*p == ';') { *p = '\0'; break; }
        if (*p == ',' || *p == '\t' || *p == ' ') {
            if (in_token) {
                *p = '\0';
                if (strlen(token) > 0) {
                    tokens[n++] = token;
                    if (n >= MAX_TOKENS) break;
                }
                in_token = 0;
            }
        } else {
            if (!in_token) { token = p; in_token = 1; }
        }
    }
    if (in_token && strlen(token) > 0 && n < MAX_TOKENS)
        tokens[n++] = token;

    *ntok = n;
    return tokens;
}

Tryte *asm_assemble(const char *source, size_t *len) {
    Tryte *code = NULL;
    AsmResult result = asm_assemble_ex(source, &code, len);
    if (!result.valid && code) {
        free(code);
        return NULL;
    }
    return code;
}

static Tryte make_tryte(int64_t val) {
    Tryte t;
    tryte_from_int64(val, &t);
    return t;
}

AsmResult asm_assemble_ex(const char *source, Tryte **code, size_t *len) {
    AsmResult result = {0, NULL, 0};
    *code = NULL;
    *len = 0;

    SymTable st;
    st.count = 0;

    Tryte *output = (Tryte *)calloc(MAX_CODE, sizeof(Tryte));
    if (!output) { result.error_msg = "out of memory"; return result; }

    int output_len = 0;

    struct {
        int instr_idx;
        int operand_offset;
        char label[64];
        int needs_resolve;
    } pending_refs[MAX_CODE];

    int npending = 0;

    const char *p = source;
    int line_num = 0;

    while (*p && output_len < MAX_CODE) {
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        int line_len = (int)(p - line_start);
        if (*p == '\n') { line_num++; p++; }
        if (line_len == 0 || line_start[0] == ';' || line_start[0] == '#') continue;

        char line[1024];
        int copy_len = line_len < 1023 ? line_len : 1023;
        strncpy(line, line_start, copy_len);
        line[copy_len] = '\0';

        int ntok;
        char **tokens = tokenize_line(line, &ntok);
        if (ntok == 0) continue;

        int ti = 0;

        if (is_label_def(tokens[ti])) {
            size_t nlen = strlen(tokens[ti]);
            tokens[ti][nlen - 1] = '\0';
            if (!is_valid_label(tokens[ti])) {
                result.valid = 0;
                result.error_msg = "invalid label name";
                result.error_line = line_num;
                free(output);
                return result;
            }
            if (sym_find(&st, tokens[ti]) < 0)
                sym_add(&st, tokens[ti], output_len);
            ti++;
        }

        if (ti >= ntok) continue;

        if (strcmp(tokens[ti], ".str") == 0 || strcmp(tokens[ti], ".db") == 0) {
            ti++;
            if (ti < ntok && is_string_literal(tokens[ti])) {
                const char *s = tokens[ti];
                size_t slen = strlen(s);
                if (slen >= 3) {
                    for (size_t ci = 1; ci < slen - 1; ci++)
                        output[output_len++] = make_tryte((int64_t)(unsigned char)s[ci]);
                }
            }
            continue;
        }

        const char *mnemonic = tokens[ti++];
        Opcode op = opcode_from_name(mnemonic);

        if (op == OP_HLT && strcasecmp(mnemonic, "HLT") != 0) {
            result.valid = 0;
            result.error_msg = "unknown instruction";
            result.error_line = line_num;
            free(output);
            return result;
        }

        int reg_dest = 0, reg_src = 0;
        int64_t operand_val = 0;
        int has_operand = instr_has_operand(op);
        char label_ref[64] = "";
        int has_label_ref = 0;

        if (op == OP_PUSH || op == OP_POP || op == OP_IN || op == OP_OUT) {
            if (ti < ntok) {
                int r = reg_from_name(tokens[ti]);
                if (r >= 0) {
                    if (op == OP_PUSH || op == OP_OUT) reg_src = r;
                    else reg_dest = r;
                    ti++;
                }
            }
        } else if (instr_uses_regs(op)) {
            if (ti < ntok && is_register(tokens[ti]))
                reg_dest = reg_from_name(tokens[ti++]);
            if (ti < ntok && tokens[ti][0] == ',') ti++;
            if (ti < ntok && is_register(tokens[ti]))
                reg_src = reg_from_name(tokens[ti++]);
            if (has_operand) {
                if (ti < ntok && tokens[ti][0] == ',') ti++;
                if (ti < ntok) {
                    int64_t v;
                    if (parse_int(tokens[ti], &v)) {
                        operand_val = v;
                    } else if (is_ternary_literal(tokens[ti])) {
                        operand_val = ternary_literal_val(tokens[ti]);
                    } else if (is_string_literal(tokens[ti])) {
                        operand_val = string_first_char_val(tokens[ti]);
                        if (operand_val < 0) {
                            result.valid = 0;
                            result.error_msg = "empty string literal";
                            result.error_line = line_num;
                            free(output);
                            return result;
                        }
                    } else {
                        strncpy(label_ref, tokens[ti], 63);
                        label_ref[63] = '\0';
                        has_label_ref = 1;
                    }
                    ti++;
                }
            }
        } else if (has_operand) {
            if (ti < ntok) {
                int64_t v;
                if (parse_int(tokens[ti], &v)) {
                    operand_val = v;
                } else if (is_ternary_literal(tokens[ti])) {
                    operand_val = ternary_literal_val(tokens[ti]);
                } else if (is_string_literal(tokens[ti])) {
                    operand_val = string_first_char_val(tokens[ti]);
                    if (operand_val < 0) {
                        result.valid = 0;
                        result.error_msg = "empty string literal";
                        result.error_line = line_num;
                        free(output);
                        return result;
                    }
                } else {
                    strncpy(label_ref, tokens[ti], 63);
                    label_ref[63] = '\0';
                    has_label_ref = 1;
                }
                ti++;
            }
        }

        Tryte encoded = encode_instr(op, reg_dest, reg_src);
        output[output_len++] = encoded;

        if (has_label_ref) {
            if (npending < MAX_CODE) {
                pending_refs[npending].instr_idx = output_len;
                pending_refs[npending].needs_resolve = 1;
                snprintf(pending_refs[npending].label, 64, "%s", label_ref);
                npending++;
            }
            Tryte zero;
            tryte_zero(&zero);
            output[output_len++] = zero;
        } else if (has_operand) {
            output[output_len++] = make_tryte(operand_val);
        }
    }

    for (int i = 0; i < npending; i++) {
        int addr = sym_find(&st, pending_refs[i].label);
        if (addr < 0) {
            result.valid = 0;
            result.error_msg = "undefined label";
            free(output);
            return result;
        }
        output[pending_refs[i].instr_idx] = make_tryte((int64_t)addr);
    }

    *code = output;
    *len = (size_t)output_len;
    result.valid = 1;
    return result;
}
