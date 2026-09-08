#include "proyecto0.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int maxTemp = 0;
int hayError = 0;
NodoTrie* raiz = NULL;


int main(int argc, char *argv[]){

// Se recibe los argumentos, se maneja la apertura de archivos y se llama al parser.
    if (argc < 2) {
        fprintf(stderr, "Error: Se requiere un archivo fuente .micro.\n");
        fprintf(stderr, "Uso: %s <ruta_al_archivo.micro>\n", argv[0]);
        return EXIT_FAILURE;
    }

    raiz = crearNodo('\0');
    if (raiz == NULL) {
        fprintf(stderr, "Error fatal: No se pudo asignar memoria para el Trie.\n");
        return EXIT_FAILURE;
    }

    const char* direccionMicro = argv[1];
    char direccionASM[512];
    char direccionExec[512];

    snprintf(direccionASM, sizeof(direccionASM), "%s.s", direccionMicro);
    snprintf(direccionExec, sizeof(direccionExec), "%s.out", direccionMicro);

    archivo = fopen(direccionMicro, "r");
    if (!archivo) {
        fprintf(stderr, "Error: No se pudo abrir el archivo fuente '%s'.\n", direccionMicro);
        return EXIT_FAILURE;
    }

    archivoASM = fopen(direccionASM, "w");
    if (!archivoASM) {
        fprintf(stderr, "Error: No se pudo crear el archivo de salida '%s'.\n", direccionASM);
        fclose(archivo);
        return EXIT_FAILURE;
    }

    tokenActual = scanner();

    systemGoal();

    fclose(archivo);
    fclose(archivoASM);

    // Si hubo algún error, se aborta la escritura del archivo ensamblador.

    if (hayError) {
        fprintf(stderr, "Se ha detenido la compilación debido a errores en el programa fuente.\n");
        remove(direccionASM);
        return EXIT_FAILURE;
    }

    // Se define el comando de ejecución.
    char command[2048];
    snprintf(command, sizeof(command), "gcc -no-pie \"%s\" -o \"%s\"", direccionASM, direccionExec);

    int estadoCompilacion = system(command);
    if (estadoCompilacion != 0) {
        fprintf(stderr, "Error durante la compilación del archivo .s con GCC.\n");
        return EXIT_FAILURE;
    }
    
    snprintf(command, sizeof(command), "./\"%s\"", direccionExec);

    int exec_status = system(command);

    return exec_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

//----------- Tries -----------

// Obtiene el índice de un caracter en el trie.
int obtenerPos(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }

    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 26;
    }
    if (c == '_') {
        return 52;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 53;
    }
    return -1;
}

// Se encarga de la creación de nodos.
NodoTrie* crearNodo(char letra) {
    NodoTrie* nodo = (NodoTrie*) malloc (sizeof(NodoTrie)); 

    if (!nodo) {
        fprintf(stderr, "Error: Memoria insuficiente.\n");
        hayError = 1;

        if (archivo){
            fclose(archivo);
        }
        if (archivoASM){
            fclose(archivoASM);
        }
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N; i++) {
        nodo -> hijos[i] = NULL;
    }
    nodo -> terminal = 0;
    nodo -> letra = letra;
    nodo -> info = NULL;
    return nodo;
}

// Maneja la inserción de un identificador en el trie.
NodoTrie* enter(char* palabra) {
    NodoTrie* temp = raiz;

    for (int i = 0; palabra[i] != '\0' ; i++) {
        int pos = obtenerPos(palabra[i]);
        if (pos == -1) {
            continue;
        }

        if (temp -> hijos[pos] == NULL) {
            temp -> hijos[pos] = crearNodo(palabra[i]);
        }
        temp = temp -> hijos[pos];
    }

    temp -> terminal = 1;

    if (temp -> info == NULL) {
        temp -> info = (Info*) malloc (sizeof(Info));
        strcpy(temp -> info -> nombre, palabra);
        temp -> info -> enAsm = 0;
    }

    return raiz;
}

// Maneja la búsqueda de un identificador en el trie.
int lookUp(char* palabra) {
    NodoTrie* temp = raiz;

    for (int i = 0; palabra[i] != '\0' ; i++) {
        int pos = obtenerPos(palabra[i]);
        if (pos == -1 || temp -> hijos[pos] == NULL) {
            return 0;
        }
        temp = temp -> hijos[pos];
    }
    if (temp != NULL && temp -> terminal == 1) {
        return 1;
    }
    return 0;
}

//-----------------------------


