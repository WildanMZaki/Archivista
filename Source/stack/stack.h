#ifndef DYNAMIC_STACK_H
#define DYNAMIC_STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct tNode *address;
typedef struct tNode
{
    void *data;
    int dataSize;
    address next;
} Node;

/* Representasi stack dinamis dengan pointer */
typedef struct
{
    address bottom;
    address top;
} Stack;

/*********** PROTOTYPE ****************/

/* Predikat */
bool isStackEmpty(Stack S);

/* Konstruktor */
void CreateStack(Stack *S);

/* Manajemen memori - data akan di-copy sebesar dataSize bytes */
address Alokasi(void *data, int dataSize);
void DeAlokasi(address P);

/* Operasi utama stack - dataSize dalam bytes */
void Push(Stack *S, void *data, int dataSize);
bool Pop(Stack *S, void **out, int *outSize);

/* Operasi berbasis address */
void InsertLast(Stack *S, address P);
void DelLast(Stack *S, address *P);

/* Menampilkan isi stack dari Top ke Bottom (debug) */
void PrintStack(Stack S);

/* Menghapus seluruh isi stack */
void DeleteAll(Stack *S);

#endif