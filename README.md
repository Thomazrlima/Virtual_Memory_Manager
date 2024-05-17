<p align="center">
  <img
    src="https://img.shields.io/badge/Status-Em%20desenvolvimento-green?style=flat-square"
    alt="Status"
  />
</p>

<p align="center">
  <img
    src="https://img.shields.io/github/repo-size/Thomazrlima/Virtual_Memory_Manager?style=flat"
    alt="Repository Size"
  />
  <img
    src="https://img.shields.io/github/languages/count/Thomazrlima/Virtual_Memory_Manager?style=flat&logo=python"
    alt="Language Count"
  />
  <img
    src="https://img.shields.io/github/commit-activity/t/Thomazrlima/Virtual_Memory_Manager?style=flat&logo=github"
    alt="Commit Activity"
  />
  <a href="LICENSE.md"
    ><img
      src="https://img.shields.io/github/license/Thomazrlima/Virtual_Memory_Manager"
      alt="License"
  /></a>
</p>

## 👀 Descrição do Projeto

Este projeto consiste em um programa que traduz endereços lógicos para físicos para um espaço de endereço virtual de tamanho 2^16 = 65.536 bytes. O programa lê de um arquivo contendo endereços lógicos e, utilizando uma TLB (Translation Lookaside Buffer) e uma tabela de páginas, traduz cada endereço lógico para seu correspondente endereço físico e exibe o valor do byte armazenado no endereço físico traduzido.

O objetivo deste projeto é usar a simulação para entender os passos envolvidos na tradução de endereços lógicos para físicos, incluindo a resolução de falhas de página usando paginação sob demanda, gerenciamento de uma TLB e implementação de um algoritmo de substituição de páginas.

## 📜 Especificações

- **Endereços Lógicos**: O programa lê um arquivo contendo vários números inteiros de 32 bits que representam endereços lógicos. No entanto, apenas endereços de 16 bits são utilizados, então é necessário mascarar os 16 bits mais à direita de cada endereço lógico. Esses 16 bits são divididos em:
  - **Número de Página**: 8 bits
  - **Deslocamento**: 8 bits
  - Estrutura dos endereços: `[ Número de Página (8 bits) | Deslocamento (8 bits) ]`

- **Tabela de Páginas**:
  - 28 entradas na tabela de páginas
  - Tamanho da página: 28 bytes

- **TLB (Translation Lookaside Buffer)**:
  - 16 entradas na TLB
  - Algoritmo de substituição implementado

- **Memória Física**:
  - Tamanho do quadro: 28 bytes
  - 128 quadros
  - Memória física total: 32.768 bytes (128 quadros × 256 bytes por quadro)

## 📬 Tradução de Endereços

O processo de tradução de endereços lógicos para endereços físicos é realizado utilizando a TLB e a tabela de páginas conforme descrito na Seção 9.3 do livro-texto. O fluxo do processo é o seguinte:

1. **Extração do Número da Página**: O número da página é extraído do endereço lógico.
2. **Consulta na TLB**: A TLB é consultada para verificar se há um acerto.
   - **Acerto na TLB**: O número do quadro é obtido da TLB.
   - **Falta na TLB**: A tabela de páginas é consultada.
3. **Consulta na Tabela de Páginas**:
   - Se o número do quadro estiver na tabela de páginas, ele é usado.
   - Se não estiver, ocorre uma falha de página e a página deve ser carregada na memória física.
   
4. **Atualização da TLB**: Após resolver a falta na TLB (ou na tabela de páginas), a TLB é atualizada conforme o algoritmo de substituição implementado.

5. **Cálculo do Endereço Físico**: O endereço físico é calculado combinando o número do quadro obtido com o deslocamento original.

6. **Leitura do Byte**: O valor do byte armazenado no endereço físico traduzido é exibido.

## 🖨️ Como Executar o Programa

O programa deve ser executado da seguinte maneira:

- O programa lê o arquivo `addresses.txt`, que contém 1.000 endereços lógicos variando de 0 a 65.535. O programa traduz cada endereço lógico para um endereço físico e determina o conteúdo do byte com sinal armazenado no endereço físico traduzido. (Lembre-se de que na linguagem C, o tipo de dados `char` ocupa um byte de armazenamento, então sugere-se usar valores `char`.)

- O programa exibe os seguintes valores:
  1. O endereço lógico sendo traduzido (o valor inteiro sendo lido de `addresses.txt`).
  2. O endereço físico correspondente (o que o programa traduz o endereço lógico para).
  3. O valor do byte com sinal armazenado na memória física no endereço físico traduzido.

- O arquivo `correct.txt` é fornecido e contém os valores de saída corretos para o arquivo `addresses.txt`. Esse arquivo deve ser usado para verificar se o programa está traduzindo corretamente endereços lógicos para físicos.

## 📈 Estatísticas

Após a conclusão, o programa relata as seguintes estatísticas:
1. **Taxa de falhas de página**—A porcentagem de referências de endereço que resultaram em falhas de página.
2. **Taxa de acertos na TLB**—A porcentagem de referências de endereço que foram resolvidas na TLB.

Como os endereços lógicos em `addresses.txt` foram gerados aleatoriamente e não refletem nenhuma localidade de acesso à memória, não se espera uma alta taxa de acertos na TLB.

## ♟️ Objetivos de Aprendizado

- Como funciona a tradução de endereços lógicos para físicos.
- O processo de gerenciamento de uma TLB.
- Implementação de algoritmos de substituição de páginas.
- Resolução de falhas de página utilizando paginação sob demanda.

## ⚖️ License

[MIT](https://github.com/Thomazrlima/Virtual_Memory_Manager/LICENSE.md)
