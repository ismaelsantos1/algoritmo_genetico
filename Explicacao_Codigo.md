# Documentação Detalhada - Algoritmo Genético (Mochila Binária)

Este documento exibe e explica minuciosamente o papel de cada estrutura, variável e lógica implementada no nosso arquivo `main.c`.

---

## 1. Bibliotecas
* `<stdio.h>`: Fornece as funções de entrada/saída básicas (`printf` para imprimir na tela, `fopen` e `fscanf` para manipular arquivos).
* `<stdlib.h>`: Fornece funções gerais de utilidade, como o conversor `atoi`/`atof` (texto para número da linha de comando), `malloc`/`free` para gerenciamento e alocação dinâmica de memória RAM, e `rand` para usar na geração de números aleatórios.
* `<time.h>`: Fornece acesso ao relógio do sistema para inicializar a "semente" aleatória usando o `time(NULL)`.
* `<windows.h>`: É a biblioteca proprietária com as API's de sistema do próprio Windows. A usamos especificamente para acessar a varredura atômica de tempo (`QueryPerformanceCounter`), que é a função de alta definição de tempo, vital para calcular precisamente seus gastos computacionais sem depender de relógios genéricos.
* `<stdbool.h>`: Inclui os tipos booleanos padrão (`true` e `false`), sendo útil para construir nossos cromossomos e economizar tamanho.

---

## 2. Estruturas (Structs)

* `typedef struct { int valor; int peso; } Item;`
  Representa os objetos na entrada para a mochila.
  * `valor`: Benefício final caso inserido na mochila.
  * `peso`: Carga limite que consome da mochila.

* `typedef struct { bool *genes; int fitness; int peso_total; int valor_total; } Individuo;`
  Representa um cromossomo matemático ou tentativa de solução para o problema.
  * `genes`: É o Array (vetor), contendo os bit-flips [0 ou 1]. Se o index X do Array for `1`, significa que nosso indivíduo tem o Item X, caso for estado lógico oposto `0`, não possui aliás.
  * `fitness`: Medidor universal classificatório de quão boa/ruim é a solução perante os outros colegas biológicos.
  * `peso_total`: Guarda o somatório de pesos providos pela simulação.
  * `valor_total`: Somatório de valor base da simulação.

---

## 3. Variáveis Globais (Variáveis de Configuração Constantes)

Estas são construídas de modo global por facilidade no reaproveitamento arquitetônico, pra contornar o excesso de informações exigidas e não trafegar parâmetros a rodo:
* `NUM_ITEMS` / `CAPACIDADE`: Informações descritivas extraídas a partir da análise de texto no arquivo `entrada.txt`.
* `*itens`: Uma vitrine generalista e imutável que guarda o endereço de todos os itens criados do `entrada.txt` após sua leitura.
* `POP_SIZE` (População), `MUTATION_RATE` (Taxa da Mutação), `MAX_GEN` (Nº de Gerações), `NUM_THREADS`: Valores injetados através da linha de comando, ditam a dinâmica final do script.

---

## 4. Utilitários e Funções Auxiliares

* `get_time_ms()`: Operação aritmética simples que divide saltos elétricos de contagem perante os impulsos de CPU (frequência) pra traduzir métricas numéricas irreais num valor agradável do tipo _Double_ ("Milissegundos computacionais reais").
* `random_int(min, max)`: Uma fórmula com operador módulo matemático percentual (`%`), em tese ele processa pseudo-aleatoriamente um espectro equilibrado das bordas e margens solicitadas entre mínimo/máximo.
* `random_float()`: Divide por peso aleatórios e gera casas decimais menores (entre `0.0` a `1.0`) para usar como base pra checar a nossa porcentagem mutante.

---

## 5. Coração Evolutivo (Funções do Algoritmo)

### `criar_individuo()`
Instancia estruturalmente uma vida: simplesmente aloca no computador espaço numérico para o preenchimento do DNA nos `genes` com o uso da mecânica do sistema de alocação de memória C pura (`malloc`). Se encarrega de zerar em estado pacífico pra que nenhum lixo de ponteiros queime as lógicas de aptidão mais tarde.

