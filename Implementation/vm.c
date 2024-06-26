#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 128
#define PAGE_SIZE 256
#define TLB_SIZE 16

typedef struct 
{
    int num_pagina;
    int num_frame;
    int ultimo_acesso;
    int tempo;
    int ocupado;
} PageTable;

typedef struct
{
    char dados[PAGE_SIZE];
} Frame;

typedef struct
{
    int num_pagina;
    int num_frame;
    int pageTableIndex;
} TLBEntry;

Frame memoria[FRAME_TAMANHO];
PageTable page_table[FRAME_TAMANHO];
TLBEntry tlb[TLB_SIZE];

int page_faults = 0;
int tlb_hit = 0;
int tlb_index = 0;

// Leitura dos Addresses
int* ler_enderecos(char* caminho, int* tamanho);
char** int_para_binario(int* enderecos, int tamanho);
char** extrair_offset(char** enderecos_binarios, int tamanho);
char** extrair_pagina(char** enderecos_binarios, int tamanho);

// Memoria
void iniciar_page_table();

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer);
void acessar_memoria(FILE *backing_store, int num_pagina, int offset,FILE * arquivo, char * algoritmo);

// Algoritmo Queue
int remover_fifo();
void atualizar_tempo();
int remover_lru();

// Print
void imprimir(int endereco_virtual, int endereco_fisico, int valor, int index_tlb, FILE * arquivo);
void imprimir_resultados(int tamanho, FILE * arquivo);




int main(int argc, char *argv[])
{
    int tamanho;
    int* enderecos = ler_enderecos(argv[1], &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    iniciar_page_table();
    FILE *backing_store = fopen("BACKING_STORE.bin", "rb");
    FILE * saida = fopen("correct.txt","w");
    if (enderecos)
    {
        char** offset = extrair_offset(enderecos_binarios, tamanho);
        char** pagina = extrair_pagina(enderecos_binarios, tamanho);

        for (int i = 0; i < tamanho; i++)
        {
            int num_pagina = (int)strtol(pagina[i], NULL, 2);
            int offset_val = (int)strtol(offset[i], NULL, 2);
            acessar_memoria(backing_store, num_pagina, offset_val,saida,argv[2]);
        }

        

        for (int i = 0; i < tamanho; i++)
        {
            free(enderecos_binarios[i]);
            free(offset[i]);
            free(pagina[i]);
        }

        free(enderecos);
        free(enderecos_binarios);
        free(offset);
        free(pagina);
    }

    imprimir_resultados(tamanho,saida);

    fclose(backing_store);
    fclose(saida);
    return 0;
}


void acessar_memoria(FILE *backing_store, int num_pagina, int offset, FILE * arquivo, char * algoritmo)
{
    int frame_encontrado = -1;
    int valor = 0;
    int offset_index = offset -1;

    

    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i].num_pagina == num_pagina )
        {
            atualizar_tempo();
            frame_encontrado = tlb[i].num_frame;
            page_table[tlb[i].pageTableIndex].ultimo_acesso++;
            valor = memoria[frame_encontrado].dados[offset_index];
            imprimir((num_pagina * PAGE_SIZE) + offset, (page_table[tlb[i].pageTableIndex].num_frame * 256) + offset, valor,i,arquivo);
            tlb_hit++;
            return;
        }
    }

    for (int i = 0; i < FRAME_TAMANHO; i++)
    {
        if (page_table[i].num_pagina == num_pagina && page_table[i].ocupado == 1)
        {
            atualizar_tempo();
            frame_encontrado = page_table[i].num_frame;
            valor = memoria[frame_encontrado].dados[offset_index];
            page_table[i].ultimo_acesso++;
            tlb[tlb_index].num_pagina = num_pagina;
            tlb[tlb_index].num_frame = frame_encontrado;
            tlb[tlb_index].pageTableIndex = i;
            imprimir((num_pagina * PAGE_SIZE) + offset,  (page_table[i].num_frame * 256) + offset, valor,tlb_index,arquivo);
            tlb_index = (tlb_index + 1) % 16;
            return;
        }
    }

    if (frame_encontrado == -1)
    {
        page_faults++;
        for (int i = 0; i < FRAME_TAMANHO; i++)
        {
            if (page_table[i].ocupado == 0)
            {
                page_table[i].ocupado = 1;
                page_table[i].num_pagina = num_pagina;
                page_table[i].ultimo_acesso++;
                atualizar_tempo();
                ler_backing_store(backing_store, num_pagina, memoria[page_table[i].num_frame].dados);
                valor = memoria[page_table[i].num_frame].dados[offset_index];
                tlb[tlb_index].num_pagina = num_pagina;
                tlb[tlb_index].num_frame = page_table[i].num_frame;
                tlb[tlb_index].pageTableIndex = i;
                imprimir((num_pagina * PAGE_SIZE) + offset, (page_table[i].num_frame * 256) + offset, valor,tlb_index,arquivo);
                tlb_index = (tlb_index + 1) % 16;
                return;
            }
        }
        
        int index;
        if(strcmp(algoritmo,"fifo") == 0)
        {
            index = remover_fifo();
        }
        else if (strcmp(algoritmo,"lru") == 0)
        {
            index = remover_lru();
        }
        
        page_table[index].ocupado = 1;
        page_table[index].num_pagina = num_pagina;
        page_table[index].ultimo_acesso++;
        page_table[index].tempo = 0;
        atualizar_tempo();
        ler_backing_store(backing_store, num_pagina, memoria[page_table[index].num_frame].dados);
        valor = memoria[page_table[index].num_frame].dados[offset_index];
        page_table[index].ultimo_acesso++;
        tlb[tlb_index].num_pagina = page_table[index].num_pagina;
        tlb[tlb_index].num_frame = page_table[index].num_frame;
        tlb[tlb_index].pageTableIndex = index;
        imprimir((num_pagina * PAGE_SIZE) + offset, (page_table[index].num_frame * 256) + offset, valor,tlb_index,arquivo);
        tlb_index = (tlb_index + 1) % 16;
        return;
    }

}



