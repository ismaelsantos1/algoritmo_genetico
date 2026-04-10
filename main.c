#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>

// Estruturas
typedef struct {
    int valor;
    int peso;
} Item;

typedef struct {
    bool *genes;
    int fitness;
    int peso_total;
    int valor_total;
} Individuo;

// Globais
int NUM_ITEMS = 0;
int CAPACIDADE = 0;
Item *itens = NULL;

int POP_SIZE = 0;
float MUTATION_RATE = 0.0;
int MAX_GEN = 0;
int NUM_THREADS = 1;

// Funções de utilidade
double get_time_ms(LARGE_INTEGER start, LARGE_INTEGER end, LARGE_INTEGER freq) {
    return (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
}

int random_int(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

Individuo criar_individuo() {
    Individuo ind;
    ind.genes = (bool*)malloc(NUM_ITEMS * sizeof(bool));
    ind.fitness = 0;
    ind.peso_total = 0;
    ind.valor_total = 0;
    return ind;
}

void avaliar_individuo(Individuo *ind) {
    ind->peso_total = 0;
    ind->valor_total = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (ind->genes[i]) {
            ind->peso_total += itens[i].peso;
            ind->valor_total += itens[i].valor;
        }
    }
    
    // Penalidade se a mochila estourar a capacidade
    if (ind->peso_total > CAPACIDADE) {
        ind->fitness = 0;
    } else {
        ind->fitness = ind->valor_total;
    }
}

void init_populacao(Individuo *pop) {
    for (int i = 0; i < POP_SIZE; i++) {
        pop[i] = criar_individuo();
        for (int j = 0; j < NUM_ITEMS; j++) {
            pop[i].genes[j] = random_int(0, 1);
        }
        avaliar_individuo(&pop[i]);
    }
}

int selecao_torneio(Individuo *pop) {
    int melhor_idx = random_int(0, POP_SIZE - 1);
    for (int i = 1; i < 3; i++) { // Torneio de tamanho 3
        int idx = random_int(0, POP_SIZE - 1);
        if (pop[idx].fitness > pop[melhor_idx].fitness) {
            melhor_idx = idx;
        }
    }
    return melhor_idx;
}

void crossover(Individuo p1, Individuo p2, Individuo *f1, Individuo *f2) {
    int ponto = random_int(1, NUM_ITEMS - 1); 
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (i < ponto) {
            f1->genes[i] = p1.genes[i];
            f2->genes[i] = p2.genes[i];
        } else {
            f1->genes[i] = p2.genes[i];
            f2->genes[i] = p1.genes[i];
        }
    }
}

void mutacao(Individuo *ind) {
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (random_float() < MUTATION_RATE) {
            ind->genes[i] = !ind->genes[i];
        }
    }
}

int pegar_melhor_individuo(Individuo *pop) {
    int melhor = 0;
    for (int i = 1; i < POP_SIZE; i++) {
        if (pop[i].fitness > pop[melhor].fitness) {
            melhor = i;
        }
    }
    return melhor;
}

void copiar_individuo(Individuo *dest, Individuo *src) {
    for (int i = 0; i < NUM_ITEMS; i++) {
        dest->genes[i] = src->genes[i];
    }
    dest->fitness = src->fitness;
    dest->peso_total = src->peso_total;
    dest->valor_total = src->valor_total;
}

