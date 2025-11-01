
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>    // _beginthreadex() e _endthreadex()
#include <conio.h>      // _getch
#include <cstdlib>      // rand_s

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
DWORD WINAPI TarefaLeituraTeclado(LPVOID lpParam);

// --- Variáveis Globais ---
ListaCircular lista1;
HANDLE hThreads[4]; // 0: Medicao, 1: CLP, 2: Retirada, 3: Teclado
HANDLE hEventoTermino;
HANDLE hEventoBloqueioMedicao, hEventoBloqueioCLP, hEventoBloqueioRetirada, hEventoESC;

void GerarDados(int tipo, int nseq, char* VariavelRetorno)
{ // de acordo com minhas fontes, é recomendado usar rand_s, já q é aplicação multithread. Só que tava dando mto erro de compilação.
    //como a função é chamada entre mutexes, talvez n de problema
    if (tipo == 11)
    {
        // Gera os dados 

		int id_disco = (rand() % 2) + 1;

        float gr_med = (float)(rand() % 10001) / 100.0f;
        
        float gr_max = (float)(rand() % 10001) / 100.0f;

        float gr_min = (float)(rand() % 10001) / 100.0f;

        float sigma = (float)(rand() % 10001) / 100.0f;

		// Obtém o timestamp
		char timestamp[9];
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(timestamp, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

        sprintf(lista1.buffer[lista1.tail],
            "11/%04ld/%02d/%s/%06.2f/%06.2f/%06.2f/%06.2f",
            nseq,
            id_disco,
            timestamp,
            gr_med,
            gr_max,
            gr_min,
            sigma
        );
    }
    if (tipo == 44)
    {
        // Gera os dados
        float vel, incl, pot, vz_ent, vz_saida;
        int id_disco = (rand() % 6) + 1;

        vel = (float)(rand() % 10001) / 10.0f;
 
        incl = (float)(rand() % 451) / 10.0f;
       
        pot = (float)(rand() % 2001) / 1000.0f;
        
        vz_ent = (float)(rand() % 10001) / 10.0f;
        
        vz_saida = (float)(rand() % 10001) / 10.0f;

		//obtém o timestamp
        char timeStr[13]; // HH:MM:SS:MMM + \0
        SYSTEMTIME st;
        GetLocalTime(&st);
        sprintf(timeStr, "%02d:%02d:%02d:%03d",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds); // esse timestamp aq n tá mostrando os milissegundos

        sprintf(VariavelRetorno,
            "44/%04d/%02d/%s/%06.1f/%04.1f/%05.3f/%06.1f/%06.1f", 
            nseq,
            id_disco,
            timeStr,
            vel,
            incl,
            pot,
            vz_ent,
            vz_saida
        );
    }
}


int main() {
    DWORD dwThreadId;

    printf("Processo Principal iniciado.\n");

    // Etapa 1: Disparar os processos de exibição
    //IniciarProcesso("C:\\Users\\marcos\\source\\repos\\TP_ATR\\bin\\Processo_exibicao_dados.exe");
    //IniciarProcesso("C:\\Users\\marcos\\source\\repos\\TP_ATR\\bin\\Processo_analise_granulometria.exe");
	IniciarProcesso("Processo_exibicao_dados.exe");
	IniciarProcesso("Processo_analise_granulometria.exe");

    // Etapa 2: Inicializar a lista circular e seus objetos de sincronização
    InicializarLista(lista1, TAMANHO_LISTA_1);

    // Etapa 3: Criar os eventos para controle (bloqueio e término)
    hEventoTermino = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventoBloqueioMedicao = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventoBloqueioCLP = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventoBloqueioRetirada = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventoESC = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Etapa 4: Criar as threads de trabalho
    printf("Criando threads de trabalho...\n");
    hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaLeituraMedicao, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[0]) printf("Thread de Medicao criada com Id = %0x\n", dwThreadId);

    hThreads[1] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaLeituraCLP, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[1]) printf("Thread do CLP criada com Id = %0x\n", dwThreadId);

    hThreads[2] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaRetiradaMensagens, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[2]) printf("Thread de Retirada criada com Id = %0x\n", dwThreadId);

    hThreads[3] = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)TarefaLeituraTeclado, NULL, 0, (CAST_LPDWORD)&dwThreadId);
    if (hThreads[3]) printf("Thread de Leitura do teclado criada com Id = %0x\n", dwThreadId);

    printf("\n--- Sistema em operacao ---\n");
    printf("Pressione 'm', 'p', 'r' para pausar/retomar as tarefas.\n");
    printf("Pressione ESC para finalizar.\n");

    // Etapa 5: Loop de leitura do teclado
    //int tecla;
    //do {
    //    tecla = _getch();
    //    switch (tecla) {
    //    case 'm': case 'M':
    //        printf("[Controle] Comando para bloquear/desbloquear Medicao recebido.\n");
    //        SetEvent(hEventoBloqueioMedicao);
    //        break;
    //    case 'p': case 'P':
    //        printf("[Controle] Comando para bloquear/desbloquear CLP recebido.\n");
    //        SetEvent(hEventoBloqueioCLP);
    //        break;
    //    case 'r': case 'R':
    //        printf("[Controle] Comando para bloquear/desbloquear Retirada recebido.\n");
    //        SetEvent(hEventoBloqueioRetirada);
    //        break;
    //    }
    //} while (tecla != 27); // ESC

    // Etapa 5: Sinalizar o término e aguardar as threads
	WaitForSingleObject(hEventoESC, INFINITE);
    printf("\nTecla ESC pressionada. Notificando threads para terminarem...\n");
    SetEvent(hEventoTermino);

    WaitForMultipleObjects(4, hThreads, TRUE, INFINITE);
    printf("Todas as threads de trabalho terminaram.\n");

    // Etapa 6: Limpeza dos handles
    CloseHandle(hThreads[0]);
    CloseHandle(hThreads[1]);
    CloseHandle(hThreads[2]);
	CloseHandle(hThreads[3]);
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
        //GerarDados(11, contadorMsg, lista1.buffer[lista1.tail]);
		//contadorMsg++;
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
  //      GerarDados(44, contadorMsg, lista1.buffer[lista1.tail]);
		//printf("[CLP] Depositou: %s\n", lista1.buffer[lista1.tail]);
		//contadorMsg++;
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

DWORD WINAPI TarefaLeituraTeclado(LPVOID lpParam) {

    int tecla;
    while ((true)) {
        while (_kbhit()) {
            _getch();
        }
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

        if (tecla == 27) { // ESC
			// Sinaliza o termino
            printf("\nTecla ESC pressionada. Notificando threads para terminarem...\n");
            SetEvent(hEventoTermino);
            return 0;
        }
    }
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