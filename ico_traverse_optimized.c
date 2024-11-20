#include <stdio.h>

#define NUM_VERTICES_VISITED 36
#define NUM_EDGES_VISITED 35 // (NUM_VERTICES_VISITED - 1) since edge_n is connected to vertex_n and vertex_n+1
#define START_NODE 0
#define VERTEX_COUNT 12
#define EDGES_PER_VERTEX 5
#define EDGE_COUNT 30 // (VERTEX_COUNT * EDGES_PER_VERTEX / VERTICES_PER_EDGE) vertices per edge is always 2
#define MEDIAN_NODE 17 // (NUM_VERTICES_VISITED / 2 - 1) the edge connecting vertex 17 and vertex 18 is the exact midpoint of the path

//#define DEBUG_PRINT_REGULAR_UPDATES

/// the first index represents one of the vertices of the icosahedron (0-11)
/// the second index represents one of the 5 edges connected to that vertex (0-5)
/// the value stored represents which vertex lies at the other end of the indicated edge
const int vertexMap[VERTEX_COUNT][EDGES_PER_VERTEX] =
    {
        {3, 5, 6, 8, 9},   // 0
        {6, 7, 8, 10, 11}, // 1
        {3, 4, 7, 9, 11},  // 2
        {0, 2, 4, 5, 9},   // 3
        {2, 3, 5, 10, 11}, // 4
        {0, 3, 4, 8, 10},  // 5
        {0, 1, 7, 8, 9},   // 6
        {1, 2, 6, 9, 11},  // 7
        {0, 1, 5, 6, 10},  // 8
        {0, 2, 3, 6, 7},   // 9
        {1, 4, 5, 8, 11},  // 10
        {1, 2, 4, 7, 10}   // 11
    };

/// there are 30 edges on an icosahedron, each edge has an entry in this table
/// each entry in the table consists of the two vertices that lie on either end of that edge
/// this array is not used in this code, but was used to generate edgeLookup
const int edgeList[EDGE_COUNT][2] =
    {
        {0, 3},
        {0, 5},
        {0, 6},
        {0, 8},
        {0, 9},
        {1, 6},
        {1, 7},
        {1, 8},
        {1, 10},
        {1, 11},
        {2, 3},
        {2, 4},
        {2, 7},
        {2, 9},
        {2, 11},
        {3, 4},
        {3, 5},
        {3, 9},
        {4, 5},
        {4, 10},
        {4, 11},
        {5, 8},
        {5, 10},
        {6, 7},
        {6, 8},
        {6, 9},
        {7, 9},
        {7, 11},
        {8, 10},
        {10, 11},
    };

/// each index represents a vertex on the icosahedron (0-11)
/// if an edge connects those vertices, then the value stored at the appropriate cell will be the index of the edge (0-29)
/// if no edge connects those vertices, then the value stored at the appropriate cell will be -1
/// array is symmetric about the line index1 == index2 ensuring there is no need to 'sort' vertices before addressing into the array
const int edgeLookup[VERTEX_COUNT][VERTEX_COUNT] =
	{
		{-1, -1, -1, 0, -1, 1, 2, -1, 3, 4, -1, -1},
		{-1, -1, -1, -1, -1, -1, 5, 6, 7, -1, 8, 9},
		{-1, -1, -1, 10, 11, -1, -1, 12, -1, 13, -1, 14},
		{0, -1, 10, -1, 15, 16, -1, -1, -1, 17, -1, -1},
		{-1, -1, 11, 15, -1, 18, -1, -1, -1, -1, 19, 20},
		{1, -1, -1, 16, 18, -1, -1, -1, 21, -1, 22, -1},
		{2, 5, -1, -1, -1, -1, -1, 23, 24, 25, -1, -1},
		{-1, 6, 12, -1, -1, -1, 23, -1, -1, 26, -1, 27},
		{3, 7, -1, -1, -1, 21, 24, -1, -1, -1, 28, -1},
		{4, -1, 13, 17, -1, -1, 25, 26, -1, -1, -1, -1},
		{-1, 8, -1, -1, 19, 22, -1, -1, 28, -1, -1, 29},
		{-1, 9, 14, -1, 20, -1, -1, 27, -1, -1, 29, -1}
	};

int vertex_choices[NUM_EDGES_VISITED]; // each choice is an index into vertexMap, which can get the next node in the path
int vertex_order[NUM_VERTICES_VISITED]; // generated from vertex_choices, yields each vertex visited by the path in order
int times_vertices_visited[VERTEX_COUNT]; // how many times is a vertex visited along the path (vertices have 5 connecting edges, so must be visited at least 3 times to catch all)
int times_edges_visited[EDGE_COUNT]; // how many times is an edge visited along the path

static inline void clearPath()
{
	for (int i = 0; i < NUM_EDGES_VISITED; i++)
	{
		vertex_choices[i] = 0;
	}
}

