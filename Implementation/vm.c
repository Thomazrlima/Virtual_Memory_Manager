#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 128
#define PAGE_SIZE 256
#define TLB_SIZE 16

typedef struct {
    int num_pagina;
    int num_frame;
    int valido;
} TLB_Entry;

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

Frame memoria[FRAME_TAMANHO];
PageTable page_table[FRAME_TAMANHO];
TLB_Entry TLB[TLB_SIZE];
int page_faults = 0;

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

// Verificação de memória
int verificar_page_table(int num_pagina);
int encontrar_frame_vazio();
int substituir_frame();
void acessar_memoria(FILE *backing_store, int num_pagina, int offset, int tempo);

//TLB

void iniciar_TLB();
int buscar_TLB(int num_pagina);
void atualizar_TLB(int num_pagina, int num_frame);

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer);

// FIFO Queue
void adicionar_fifo(Frame *memoria, int frame);
int remover_fifo(Frame *memoria);

// Print
void imprimir(int endereco_virtual, int frame, int valor, int indice_TLB);
void imprimir_resultados(int tamanho);

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    iniciar_memoria();
    iniciar_page_table();
    iniciar_TLB();

    char** offset = extrair_offset(enderecos_binarios, tamanho);
    char** pagina = extrair_pagina(enderecos_binarios, tamanho);

    FILE *backing_store = fopen("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/BACKING_STORE.bin", "rb");

    for (int i = 0; i < tamanho; i++) {
        int num_pagina = (int)strtol(pagina[i], NULL, 2);
        int offset_val = (int)strtol(offset[i], NULL, 2);
        acessar_memoria(backing_store, num_pagina, offset_val, i);
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

    imprimir_resultados(tamanho);

    return 0;
}

// Leitura dos Addresses
int* ler_enderecos(const char* caminho, int* tamanho) {
    FILE *file = fopen(caminho, "r");

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
    //memoria[num_frame].ultimo_acesso = tempo;
    memoria[num_frame].tempo++;
    atualizar_page_table(num_pagina, num_frame);
    adicionar_fifo(memoria, num_frame);
}

void atualizar_page_table(int num_pagina, int num_frame) {
    page_table[num_pagina].num_pagina = num_frame;
}

// Verificação de memória
int verificar_page_table(int num_pagina) {
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (memoria[i].ocupado && memoria[i].num_pagina == num_pagina) {
            return i;
        }
    }
    return -1;
}

int encontrar_frame_vazio() {
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (!memoria[i].ocupado) {
            return i;
        }
    }
    return -1;
}

int substituir_frame() {
    return remover_fifo(memoria);
}

void acessar_memoria(FILE *backing_store, int num_pagina, int offset, int tempo) {
    int frame_encontrado = buscar_TLB(num_pagina);
    if (frame_encontrado != -1) {
        memoria[frame_encontrado].ultimo_acesso = tempo;
    } else {
        int frame_encontrado_memoria = verificar_page_table(num_pagina);
        if (frame_encontrado_memoria != -1) {
            frame_encontrado = frame_encontrado_memoria;
            memoria[frame_encontrado_memoria].ultimo_acesso = tempo;
        } else {
            page_faults++;
            int frame_vazio = encontrar_frame_vazio();

            if (frame_vazio != -1) {
                ler_backing_store(backing_store, num_pagina, memoria[frame_vazio].dados);
                atualizar_frame(frame_vazio, num_pagina);
                frame_encontrado = frame_vazio;
            } else {
                int frame_substituir = substituir_frame();
                ler_backing_store(backing_store, num_pagina, memoria[frame_substituir].dados);
                atualizar_frame(frame_substituir, num_pagina);
                frame_encontrado = frame_substituir;
            }
        }
        atualizar_TLB(num_pagina, frame_encontrado);
    }

    int indice_TLB = buscar_TLB(num_pagina);
    imprimir(num_pagina * PAGE_SIZE + offset, frame_encontrado, memoria[frame_encontrado].dados[offset], indice_TLB);
}

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer) {
    int offset = num_pagina * PAGE_SIZE;
    fseek(backing_store, offset, SEEK_SET);
    fread(buffer, sizeof(char), PAGE_SIZE, backing_store);
}

// FIFO Queue
void adicionar_fifo(Frame *memoria, int frame) {
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (memoria[i].num_frame == frame) {
            return;
        }
    }

    int frame_vazio = -1;
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (!memoria[i].ocupado) {
            frame_vazio = i;
            break;
        }
    }

    if (frame_vazio != -1) {
        memoria[frame_vazio].num_frame = frame;
        memoria[frame_vazio].ocupado = 1;
        memoria[frame_vazio].ultimo_acesso++;
        memoria[frame_vazio].tempo++;
    } else {
        int frame_substituir = remover_fifo(memoria);
        memoria[frame_substituir].num_frame = frame;
        memoria[frame_substituir].ocupado = 1;
        memoria[frame_substituir].ultimo_acesso++;
        memoria[frame_substituir].tempo++;
    }
}

int remover_fifo(Frame *memoria) {
    int frame_substituir = 0;
    int menor_tempo = memoria[0].tempo;

    for (int i = 1; i < FRAME_TAMANHO; i++) {
        if (memoria[i].tempo < menor_tempo) {
            menor_tempo = memoria[i].tempo;
            frame_substituir = i;
        }
    }

    memoria[frame_substituir].num_pagina = -1;
    memoria[frame_substituir].ocupado = 0;
    memoria[frame_substituir].ultimo_acesso = 0;
    memoria[frame_substituir].tempo = 0;
    memset(memoria[frame_substituir].dados, 0, PAGE_SIZE);

    return frame_substituir;
}

//TLB
void iniciar_TLB() {
    for (int i = 0; i < TLB_SIZE; i++) {
        TLB[i].num_pagina = -1;
        TLB[i].num_frame = -1;
        TLB[i].valido = 0;
    }
}

int buscar_TLB(int num_pagina) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].num_pagina == num_pagina && TLB[i].valido) {
            return i;
        }
    }
    return -1;
}

void atualizar_TLB(int num_pagina, int num_frame) {
    static int proxima_entrada = 0;

    TLB[proxima_entrada].num_pagina = num_pagina;
    TLB[proxima_entrada].num_frame = num_frame;
    TLB[proxima_entrada].valido = 1;

    proxima_entrada = (proxima_entrada + 1) % TLB_SIZE;
}

void imprimir(int endereco_virtual, int frame, int valor, int indice_TLB) {
    int num_pagina = endereco_virtual / PAGE_SIZE;
    int offset = endereco_virtual % PAGE_SIZE;

    int endereco_fisico = frame * PAGE_SIZE + offset;

    printf("Virtual address: %d TLB: %d Physical address: %d Value: %d\n", endereco_virtual, indice_TLB, endereco_fisico, valor);
}

void imprimir_resultados(int tamanho) {
    printf("Number of Translated Addresses = %d\n", tamanho);
    printf("Page Faults = %d\n", page_faults);
}