### `avaliar_individuo(Individuo *ind)`
É o juíz de cada ciclo: roda num `for` somando a vitrine inteira dos lugares em que o Gene está em verdadeiro. Ao final, pondera as restrições: o total da carga corrompe a premissa da `CAPACIDADE` máxima suportável estipulada em arquivo limitando a carga transportada? Se sim, aplica a penalidade `ind->fitness = 0` (zerando a viabilidade de cruzamento pra banir a ideia deficiente do espectro evolutivo). Caso contrário, seu medidor viável transforma a aptidão no próprio potencial da riqueza (`ind->fitness = valor_total`).

### `init_populacao()`
A geração gênesis das soluções! O berçário cria indivíduos com apostas puramente aleatórias (uma roleta de Bit 0 vs Bit 1), testando-os no funil juiz (`avaliar_individuo`).

### `selecao_torneio()`
Metodologia comparativa de amostras aleatorizadas e disputas paremanadas. Pega três candidatos soltos usando a escolha da roleta e só retorna (via Índice retornado int `melhor_idx`) qual tem efetivamente mais poder com o seu `fitness`. 

### `crossover(Filho f1 e f2)`
O cruzamento clássico de Ponto Único. A gente rola o tempo aleatoriamente selecionando uma métrica pra servir como Ponto de Corte e de lá injeta os genes passados pros filhos em ordem respectiva e simétrica trocando quem fornece.

### `mutacao(Individuo *ind)`
A aniquilação para a manutenção caótica e evolução do bioma! Caminhamos por todas as amostras do array de DNA e roletamos perante o limite imposto pela `MUTATION_RATE`. Aquilo que tiver "sorte": fazemos a inversão bool matemática booleana. Onde não passava a se ver uma faca se adicionou pra ver como o indivíduo sobrevive e pontua, tudo visando escapar do terrível Ótimo Local matemático.

### Funções `pegar_melhor_individuo` e `copiar_individuo`
Procedimentos cruciais em C para fazer "Bypass". Eles não deixam referências se reescreverem em lugares proibidos, garantem que quem transfere pra nova geração consiga reescrever ponteiros limpos de fato, e pegam isoladamente na lupa os maiores desempenhos com loopings fixos de checagem.

---

## 6. A Leitura e o Main Loop

### `ler_entrada(nome_arquivo)`
Usa função embutida `<stdio.h>` de manipulação por buffer. As rotinas `fscanf(%s %d, buffer, variavel)` sugam e limpam do contexto do leitor qualquer palavra usada no arquivo, e enviam a parte numérica para salvar na variável apropriada internamente pra usar a `CAPACIDADE` / `NUM_ITEMS` limpas antes de varrer propriamente alocando na `vitrine` inteira usando outra varredura.

### O Arquétipo do Script  (`main()`)
1. Liga conferindo se foram informados todos argumentos (`argv`).
2. Semeia algoritmos de sorte baseado no número atômico do relógio pra garantir diferentes matrizes de caos a cada "Run" ou rodar de console (`srand(time(NULL))`).
3. Dispara a captura de Time Stamp Inicial.
4. Gera e distribui alocação pros grupos biossociais ("Velha" vs "Nova" `nova_populacao`). 
5. Repete no Processo:
    - Uso primordial do Conceito de **Elitismo**: Transfere invicto e garantido a cópia do maior campeão em um berço preferencial Index Zero de quem será julgado para ele nunca sofrer tragédias.
    - O Loop Principal recruta competidores pelo Torneio Cíclico; efetua os milagres (cruzamento com o *crossover* entre heróis, desgraças e percursos da mutação randômica por chance) e finaliza o ciclo transferindo-os.
    - Por fim sobreescreve formalmente o mapa do bioma velho pelo da evolução novinha e dá um passe na Geração atual testada.
6. Fecha os Loops! Recupera informações sobre tempo exato. Dá o balanço oficial da resposta montando em loop visual a estrutura final (`0 1 0 0 1 ...`) dos achismos com o DNA, limpa todos vestígios de lixo na máquina com coletas `free()`, terminando a sessão.
