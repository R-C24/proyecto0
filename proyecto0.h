#ifndef PROYECTO0_H
#define PROYECTO0_H

#include <stdio.h>

#define N 63
#define tamBuffer 1024

typedef struct Info Info;

struct Info {
    char nombre[33];
    int enAsm;
};

typedef struct NodoTrie NodoTrie;

struct NodoTrie {
    char letra;
    NodoTrie* hijos[N];
    int terminal;
    Info* info;
};

extern NodoTrie* raiz; //ECHARLE UN OJO

NodoTrie* crearNodo(char letra);
NodoTrie* enter(char* palabra);
int lookUp(char* palabra);
Info* checkId(char* palabra);

int obtenerPos(char c);

void generarData(NodoTrie* nodo);


typedef enum {BeginSym, EndSym, ReadSym, WriteSym, Id, IntLiteral, LParen,
    RParen, SemiColon, Comma, AssignOp, PlusOp, MinusOp, EofSym} Token; //

Token tokenActual;
FILE *archivo;
FILE *archivoASM;

int inspect();
void advance();
int readChar();
int eof();

Token scanner();

char tokenBuffer[tamBuffer];
int posBuffer = 0;

void bufferChar(int c);
void clearBuffer();

Token checkReserved();

void match(Token token);

typedef struct OpRec OpRec;

struct OpRec {
    Token op;
};

typedef enum {IdExpr, LiteralExpr, TempExpr} ExprKind;

typedef struct ExprRec ExprRec;

struct ExprRec {
    ExprKind kind;
    char nombre[33];
    int valor;
};


void program();
void statementList();
void statement();
void idList();
void exprList();
ExprRec expression();
ExprRec primary();
OpRec addOp();
void systemGoal();

ExprRec genInfix(ExprRec e1, OpRec op, ExprRec e2);
void generateX86(ExprRec e1, OpRec opcode, ExprRec e2, ExprRec res);
void generate(char* opcode, char* arg1, char* arg2, char* res);
void assignX86(char* obj, ExprRec fuente);

char* extractOp(OpRec oprec);
char* extractExpr(ExprRec exprRec, char* buffer, size_t tam);


char* getTemp();
void resetTemp();

void start();
void finish();

ExprRec processId(char* lexema);
ExprRec processLiteral(char* lexema);
void readId(ExprRec inVar);
void writeExpr(ExprRec outExpr);

extern int maxTemp;
extern int hayError;

#endif //PROYECTO0_H
