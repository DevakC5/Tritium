#!/usr/bin/env python3
"""Tritium High-Level Language Compiler

Translates a C-like high-level language into Tritium assembly (.trc).

Usage:
    python compiler/trc_compile.py source.hl          # print assembly to stdout
    python compiler/trc_compile.py source.hl -o out.asm  # write to file
    python compiler/trc_compile.py source.hl | bin/trc   # compile & run
"""

import sys
import re
import os


# --- Token types ---
TOK_EOF = 'EOF'
TOK_IDENT = 'IDENT'
TOK_NUMBER = 'NUMBER'
TOK_STRING = 'STRING'
TOK_SEMI = ';'
TOK_LBRACE = '{'
TOK_RBRACE = '}'
TOK_LPAREN = '('
TOK_RPAREN = ')'
TOK_EQ = '='
TOK_EQEQ = '=='
TOK_NE = '!='
TOK_GT = '>'
TOK_LT = '<'
TOK_GE = '>='
TOK_LE = '<='
TOK_PLUS = '+'
TOK_MINUS = '-'
TOK_STAR = '*'
TOK_SLASH = '/'
TOK_PERCENT = '%'
TOK_BANG = '!'

KEYWORDS = {
    'let': 'LET',
    'if': 'IF',
    'else': 'ELSE',
    'while': 'WHILE',
    'print': 'PRINT',
    'input': 'INPUT',
}

Token = tuple  # (type, value, line, col)


class LexerError(Exception):
    pass


def tokenize(source):
    tokens = []
    i = 0
    line = 1
    col = 0
    src_len = len(source)

    while i < src_len:
        ch = source[i]
        col += 1

        if ch in ' \t\r':
            i += 1
            continue
        if ch == '\n':
            line += 1
            col = 0
            i += 1
            continue

        # Single-line comments
        if ch == '/' and i + 1 < src_len and source[i + 1] == '/':
            while i < src_len and source[i] != '\n':
                i += 1
            continue

        # Block comments
        if ch == '/' and i + 1 < src_len and source[i + 1] == '*':
            i += 2
            col += 1
            while i + 1 < src_len and not (source[i] == '*' and source[i + 1] == '/'):
                if source[i] == '\n':
                    line += 1
                    col = 0
                i += 1
                col += 1
            if i + 1 < src_len:
                i += 2
            continue

        # Two-char operators
        two = source[i:i + 2]
        if two == '==':
            tokens.append((TOK_EQEQ, '==', line, col))
            i += 2
            continue
        if two == '!=':
            tokens.append((TOK_NE, '!=', line, col))
            i += 2
            continue
        if two == '>=':
            tokens.append((TOK_GE, '>=', line, col))
            i += 2
            continue
        if two == '<=':
            tokens.append((TOK_LE, '<=', line, col))
            i += 2
            continue

        # Single-char operators
        if ch in ';{}()=><+-*/%!':
            tok_map = {
                ';': TOK_SEMI, '{': TOK_LBRACE, '}': TOK_RBRACE,
                '(': TOK_LPAREN, ')': TOK_RPAREN, '=': TOK_EQ,
                '>': TOK_GT, '<': TOK_LT, '+': TOK_PLUS,
                '-': TOK_MINUS, '*': TOK_STAR, '/': TOK_SLASH,
                '%': TOK_PERCENT, '!': TOK_BANG,
            }
            tokens.append((tok_map[ch], ch, line, col))
            i += 1
            continue

        # String literals
        if ch == '"':
            start_col = col
            j = i + 1
            s = ''
            while j < src_len:
                if source[j] == '"':
                    tokens.append((TOK_STRING, s, line, start_col))
                    i = j + 1
                    break
                if source[j] == '\\' and j + 1 < src_len:
                    esc = source[j + 1]
                    if esc == 'n':
                        s += '\n'
                    elif esc == 't':
                        s += '\t'
                    elif esc == '\\':
                        s += '\\'
                    elif esc == '"':
                        s += '"'
                    else:
                        s += esc
                    j += 2
                else:
                    if source[j] == '\n':
                        line += 1
                        col = 0
                    s += source[j]
                    j += 1
            else:
                raise LexerError(line, start_col, 'unterminated string')
            col += j - i
            continue

        # Numbers
        if ch.isdigit() or (ch == '-' and i + 1 < src_len and source[i + 1].isdigit()):
            start = i
            if ch == '-':
                i += 1
            while i < src_len and source[i].isdigit():
                i += 1
            tokens.append((TOK_NUMBER, int(source[start:i]), line, col))
            col += i - start
            continue

        # Identifiers and keywords
        if ch.isalpha() or ch == '_':
            start = i
            while i < src_len and (source[i].isalnum() or source[i] == '_'):
                i += 1
            word = source[start:i]
            if word in KEYWORDS:
                tokens.append((KEYWORDS[word], word, line, col))
            else:
                tokens.append((TOK_IDENT, word, line, col))
            col += i - start
            continue

        raise LexerError(line, col, f"unexpected character {ch!r}")

    tokens.append((TOK_EOF, None, line, col))
    return tokens


