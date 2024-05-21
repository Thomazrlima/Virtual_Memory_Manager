#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_TAMANHO 128
#define PAGE_SIZE 256
#define MEMORY_SIZE 65536

typedef struct {
    int num_pagina;
} PageTableEntry;

typedef struct {
    int num_frame;
    int num_pagina;
    int ocupado;
    int ultimo_acesso;
    int tempo;
    char dados[PAGE_SIZE];
} Frame;

Frame memoria[FRAME_TAMANHO];
PageTableEntry page_table[FRAME_TAMANHO];

// Leitura do Addresses
int* ler_enderecos(const char* caminho, int* tamanho);
char** int_para_binario(int* enderecos, int tamanho);
char** extrair_offset(char** enderecos_binarios, int tamanho);
char** extrair_pagina(char** enderecos_binarios, int tamanho);

// Memoria
void iniciar_memoria();
void iniciar_page_table();
void atualizar_frame(int indice_frame, int num_pagina, int tempo_atual);
void atualizar_page_table(int num_pagina, int num_frame);

//Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer);
void acessar_memoria(FILE *backing_store, int num_pagina, int offset, int tempo_atual);

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

    // Inicializa a memória e a tabela de páginas
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
    }

    return 0;
}

// Leitura do addresses
int* ler_enderecos(const char* caminho, int* tamanho) {
    FILE *file = fopen(caminho, "r");
    if (!file) {
        return NULL;
    }

    int* enderecos = NULL;
    int contador = 0;
    int capacidade = 10;

    enderecos = (int*)malloc(capacidade * sizeof(int));

    while (fscanf(file, "%d", &enderecos[contador]) == 1) {
        contador++;
        if (contador >= capacidade) {
            capacidade *= 2;
            int* novos_enderecos = (int*)realloc(enderecos, capacidade * sizeof(int));
            enderecos = novos_enderecos;
        }
    }

    fclose(file);

    int* enderecos_finais = (int*)realloc(enderecos, contador * sizeof(int));

    *tamanho = contador;
    return enderecos_finais;
}

char** int_para_binario(int* enderecos, int tamanho) {
    char** enderecos_binarios = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        int numero = enderecos[i];
        int tamanho_binario = 0;
        int temp = numero;

        while (temp > 0) {
            tamanho_binario++;
            temp >>= 1;
        }

        int zero = 16 - tamanho_binario;
        if (zero < 0) {
            zero = 0;
        }

        enderecos_binarios[i] = (char*)malloc((16 + 1) * sizeof(char));
        enderecos_binarios[i][16] = '\0';

        for (int j = 0; j < zero; j++) {
            enderecos_binarios[i][j] = '0';
        }

        for (int j = 15; j >= zero; j--) {
            enderecos_binarios[i][j] = (numero & 1) + '0';
            numero >>= 1;
        }
    }

    return enderecos_binarios;
}

char** extrair_offset(char** enderecos_binarios, int tamanho) {
    char** offset = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        int len = strlen(enderecos_binarios[i]);
        offset[i] = (char*)malloc(9 * sizeof(char));

        if (len > 8) {
            for (int j = 0; j < 8; j++) {
                offset[i][j] = enderecos_binarios[i][len - 8 + j];
            }
        } else {
            for (int j = 0; j < len; j++) {
                offset[i][j] = enderecos_binarios[i][j];
            }
            for (int j = len; j < 8; j++) {
                offset[i][j] = '0';
            }
        }
        offset[i][8] = '\0';
    }

    return offset;
}

char** extrair_pagina(char** enderecos_binarios, int tamanho) {
    char** pagina = (char**)malloc(tamanho * sizeof(char*));

    for (int i = 0; i < tamanho; i++) {
        pagina[i] = (char*)malloc(9 * sizeof(char));
        strncpy(pagina[i], enderecos_binarios[i] + 0, 8);
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

void atualizar_frame(int indice_frame, int num_pagina, int tempo_atual) {
    memoria[indice_frame].num_pagina = num_pagina;
    memoria[indice_frame].ocupado = 1;
    memoria[indice_frame].ultimo_acesso = tempo_atual;
    memoria[indice_frame].tempo = tempo_atual;
    atualizar_page_table(num_pagina, indice_frame);
}

void atualizar_page_table(int num_pagina, int num_frame) {
    page_table[num_frame].num_pagina = num_pagina;
}

//Backing_Storage
void ler_backing_store(FILE *backing_store, int num_pagina, char *buffer) {
    int offset = num_pagina * PAGE_SIZE;
    fseek(backing_store, offset, SEEK_SET);
    fread(buffer, sizeof(char), PAGE_SIZE, backing_store);
}

void acessar_memoria(FILE *backing_store, int num_pagina, int offset, int tempo_atual) {
    int frame_encontrado = -1;
    for (int i = 0; i < FRAME_TAMANHO; i++) {
        if (page_table[i].num_pagina == num_pagina) {
            frame_encontrado = i;
            break;
        }
    }

    if (frame_encontrado != -1) {
        printf("Pagina %d encontrada no frame %d\n", num_pagina, frame_encontrado);
        atualizar_frame(frame_encontrado, num_pagina, tempo_atual);
    } else {
        printf("Pagina %d nao encontrada na memoria, buscando no BACKING STORE\n", num_pagina);

        int frame_vazio = -1;
        for (int i = 0; i < FRAME_TAMANHO; i++) {
            if (memoria[i].ocupado == 0) {
                frame_vazio = i;
                break;
            }
        }

        if (frame_vazio != -1) {
            printf("Frame vazio encontrado na memoria (frame %d)\n", frame_vazio);
            char buffer[PAGE_SIZE];
            ler_backing_store(backing_store, num_pagina, buffer);
            memcpy(memoria[frame_vazio].dados, buffer, PAGE_SIZE);

            atualizar_frame(frame_vazio, num_pagina, tempo_atual);
        } else {
            printf("Realizando substituicao\n");
            // Implementação da substituição de página
        }
    }
}