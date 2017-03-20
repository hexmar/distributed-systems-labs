#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct E {
	int nodes[2], weight;
	struct E *next;
} EDGE;

int main(int argc, char **argv)
{
	// TODO: Test argc == 3
	if (argc != 3)
		{
			puts("Error! Usage: ./gen <Nodes count> <Output file>");
			return 1;
		}
	int count = atoi(argv[1]), name_length = 0;
		{
			unsigned int j = 10;
			for (int i = 1; name_length == 0; i++)
				{
					if (count < j)
						name_length = i + 1;
					else
						j *= 10;
				}
		}
	char **nodes = (char**)malloc(count * sizeof(char*));
	for (int i = 0; i < count; i++)
		{
			nodes[i] = (char*)malloc(name_length * sizeof(char));
			nodes[i][0] = 'N';
			int j = i + 1;
			for (int k = name_length - 1; k > 0; k--)
				{
					nodes[i][k] = j % 10 + '0';
					j /= 10;
				}
		}
	FILE *file = fopen(argv[2], "w");
	for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < name_length; j++)
				fputc(nodes[i][j], file);
			if (i == (count - 1))
				fputc(10, file);
			else
				fputc(' ', file);
		}
	srand(time(NULL));
	EDGE *start = NULL, *end = NULL;
	int edge_count = 0;
	for (int i = 0; i < count; i++)
		{
			if (start == NULL)
				start = end = (EDGE*)malloc(sizeof(EDGE));
			else
				end = end->next = (EDGE*)malloc(sizeof(EDGE));
			edge_count++;
			end->nodes[0] = i;
			do
				{
					end->nodes[1] = rand() % count;
				}
			while (end->nodes[1] == i);
			end->weight = rand() % 99 + 1;
			end = end->next = (EDGE*)malloc(sizeof(EDGE));
			edge_count++;
			end->next = NULL;
			end->nodes[0] = i;
			do
				{
					end->nodes[1] = rand() % count;
				}
			while (end->nodes[1] == i);
			end->weight = rand() % 99 + 1;
		}
	fprintf(file, "%d\n", edge_count);
	for (EDGE *iter = start; iter != NULL; iter = iter->next)
		{
			for (int i = 0; i < name_length; i++)
				fputc(nodes[iter->nodes[0]][i], file);
			fputc(' ', file);
			for (int i = 0; i < name_length; i++)
				fputc(nodes[iter->nodes[1]][i], file);
			fputc(' ', file);
			fprintf(file, "%d", iter->weight);
			fputc(10, file);
		}
	fclose(file);
	for (; start != NULL; start = start->next)
		free(start);
	for (int i = 0; i < count; i++)
		free(nodes[i]);
	free(nodes);
	return 0;
}