# --- AST node types ---

class ASTNode:
    pass


class Program(ASTNode):
    def __init__(self, statements):
        self.statements = statements


class LetDecl(ASTNode):
    def __init__(self, name, init_expr, line, col):
        self.name = name
        self.init_expr = init_expr
        self.line = line
        self.col = col


class Assignment(ASTNode):
    def __init__(self, name, expr, line, col):
        self.name = name
        self.expr = expr
        self.line = line
        self.col = col


class IfStmt(ASTNode):
    def __init__(self, condition, then_block, else_block, line, col):
        self.condition = condition
        self.then_block = then_block
        self.else_block = else_block
        self.line = line
        self.col = col


class WhileStmt(ASTNode):
    def __init__(self, condition, body, line, col):
        self.condition = condition
        self.body = body
        self.line = line
        self.col = col


class PrintStmt(ASTNode):
    def __init__(self, value, line, col):
        self.value = value
        self.line = line
        self.col = col


class InputStmt(ASTNode):
    def __init__(self, name, line, col):
        self.name = name
        self.line = line
        self.col = col


class Block(ASTNode):
    def __init__(self, statements):
        self.statements = statements


class BinOp(ASTNode):
    def __init__(self, op, left, right, line, col):
        self.op = op
        self.left = left
        self.right = right
        self.line = line
        self.col = col


class UnaryOp(ASTNode):
    def __init__(self, op, operand, line, col):
        self.op = op
        self.operand = operand
        self.line = line
        self.col = col


class NumberLit(ASTNode):
    def __init__(self, value, line, col):
        self.value = value
        self.line = line
        self.col = col


class StringLit(ASTNode):
    def __init__(self, value, line, col):
        self.value = value
        self.line = line
        self.col = col


class VarRef(ASTNode):
    def __init__(self, name, line, col):
        self.name = name
        self.line = line
        self.col = col


class CondExpr(ASTNode):
    """A comparison expression used as a condition."""
    def __init__(self, op, left, right, line, col):
        self.op = op
        self.left = left
        self.right = right
        self.line = line
        self.col = col


# --- Parser ---