// Se encarga de revisar que el identificador no supere los 32 caracteres (y, en caso de que no cumpla con esto, lo recorta).
// Si el identificador no existe dentro del trie, lo manda a insertar. Si existe, lo manda a buscar.
Info* checkId(char* palabra) {
    if (strlen(palabra) > 32) {
        fprintf(stderr, "Aviso: El identificador '%s' supera los 32 caracteres y será truncado.\n", palabra);
    }
    char palabra32[34];
    strncpy(palabra32, palabra, 32);
    palabra32[32] = '\0';

    if (!lookUp(palabra32)) {
        enter(palabra32);
    }

    NodoTrie* temp =  raiz;
    for (int i = 0; palabra32[i] != '\0' ; i++) {
        int pos = obtenerPos(palabra32[i]);
        if (pos == -1 || temp -> hijos[pos] == NULL) {
            return NULL;
        }
        temp = temp -> hijos[pos];
    }

    Info* infoVar =  temp -> info;

    if (infoVar->enAsm == 0) {
        infoVar->enAsm = 1;
    }

    return infoVar;
}

// Nos permite ver el siguiente caracter.
int inspect() {
    int c = fgetc(archivo);
    if (c != EOF) {
        ungetc(c, archivo);
    }
    return c;
}

// Se mueve hacia el siguiente caracter.
void advance() {
    if (!feof(archivo)) {
        fgetc(archivo);
    }
}

// Lee el siguiente caracter.
int readChar() {
    return fgetc(archivo);
}

// Revisa si el siguiente es End Of File.
int eof(){
    return (inspect() == EOF);
}

// Guarda en el buffer.
void bufferChar(int c) {
    if (c == EOF) {
        return;
    }

    if (posBuffer < (tamBuffer - 1)) {
        tokenBuffer[posBuffer] = (char) c;
        posBuffer++;
        tokenBuffer[posBuffer] = '\0';
    } else {
        fprintf(stderr, "El buffer se ha desbordado.\n");
        hayError = 1;

        if (archivo){
            fclose(archivo);
        }
        if (archivoASM){
            fclose(archivoASM);
        }
        exit(EXIT_FAILURE);
    }
}

//Limpia el buffer.
void clearBuffer() {
    posBuffer = 0;
    tokenBuffer[0] = '\0';
}

// Se encarga de revisar si una palabra es reservada y devuelve el Token correspondiente.
Token checkReserved() {

    if (strcmp(tokenBuffer, "begin") == 0) {
        return BeginSym;
    }
    if (strcmp(tokenBuffer, "end") == 0) {
        return EndSym;
    }
    if (strcmp(tokenBuffer, "read") == 0) {
        return ReadSym;
    }
    if (strcmp(tokenBuffer, "write") == 0) {
        return WriteSym;
    }
    return Id;
}

// Maneja la lógica de cada caracter.
Token scanner() {
    clearBuffer();
    while (!eof()) {
        int currentChar = readChar();
            if (isspace(currentChar)) {
                continue;
            }
            if (isalpha(currentChar)) {
                bufferChar(currentChar);
                while (isalnum(inspect()) || inspect() == '_') {
                    bufferChar(readChar());
                }
                return checkReserved();
            }
            if (isdigit(currentChar)) {
                bufferChar(currentChar);
                while (isdigit(inspect())) {
                    bufferChar(readChar());
                }
                return IntLiteral;
            }
            switch (currentChar) {
                case '(':
                    return LParen;
                case ')':
                    return RParen;
                case ';':
                    return SemiColon;
                case ',':
                    return Comma;
                case '+':
                    return PlusOp;
                case ':':
                    if (inspect() == '=') {
                        advance();
                        return AssignOp;
                    } else {
                        fprintf(stderr, "Error léxico: Carácter no reconocido '%c'\n", currentChar);
                        hayError = 1;
                        continue;
                    }
                case '-':
                    /*if (inspect() == '-') {
                        advance();
                        return MinusOp;
                    } else { */
                        int c;
                        while ((c = readChar()) != '\n' && c != EOF) {}
                        continue;
                    //}
                default:
                    fprintf(stderr, "Error léxico: Carácter no reconocido '%c'\n", currentChar);
                    hayError = 1;
                    continue;
            }
        }
        return EofSym;
}

// Se asegura que el token recibido calce con el esperado.
void match(Token token) {
    if (tokenActual == token) {
        tokenActual = scanner();
    } else {
        fprintf(stderr, "Error sintáctico: Se esperaba el token %d, pero se encontró %d en el buffer ('%s').\n",
                token, tokenActual, tokenBuffer);
        hayError = 1;

        if (archivo){
            fclose(archivo);
        }
        if (archivoASM){
            fclose(archivoASM);
        }
        exit(EXIT_FAILURE);
    }
}


// Las siguientes funciones se encargan de gestionar las reglas de producción y la información semántica de la gramática definida para Micro.

void systemGoal() {
    program();
    match(EofSym);
}

