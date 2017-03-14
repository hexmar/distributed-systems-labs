#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct E {
	char vert[2];
	struct E *next[2], *prev[2];
	int weight;
} EDGE;

typedef struct G {
	char *vertices, *tree;
	int size, treeSize;
	EDGE **edges, **tree_edges;
} GRAPH;

typedef struct A {
	GRAPH *graph;
	int thread_number, *cores;
	unsigned int *edges_count;
	pthread_barrier_t *barrier;
	EDGE **min, ***edges;
} ARG;

int VertToInt(GRAPH*, char);
void *FindMin(void*);

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Invalid number of arguments. Two or three arguments are required.\n");
		exit(EXIT_FAILURE);
	}
	char a[1024];
	int i, j, cores = atoi(argv[1]);
	{
		FILE *file_input;
		if (argc == 3)
			file_input = fopen(argv[2], "r");
		else
			file_input = fopen("input", "r");
		for (i = 0; i < 1024; i++)
			a[i] = 0;
		for (i = 0; feof(file_input) == 0; i++)
			fscanf(file_input, "%c", &(a[i]));
		fclose(file_input);
	}
	GRAPH graph;
	for (i = 0; a[i] != 10; i++);
	graph.size = i;
	graph.vertices = (char*)calloc(i, sizeof(char));
	graph.tree = (char*)calloc(i, sizeof(char));
	graph.tree_edges = (EDGE**)malloc((i - 1) * sizeof(EDGE*));
	for (int j = 0; j < i; j++)
		graph.vertices[j] = a[j];
	graph.edges = (EDGE**)calloc(i, sizeof(EDGE*));
	EDGE *edge;
	while (a[i + 1] != 0) {
		edge = (EDGE*)malloc(sizeof(EDGE));
		edge->vert[0] = a[++i];
		edge->vert[1] = a[++i];
		i += 2;
		edge->weight = 0;
		while (a[i] != 10)
			edge->weight = edge->weight * 10 - 48 + a[i++];
		for (int j = 0; j < 2; j++) {
			edge->next[j] = NULL;
			int vert = VertToInt(&graph, edge->vert[j]);
			if (graph.edges[vert] == NULL) {
				graph.edges[vert] = edge;
				edge->prev[j] = NULL;
			}
			else {
				EDGE *iter = graph.edges[vert];
				int vie = iter->vert[0] == edge->vert[j] ? 0 : 1;
				while (iter->next[vie] != NULL)
				{
					iter = iter->next[vie];
					vie = iter->vert[0] == edge->vert[j] ? 0 : 1;
				}
				iter->next[vie] = edge;
				edge->prev[j] = iter;
			}
		}
	}
	unsigned int *edges_in_row = (unsigned int*)calloc(graph.size, sizeof(unsigned int));
	EDGE ***edges_array = (EDGE***)calloc(graph.size, sizeof(EDGE**));
	for (i = 0; i < graph.size; i++)
	{
		EDGE *iter = graph.edges[i];
		while (iter != NULL)
		{
			if (iter->vert[0] == graph.vertices[i])
				iter = iter->next[0];
			else
				iter = iter->next[1];
			edges_in_row[i] += 1;
		}
		edges_array[i] = (EDGE**)calloc(edges_in_row[i], sizeof(EDGE*));
		iter = graph.edges[i];
		for (int j = 0; j < edges_in_row[i]; j++)
		{
			edges_array[i][j] = iter;
			if (iter->vert[0] == graph.vertices[i])
				iter = iter->next[0];
			else
				iter = iter->next[1];
		}
	}
	ARG *arg;
	/*	ARG *arg = (ARG*)malloc(sizeof(ARG));
	pthread_t thread[cores];
	pthread_barrier_init(&barrier, NULL, 5);*/
	graph.tree[0] = graph.vertices[0];
	graph.treeSize = 1;
	EDGE **mins;
	pthread_t *thread;
	pthread_barrier_t barrier;
	while (graph.treeSize < graph.size) {
		mins = (EDGE**)calloc(cores, sizeof(EDGE*));
		thread = (pthread_t*)malloc(cores * sizeof(pthread_t));
		pthread_barrier_init(&barrier, NULL, cores + 1);
		for (i = 0; i < cores; i++) {
			arg = (ARG*)malloc(sizeof(ARG));
			arg->graph = &graph;
			arg->min = mins;
			arg->thread_number = i;
			arg->cores = &cores;
			arg->barrier = &barrier;
			arg->edges = edges_array;
			arg->edges_count = edges_in_row;
			pthread_create(&(thread[i]), NULL, &FindMin, arg);
		}
		pthread_barrier_wait(&barrier);
		pthread_barrier_destroy(&barrier);
		EDGE *min = NULL;
		for (j = 0; j < cores; j++)
			if (mins[j] != NULL)
				{
					if (min == NULL)
						min = mins[j];
					else if (mins[j]->weight < min->weight)
						min = mins[j];
				}
		//printf("\n%d\n", min->weight);
		/* Delete tree edges */
		for (j = 0; j < graph.treeSize; j++)
			if (graph.tree[j] == min->vert[0])
			{
				graph.tree[graph.treeSize] = min->vert[1];
				break;
			}
			else if (graph.tree[j] == min->vert[1])
			{
				graph.tree[graph.treeSize] = min->vert[0];
				break;
			}
		graph.tree_edges[graph.treeSize - 1] = min;
		i = VertToInt(&graph, graph.tree[graph.treeSize]);
		for (j = 0; j < edges_in_row[i]; j++)
		{
			for (int k = 0; k < graph.treeSize; k++)
			{
				int vie = edges_array[i][j]->vert[0] == graph.tree[graph.treeSize] ? 1 : 0;
				if (edges_array[i][j]->vert[vie] == graph.tree[k])
				{
					int move = 0, tree_row = VertToInt(&graph, edges_array[i][j]->vert[vie]);
					for (int l = 0; l < edges_in_row[tree_row]; l++)
					{
						if (move)
							edges_array[tree_row][l] = edges_array[tree_row][l + 1];
						else if (edges_array[tree_row][l] == edges_array[i][j])
						{
							move = 1;
							edges_array[tree_row][l] = edges_array[tree_row][l + 1];
							edges_in_row[tree_row]--;
						}
					}
					edges_array[i][j] = NULL;
					break;
				}
			}
		}
		graph.treeSize++;
		j = 0;
		for (int move = 0; j < edges_in_row[i]; j++)
		{
			if (move)
				edges_array[i][j] = edges_array[i][j + move];
			if (edges_array[i][j] == NULL)
			{
				do
				{
					move++;
					edges_in_row[i]--;
				} while ((edges_array[i][j + move] == NULL) && (edges_in_row[i] > j));
				edges_array[i][j] = edges_array[i][j + move];
			}
		}
	}
	printf("\n%c\n", graph.tree[0]);
	for (i = 1; i < graph.treeSize; i++)
	{
		printf("%c - %c%c %d\n",
			graph.tree[i],
			graph.tree_edges[i - 1]->vert[0],
			graph.tree_edges[i - 1]->vert[1],
			graph.tree_edges[i - 1]->weight);
	}
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
	int step_left = arg->thread_number, vert = 0, row = VertToInt(arg->graph, arg->graph->tree[vert]);
	arg->min[arg->thread_number] = NULL;
	while (vert < arg->graph->treeSize)
	{
		if (step_left < arg->edges_count[row])
		{
			if (arg->min[arg->thread_number] == NULL)
				arg->min[arg->thread_number] = arg->edges[row][step_left];
			else if (arg->min[arg->thread_number]->weight > arg->edges[row][step_left]->weight)
				arg->min[arg->thread_number] = arg->edges[row][step_left];
			step_left += *arg->cores;
		}
		else
		{
			step_left -= arg->edges_count[row];
			vert++;
			if (vert < arg->graph->treeSize)
				row = VertToInt(arg->graph, arg->graph->tree[vert]);
		}
	}
	pthread_barrier_wait(arg->barrier);
	free(arg);
	return NULL;
}