class ParseError(Exception):
    def __init__(self, line, col, msg):
        self.line = line
        self.col = col
        super().__init__(f"line {line}, col {col}: {msg}")


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos]

    def advance(self):
        tok = self.tokens[self.pos]
        self.pos += 1
        return tok

    def expect(self, typ):
        tok = self.peek()
        if tok[0] != typ:
            raise ParseError(tok[2], tok[3],
                             f"expected {typ}, got {tok[0]} ({tok[1]!r})")
        return self.advance()

    def parse(self):
        stmts = []
        while self.peek()[0] != TOK_EOF:
            stmts.append(self.parse_statement())
        return Program(stmts)

    def parse_statement(self):
        tok = self.peek()
        t = tok[0]

        if t == 'LET':
            return self.parse_let()
        if t == 'IF':
            return self.parse_if()
        if t == 'WHILE':
            return self.parse_while()
        if t == 'PRINT':
            return self.parse_print()
        if t == 'INPUT':
            return self.parse_input()
        if t == TOK_LBRACE:
            return self.parse_block()
        if t == TOK_SEMI:
            self.advance()
            return None
        if t == TOK_IDENT:
            return self.parse_assignment_or_expr()
        raise ParseError(tok[2], tok[3], f"unexpected token {tok[1]!r}")

    def parse_let(self):
        self.advance()
        tok = self.expect(TOK_IDENT)
        name = tok[1]
        line, col = tok[2], tok[3]
        init_expr = None
        if self.peek()[0] == TOK_EQ:
            self.advance()
            init_expr = self.parse_expr()
        self.expect(TOK_SEMI)
        return LetDecl(name, init_expr, line, col)

    def parse_assignment_or_expr(self):
        tok = self.advance()
        name = tok[1]
        line, col = tok[2], tok[3]
        if self.peek()[0] == TOK_EQ:
            self.advance()
            expr = self.parse_expr()
            self.expect(TOK_SEMI)
            return Assignment(name, expr, line, col)
        raise ParseError(line, col, "expected '=' after identifier")

    def parse_if(self):
        self.advance()
        line, col = self.peek()[2], self.peek()[3]
        condition = self.parse_condition()
        then_block = self.parse_block()
        else_block = None
        if self.peek()[0] == 'ELSE':
            self.advance()
            else_block = self.parse_block()
        return IfStmt(condition, then_block, else_block, line, col)

    def parse_while(self):
        self.advance()
        line, col = self.peek()[2], self.peek()[3]
        condition = self.parse_condition()
        body = self.parse_block()
        return WhileStmt(condition, body, line, col)

    def parse_print(self):
        self.advance()
        tok = self.peek()
        line, col = tok[2], tok[3]
        if tok[0] == TOK_STRING:
            self.advance()
            value = StringLit(tok[1], line, col)
        else:
            value = self.parse_expr()
        self.expect(TOK_SEMI)
        return PrintStmt(value, line, col)

    def parse_input(self):
        self.advance()
        tok = self.expect(TOK_IDENT)
        name = tok[1]
        line, col = tok[2], tok[3]
        self.expect(TOK_SEMI)
        return InputStmt(name, line, col)

    def parse_block(self):
        self.expect(TOK_LBRACE)
        stmts = []
        while self.peek()[0] != TOK_RBRACE and self.peek()[0] != TOK_EOF:
            stmts.append(self.parse_statement())
        self.expect(TOK_RBRACE)
        return Block([s for s in stmts if s is not None])

    def parse_condition(self):
        """Parse a condition expression for if/while.
        Returns either a regular expression (for truthiness) or a CondExpr."""
        left = self.parse_expr()
        tok = self.peek()
        if tok[0] in (TOK_EQEQ, TOK_NE, TOK_GT, TOK_LT, TOK_GE, TOK_LE):
            self.advance()
            op = tok[1]
            right = self.parse_expr()
            line, col = tok[2], tok[3]
            return CondExpr(op, left, right, line, col)
        return left

    def parse_expr(self):
        return self.parse_comparison()

    def parse_comparison(self):
        left = self.parse_additive()
        tok = self.peek()
        if tok[0] in (TOK_EQEQ, TOK_NE, TOK_GT, TOK_LT, TOK_GE, TOK_LE):
            self.advance()
            op = tok[1]
            right = self.parse_additive()
            line, col = tok[2], tok[3]
            left = CondExpr(op, left, right, line, col)
        return left

    def parse_additive(self):
        left = self.parse_term()
        while self.peek()[0] in (TOK_PLUS, TOK_MINUS):
            tok = self.advance()
            right = self.parse_term()
            left = BinOp(tok[1], left, right, tok[2], tok[3])
        return left

    def parse_term(self):
        left = self.parse_unary()
        while self.peek()[0] in (TOK_STAR, TOK_SLASH, TOK_PERCENT):
            tok = self.advance()
            right = self.parse_unary()
            left = BinOp(tok[1], left, right, tok[2], tok[3])
        return left

    def parse_unary(self):
        tok = self.peek()
        if tok[0] in (TOK_MINUS, TOK_PLUS, TOK_BANG):
            self.advance()
            operand = self.parse_unary()
            return UnaryOp(tok[1], operand, tok[2], tok[3])
        return self.parse_primary()

    def parse_primary(self):
        tok = self.peek()
        if tok[0] == TOK_NUMBER:
            self.advance()
            return NumberLit(tok[1], tok[2], tok[3])
        if tok[0] == TOK_STRING:
            self.advance()
            return StringLit(tok[1], tok[2], tok[3])
        if tok[0] == TOK_IDENT:
            self.advance()
            return VarRef(tok[1], tok[2], tok[3])
        if tok[0] == TOK_LPAREN:
            self.advance()
            expr = self.parse_expr()
            self.expect(TOK_RPAREN)
            return expr
        raise ParseError(tok[2], tok[3], f"unexpected token {tok[1]!r}")