void program() {
    start();
    match(BeginSym);
    statementList();
    match(EndSym);
    finish();
}

void statementList() {
    statement();
    while(tokenActual == Id || tokenActual == ReadSym || tokenActual == WriteSym) {
        statement();
    }
}

void statement() {
    switch (tokenActual) {
        case Id:
            ExprRec target = processId(tokenBuffer);
            match(Id);
            match(AssignOp);
            ExprRec fuente = expression();
            match(SemiColon);
            assignX86(target.nombre, fuente);
            break;

        case ReadSym:
            match(ReadSym);
            match(LParen);
            idList();
            match(RParen);
            match(SemiColon);
            break;

        case WriteSym:
            match(WriteSym);
            match(LParen);
            exprList();
            match(RParen);
            match(SemiColon);
            break;
        default:
            fprintf(stderr, "Error sintáctico: El token no '%d' no corresponde a la gramática.", tokenActual);
            exit(EXIT_FAILURE);
    }
}

void idList() {
    ExprRec target = processId(tokenBuffer);
    match(Id);
    readId(target);

    while (tokenActual == Comma) {
        match(Comma);
        ExprRec nextTarget = processId(tokenBuffer);
        match(Id);
        readId(nextTarget);
    }
}

void exprList() {
    ExprRec val = expression();
    writeExpr(val);

    while (tokenActual == Comma) {
        match(Comma);
        ExprRec nextVal = expression();
        writeExpr(nextVal);
    }
}

ExprRec expression() {
    ExprRec leftOp = primary();
    while (tokenActual == PlusOp) { // while (tokenActual == PlusOp || tokenActual == MinusOp)
        OpRec op = addOp();

        ExprRec rightOp = primary();
        leftOp = genInfix(leftOp, op, rightOp);
    }
    return leftOp;
}

ExprRec primary() {
    ExprRec res;

    switch (tokenActual) {
        /*case MinusOp:
            match(MinusOp);
            ExprRec zeroRec = processLiteral("0");
            ExprRec rightRec = primary();
            OpRec subOp;
            subOp.op = MinusOp;
            res = genInfix(zeroRec, subOp, rightRec);
            break;*/
        case LParen:
            match(LParen);
            res = expression();
            match(RParen);
            break;
        case Id:
            res = processId(tokenBuffer);
            match(Id);
            break;
        case IntLiteral:
            res = processLiteral(tokenBuffer);
            match(IntLiteral);
            break;
        default:
            fprintf(stderr, "Error sintáctico: El token no '%d' no corresponde a la gramática.", tokenActual);
            exit(EXIT_FAILURE);
    }
    return res;
}
OpRec addOp() {
    OpRec o;
    o.op = tokenActual;

    switch (tokenActual) {
        case PlusOp:
            match(PlusOp);
            break;
        /*case MinusOp:
            match(MinusOp);
            break;*/
        default:
            fprintf(stderr, "Error sintáctico: El token no '%d' no corresponde a la gramática.", tokenActual);
            exit(EXIT_FAILURE);
    }
    return o;
}


// Encargado del manejo de temporales.
char* getTemp() {
    maxTemp++;
    static char tempName[34];
    snprintf(tempName, sizeof(tempName), "Temp_%d", maxTemp);
    checkId(tempName);
    return tempName;
}

// Resetea temporales.
void resetTemp() {
    maxTemp = 0;
}

// Traduce de token a texto en el caso de operaciones.
char* extractOp(OpRec oprec) {
    switch (oprec.op) {
        case PlusOp:
            return  "addl";
        /*case MinusOp:
            return  "subl";*/
        default:
            return "";
    }
}

// Traduce de token a texto en el caso de expresiones.
char* extractExpr(ExprRec exprRec, char* buffer, size_t tam) {
    switch (exprRec.kind) {
        case IdExpr:
            snprintf(buffer, tam, "%s", exprRec.nombre);
            break;
        case TempExpr:
            snprintf(buffer, tam, "%s", exprRec.nombre);
            break;
        case LiteralExpr:
            snprintf(buffer, tam, "$%d", exprRec.valor);
            break;
        default:
            buffer[0] = '\0';
            break;
    }
    return buffer;
}

// Escribe instrucciones en x86 dependiendo de la cantidad de argumentos.
void generate(char* opcode, char* arg1, char* arg2, char* res) {
    if (arg1 && strlen(arg1) > 0 && arg2 && strlen(arg2) > 0 && res && strlen(res) > 0) {
        fprintf(archivoASM, "    %s %s, %s, %s\n", opcode, arg1, arg2, res);
    }
    else if (arg1 && strlen(arg1) > 0 && arg2 && strlen(arg2) > 0) {
        fprintf(archivoASM, "    %s %s, %s\n", opcode, arg1, arg2);
    }
    else if (arg1 && strlen(arg1) > 0) {
        fprintf(archivoASM, "    %s %s\n", opcode, arg1);
    }
    else {
        fprintf(archivoASM, "    %s\n", opcode);
    }
}

