#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct E {
	char vert[2];
	struct E *next[2], *prev[2];
	int weight;
} EDGE;

typedef struct G {
	char *vertices, *tree;
	int **weights, size, treeSize;
	EDGE **edges;
} GRAPH;

typedef struct A {
	GRAPH *graph;
	int threadNumber, *min;
	pthread_barrier_t *barrier;
	sem_t *semaphore;
} ARG;

int VertToInt(GRAPH*, char);
void *FindMin(void*);

int main(int argc, char **argv)
{
	char a[256];
	int i;
	for (i = 0; i < 256; i++)
		a[i] = 0;
	if (argc != 2) {
		printf("Invalid number of arguments. Two arguments needed.\n");
		exit(EXIT_FAILURE);
	}
	int cores = atoi(argv[1]);
	FILE *fIn = fopen("input", "r");
	for (i = 0; feof(fIn) == 0; i++)
		fscanf(fIn, "%c", &(a[i]));
/*	for (i = 0; i < 256; i++)
		printf("%d ", a[i]);*/
	GRAPH graph;
	for (i = 0; a[i] != 10; i++);
	graph.size = i;
	graph.vertices = (char*)calloc(i, sizeof(char));
	graph.tree = (char*)calloc(i, sizeof(char));
	for (int j = 0; j < i; j++)
		graph.vertices[j] = a[j];
	graph.edges = (EDGE**)malloc(i * sizeof(EDGE*));
	int row, col;
	EDGE *edge;
	while (a[i + 1] != 0) {
		edge = (EDGE*)malloc(sizeof(EDGE));
		edge.vert[0] = a[++i];
		edge.vert[1] = a[++i];
		i += 2;
		while (a[i] != 10)
			edge.weight = edge.weight * 10 - 48 + a[i++];
		for (int j = 0; j < graph.size; j++) {
			for (int k = 0; k < graph.size; k++)
				printf("%d ", graph.weights[j][k]);
			putchar('\n');
		}
		putchar('\n');
	}
	ARG *arg;
/*	ARG *arg = (ARG*)malloc(sizeof(ARG));
	pthread_t thread[cores];
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, 5);*/
	graph.tree[0] = graph.vertices[0];
	graph.treeSize = 1;
	int *mins;
	pthread_t *thread;
	sem_t *semaphore = (sem_t*)malloc(sizeof(sem_t));
	while (graph.treeSize < graph.size) {
		if (cores <= graph.treeSize) {
			sem_init(semaphore, 0, cores);
			mins = (int*)calloc(graph.treeSize, sizeof(int));
			thread = (pthread_t*)malloc(graph.treeSize * sizeof(pthread_t));
			for (i = 0; i < graph.treeSize; i++) {
				arg = (ARG*)malloc(sizeof(ARG));
				arg->graph = &graph;
				arg->min = mins;
				arg->barrier = NULL;
				arg->semaphore = semaphore;
				arg->threadNumber = i;
				sem_wait(semaphore);
				pthread_create(&(thread[i]), NULL, &FindMin, arg);
			}
			do {
				sem_getvalue(semaphore, &i);
			} while (i != cores);
			sem_destroy(semaphore);
			free(thread);
		} else { // cores >= treeSize
			int div = cores / graph.treeSize;
			int mod = cores % graph.treeSize;
			mins = (int*)calloc(cores, sizeof(int));
			thread = (pthread_t*)malloc(cores * sizeof(pthread_t));
			for (i = 0; i < cores; i++) {
				arg = (ARG*)malloc(sizeof(ARG));
				arg->graph = &graph;
				arg->min = mins;
				arg->barrier = NULL;
				arg->semaphore = NULL;
				arg->threadNumber = i;
				pthread_create(&(thread[i]), NULL, &FindMin, arg);
			}
		}
		i = mins[0];
		free(mins);
		break;
	}
	printf("Min A=%d\n", i);
	return 0;
}

int VertToInt(GRAPH *Graph, char Vert) {
	int i;
	for (i = 0; i < Graph->size; i++)
		if (Graph->vertices[i] == Vert)
			return i;
	return -1;
}

void *FindMin(void *Arg) {
	ARG *arg = (ARG*)Arg;
	int row = VertToInt(arg->graph, arg->graph->tree[arg->threadNumber]), flag;
	int *excludes = (int*)malloc(arg->graph->treeSize * sizeof(int));
	for (int i = 0; i < arg->graph->treeSize; i++)
		excludes[i] = VertToInt(arg->graph, arg->graph->tree[i]);
	for (int i = 0; i < arg->graph->size; i++) {
		flag = 1;
		for (int j = 0; (j < arg->graph->treeSize) && flag; j++)
			if (excludes[j] == i)
				flag = 0;
		if ((arg->graph->weights[row][i] != 0) && flag)
			if ((arg->min[arg->threadNumber] == 0) || 
				(arg->min[arg->threadNumber] > arg->graph->weights[row][i]))
				arg->min[arg->threadNumber] = arg->graph->weights[row][i];
	}
	sem_post(arg->semaphore);
	free(arg);
	return NULL;
}
