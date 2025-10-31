// =======================================================================
// ProcessoAnaliseGranulometria.cpp
// Simula o projetor para a análise de granulometria
// =======================================================================

#include <stdio.h>    // Adicionado para a função printf
#include <conio.h>    // Adicionado para a função _getch

int main() {
    printf("--- PROJETOR DE ANALISE DE GRANULOMETRIA ---\n\n");
    printf("Esta janela exibira os dados da medicao de granulometria.\n");
    printf("Para a Etapa 1, esta tarefa apenas exibe esta mensagem inicial.\n");
    printf("\n[Estado: ATIVO]\n");
    printf("\nPressione qualquer tecla para fechar esta janela.\n");

    _getch(); // Espera uma tecla ser pressionada para terminar

    return 0;
}