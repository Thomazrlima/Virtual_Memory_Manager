#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 16
#define PAGE_SIZE 256
#define MEMORY_SIZE 65536
#define TLB_SIZE 16

typedef struct {
    int num_pagina;
    int num_frame;
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
TLB_Entry tlb[TLB_SIZE];

int tlb_hit_count = 0;
int tlb_miss_count = 0;

// Leitura dos Addresses
int* ler_enderecos(const char* caminho, int* tamanho);
char** int_para_binario(int* enderecos, int tamanho);
char** extrair_offset(char** enderecos_binarios, int tamanho);
char** extrair_pagina(char** enderecos_binarios, int tamanho);

// Memoria
void iniciar_memoria();
void iniciar_page_table();
void iniciar_tlb();
void atualizar_frame(int num_frame, int num_pagina);
void atualizar_page_table(int num_pagina, int num_frame);
int consultar_tlb(int num_pagina);
void adicionar_tlb(int num_pagina, int num_frame);

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer);
void acessar_memoria(FILE *backing_store, int num_pagina, int offset);

// FIFO Queue
void adicionar_fifo(Frame *memoria, int frame);
int remover_fifo(Frame *memoria);

// Print
void imprimir(int endereco_virtual);

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    iniciar_memoria();
    iniciar_page_table();
    iniciar_tlb();

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

    printf("TLB Hit Count: %d\n", tlb_hit_count);
    printf("TLB Miss Count: %d\n", tlb_miss_count);

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

void iniciar_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].num_pagina = -1;
        tlb[i].num_frame = -1;
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

int consultar_tlb(int num_pagina) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].num_pagina == num_pagina) {
            tlb_hit_count++;
            return tlb[i].num_frame;
        }
    }
    tlb_miss_count++;
    return -1;
}

void adicionar_tlb(int num_pagina, int num_frame) {
    int index = tlb_miss_count % TLB_SIZE;
    tlb[index].num_pagina = num_pagina;
    tlb[index].num_frame = num_frame;
}

// Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer) {
    int offset = num_pagina * PAGE_SIZE;
    fseek(backing_store, offset, SEEK_SET);
    fread(buffer, sizeof(char), PAGE_SIZE, backing_store);
}

void acessar_memoria(FILE *backing_store, int num_pagina, int offset) {
    int frame_encontrado = consultar_tlb(num_pagina);
    if (frame_encontrado != -1) {
        memoria[frame_encontrado].ultimo_acesso++;
    } else {
        frame_encontrado = -1;
        for (int i = 0; i < FRAME_TAMANHO; i++) {
            if (memoria[i].ocupado && memoria[i].num_pagina == num_pagina) {
                frame_encontrado = i;
                break;
            }
        }
        if (frame_encontrado != -1) {
            memoria[frame_encontrado].ultimo_acesso++;
        } else {
            int frame_vazio = -1;
            for (int i = 0; i < FRAME_TAMANHO; i++) {
                if (!memoria[i].ocupado) {
                    frame_vazio = i;
                    break;
                }
            }
            if (frame_vazio != -1) {
                ler_backing_store(backing_store, num_pagina, memoria[frame_vazio].dados);
                atualizar_frame(frame_vazio, num_pagina);
                adicionar_tlb(num_pagina, frame_vazio);
            } else {
                int frame_substituir = remover_fifo(memoria);
                ler_backing_store(backing_store, num_pagina, memoria[frame_substituir].dados);
                atualizar_frame(frame_substituir, num_pagina);
                adicionar_tlb(num_pagina, frame_substituir);
            }
        }
    }

    // Adicione a chamada da função para imprimir o endereço físico correspondente
    imprimir(num_pagina * PAGE_SIZE + offset);
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
    int frame_substituir = -1;
    int menor_tempo = memoria[0].tempo;

    for (int i = 1; i < FRAME_TAMANHO; i++) {
        if (memoria[i].tempo < menor_tempo) {
            menor_tempo = memoria[i].tempo;
            frame_substituir = i;
        }
    }

    if (frame_substituir == -1) {
        frame_substituir = 0;
    }

    return frame_substituir;
}

void imprimir(int endereco_virtual) {
    int num_pagina = endereco_virtual / PAGE_SIZE;
    int offset = endereco_virtual % PAGE_SIZE;

    int frame = -1;
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (memoria[i].ocupado && memoria[i].num_pagina == num_pagina) {
            frame = i;
            break;
        }
    }

    if (frame != -1) {
        int endereco_fisico = frame * PAGE_SIZE + offset;
        printf("Virtual address: %d Physical address: %d Page number: %d\n", endereco_virtual, endereco_fisico, num_pagina);
    } else {
        printf("Página correspondente ao endereço virtual não encontrada na memória.\n");
    }
}
