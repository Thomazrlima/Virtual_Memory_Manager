#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int* entrada(const char* caminho, int* size);
char** inttobin(int* endereco, int size);
char** instrucao(int* endereco, int size);

int main() {
    int size;
    int* endereco = entrada("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &size);
    char** enderecobin = inttobin(endereco, size);

    if (endereco) {
        printf("Lidos (%d):\n", size);
        for (int i = 0; i < size; i++) {
            printf("%d\n", endereco[i]);
        }

        printf("\nBinario:\n");
        for (int i = 0; i < size; i++) {
            printf("%s\n", enderecobin[i]);
            free(enderecobin[i]);
        }

        free(endereco);
        free(enderecobin);
    }

    return 0;
}

int* entrada(const char* caminho, int* size) {
    FILE *file = fopen(caminho, "r");
    if (!file) {
        return NULL;
    }

    int* endereco = NULL;
    int count = 0;
    int capacity = 10;

    endereco = (int*)malloc(capacity * sizeof(int));

    while (fscanf(file, "%d", &endereco[count]) == 1) {
        count++;
        if (count >= capacity) {
            capacity *= 2;
            int* novoEndereco = (int*)realloc(endereco, capacity * sizeof(int));
            endereco = novoEndereco;
        }
    }

    fclose(file);

    int* enderecofinal = (int*)realloc(endereco, count * sizeof(int));

    *size = count;
    return enderecofinal;
}

char** inttobin(int* endereco, int size) {
    char** enderecobin = (char**)malloc(size * sizeof(char*));

    for (int i = 0; i < size; i++) {
        int numero = endereco[i];
        int temp = numero;
        int tamanho = 1;

        while (temp >>= 1) tamanho++;

        enderecobin[i] = (char*)malloc((tamanho + 1) * sizeof(char));
        enderecobin[i][tamanho] = '\0';

        for (int j = tamanho - 1; j >= 0; j--) {
            enderecobin[i][j] = (numero & 1) + '0';
            numero >>= 1;
        }
    }

    return enderecobin;
}