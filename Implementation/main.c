#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int* ler_enderecos(const char* caminho, int* tamanho);
char** int_para_binario(int* enderecos, int tamanho);
char** extrair_offset(char** enderecos_binarios, int tamanho);
char** extrair_pagina(char** enderecos_binarios, int tamanho); // Alterei o nome da função para extrair_pagina

int main() {
    int tamanho;
    int* enderecos = ler_enderecos("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &tamanho);
    char** enderecos_binarios = int_para_binario(enderecos, tamanho);

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
        char** pagina = extrair_pagina(enderecos_binarios, tamanho); // Chamada corrigida para extrair_pagina

        printf("\nOffset:\n");
        for (int i = 0; i < tamanho; i++) {
            printf("%s\n", offset[i]);
        }

        printf("\nPagina:\n"); // Corrigi o nome da seção impressa para "Pagina"
        for (int i = 0; i < tamanho; i++) {
            printf("%s\n", pagina[i]);
        }

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

        if (tamanho_binario == 0) {
            tamanho_binario = 1;
        }

        enderecos_binarios[i] = (char*)malloc((tamanho_binario + 1) * sizeof(char));
        enderecos_binarios[i][tamanho_binario] = '\0';

        for (int j = tamanho_binario - 1; j >= 0; j--) {
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
        int comp = strlen(enderecos_binarios[i]);
        pagina[i] = (char*)malloc(9 * sizeof(char));

        if (comp >= 16) {
            for (int j = 0; j < 8; j++) {
                pagina[i][j] = enderecos_binarios[i][comp - 16 + j];
            }
        } else if (comp > 8) {
            int inicio = comp - 16;
            if (inicio < 0) {
                inicio = 0;
            }
            int copiar = comp - 8 - inicio;
            for (int j = 0; j < copiar; j++) {
                pagina[i][j] = enderecos_binarios[i][inicio + j];
            }
            for (int j = copiar; j < 8; j++) {
                pagina[i][j] = '0';
            }
        } else {
            for (int j = 0; j < 8; j++) {
                pagina[i][j] = '0';
            }
        }
        pagina[i][8] = '\0';
    }

    return pagina;
}