void ler_entrada(const char *nome_arquivo) {
    FILE *file = fopen(nome_arquivo, "r");
    if (!file) {
        printf("Erro ao abrir arquivo %s\n", nome_arquivo);
        exit(1);
    }
    
    char buffer[256];
    // Lê a config ignorando as strings (rótulos)
    if (fscanf(file, "%s %d", buffer, &CAPACIDADE) != 2) {
        printf("Erro ao ler Capacidade do problema.\n");
        exit(1);
    }
    if (fscanf(file, "%s %d", buffer, &NUM_ITEMS) != 2) {
        printf("Erro ao ler Numero de Itens do problema.\n");
        exit(1);
    }
    
    // Pula a linha de cabeçalho "Valor Peso"
    fscanf(file, "%s %s", buffer, buffer);
    
    itens = (Item*)malloc(NUM_ITEMS * sizeof(Item));
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (fscanf(file, "%d %d", &itens[i].valor, &itens[i].peso) != 2) {
            printf("Erro ao ler item %d.\n", i);
        }
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    // Valida os parametros da linha de comando
    if (argc < 5) {
        printf("Uso: %s <tamanho_populacao> <taxa_mutacao> <max_geracoes> <num_threads>\n", argv[0]);
        printf("Exemplo: %s 100 0.05 1000 1\n", argv[0]);
        return 1;
    }
    
    POP_SIZE = atoi(argv[1]);
    MUTATION_RATE = atof(argv[2]);
    MAX_GEN = atoi(argv[3]);
    NUM_THREADS = atoi(argv[4]);
    
    srand((unsigned int)time(NULL));
    
    // Ler itens arquivo
    ler_entrada("entrada.txt");
    
    // Estruturas de tempo de alta precisão
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    // Alocação da Memória
    Individuo *populacao = (Individuo*)malloc(POP_SIZE * sizeof(Individuo));
    Individuo *nova_populacao = (Individuo*)malloc(POP_SIZE * sizeof(Individuo));
    
    for (int i = 0; i < POP_SIZE; i++) {
        nova_populacao[i] = criar_individuo();
    }
    
    init_populacao(populacao);
    
    Individuo melhor_absoluto = criar_individuo();
    melhor_absoluto.fitness = -1;
    
    int g = 0;
    while (g < MAX_GEN) {
        int idx_melhor = pegar_melhor_individuo(populacao);
        
        // Mantém registro do melhor absoluto já encontrado
        if (populacao[idx_melhor].fitness > melhor_absoluto.fitness) {
            copiar_individuo(&melhor_absoluto, &populacao[idx_melhor]);
        }
        
        // Elitismo clássico: Preserva o melhor indivíduo da geração na próxima (índice 0)
        copiar_individuo(&nova_populacao[0], &populacao[idx_melhor]);
        
        int i = 1;
        while (i < POP_SIZE) {
            int p1 = selecao_torneio(populacao);
            int p2 = selecao_torneio(populacao);
            
            Individuo f1 = criar_individuo();
            Individuo f2 = criar_individuo();
            
            crossover(populacao[p1], populacao[p2], &f1, &f2);
            
            mutacao(&f1);
            avaliar_individuo(&f1);
            copiar_individuo(&nova_populacao[i], &f1);
            i++;
            
            if (i < POP_SIZE) {
                mutacao(&f2);
                avaliar_individuo(&f2);
                copiar_individuo(&nova_populacao[i], &f2);
                i++;
            }
            
            free(f1.genes);
            free(f2.genes);
        }
        
        // Cópia da população
        for (int i = 0; i < POP_SIZE; i++) {
            copiar_individuo(&populacao[i], &nova_populacao[i]);
        }
        
        g++;
    }
    
    QueryPerformanceCounter(&end);
    double tempo_execucao_ms = get_time_ms(start, end, frequency);
    
    // Resultados
    printf("==== RESULTADOS AG MOCHILA ====\n");
    printf("Melhor Valor  : %d\n", melhor_absoluto.valor_total);
    printf("Peso Utilizado: %d / %d (Capacidade)\n", melhor_absoluto.peso_total, CAPACIDADE);
    printf("Tempo Execucao: %.4f ms\n", tempo_execucao_ms);
    printf("Geracoes Exec : %d\n", MAX_GEN);
    printf("Itens Sel.    : [ ");
    for (int i = 0; i < NUM_ITEMS; i++) {
        printf("%d ", melhor_absoluto.genes[i]);
    }
    printf("]\n");
    
    // Liberações
    for (int i = 0; i < POP_SIZE; i++) {
        free(populacao[i].genes);
        free(nova_populacao[i].genes);
    }
    free(populacao);
    free(nova_populacao);
    free(melhor_absoluto.genes);
    free(itens);
    
    return 0;
}
