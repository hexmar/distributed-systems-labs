#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

struct E
{
  int vert[2], weight;
  struct E *next[2], *prev[2];
};
typedef struct E EDGE;

struct G
{
  char **vertices;
  int size, tree_size, *tree;
  EDGE **edges, **tree_edges;
};
typedef struct G GRAPH;

struct A
{
  GRAPH *graph;
  int thread_number, *cores, *row;
  unsigned int *edges_count;
  pthread_barrier_t *barrier;
  EDGE **min, ***edges;
};
typedef struct A ARG;

struct F { EDGE *edge; struct F *next; };
typedef struct F FE;

int VertToInt (GRAPH *, char *);
void *FindMin (void *);
void AddEdgeToFE (FE *, EDGE *);

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("Invalid number of arguments. Two or three arguments are required.\n");
      exit (EXIT_FAILURE);
    }
  char a, edge_title[8];
  int i, j, cores = atoi (argv[1]), row = -1;
  struct timeval start_time, stop_time;
  start_time.tv_sec = stop_time.tv_sec = start_time.tv_usec = stop_time.tv_usec = 0;
  for (i = 0; i < 8; i++)
    edge_title[i] = 0;
  FILE *file_input;
  if (argc == 3)
    file_input = fopen (argv[2], "r");
  else
    file_input = fopen ("input", "r");
  j = 0;
  do
    {
      fscanf (file_input, "%c", &a);
      if ((a == 10) || (a == ' '))
        j++;
    }
  while (a != 10);
  rewind (file_input);
  /* Graph init */
  GRAPH graph;
  graph.size = j;
  graph.vertices = (char **) malloc (graph.size * sizeof (char *));
  graph.tree = (int *) calloc (graph.size, sizeof (int));
  graph.tree_edges = (EDGE **) malloc ((graph.size - 1) * sizeof (EDGE *));
  graph.edges = (EDGE **) calloc (graph.size, sizeof (EDGE *));
  /* Graph.vertices init */
  graph.vertices[0] = (char *) calloc (8, sizeof (char));
  i = j = 0;
  do
    {
      fscanf (file_input, "%c", &a);
      if (a == ' ')
        {
          i = 0;
          graph.vertices[++j] = (char *) calloc (8, sizeof (char));
        }
      else
        {
          if (a != 10)
            graph.vertices[j][i++] = a;
        }
    }
  while (a != 10);
  /* Graph.edges init */
  int file_rows = 0;
  do
    {
      fscanf (file_input, "%c", &a);
      if (a != 10)
        file_rows = file_rows * 10 - 48 + a;
    }
  while (a != 10);
  EDGE *edge;
  do
    {
      edge = (EDGE *) malloc (sizeof (EDGE));
      i = 0;
      do
        {
          fscanf (file_input, "%c", &a);
          if (a != ' ')
            edge_title[i++] = a;
        }
      while (a != ' ');
      edge->vert[0] = VertToInt (&graph, edge_title);
      i = 0;
      do
        {
          fscanf (file_input, "%c", &a);
          if (a != ' ')
            edge_title[i++] = a;
        }
      while (a != ' ');
      edge->vert[1] = VertToInt (&graph, edge_title);
      edge->weight = 0;
      do
        {
          fscanf (file_input, "%c", &a);
          if (a != 10)
            edge->weight = edge->weight * 10 - 48 + a;
        }
      while (a != 10);
      for (j = 0; j < 2; j++)
        {
          edge->next[j] = NULL;
          if (graph.edges[edge->vert[j]] == NULL)
            {
              graph.edges[edge->vert[j]] = edge;
              edge->prev[j] = NULL;
            }
          else 
            {
              EDGE *iter = graph.edges[edge->vert[j]];
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
  while (--file_rows);
  fclose (file_input);
  unsigned int *edges_in_row = (unsigned int*) calloc (graph.size, sizeof (unsigned int));
  EDGE ***edges_array = (EDGE ***) calloc (graph.size, sizeof (EDGE **));
  for (i = 0; i < graph.size; i++)
    {
      EDGE *iter = graph.edges[i];
      while (iter != NULL)
        {
          if (iter->vert[0] == i)
            iter = iter->next[0];
          else
            iter = iter->next[1];
          edges_in_row[i] += 1;
        }
      edges_array[i] = (EDGE **) calloc (edges_in_row[i], sizeof (EDGE *));
      iter = graph.edges[i];
      for (j = 0; j < edges_in_row[i]; j++)
        {
          edges_array[i][j] = iter;
          if (iter->vert[0] == i)
            iter = iter->next[0];
          else
            iter = iter->next[1];
        }
    }
  ARG **args = (ARG **) calloc (cores, sizeof (ARG *));
  EDGE **mins = (EDGE **) calloc (cores, sizeof (EDGE *));
  pthread_t *thread = (pthread_t *) malloc (cores * sizeof (pthread_t));
  pthread_attr_t p_attr;
  pthread_attr_init (&p_attr);
  pthread_attr_setdetachstate (&p_attr, PTHREAD_CREATE_DETACHED);
  pthread_barrier_t barrier;
  pthread_barrier_init (&barrier, NULL, cores + 1);
  for (i = 0; i < cores; i++)
    {
      args[i] = (ARG *) malloc (sizeof (ARG));
      args[i]->graph = &graph;
      args[i]->min = mins;
      args[i]->thread_number = i;
      args[i]->cores = &cores;
      args[i]->barrier = &barrier;
      args[i]->edges = edges_array;
      args[i]->edges_count = edges_in_row;
      args[i]->row = &row;
      pthread_create (&(thread[i]), &p_attr, &FindMin, args[i]);
    }
  gettimeofday (&start_time, NULL);
  graph.tree[0] = 0;
  graph.tree_size = 1;
  while (graph.tree_size < graph.size) 
    {
      if (graph.tree_size > 1)
        {
          /* Delete edges to tree vertices from new tree vertice */
          row = i = graph.tree[graph.tree_size - 1];
          pthread_barrier_wait (&barrier);
          pthread_barrier_wait (&barrier);
          row = -1;
          /* Move from right to NULLs */
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
                    }
                  while ((edges_array[i][j + move] == NULL) && (edges_in_row[i] > j));
                  edges_array[i][j] = edges_array[i][j + move];
                }
            }
        }
      /* Start threads */
      pthread_barrier_wait (&barrier);
      /* Wait for result */
      pthread_barrier_wait (&barrier);
      EDGE *min = NULL;
      for (j = 0; j < cores; j++)
        if (mins[j] != NULL)
          {
            if (min == NULL)
              min = mins[j];
            else if (mins[j]->weight < min->weight)
              min = mins[j];
          }
      /* Add vertice to graph.tree */
      for (j = 0; j < graph.tree_size; j++)
        if (graph.tree[j] == min->vert[0])
          graph.tree[j = graph.tree_size] = min->vert[1];
        else if (graph.tree[j] == min->vert[1])
          graph.tree[j = graph.tree_size] = min->vert[0];
      /* Add edge to graph.tree_edges */
      graph.tree_edges[graph.tree_size - 1] = min;
      graph.tree_size++;
    }
  gettimeofday (&stop_time, NULL);
  FILE *file_output;
  file_output = fopen ("output", "w");
  fprintf (file_output, "%s\n", graph.vertices[graph.tree[0]]);
  for (i = 1; i < graph.tree_size; i++)
    {
      fprintf (file_output, "%s - %s %s %d\n",
              graph.vertices[graph.tree[i]],
              graph.vertices[graph.tree_edges[i - 1]->vert[0]],
              graph.vertices[graph.tree_edges[i - 1]->vert[1]],
              graph.tree_edges[i - 1]->weight);
    }
  fclose (file_output);
  long long result_time = (stop_time.tv_sec - start_time.tv_sec) * 1e6
                          + (stop_time.tv_usec - start_time.tv_usec);
  printf ("\n%lld milliseconds\n", result_time);
  cores = 0;
  pthread_barrier_wait (&barrier);
  pthread_barrier_destroy (&barrier);
  pthread_attr_destroy (&p_attr);
  free (thread);
  free (mins);
  for (i = 0; i < graph.size; i++)
    free (edges_array[i]);
  free (edges_array);
  free (edges_in_row);
  free (graph.tree);
  free (graph.tree_edges);
  FE *fe = NULL;
  for (i = 0; i < graph.size; i++)
    {
      edge = graph.edges[i];
      while (edge != NULL)
        {
          AddEdgeToFE(fe, edge);
          edge = edge->vert[0] == i ? edge->next[0] : edge->next[1];
        }
    }
  while (fe != NULL)
    {
      FE *rem = fe;
      free (fe->edge);
      fe = fe->next;
      free (rem);
    }
  for (i = 0; i < graph.size; i++)
    free (graph.vertices[i]);
  free (graph.vertices);
  free (graph.edges);
  return 0;
}