# --- Code Generator ---

RAM_BASE = 4096


class CodegenError(Exception):
    pass


class CodeGenerator:
    def __init__(self):
        self.vars = {}          # name -> ram_addr
        self.next_var_addr = RAM_BASE
        self.label_counter = 0
        self.strings = []       # (label, content)
        self.lines = []
        self.used_before_decl = set()

    def new_label(self, prefix):
        lbl = f"{prefix}_{self.label_counter}"
        self.label_counter += 1
        return lbl

    def add_string(self, content):
        lbl = self.new_label('str')
        self.strings.append((lbl, content))
        return lbl

    def get_var_addr(self, name):
        if name in self.vars:
            return self.vars[name]
        raise CodegenError(f"undefined variable '{name}'")

    def declare_var(self, name):
        if name in self.vars:
            raise CodegenError(f"variable '{name}' already declared")
        addr = self.next_var_addr
        self.next_var_addr += 1
        self.vars[name] = addr
        return addr

    def emit(self, line=''):
        self.lines.append(line)

    def gen_program(self, prog):
        for stmt in prog.statements:
            if stmt is not None:
                self.gen_statement(stmt)
        self.emit('HLT')
        self.emit('')
        for lbl, content in self.strings:
            self.emit(f'{lbl}:')
            self.emit(f'.str "{content}"')
            self.emit(f'.db 0')
            self.emit('')
        return '\n'.join(self.lines)

    def gen_statement(self, stmt):
        if isinstance(stmt, LetDecl):
            self.gen_let(stmt)
        elif isinstance(stmt, Assignment):
            self.gen_assignment(stmt)
        elif isinstance(stmt, IfStmt):
            self.gen_if(stmt)
        elif isinstance(stmt, WhileStmt):
            self.gen_while(stmt)
        elif isinstance(stmt, PrintStmt):
            self.gen_print(stmt)
        elif isinstance(stmt, InputStmt):
            self.gen_input(stmt)
        elif isinstance(stmt, Block):
            for s in stmt.statements:
                self.gen_statement(s)

    def gen_let(self, stmt):
        addr = self.declare_var(stmt.name)
        if stmt.init_expr is not None:
            self.gen_expr_into_acc(stmt.init_expr)
            self.emit(f'STORE ACC, {addr}')
        else:
            self.emit(f'LOAD ACC, 0')
            self.emit(f'STORE ACC, {addr}')

    def gen_assignment(self, stmt):
        addr = self.get_var_addr(stmt.name)
        self.gen_expr_into_acc(stmt.expr)
        self.emit(f'STORE ACC, {addr}')

    def gen_if(self, stmt):
        end_label = self.new_label('end')
        else_label = None
        if stmt.else_block is not None:
            else_label = self.new_label('else')

        if isinstance(stmt.condition, CondExpr):
            self.gen_cond_jump(stmt.condition, else_label or end_label)
        else:
            self.gen_expr_into_acc(stmt.condition)
            self.emit(f'JZ {else_label or end_label}')

        self.gen_block(stmt.then_block)
        if stmt.else_block is not None:
            self.emit(f'JMP {end_label}')
            self.emit(f'{else_label}:')
            self.gen_block(stmt.else_block)
        self.emit(f'{end_label}:')

    def gen_while(self, stmt):
        start_label = self.new_label('while')
        end_label = self.new_label('end')
        self.emit(f'{start_label}:')

        if isinstance(stmt.condition, CondExpr):
            self.gen_cond_jump(stmt.condition, end_label)
        else:
            self.gen_expr_into_acc(stmt.condition)
            self.emit(f'JZ {end_label}')

        self.gen_block(stmt.body)
        self.emit(f'JMP {start_label}')
        self.emit(f'{end_label}:')

    def gen_block(self, block):
        for s in block.statements:
            self.gen_statement(s)

    def gen_cond_jump(self, cond, target_label):
        """Generate CMP + conditional jump for a CondExpr.
        The jump goes to target_label when the condition is FALSE."""
        self.gen_expr_into_acc(cond.left)
        self.emit('MOV B, ACC')
        self.gen_expr_into_acc(cond.right)
        self.emit('CMP B, ACC')

        cmp_to_jump = {
            '>': 'JLE',
            '<': 'JGE',
            '==': 'JNE',
            '!=': 'JE',
            '>=': 'JL',
            '<=': 'JG',
        }
        jump = cmp_to_jump.get(cond.op)
        if jump is None:
            raise CodegenError(f"unknown comparison operator {cond.op!r}")
        self.emit(f'{jump} {target_label}')

    def gen_print(self, stmt):
        if isinstance(stmt.value, StringLit):
            lbl = self.add_string(stmt.value.value)
            self.emit(f'OUTSTR {lbl}')
        else:
            self.gen_expr_into_acc(stmt.value)
            self.emit(f'OUTNUM ACC')

    def gen_input(self, stmt):
        addr = self.get_var_addr(stmt.name)
        self.emit(f'IN ACC')
        self.emit(f'STORE ACC, {addr}')

    def gen_expr_into_acc(self, expr):
        """Generate code that evaluates expr and leaves result in ACC."""
        if isinstance(expr, NumberLit):
            self.emit(f'LOAD ACC, {expr.value}')

        elif isinstance(expr, VarRef):
            addr = self.get_var_addr(expr.name)
            self.emit(f'LD ACC, {addr}')

        elif isinstance(expr, BinOp):
            self.gen_binop(expr)

        elif isinstance(expr, UnaryOp):
            self.gen_unary(expr)

        elif isinstance(expr, CondExpr):
            self.gen_cond_expr_value(expr)

        else:
            raise CodegenError(f"unexpected expression type {type(expr).__name__}")

    def gen_binop(self, expr):
        op_map = {
            '+': 'ADD',
            '-': 'SUB',
            '*': 'MUL',
            '/': 'DIV',
            '%': 'MOD',
        }
        asm_op = op_map.get(expr.op)
        if asm_op is None:
            raise CodegenError(f"unsupported binary operator {expr.op!r}")

        if expr.op == '/':
            self.gen_div(expr)
            return

        self.gen_expr_into_acc(expr.left)
        self.emit('PUSH ACC')
        self.gen_expr_into_acc(expr.right)
        self.emit('MOV B, ACC')
        self.emit('POP ACC')
        self.emit(f'{asm_op} ACC, B')

    def gen_div(self, expr):
        self.gen_expr_into_acc(expr.left)
        self.emit('MOV B, ACC')
        self.gen_expr_into_acc(expr.right)
        self.emit('SWAP ACC, B')
        self.emit('DIV ACC, B')

    def gen_unary(self, expr):
        self.gen_expr_into_acc(expr.operand)
        if expr.op == '-':
            self.emit('NEG ACC')
        elif expr.op == '+':
            pass
        elif expr.op == '!':
            self.emit('CMP ACC, B')
            self.emit('MOV ACC, FL')

    def gen_cond_expr_value(self, expr):
        """Evaluate a comparison as a value (-1, 0, or 1) into ACC."""
        self.gen_expr_into_acc(expr.left)
        self.emit('MOV B, ACC')
        self.gen_expr_into_acc(expr.right)
        self.emit('CMP B, ACC')
        self.emit('MOV ACC, FL')


def compile_source(source, filename='<input>'):
    try:
        tokens = tokenize(source)
    except LexerError as e:
        return None, str(e)

    try:
        parser = Parser(tokens)
        prog = parser.parse()
    except ParseError as e:
        return None, str(e)

    try:
        cg = CodeGenerator()
        asm = cg.gen_program(prog)
        return asm, None
    except CodegenError as e:
        return None, str(e)


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description='Compile Tritium high-level language to assembly')
    parser.add_argument('input', help='Source file (.hl)')
    parser.add_argument('-o', '--output', help='Output file (default: stdout)')
    args = parser.parse_args()

    try:
        with open(args.input) as f:
            source = f.read()
    except FileNotFoundError:
        print(f"Error: file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    asm, error = compile_source(source, args.input)
    if error:
        print(f"Compilation error: {error}", file=sys.stderr)
        sys.exit(1)

    output = asm + '\n'
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output)
        print(f"Wrote {args.output}")
    else:
        sys.stdout.write(output)


if __name__ == '__main__':
    main()
