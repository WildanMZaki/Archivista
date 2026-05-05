#include "stack.h"

/* Mengembalikan true jika stack kosong */
bool isStackEmpty(Stack S)
{
    return (S.bottom == NULL);
}

/* Membentuk stack kosong */
void CreateStack(Stack *S)
{
    S->bottom = NULL;
    S->top = NULL;
}

/* Alokasi node baru dengan copy data */
address Alokasi(void *data, int dataSize)
{
    address P = (address)malloc(sizeof(Node));

    if (P != NULL)
    {
        P->data = malloc(dataSize);
        if (P->data != NULL)
        {
            memcpy(P->data, data, dataSize);
            P->dataSize = dataSize;
            P->next = NULL;
        }
        else
        {
            free(P);
            P = NULL;
        }
    }

    return P;
}

/* Dealokasi node dan data-nya */
void DeAlokasi(address P)
{
    if (P != NULL)
    {
        if (P->data != NULL)
        {
            free(P->data);
        }
        free(P);
    }
}

/* Push nilai (copy data) ke top stack */
void Push(Stack *S, void *data, int dataSize)
{
    address P = Alokasi(data, dataSize);

    if (P != NULL)
    {
        InsertLast(S, P);
    }
}

/* Pop elemen paling atas stack, return data ke *out */
bool Pop(Stack *S, void **out, int *outSize)
{
    address P;

    if (isStackEmpty(*S))
    {
        return false;
    }

    DelLast(S, &P);
    if (P != NULL)
    {
        *out = P->data;
        *outSize = P->dataSize;
        free(P);
        return true;
    }

    return false;
}

/* Menambahkan node di posisi paling atas */
void InsertLast(Stack *S, address P)
{
    if (S->bottom == NULL)
    {
        S->bottom = P;
        S->top = P;
    }
    else
    {
        S->top->next = P;
        S->top = P;
    }

    P->next = NULL;
}

/* Menghapus node paling atas */
void DelLast(Stack *S, address *P)
{
    address current, previous;

    if (S->bottom == NULL)
    {
        *P = NULL;
    }
    else if (S->bottom->next == NULL)
    {
        *P = S->bottom;
        S->bottom = NULL;
        S->top = NULL;
    }
    else
    {
        previous = S->bottom;
        current = previous->next;

        while (current->next != NULL)
        {
            previous = current;
            current = current->next;
        }

        *P = current;
        S->top = previous;
        previous->next = NULL;
    }

    if (*P != NULL)
    {
        (*P)->next = NULL;
    }
}

/* Menampilkan isi stack dari Top ke Bottom (debug) */
void PrintStack(Stack S)
{
    address nodes[100];
    address current;
    int count = 0;
    int i;

    printf("\n----- Isi Stack -----\n");
    printf("Bottom Address = %p\n", (void *)S.bottom);
    printf("Top Address    = %p\n\n", (void *)S.top);

    if (S.bottom == NULL)
    {
        printf("Stack kosong.\n");
    }
    else
    {
        current = S.bottom;

        while (current != NULL && count < 100)
        {
            nodes[count] = current;
            count++;
            current = current->next;
        }

        printf("Address         ||   Size  ||  Data\n");
        printf("----------------||---------||---------\n");

        for (i = count - 1; i >= 0; i--)
        {
            current = nodes[i];

            printf("%14p ||  %5d  ||  %p", (void *)current, current->dataSize, current->data);

            if (current == S.bottom && current == S.top)
            {
                printf("   <- Bottom & Top");
            }
            else if (current == S.top)
            {
                printf("   <- Top");
            }
            else if (current == S.bottom)
            {
                printf("   <- Bottom");
            }

            printf("\n");
        }

        printf("----------------||---------||---------\n");
    }

    printf("========================\n\n");
}

/* Menghapus seluruh isi stack */
void DeleteAll(Stack *S)
{
    address P;

    while (!isStackEmpty(*S))
    {
        DelLast(S, &P);
        DeAlokasi(P);
    }
}
