#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "error.h"

Token *currentToken;
Token *lookAhead;

// -------------------------- Scanner / Token --------------------------
void scan(void) {
    Token* tmp = currentToken;
    currentToken = lookAhead;
    lookAhead = getValidToken();
    free(tmp);
}

void eat(TokenType tokenType) {
    if (lookAhead->tokenType == tokenType) {
        printToken(lookAhead);
        scan();
    } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

// -------------------------- Program / Block --------------------------
void compileProgram(void) {
    eat(KW_PROGRAM);
    eat(TK_IDENT);
    eat(SB_SEMICOLON);
    compileBlock();
    eat(SB_PERIOD);
}

void compileBlock(void) {
    if (lookAhead->tokenType == KW_CONST) {
        eat(KW_CONST);
        compileConstDecl();
        compileConstDecls();
    }
    if (lookAhead->tokenType == KW_TYPE) {
        eat(KW_TYPE);
        compileTypeDecl();
        compileTypeDecls();
    }
    if (lookAhead->tokenType == KW_VAR) {
        eat(KW_VAR);
        compileVarDecl();
        compileVarDecls();
    }
    compileSubDecls();
    eat(KW_BEGIN);
    compileStatements();
    eat(KW_END);
}

// -------------------------- Declarations --------------------------
void compileConstDecls(void) {
    while (lookAhead->tokenType == TK_IDENT) compileConstDecl();
}

void compileConstDecl(void) {
    eat(TK_IDENT);
    eat(SB_EQ);
    compileConstant();
    eat(SB_SEMICOLON);
}

void compileTypeDecls(void) {
    while (lookAhead->tokenType == TK_IDENT) compileTypeDecl();
}

void compileTypeDecl(void) {
    eat(TK_IDENT);
    eat(SB_EQ);
    compileType();
    eat(SB_SEMICOLON);
}

void compileVarDecls(void) {
    while (lookAhead->tokenType == TK_IDENT) compileVarDecl();
}

void compileVarDecl(void) {
    eat(TK_IDENT);
    while (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        eat(TK_IDENT);
    }
    eat(SB_COLON);
    compileType();
    eat(SB_SEMICOLON);
}

// -------------------------- Subroutines --------------------------
void compileSubDecls(void) {
    while (lookAhead->tokenType == KW_FUNCTION || lookAhead->tokenType == KW_PROCEDURE) {
        if (lookAhead->tokenType == KW_FUNCTION) compileFuncDecl();
        else compileProcDecl();
    }
}

void compileFuncDecl(void) {
    eat(KW_FUNCTION);
    eat(TK_IDENT);
    if (lookAhead->tokenType == SB_LPAR) {
        eat(SB_LPAR);
        compileParams();
        eat(SB_RPAR);
    }
    eat(SB_COLON);
    compileBasicType();
    eat(SB_SEMICOLON);
    compileBlock();
    eat(SB_SEMICOLON);
}

void compileProcDecl(void) {
    eat(KW_PROCEDURE);
    eat(TK_IDENT);
    if (lookAhead->tokenType == SB_LPAR) {
        eat(SB_LPAR);
        compileParams();
        eat(SB_RPAR);
    }
    eat(SB_SEMICOLON);
    compileBlock();
    eat(SB_SEMICOLON);
}

// -------------------------- Parameters (skeleton) --------------------------
void compileParams(void) {
    while (lookAhead->tokenType == TK_IDENT) {
        eat(TK_IDENT);
        while (lookAhead->tokenType == SB_COMMA) {
            eat(SB_COMMA);
            eat(TK_IDENT);
        }
        eat(SB_COLON);
        compileType();
        if (lookAhead->tokenType == SB_SEMICOLON) {
            eat(SB_SEMICOLON);
        } else {
            break;
        }
    }
}

// -------------------------- Statements --------------------------
void compileStatements(void) {
    if (lookAhead->tokenType == SB_SEMICOLON || lookAhead->tokenType == KW_END || lookAhead->tokenType == KW_ELSE)
        return;
    if (lookAhead->tokenType == TK_IDENT && strcmp(lookAhead->string, "until") == 0)
        return;
    compileStatement();
    while (lookAhead->tokenType == SB_SEMICOLON) {
        eat(SB_SEMICOLON);
        if (lookAhead->tokenType == KW_END || lookAhead->tokenType == KW_ELSE) break;
        if (lookAhead->tokenType == TK_IDENT && strcmp(lookAhead->string, "until") == 0) break;
        compileStatement();
    }
}

void compileStatement(void) {
    if (lookAhead->tokenType == TK_IDENT && strcmp(lookAhead->string, "repeat") == 0) {
        scan();
        compileStatements();
        if (lookAhead->tokenType == TK_IDENT && strcmp(lookAhead->string, "until") == 0)
            scan();
        else error(ERR_INVALIDSTATEMENT, lookAhead->lineNo, lookAhead->colNo);
        compileCondition();
        return;
    }

    switch (lookAhead->tokenType) {
        case TK_IDENT:
            compileAssignSt();
            break;
        case KW_CALL:
            compileCallSt();
            break;
        case KW_BEGIN:
            compileGroupSt();
            break;
        case KW_IF:
            compileIfSt();
            break;
        case KW_WHILE:
            compileWhileSt();
            break;
        case KW_FOR:
            compileForSt();
            break;
        default:
            error(ERR_INVALIDSTATEMENT, lookAhead->lineNo, lookAhead->colNo);
            break;
    }
}

// -------------------------- Assign / Call / Group --------------------------
void compileAssignSt(void) {
    eat(TK_IDENT);
    while (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        eat(TK_IDENT);
    }
    eat(SB_ASSIGN);
    compileExpression();
    while (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        compileExpression();
    }
}

void compileCallSt(void) {
    eat(KW_CALL);
    eat(TK_IDENT);
    if (lookAhead->tokenType == SB_LPAR) {
        eat(SB_LPAR);
        compileArguments();
        eat(SB_RPAR);
    }
}

void compileGroupSt(void) {
    eat(KW_BEGIN);
    compileStatements();
    eat(KW_END);
}

void compileIfSt(void) {
    eat(KW_IF);
    compileCondition();
    eat(KW_THEN);
    compileStatement();
    if (lookAhead->tokenType == KW_ELSE) compileElseSt();
}

void compileElseSt(void) {
    eat(KW_ELSE);
    compileStatement();
}

void compileWhileSt(void) {
    eat(KW_WHILE);
    compileCondition();
    eat(KW_DO);
    compileStatement();
}

void compileForSt(void) {
    eat(KW_FOR);
    eat(TK_IDENT);
    eat(SB_ASSIGN);
    compileExpression();
    if (lookAhead->tokenType == KW_TO) eat(KW_TO);
    compileExpression();
    eat(KW_DO);
    compileStatement();
}

// -------------------------- Expressions / Conditions --------------------------
void compileCondition(void) {
    compileExpression();
    if (lookAhead->tokenType == SB_EQ || lookAhead->tokenType == SB_NEQ ||
        lookAhead->tokenType == SB_LT || lookAhead->tokenType == SB_LE ||
        lookAhead->tokenType == SB_GT || lookAhead->tokenType == SB_GE) {
        eat(lookAhead->tokenType);
        compileExpression();
    }
}

void compileExpression(void) {
    if (lookAhead->tokenType == SB_PLUS || lookAhead->tokenType == SB_MINUS) scan();
    compileExpression2();
}

void compileExpression2(void) {
    compileTerm();
    while (lookAhead->tokenType == SB_PLUS || lookAhead->tokenType == SB_MINUS) {
        scan();
        compileTerm();
    }
}

void compileTerm(void) {
    compileFactor();
    while (lookAhead->tokenType == SB_TIMES || lookAhead->tokenType == SB_SLASH) {
        scan();
        compileFactor();
    }
}

void compileFactor(void) {
    switch (lookAhead->tokenType) {
        case TK_NUMBER: eat(TK_NUMBER); break;
        case TK_CHAR: eat(TK_CHAR); break;
        case TK_IDENT:
            eat(TK_IDENT);
            if (lookAhead->tokenType == SB_LPAR) {
                eat(SB_LPAR);
                compileArguments();
                eat(SB_RPAR);
            }
            break;
        case SB_LPAR:
            eat(SB_LPAR);
            compileExpression();
            eat(SB_RPAR);
            break;
        default:
            error(ERR_INVALIDFACTOR, lookAhead->lineNo, lookAhead->colNo);
            break;
    }
}

void compileArguments(void) {
    compileExpression();
    while (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        compileExpression();
    }
}

// -------------------------- Constants / Types --------------------------
void compileConstant(void) {
    if (lookAhead->tokenType == SB_MINUS) {
        eat(SB_MINUS);
        compileUnsignedConstant();
    } else compileUnsignedConstant();
}

void compileUnsignedConstant(void) {
    if (lookAhead->tokenType == TK_NUMBER) eat(TK_NUMBER);
    else if (lookAhead->tokenType == TK_IDENT) eat(TK_IDENT);
    else if (lookAhead->tokenType == TK_CHAR) eat(TK_CHAR);
    else error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo);
}

void compileType(void) {
    if (lookAhead->tokenType == KW_ARRAY) {
        eat(KW_ARRAY);
        eat(SB_LPAR);
        eat(TK_NUMBER);
        eat(SB_RPAR);
        eat(KW_OF);
        compileBasicType();
    } else compileBasicType();
}

void compileBasicType(void) {
    if (lookAhead->tokenType == KW_INTEGER) eat(KW_INTEGER);
    else if (lookAhead->tokenType == KW_CHAR) eat(KW_CHAR);
    else if (lookAhead->tokenType == TK_IDENT) eat(TK_IDENT);
    else missingToken(TK_IDENT, lookAhead->lineNo, lookAhead->colNo);
}

// -------------------------- Entry point --------------------------
int compile(char *fileName) {
    if (openInputStream(fileName) == IO_ERROR) return IO_ERROR;
    currentToken = NULL;
    lookAhead = getValidToken();
    compileProgram();
    free(currentToken);
    free(lookAhead);
    closeInputStream();
    return IO_SUCCESS;
}

