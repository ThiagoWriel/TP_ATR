
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>    // _beginthreadex() e _endthreadex()
#include <conio.h>      // _getch

#define TAMANHO_LISTA_1 200

// Casting para a função _beginthreadex, conforme estilo do livro
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// --- Estrutura da Lista circular ---
struct ListaCircular {
    char buffer[TAMANHO_LISTA_1][100];
    int head;
    int tail;
    HANDLE hMutex;
    HANDLE hSemaforoCheios;
    HANDLE hSemaforoVazios;
};

// --- Protótipos de Funcao ---
void IniciarProcesso(const char* nomeProcesso);
void InicializarLista(ListaCircular& lista, int tamanho);
DWORD WINAPI TarefaLeituraMedicao(LPVOID lpParam);
DWORD WINAPI TarefaLeituraCLP(LPVOID lpParam);
DWORD WINAPI TarefaRetiradaMensagens(LPVOID lpParam);

// --- Variáveis Globais ---
ListaCircular lista1;
HANDLE hThreads[3]; // 0: Medicao, 1: CLP, 2: Retirada
HANDLE hEventoTermino;
HANDLE hEventoBloqueioMedicao, hEventoBloqueioCLP, hEventoBloqueioRetirada;


int main() {
    DWORD dwThreadId;

    printf("Processo Principal iniciado.\n");

    // Etapa 1: Disparar os processos de exibição
    IniciarProcesso("Processo_exibicao_dados.exe");
    IniciarProcesso("Processo_analise_granulometria.exe");

    // Etapa 2: Inicializar a lista circular e seus objetos de sincronização
    InicializarLista(lista1, TAMANHO_LISTA_1);

    // Etapa 3: Criar os eventos para controle (bloqueio e término)
    hEventoTermino = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventoBloqueioMedicao = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventoBloqueioCLP = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventoBloqueioRetirada = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Etapa 4: Criar as threads de trabalho
    printf("Criando threads de trabalho...\n");
    hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaLeituraMedicao, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[0]) printf("Thread de Medicao criada com Id = %0x\n", dwThreadId);

    hThreads[1] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaLeituraCLP, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[1]) printf("Thread do CLP criada com Id = %0x\n", dwThreadId);

    hThreads[2] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaRetiradaMensagens, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[2]) printf("Thread de Retirada criada com Id = %0x\n", dwThreadId);

    printf("\n--- Sistema em operacao ---\n");
    printf("Pressione 'm', 'p', 'r' para pausar/retomar as tarefas.\n");
    printf("Pressione ESC para finalizar.\n");

    // Etapa 5: Loop de leitura do teclado
    int tecla;
    do {
        tecla = _getch();
        switch (tecla) {
        case 'm': case 'M':
            printf("[Controle] Comando para bloquear/desbloquear Medicao recebido.\n");
            SetEvent(hEventoBloqueioMedicao);
            break;
        case 'p': case 'P':
            printf("[Controle] Comando para bloquear/desbloquear CLP recebido.\n");
            SetEvent(hEventoBloqueioCLP);
            break;
        case 'r': case 'R':
            printf("[Controle] Comando para bloquear/desbloquear Retirada recebido.\n");
            SetEvent(hEventoBloqueioRetirada);
            break;
        }
    } while (tecla != 27); // ESC

    // Etapa 6: Sinalizar o término e aguardar as threads
    printf("\nTecla ESC pressionada. Notificando threads para terminarem...\n");
    SetEvent(hEventoTermino);

    WaitForMultipleObjects(3, hThreads, TRUE, INFINITE);
    printf("Todas as threads de trabalho terminaram.\n");

    // Etapa 7: Limpeza dos handles
    CloseHandle(hThreads[0]);
    CloseHandle(hThreads[1]);
    CloseHandle(hThreads[2]);
    CloseHandle(hEventoTermino);
    CloseHandle(hEventoBloqueioMedicao);
    CloseHandle(hEventoBloqueioCLP);
    CloseHandle(hEventoBloqueioRetirada);
    CloseHandle(lista1.hMutex);
    CloseHandle(lista1.hSemaforoCheios);
    CloseHandle(lista1.hSemaforoVazios);

    printf("\nAcione uma tecla para terminar o programa principal.\n");
    _getch();
    return EXIT_SUCCESS;
}


// --- Implementação das Funções das Threads e Funções Auxiliares ---

