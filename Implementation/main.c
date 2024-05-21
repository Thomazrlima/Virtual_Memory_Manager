#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 128
#define PAGE_SIZE 256
#define MEMORY_SIZE 65536

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
void adicionar_fifo(Frame *memoria, int frame);
int remover_fifo(Frame *memoria);

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    iniciar_memoria();
    iniciar_page_table();

    if (enderecos) {
        printf("Lidos (%d):\n", tamanho);
        for (int i = 0; i < tamanho; i++) {
            printf("%d\n", enderecos[i]);
        }

        printf("\nBinario:\n");
        for (int i = 0; i < tamanho; i++) {
            printf("%s\n", enderecos_binarios[i]);
        }

        char** offset = extrair_offset(enderecos_binarios, tamanho);
        char** pagina = extrair_pagina(enderecos_binarios, tamanho);

        printf("\nOffset:\n");
        for (int i = 0; i < tamanho; i++) {
            printf("%s\n", offset[i]);
        }

        printf("\nPagina:\n");
        for (int i = 0; i < tamanho; i++) {
            printf("%s\n", pagina[i]);
        }

        FILE *backing_store = fopen("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/BACKING_STORE.bin", "rb");

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
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (memoria[i].ocupado && memoria[i].num_pagina == num_pagina) {
            frame_encontrado = i;
            break;
        }
    }
    if (frame_encontrado != -1) {
        printf("Pagina %d encontrada no frame %d\n", num_pagina, frame_encontrado);
        memoria[frame_encontrado].ultimo_acesso++;
    } else {
        printf("Pagina %d nao encontrada na memoria\n", num_pagina);

        int frame_vazio = -1;
        for (int i = 0; i < FRAME_TAMANHO; i++) {
            if (!memoria[i].ocupado) {
                frame_vazio = i;
                break;
            }
        }

        if (frame_vazio != -1) {
            printf("Frame vazio encontrado na memoria (frame %d)\n", frame_vazio);
            ler_backing_store(backing_store, num_pagina, memoria[frame_vazio].dados);
            atualizar_frame(frame_vazio, num_pagina);
        } else {
            int frame_substituir = remover_fifo(memoria);
            printf("Substituindo frame %d\n", frame_substituir);
            ler_backing_store(backing_store, num_pagina, memoria[frame_substituir].dados);
            atualizar_frame(frame_substituir, num_pagina);
        }
    }
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
