# Lab 1
Realizes ***Prim's algorithm*** that finds minimum spanning tree for a weighted undirected graph.
## Compile
- main.c: `gcc -o "main" "main.c" -lrt -pthread`
- gen.c: `gcc -o "gen" "gen.c"`
## Run
1. `./gen 25000 input` - Generates graph that consist of 25000 nodes and 50000 edges and writes it to 'input'
1. `./main 1` - 1 thread
1. `./main 2` - 2 threads
1. ...
