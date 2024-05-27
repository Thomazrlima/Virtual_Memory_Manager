#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 128
#define PAGE_SIZE 256
#define TLB_SIZE 16

typedef struct {
    int num_pagina;
} PageTable;

typedef struct {
    int num_frame;
    int num_pagina;
    int ocupado;
    int ultimo_acesso;
    int tempo;
    char dados[PAGE_SIZE];
} Frame;

typedef struct {
    int num_pagina;
    int num_frame;
    int valido;
} TLBEntry;

Frame memoria[FRAME_TAMANHO];
PageTable page_table[FRAME_TAMANHO];
TLBEntry tlb[TLB_SIZE];

int page_faults = 0;
int tlb_hit = 0;
int tlb_index = 0;

// Leitura dos Addresses
int* ler_enderecos(const char* caminho, int* tamanho);
char** int_para_binario(int* enderecos, int tamanho);
char** extrair_offset(char** enderecos_binarios, int tamanho);
char** extrair_pagina(char** enderecos_binarios, int tamanho);

// Memoria
void iniciar_memoria();
void iniciar_page_table();
void atualizar_frame(int num_frame, int num_pagina);
void atualizar_page_table(int num_pagina, int num_frame);

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer);
void acessar_memoria(FILE *backing_store, int num_pagina, int offset);

// FIFO Queue
int remover_fifo(Frame *memoria);

// Print
void imprimir(int endereco_virtual, int frame, int valor);
void imprimir_resultados(int tamanho);

// TLB
void atualizar_tlb(int num_pagina, int num_frame);

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    iniciar_memoria();
    iniciar_page_table();

    if (enderecos) {
        char** offset = extrair_offset(enderecos_binarios, tamanho);
        char** pagina = extrair_pagina(enderecos_binarios, tamanho);

        FILE *backing_store = fopen("BACKING_STORE.bin", "rb");

        for (int i = 0; i < tamanho; i++) {
            int num_pagina = (int)strtol(pagina[i], NULL, 2);
            int offset_val = (int)strtol(offset[i], NULL, 2);
            acessar_memoria(backing_store, num_pagina, offset_val);
        }

        fclose(backing_store);

        for (int i = 0; i < tamanho; i++) {
            free(enderecos_binarios[i]);
            free(offset[i]);
            free(pagina[i]);
        }

        free(enderecos);
        free(enderecos_binarios);
        free(offset);
        free(pagina);
    }

    imprimir_resultados(tamanho);

    return 0;
}

// Leitura dos Addresses
int* ler_enderecos(const char* caminho, int* tamanho) {
    FILE *file = fopen(caminho, "r");
    if (!file) {
        return NULL;
    }

    int* enderecos = NULL;
    int cont = 0;
    int capacidade = 10;

    enderecos = (int*)malloc(capacidade * sizeof(int));

    while (fscanf(file, "%d", &enderecos[cont]) == 1) {
        cont++;
        if (cont >= capacidade) {
            capacidade *= 2;
            enderecos = (int*)realloc(enderecos, capacidade * sizeof(int));
        }
    }

    fclose(file);

    *tamanho = cont;
    return enderecos;
}

char** int_para_binario(int* enderecos, int tamanho) {
    char** enderecos_binarios = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        int numero = enderecos[i];
        enderecos_binarios[i] = (char*)malloc(17 * sizeof(char));
        for (int j = 15; j >= 0; j--) {
            enderecos_binarios[i][j] = (numero & 1) + '0';
            numero >>= 1;
        }
        enderecos_binarios[i][16] = '\0';
    }

    return enderecos_binarios;
}

char** extrair_offset(char** enderecos_binarios, int tamanho) {
    char** offset = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        offset[i] = (char*)malloc(9 * sizeof(char));
        strncpy(offset[i], enderecos_binarios[i] + 8, 8);
        offset[i][8] = '\0';
    }

    return offset;
}

char** extrair_pagina(char** enderecos_binarios, int tamanho) {
    char** pagina = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        pagina[i] = (char*)malloc(9 * sizeof(char));
        strncpy(pagina[i], enderecos_binarios[i], 8);
        pagina[i][8] = '\0';
    }

    return pagina;
}