int
VertToInt (GRAPH *graph, char *vert)
{
  int i, j, b;
  for (i = 0; i < graph->size; i++)
    {
      b = 1;
      for (j = 0; b && (vert[j] != 0) && (j < 8); j++)
        if (vert[j] != graph->vertices[i][j])
          b = 0;
      if (b)
        return i;
    }
  return -1;
}

void *
FindMin (void *Arg)
{
  ARG *arg = (ARG *) Arg;
  int row;
  pthread_barrier_wait (arg->barrier);
  do
    {
      if (*arg->row == -1)
        {
          arg->min[arg->thread_number] = NULL;
          for (int j = arg->thread_number; j < arg->graph->tree_size; j += *arg->cores)
            {
              row = arg->graph->tree[j];
              for (int k = 0; k < arg->edges_count[row]; k++)
                if (arg->min[arg->thread_number] == NULL)
                  arg->min[arg->thread_number] = arg->edges[row][k];
                else if (arg->min[arg->thread_number]->weight > arg->edges[row][k]->weight)
                  arg->min[arg->thread_number] = arg->edges[row][k];
            }
        }
      else
        {
          for (int j = arg->thread_number; j < arg->edges_count[*arg->row]; j += *arg->cores)
            for (int k = 0; k < (arg->graph->tree_size - 1); k++)
              {
                int vie = arg->edges[*arg->row][j]->vert[0]
                          == arg->graph->tree[arg->graph->tree_size - 1] ? 1 : 0;
                if (arg->edges[*arg->row][j]->vert[vie] == arg->graph->tree[k])
                  {
                    int move = 0, tree_row = arg->edges[*arg->row][j]->vert[vie];
                    for (int l = 0; l < arg->edges_count[tree_row]; l++)
                      if (move)
                        arg->edges[tree_row][l] = arg->edges[tree_row][l + 1];
                      else if (arg->edges[tree_row][l] == arg->edges[*arg->row][j])
                        {
                          move = 1;
                          arg->edges[tree_row][l] = arg->edges[tree_row][l + 1];
                          arg->edges_count[tree_row]--;
                        }
                    arg->edges[*arg->row][j] = NULL;
                    break;
                  }
              }
        }
      pthread_barrier_wait (arg->barrier);
      pthread_barrier_wait (arg->barrier);
    }
  while (*arg->cores);
  free (arg);
  return NULL;
}

void
AddEdgeToFE (FE *fe, EDGE *edge)
{
  if (fe == NULL)
    {
      fe = (FE *) malloc (sizeof (FE));
      fe->edge = edge;
      fe->next = NULL;
    }
  else
    {
      FE *iter = fe;
      int b = 1;
      for (; b && (iter->next != NULL); iter = iter->next)
        if (iter->edge == edge)
          b = 0;
      if (b == 1)
        {
          iter = iter->next = (FE *) malloc (sizeof (FE));
          iter->edge = edge;
          iter->next = NULL;
        }
    }
}