// Función auxiliar para genInfix.
void generateX86(ExprRec e1, OpRec op, ExprRec e2, ExprRec res) {
    char arg1[34];
    char arg2[34];
    char resName[34];

    extractExpr(e1, arg1, sizeof(arg1));
    extractExpr(e2, arg2, sizeof(arg2));
    extractExpr(res, resName, sizeof(resName));

    char* opCode = extractOp(op);

    generate("movl", arg1, "%eax", NULL);
    generate(opCode, arg2, "%eax", NULL);
    generate("movl", "%eax", resName, NULL);
}

// Escribe asignaciones en x86.
void assignX86(char* obj, ExprRec fuente) {
    char temp[34];
    extractExpr(fuente, temp, sizeof(temp));

    generate("movl", temp, "%eax", NULL);
    generate("movl", "%eax", obj, NULL);
}

// Gestiona la generación de instrucciones en x86.
ExprRec genInfix(ExprRec e1, OpRec op, ExprRec e2) {
    ExprRec res;
    res.kind = TempExpr;

    char* temp = getTemp();
    strncpy(res.nombre, temp, sizeof(res.nombre) - 1);
    res.nombre[sizeof(res.nombre) - 1] = '\0';

    generateX86(e1, op, e2, res);
    return res;
}

// Escribe instrucciones iniciales para un programa x86. (Prólogo)
void start() {
    resetTemp();

    fprintf(archivoASM, ".section .data\n");
    fprintf(archivoASM, "    fmt_in:  .string \"%%d\"\n");
    fprintf(archivoASM, "    fmt_out: .string \"%%d\\n\"\n");

    fprintf(archivoASM, "\n.section .text\n");
    fprintf(archivoASM, ".globl main\n");
    fprintf(archivoASM, "main:\n");

    generate("pushq", "%rbp", NULL, NULL);
    generate("movq", "%rsp", "%rbp", NULL);
}

// Escribe instrucciones finales para un programa x86. (Epílogo)
void finish() {
    generate("movl", "$0", "%eax", NULL);
    generate("movq", "%rbp", "%rsp", NULL);
    generate("popq", "%rbp", NULL, NULL);
    generate("ret", "", "", "");

    fprintf(archivoASM, "\n.section .data\n");
    generarData(raiz);

    fprintf(archivoASM, "\n.section .note.GNU-stack,\"\",@progbits\n");
}

// Guardamos en un exprRec el identificador.
ExprRec processId(char* lexema) {
    ExprRec e;
    e.kind = IdExpr;

    Info* info = checkId(lexema);

    strncpy(e.nombre, info->nombre, sizeof(e.nombre) - 1);
    e.nombre[sizeof(e.nombre) - 1] = '\0';
    e.valor = 0;

    return e;
}

// Guardamos en un exprRec el valor.
ExprRec processLiteral(char* lexema) {
    ExprRec e;
    e.kind = LiteralExpr;
    e.nombre[0] = '\0';
    e.valor = atoi(lexema);
    return e;
}

// Escribe comandos de lectura en ensamblador.
void readId(ExprRec inVar) {
    char varStr[34];
    extractExpr(inVar, varStr, sizeof(varStr));

    generate("leaq", "fmt_in(%rip)", "%rdi", NULL);
    generate("leaq", varStr, "%rsi", NULL);
    generate("movl", "$0", "%eax", NULL);
    generate("call", "scanf", NULL, NULL);
}

// Escribe comandos de escritura en ensamblador.
void writeExpr(ExprRec outExpr) {
    char valStr[34];
    extractExpr(outExpr, valStr, sizeof(valStr));

    generate("leaq", "fmt_out(%rip)", "%rdi", NULL);
    if (outExpr.kind == LiteralExpr) {
        generate("movl", valStr, "%esi", NULL);
    } else {
        generate("movl", valStr, "%eax", NULL);
        generate("movl", "%eax", "%esi", NULL);
    }
    generate("movl", "$0", "%eax", NULL);
    generate("call", "printf", NULL, NULL);

    generate("movl", "$0", "%edi", NULL);
    generate("call", "fflush", NULL, NULL);
}

// Recorre el trie para escribir en la sección .data los identificadores agrupados.
void generarData(NodoTrie* nodo) {
    if (nodo->terminal && nodo->info != NULL) {
        fprintf(archivoASM, "    %s: .long 0\n", nodo->info->nombre);
    }

    for (int i = 0; i < N; i++) {
        if (nodo->hijos[i] != NULL) {
            generarData(nodo->hijos[i]);
        }
    }
}