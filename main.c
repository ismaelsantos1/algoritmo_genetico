#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>
#include <omp.h>
#include <stdint.h>

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

// RNG Thread-safe usando Xorshift32
uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

int random_int_r(int min, int max, uint32_t *seed) {
    if (max <= min) return min;
    return min + (xorshift32(seed) % (max - min + 1));
}

float random_float_r(uint32_t *seed) {
    return (float)xorshift32(seed) / (float)UINT32_MAX;
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
    #pragma omp parallel
    {
        uint32_t seed = (uint32_t)(time(NULL) ^ (omp_get_thread_num() + 1));
        #pragma omp for
        for (int i = 0; i < POP_SIZE; i++) {
            pop[i] = criar_individuo();
            for (int j = 0; j < NUM_ITEMS; j++) {
                pop[i].genes[j] = random_int_r(0, 1, &seed);
            }
            avaliar_individuo(&pop[i]);
        }
    }
}

int selecao_torneio(Individuo *pop, uint32_t *seed) {
    int melhor_idx = random_int_r(0, POP_SIZE - 1, seed);
    for (int i = 1; i < 3; i++) { // Torneio de tamanho 3
        int idx = random_int_r(0, POP_SIZE - 1, seed);
        if (pop[idx].fitness > pop[melhor_idx].fitness) {
            melhor_idx = idx;
        }
    }
    return melhor_idx;
}

void crossover(Individuo p1, Individuo p2, Individuo *f1, Individuo *f2, uint32_t *seed) {
    int ponto = random_int_r(1, NUM_ITEMS - 1, seed); 
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (i < ponto) {
            f1->genes[i] = p1.genes[i];
            if (f2) f2->genes[i] = p2.genes[i];
        } else {
            f1->genes[i] = p2.genes[i];
            if (f2) f2->genes[i] = p1.genes[i];
        }
    }
}

void mutacao(Individuo *ind, uint32_t *seed) {
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (random_float_r(seed) < MUTATION_RATE) {
            ind->genes[i] = !ind->genes[i];
        }
    }
}

int pegar_melhor_individuo(Individuo *pop) {
    int melhor = 0;
    #pragma omp parallel
    {
        int local_melhor = 0;
        #pragma omp for nowait
        for (int i = 1; i < POP_SIZE; i++) {
            if (pop[i].fitness > pop[local_melhor].fitness) {
                local_melhor = i;
            }
        }
        #pragma omp critical
        {
            if (pop[local_melhor].fitness > pop[melhor].fitness) {
                melhor = local_melhor;
            }
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
    
    // Lê N (Número de itens) e PMAX (Capacidade máxima)
    if (fscanf(file, "%d %d", &NUM_ITEMS, &CAPACIDADE) != 2) {
        printf("Erro ao ler parametros N e PMAX.\n");
        exit(1);
    }
    
    itens = (Item*)malloc(NUM_ITEMS * sizeof(Item));
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (fscanf(file, "%d %d", &itens[i].valor, &itens[i].peso) != 2) {
            printf("Erro ao ler item %d.\n", i);
        }
    }
    // A solução ótima ao final do arquivo é ignorada pois só lemos até NUM_ITEMS
    fclose(file);
}

int main(int argc, char *argv[]) {
    // Valida os parametros da linha de comando
    if (argc < 6) {
        printf("Uso: %s <tamanho_populacao> <taxa_mutacao> <max_geracoes> <num_threads> <arquivo_instancia>\n", argv[0]);
        printf("Exemplo: %s 100 0.05 1000 1 kp_instances/large_scale/knapPI_1_100_1000_1\n", argv[0]);
        return 1;
    }
    
    POP_SIZE = atoi(argv[1]);
    MUTATION_RATE = atof(argv[2]);
    MAX_GEN = atoi(argv[3]);
    NUM_THREADS = atoi(argv[4]);
    const char *arquivo_instancia = argv[5];
    
    omp_set_num_threads(NUM_THREADS);
    
    // Seed melhorada com timer de alta precisão para evitar repetição entre execuções coladas
    LARGE_INTEGER seed_time;
    QueryPerformanceCounter(&seed_time);
    srand((unsigned int)(time(NULL) ^ seed_time.QuadPart ^ GetCurrentProcessId()));
    
    // Ler itens arquivo
    ler_entrada(arquivo_instancia);
    
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
        
        #pragma omp parallel
        {
            uint32_t seed = (uint32_t)(time(NULL) ^ (omp_get_thread_num() + 1) ^ g);
            #pragma omp for
            for (int i = 1; i < POP_SIZE; i += 2) {
                int p1 = selecao_torneio(populacao, &seed);
                int p2 = selecao_torneio(populacao, &seed);
                
                Individuo *f1 = &nova_populacao[i];
                Individuo *f2 = (i + 1 < POP_SIZE) ? &nova_populacao[i+1] : NULL;
                
                crossover(populacao[p1], populacao[p2], f1, f2, &seed);
                
                mutacao(f1, &seed);
                avaliar_individuo(f1);
                
                if (f2) {
                    mutacao(f2, &seed);
                    avaliar_individuo(f2);
                }
            }
        }
        
        // Cópia da população para a próxima geração
        #pragma omp parallel for
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
