#include <stdio.h>
#include <stdlib.h>

int* entrada(const char* caminho, int* size);

int main() {
    int size;
    int* endereco = entrada("D:/PENTES/Pessoal/Virtual_Memory_Manager/Implementation/addresses.txt", &size);

    if (endereco) {
        printf("Enderecos lidos (%d):\n", size);
        for (int i = 0; i < size; i++) {
            printf("%d\n", endereco[i]);
        }
        free(endereco);
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
    if (!endereco) {
        fclose(file);
        return NULL;
    }

    while (fscanf(file, "%d", &endereco[count]) == 1) {
        count++;
        if (count >= capacity) {
            capacity *= 2;
            int* novoEndereco = (int*)realloc(endereco, capacity * sizeof(int));
            if (!novoEndereco) {
                free(novoEndereco);
                fclose(file);
                return NULL;
            }
            endereco = novoEndereco;
        }
    }

    fclose(file);

    int* enderecofinal = (int*)realloc(endereco, count * sizeof(int));
    if (!enderecofinal) {
        free(endereco);
        return NULL;
    }

    *size = count;
    return enderecofinal;
}