static inline void clearVisited()
{
	for (int i = 0; i < VERTEX_COUNT; i++)
	{
		times_vertices_visited[i] = 0;
	}
	for (int j = 0; j < EDGE_COUNT; j++)
	{
		times_edges_visited[j] = 0;
	}
}


/// returns 1 if the path was able to be incremented. 0 if there was 'overflow'
static inline int incrementPath()
{
	int able_to_inc = 0;
	// following the method of counting in a base EDGES_PER_VERTEX number:
	// - increment the least significant digit you can (digits == (EDGES_PER_VERTEX - 1) cannot be incremented)
	// - reset all below it to 0
	for (int i = (NUM_EDGES_VISITED - 1); i >= 0; i--)
	{
		if (vertex_choices[i] < (EDGES_PER_VERTEX - 1))
		{
			able_to_inc = 1;
			vertex_choices[i]++; // increment the least significant digit you can
			for (int j = i + 1; j < NUM_EDGES_VISITED; j++)
			{
				vertex_choices[j] = 0; // reset all below it to 0
			}
			
			// generate vertex_order from vertex_choices
			vertex_order[0] = START_NODE;
			for (int k = 0; k < NUM_EDGES_VISITED; k++)
			{
				// NUM_VERTICES_VISITED = NUM_EDGES_VISITED + 1, meaning vertex_order[k + 1] will always be a valid array entry
				vertex_order[k + 1] = vertexMap[vertex_order[k]][vertex_choices[k]];
			}
			break;
		}
	}
	return able_to_inc;
}

/// if the previous choice leads to an invalid arrangement, set all following choices to their max
/// ensures the next time incrementPath() is called, it will advance past the invalid choice
static inline void pruneChoices(int node)
{
	for (int i = node; i < NUM_EDGES_VISITED; i++)
	{
		vertex_choices[i] = (EDGES_PER_VERTEX - 1);
	}
}

static inline void printPath()
{
	printf("\r\n\r\n");
	for (int i = 0; i < NUM_VERTICES_VISITED; i++)
	{
		printf("%d, ", vertex_order[i]);
	}
	printf("\r\n");
}

/// follow the path from start to finish denoted by vertex_order
/// check for invalid choices along the way and prune them
/// check if all edges were visited
/// check if median node is a 'revisited' edge
static void checkPath()
{
	int next_node;
	int next_edge;
	int current_node = vertex_order[0];
	clearVisited();
	times_vertices_visited[current_node]++;
	for (int a = 1; a < NUM_VERTICES_VISITED; a++)
	{
		current_node = vertex_order[a - 1];
		next_node = vertex_order[a];
		times_vertices_visited[next_node]++;
		next_edge = edgeLookup[current_node][next_node];
		times_edges_visited[next_edge]++;
		/// it is guaranteed that the ideal path visits every vertex exactly three times
		/// since each time an edge is traversed, both vertices are visited, a 3rd revisit to an edge guarantees an unideal path:
		/// imagine entering a vertex via edge 0 and leaving via edge 1
		/// some time later, the path enters again via edge 0 and leaves via edge 2
		/// some time later, again, the path enters via edge 0 and leaves via edge 3
		/// the vertex has now been visited 3 times, but edges 4 and 5 have not been visited, requiring >= 1 additional visit, creating an unideal path
		if (times_vertices_visited[next_node] > 3 || times_edges_visited[next_edge] > 2)
		{
			pruneChoices(a);
			return;
		}
	}
	// analyze
	// ensure all edges are visited at least once
	for (int j = 0; j < EDGE_COUNT; j++)
	{
		if (times_edges_visited[j] == 0)
		{
			return;
		}
	}
	// ensure the median edge is one that has been revisited
	// the leads to the LED strips are long enough to start at adjacent vertices
	// cutting the median node out means the paths on either side of the median will be equal length
	// ensuring the median edge is revisited means cutting the median node out will not 'abandon' the corresponding edge
	current_node = vertex_order[MEDIAN_NODE];
	next_node = vertex_order[MEDIAN_NODE + 1];
	next_edge = edgeLookup[current_node][next_node];
	if (times_edges_visited[next_edge] > 1)
	{
		printPath();
	}
}

void main()
{
	#ifdef DEBUG_PRINT_REGULAR_UPDATES
	int cycle_count = 0;
	int meta_count = 0;
	int best_lowest = NUM_EDGES_VISITED;
	#endif
	clearPath();
	while (incrementPath() > 0)
	{
		checkPath();
		#ifdef DEBUG_PRINT_REGULAR_UPDATES
		cycle_count = (cycle_count + 1) % 10000000;
		if (cycle_count == 0)
		{
			meta_count = (meta_count + 1) % 1000;
			for (int i = 0; i < NUM_EDGES_VISITED; i++)
			{
				if (vertex_choices[i] > 2)
				{
					if (i < best_lowest)
					{
						best_lowest = i;
					}
					printf("\r\nlowest 3+: %d -- best: %d -- meta: %d\r\n", i, best_lowest, meta_count);
					break;
				}
			}
		}
		#endif
	}
}