DWORD WINAPI TarefaLeituraMedicao(LPVOID lpParam) {
    HANDLE eventos[2] = { hEventoTermino, hEventoBloqueioMedicao };
    BOOL isBlocked = FALSE;
    DWORD dwRet;
    int contadorMsg = 0;

    while (TRUE) {
        dwRet = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (dwRet == WAIT_OBJECT_0) { // Evento de Término
            printf("[Medicao] Sinal de termino recebido. Encerrando.\n");
            break;
        }
        if (dwRet == WAIT_OBJECT_0 + 1) { // Evento de Bloqueio
            isBlocked = !isBlocked;
            printf("[Medicao] Tarefa %s.\n", isBlocked ? "BLOQUEADA" : "RETOMADA");
        }
        if (isBlocked) {
            Sleep(100);
            continue;
        }

        // --- TRABALHO DO PRODUTOR ---
        WaitForSingleObject(lista1.hSemaforoVazios, INFINITE);
        WaitForSingleObject(lista1.hMutex, INFINITE);

        sprintf(lista1.buffer[lista1.tail], "MSG_MEDICAO_%d", contadorMsg++);
        printf("[Medicao] Depositou: %s\n", lista1.buffer[lista1.tail]);
        lista1.tail = (lista1.tail + 1) % TAMANHO_LISTA_1;

        ReleaseMutex(lista1.hMutex);
        ReleaseSemaphore(lista1.hSemaforoCheios, 1, NULL);

        Sleep((rand() % 4001) + 1000); // Temporização provisória
    }
    _endthreadex(0);
    return 0;
}

DWORD WINAPI TarefaLeituraCLP(LPVOID lpParam) {
    HANDLE eventos[2] = { hEventoTermino, hEventoBloqueioCLP };
    BOOL isBlocked = FALSE;
    DWORD dwRet;
    int contadorMsg = 0;

    while (TRUE) {
        dwRet = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (dwRet == WAIT_OBJECT_0) { // Evento de Término
            printf("[CLP] Sinal de termino recebido. Encerrando.\n");
            break;
        }
        if (dwRet == WAIT_OBJECT_0 + 1) { // Evento de Bloqueio
            isBlocked = !isBlocked;
            printf("[CLP] Tarefa %s.\n", isBlocked ? "BLOQUEADA" : "RETOMADA");
        }
        if (isBlocked) {
            Sleep(100);
            continue;
        }

        // --- TRABALHO DO PRODUTOR ---
        WaitForSingleObject(lista1.hSemaforoVazios, INFINITE);
        WaitForSingleObject(lista1.hMutex, INFINITE);

        sprintf(lista1.buffer[lista1.tail], "MSG_CLP_%d", contadorMsg++);
        printf("[CLP] Depositou: %s\n", lista1.buffer[lista1.tail]);
        lista1.tail = (lista1.tail + 1) % TAMANHO_LISTA_1;

        ReleaseMutex(lista1.hMutex);
        ReleaseSemaphore(lista1.hSemaforoCheios, 1, NULL);

        Sleep(500); // Temporização provisória
    }
    _endthreadex(0);
    return 0;
}

DWORD WINAPI TarefaRetiradaMensagens(LPVOID lpParam) {
    HANDLE eventos[2] = { hEventoTermino, hEventoBloqueioRetirada };
    BOOL isBlocked = FALSE;
    DWORD dwRet;
    char mensagemConsumida[100];

    while (TRUE) {
        dwRet = WaitForMultipleObjects(2, eventos, FALSE, 10);

        if (dwRet == WAIT_OBJECT_0) {
            printf("[Retirada] Sinal de termino recebido. Encerrando.\n");
            break;
        }
        if (dwRet == WAIT_OBJECT_0 + 1) {
            isBlocked = !isBlocked;
            printf("[Retirada] Tarefa %s.\n", isBlocked ? "BLOQUEADA" : "RETOMADA");
        }
        if (isBlocked) {
            Sleep(100);
            continue;
        }

        // --- TRABALHO DO CONSUMIDOR ---
        WaitForSingleObject(lista1.hSemaforoCheios, INFINITE);
        WaitForSingleObject(lista1.hMutex, INFINITE);

        strcpy(mensagemConsumida, lista1.buffer[lista1.head]);
        lista1.head = (lista1.head + 1) % TAMANHO_LISTA_1;
        printf("[Retirada] Consumiu: %s\n", mensagemConsumida);

        ReleaseMutex(lista1.hMutex);
        ReleaseSemaphore(lista1.hSemaforoVazios, 1, NULL);
    }
    _endthreadex(0);
    return 0;
}

void IniciarProcesso(const char* nomeProcesso) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(
        nomeProcesso,       // const char*
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,                
        &pi                 
    )) {
        printf("Falha ao criar processo: %s (Erro: %d)\n", nomeProcesso, GetLastError());
        return;
    }
    printf("Processo %s iniciado com sucesso.\n", nomeProcesso);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void InicializarLista(ListaCircular& lista, int tamanho) {
    lista.head = 0;
    lista.tail = 0;
    lista.hMutex = CreateMutex(NULL, FALSE, NULL);
    lista.hSemaforoCheios = CreateSemaphore(NULL, 0, tamanho, NULL);
    lista.hSemaforoVazios = CreateSemaphore(NULL, tamanho, tamanho, NULL);
    printf("Lista Circular e objetos de sincronizacao inicializados.\n");
}