void iniciar_page_table()
{
    for (int i = 0; i < FRAME_TAMANHO; i++)
    {
        page_table[i].tempo = 0;
        page_table[i].ultimo_acesso = 0;
        page_table[i].num_pagina = -1;
        page_table[i].num_frame = i;
    }
}




void atualizar_tempo()
{
    for (int i = 0; i < FRAME_TAMANHO; i++)
    {
      if(page_table[i].ocupado == 1)
      {
        page_table[i].tempo++;
      }
    }
}


// FIFO Queue
int remover_fifo()
{
    int page_substituir = 0;
    int maior_tempo = page_table[0].tempo;

    for (int i = 1; i < FRAME_TAMANHO; i++)
    {
        if (page_table[i].tempo > maior_tempo)
        {
            maior_tempo = page_table[i].tempo;
            page_substituir = i;
        }
    }

    return page_substituir;
}

// FIFO Queue
int remover_lru()
{
    int page_substituir = 0;
    int mais_antigo = page_table[0].ultimo_acesso;

    for (int i = 1; i < FRAME_TAMANHO; i++)
    {
        if (page_table[i].ultimo_acesso < mais_antigo)
        {
            mais_antigo = page_table[i].ultimo_acesso;
            page_substituir = i;
        }
    }

    return page_substituir;
}


// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer)
{
    int offset = num_pagina * PAGE_SIZE;
    fseek(backing_store, offset + 1, SEEK_SET);
    long size = fread(buffer, sizeof(char), PAGE_SIZE, backing_store);
}



// Leitura dos Addresses
int* ler_enderecos(char* caminho, int* tamanho)
{
    FILE *file = fopen(caminho, "r");
    if (file == NULL)
    {
        return NULL;
    }

    int* enderecos = NULL;
    int cont = 0;
    int capacidade = 10;

    enderecos = (int*)malloc(capacidade * sizeof(int));

    while (fscanf(file, "%d", &enderecos[cont]) == 1)
    {
        cont++;
        if (cont >= capacidade)
        {
            capacidade *= 2;
            enderecos = (int*)realloc(enderecos, capacidade * sizeof(int));
        }
    }

    fclose(file);

    *tamanho = cont;
    return enderecos;
}


char** int_para_binario(int* enderecos, int tamanho)
{
    char** enderecos_binarios = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++)
    {
        int numero = enderecos[i];
        enderecos_binarios[i] = (char*)malloc(17 * sizeof(char));
        for (int j = 15; j >= 0; j--)
        {
            enderecos_binarios[i][j] = (numero & 1) + '0';
            numero >>= 1;
        }
        enderecos_binarios[i][16] = '\0';
    }

    return enderecos_binarios;
}


char** extrair_offset(char** enderecos_binarios, int tamanho)
{
    char** offset = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++)
    {
        offset[i] = (char*)malloc(9 * sizeof(char));
        strncpy(offset[i], enderecos_binarios[i] + 8, 8);
        offset[i][8] = '\0';
    }

    return offset;
}


char** extrair_pagina(char** enderecos_binarios, int tamanho)
{
    char** pagina = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++)
    {
        pagina[i] = (char*)malloc(9 * sizeof(char));
        strncpy(pagina[i], enderecos_binarios[i], 8);
        pagina[i][8] = '\0';
    }

    return pagina;
}

void imprimir(int endereco_virtual, int endereco_fisico, int valor, int index_tlb,FILE * arquivo)
{
    fprintf(arquivo,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", endereco_virtual, index_tlb, endereco_fisico, valor);
}


void imprimir_resultados(int tamanho, FILE * arquivo)
{
    fprintf(arquivo,"Number of Translated Addresses = %d\n", tamanho);
    fprintf(arquivo,"Page Faults = %d\n", page_faults);
    fprintf(arquivo,"Page Fault Rate = %.3f\n", (float)page_faults / tamanho);
    fprintf(arquivo,"TLB Hits = %d\n", tlb_hit);
    fprintf(arquivo,"TLB Hit Rate = %.3f\n", (float)tlb_hit / tamanho);
}