// Memoria
void iniciar_memoria() {
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        memoria[i].num_frame = i;
        memoria[i].num_pagina = -1;
        memoria[i].ocupado = 0;
        memoria[i].ultimo_acesso = 0;
        memoria[i].tempo = 0;
    }
}

void iniciar_page_table() {
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        page_table[i].num_pagina = -1;
    }
}

void atualizar_frame(int num_frame, int num_pagina) {
    memoria[num_frame].num_pagina = num_pagina;
    memoria[num_frame].ocupado = 1;
    memoria[num_frame].ultimo_acesso++;
    memoria[num_frame].tempo++;
    atualizar_page_table(num_pagina, num_frame);
}

void atualizar_page_table(int num_pagina, int num_frame) {
    page_table[num_pagina].num_pagina = num_frame;
}

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer) {
    int offset = num_pagina * PAGE_SIZE;
    fseek(backing_store, offset, SEEK_SET);
    fread(buffer, sizeof(char), PAGE_SIZE, backing_store);
}

void acessar_memoria(FILE *backing_store, int num_pagina, int offset) {
    int frame_encontrado = -1;
    int valor = 0;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].num_pagina == num_pagina && tlb[i].valido) {
            frame_encontrado = tlb[i].num_frame;
            tlb_hit++;
            break;
        }
    }

    if (frame_encontrado != -1) {
        valor = memoria[frame_encontrado].dados[offset];
        imprimir(num_pagina * PAGE_SIZE + offset, frame_encontrado, valor);
        return;
    }

    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (memoria[i].ocupado && memoria[i].num_pagina == num_pagina) {
            frame_encontrado = i;
            break;
        }
    }

    if (frame_encontrado == -1) {
        page_faults++;
        int frame_vazio = -1;
        for (int i = 0; i < FRAME_TAMANHO; i++) {
            if (!memoria[i].ocupado) {
                frame_vazio = i;
                break;
            }
        }

        if (frame_vazio == -1) {
            frame_vazio = remover_fifo(memoria);
        }

        ler_backing_store(backing_store, num_pagina, memoria[frame_vazio].dados);
        atualizar_frame(frame_vazio, num_pagina);
        frame_encontrado = frame_vazio;
    }

    if (frame_encontrado != tlb[tlb_index].num_frame) {
        atualizar_tlb(num_pagina, frame_encontrado);
    }

    valor = memoria[frame_encontrado].dados[offset];

    imprimir(num_pagina * PAGE_SIZE + offset, frame_encontrado, valor);
    tlb_index = (tlb_index + 1) % TLB_SIZE;
}

// FIFO Queue
int remover_fifo(Frame *memoria) {
    int frame_substituir = -1;
    int menor_tempo = memoria[0].tempo;
    frame_substituir = 0;

    for (int i = 1; i < FRAME_TAMANHO; i++) {
        if (memoria[i].tempo < menor_tempo) {
            menor_tempo = memoria[i].tempo;
            frame_substituir = i;
        }
    }

    return frame_substituir;
}

// TLB
void atualizar_tlb(int num_pagina, int num_frame) {
    tlb[tlb_index].num_pagina = num_pagina;
    tlb[tlb_index].num_frame = num_frame;
    tlb[tlb_index].valido = 1;
}


void imprimir(int endereco_virtual, int frame, int valor) {
    int num_pagina = endereco_virtual / PAGE_SIZE;
    int offset = endereco_virtual % PAGE_SIZE;

    int endereco_fisico = frame * PAGE_SIZE + offset;

    printf("Virtual address: %d TLB: %d Physical address: %d Value: %d\n", endereco_virtual, tlb_index, endereco_fisico, valor);
}


void imprimir_resultados(int tamanho) {
    printf("Number of Translated Addresses = %d\n", tamanho);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", (float)page_faults / tamanho);
    printf("TLB Hits = %d\n", tlb_hit);
    printf("TLB Hit Rate = %.3f\n", (float)tlb_hit / tamanho);
}