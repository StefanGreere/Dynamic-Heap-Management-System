#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIE(assertion, call_description)					\
do {														\
	if (assertion) {										\
		fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
		perror(call_description);							\
		exit(errno);										\
	}														\
} while (0)

typedef struct info {
	int address;	// adresa de inceput al unui bloc de memorie
	int dimension;	// dimensiune blocului de memorie
	char *s;		// sirul de caractere retinut in blocul de memorie
} info;

typedef struct dll_node_t {
	void *data;	// Pentru ca datele stocate sa poata avea orice tip
	struct dll_node_t *prev, *next;
} dll_node_t;

typedef struct dl_list_t {
	dll_node_t *head;
	int data_size;
	int size;
} dl_list_t;

typedef struct sfl {
	int nr_lists;
	int address;
	int nr_bytes;
	int type;
	int *capacity;	// retine numerul de bytes al fiecarui bloc dintr-o lista
	dl_list_t **memory;	// vectorul de liste
} sfl;

// returneaza nodul dintr-o lista de la o anumita pozitie specificata
dll_node_t *dll_get_nth_node(dl_list_t *list, int n)
{
	if (list->size == 0)
		return NULL;

	dll_node_t *current = list->head;
	n = n % list->size;
	for (int i = 0; i < n; i++)
		current = current->next;

	return current;
}

// adauga un nou nod intr-o lista la o anumita pozitie
void dll_add_nth_node(dl_list_t *list, int n, void *data)
{
	// cream noul nod
	dll_node_t *new_node = malloc(sizeof(*new_node));
	DIE(!new_node, "malloc failed...");
	new_node->data = malloc(list->data_size);
	DIE(!new_node->data, "malloc failed...");
	new_node->next = NULL;
	new_node->prev = NULL;

	memcpy(new_node->data, data, list->data_size);

	if (list->size == 0) {	// daca lista e goala
		list->head = new_node;
	} else if (n >= list->size) {	// daca inseram la final
		dll_node_t *current = dll_get_nth_node(list, list->size - 1);
		current->next = new_node;
		new_node->prev = current;
	} else if (n == 0) {	// daca inseram la prima pozitie
		dll_node_t *current = dll_get_nth_node(list, 0);
		new_node->next = current;
		current->prev = new_node;
		list->head = new_node;
	} else {	// daca inseram de la mijloc
		dll_node_t *current = dll_get_nth_node(list, n);
		new_node->next = current;
		new_node->prev = current->prev;
		current->prev->next = new_node;
		current->prev = new_node;
	}
	list->size++;
}

// sterge un nod dintr-o lista si il returneaza ca sa fie eliberat din memorie
dll_node_t *ll_remove_nth_node(dl_list_t *list, int n)
{
	dll_node_t *prev, *curr;

	if (!list || !list->head)
		return NULL;

	// stergem un nod de la finalul listei
	if (n > list->size - 1)
		n = list->size - 1;

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (!prev) {
		// adica n = 0
		list->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;
	return curr;
}

// creaza o noua lista dublu inlantuita
dl_list_t *dll_create(int data_size)
{
	dl_list_t *list = malloc(sizeof(*list));
	DIE(!list, "malloc failed...");
	list->head = NULL;
	list->data_size = data_size;
	list->size = 0;
	return list;
}

// functie care creaza vectorul de liste
dl_list_t **create_list_vector(sfl *heap)
{
	// alocam memorie pentru vectorul de liste
	dl_list_t **memory = (dl_list_t **)malloc(heap->nr_lists * sizeof(*memory));
	DIE(!memory, "malloc failed...");

	heap->capacity = (int *)malloc(heap->nr_lists * sizeof(int));
	DIE(!heap->capacity, "malloc failed...");

	int i, j, dim = 8, nr_blocks;
	nr_blocks = heap->nr_bytes / dim;	// numarul de noduri
	int adrr = heap->address;	// adresa de inceput a primei liste
	for (i = 0; i < heap->nr_lists; i++) {
		memory[i] = dll_create(sizeof(info));	// cream pe rand listele
		heap->capacity[i] = dim;	// actualizam numarul de bytes din blocuri
		for (j = 0; j < nr_blocks; j++) {
			// adaugam in fiecare lista noduri cu datele specifice
			info *node = malloc(sizeof(info));
			node->address = adrr;
			node->dimension = dim;
			node->s = NULL;
			dll_add_nth_node(memory[i], j, (void *)node);
			free(node);
			adrr = adrr + dim;
		}
		dim = dim * 2;
		nr_blocks = nr_blocks / 2;
	}
	return memory;
}

// functie care calculeaza numarul de bytes alocati
int allocated_memory(dl_list_t *list)
{
	int nr = 0;
	dll_node_t *current = list->head;
	while (current) {
		nr = nr + ((info *)current->data)->dimension;
		current = current->next;
	}
	return nr;
}

// functie care calculeaza numarul de blocuri din vectorul de liste
int blocks_number(dl_list_t **memory, int nr_lists)
{
	int i, nr = 0;
	for (i = 0; i < nr_lists; i++)
		nr = nr + memory[i]->size;
	return nr;
}

// afisam datele din structurile de date
void dump_memory_print(sfl *heap, dl_list_t *mem_alloc)
{
	// afisam pentru fiecare lista datele acesteia
	for (int i = 0; i < heap->nr_lists; i++) {
		if (heap->memory[i]->size != 0) {
			printf("Blocks with %d bytes - %d free block(s) :",
				   heap->capacity[i], heap->memory[i]->size);
			dll_node_t *current = heap->memory[i]->head;
			for (int j = 0; j < heap->memory[i]->size ; j++) {
				printf(" 0x%x", ((info *)current->data)->address);
				current = current->next;
			}
			printf("\n");
		}
	}

	// afisam datele din lista de blocuri alocate
	printf("Allocated blocks :");
	if (mem_alloc->size != 0) {
		dll_node_t *curr = mem_alloc->head;
		while (curr) {
			printf(" (0x%x - %d)", ((info *)curr->data)->address,
				   ((info *)curr->data)->dimension);
			curr = curr->next;
		}
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

// functie pentru comanda dump
void dump_print(sfl *heap, dl_list_t *mem_alloc, long long total_memory,
				int malloc_calls, int nr_fragmentation, int free_calls)
{
	// afisam datele cerute
	printf("+++++DUMP+++++\n");
	printf("Total memory: %lld bytes\n", total_memory);
	int alloc_memory = allocated_memory(mem_alloc);
	printf("Total allocated memory: %d bytes\n", alloc_memory);
	printf("Total free memory: %lld bytes\n", total_memory - alloc_memory);
	printf("Free blocks: %d\n", blocks_number(heap->memory, heap->nr_lists));
	printf("Number of allocated blocks: %d\n", mem_alloc->size);
	printf("Number of malloc calls: %d\n", malloc_calls);
	printf("Number of fragmentations: %d\n", nr_fragmentation);
	printf("Number of free calls: %d\n", free_calls);
	dump_memory_print(heap, mem_alloc);
}

/*
	Functie care cauta in vectorul de liste un bloc de memorie pentru comanda
	malloc si returneaza acel bloc din lista de index pos din vectorul de liste
	sau NULL in caz ca nu s-a gasit un bloc de memorie. Se va schimba si
	valoarea parametrului frag, in caz ca blocul se va fragmenta.
*/
dll_node_t *find_block(dl_list_t **memory, int nr_bytes, int nr_lists,
					   int *frag, int *pos, int *v)
{
	// cautam la nivel de vector, o lista de blocuri cu dimensiunea potrivita
	int i, found = 0, dim, position;
	dll_node_t *current;
	for (i = 0; i < nr_lists; i++) {
		dim = v[i];	// dimensiunea unui bloc din lista i
		if (nr_bytes > dim) {
			continue;
		} else {
			// parcurgem lista sa gasim nodul
			current = memory[i]->head;
			if (current) {
				found = 1;	// am gasit un nod
				position = i;
				break;
			}

			if (found == 0) {	// nu am gasit un block de memorie in lista
				// continuam cautarea in vector
				continue;
			}
		}
	}
	if (found == 0)
		return NULL;

	if (dim != nr_bytes)
		*frag = 1;
		// else, frag ramane 0, deci nu se fragmeneaza

	*pos = position;
	return current;
}

/*
	returneaza pozitia dintr-o lista cu adrese crescatoare, unde sa se
	introduca un nod cu o noua adresa
*/
int position_index(dl_list_t *list, int address)
{
	dll_node_t *current = list->head;
	int index = 0;
	while (current && ((info *)current->data)->address < address) {
		current = current->next;
		index++;
	}
	return index;
}

/*
	verifica daca exista o lista de dimensiune dimension in sfl si o
	returneaza in caz ca o gaseste, altfel returneaza NULL
*/
dl_list_t *exist_list(sfl *heap, int dimension)
{
	int i, dim_list;
	for (i = 0; i < heap->nr_lists; i++) {
		dim_list = heap->capacity[i];
		if (dimension == dim_list)
			return heap->memory[i];
	}
	return NULL;
}

/*
	Adauga o noua lista cu blocuri de o noua dimensiune iar aopi se ordoneaza
	tot vectorul de liste dupa dimensiunea unui bloc. Se va ordona in acelasi
	timp si vectorul care retine dimensiunea blocurilor.
*/
void add_new_list(sfl *heap, int new_addr, int new_dim)
{
	int i, j;
	heap->memory[heap->nr_lists - 1] = dll_create(sizeof(info));
	heap->capacity[heap->nr_lists - 1] = new_dim;
	// se adauga blocul cu noile date
	info *node = malloc(sizeof(info));
	DIE(!node, "malloc failed...");
	node->address = new_addr;
	node->dimension = new_dim;
	node->s = NULL;
	dll_add_nth_node(heap->memory[heap->nr_lists - 1], 0, (void *)node);
	free(node);

	// se face sortarea crescatoare
	for (i = 0; i < heap->nr_lists - 1; i++) {
		for (j = i + 1; j < heap->nr_lists; j++) {
			if (heap->capacity[i] > heap->capacity[j]) {
				dl_list_t *aux;
				aux = heap->memory[i];
				heap->memory[i] = heap->memory[j];
				heap->memory[j] = aux;

				int copy;
				copy = heap->capacity[i];
				heap->capacity[i] = heap->capacity[j];
				heap->capacity[j] = copy;
			}
		}
	}
}

// functia pentru comanda malloc
void malloc_command(sfl *heap, dl_list_t *mem_alloc, int *malloc_calls,
					int *nr_fragmentation)
{
	int frag = 0;	// presupunem ca nu se fragmenteaza
	int index = 0;

	// cautam in vectorul de liste un bloc de memorie de dimensiunea nr_bytes
	dll_node_t *mem_block = find_block(heap->memory, heap->nr_bytes,
										heap->nr_lists, &frag, &index,
										heap->capacity);
	if (!mem_block) {
		// daca nu am gasit niciun bloc se va afisa mesajul semnificativ
		printf("Out of memory\n");
	} else {
		// am gasit un bloc de memorie
		*malloc_calls = *malloc_calls + 1;	// numarul de apeluri malloc

		// calculam noile dimensiuni
		int new_dim = heap->capacity[index] - heap->nr_bytes;
		int addr = ((info *)mem_block->data)->address;
		int new_adr = ((info *)mem_block->data)->address + heap->nr_bytes;

		// stergem nodul din lista respectiva si eliberam memoria
		dll_node_t *node = ll_remove_nth_node(heap->memory[index], 0);
		if (((info *)node->data)->s)	// daca s-a alocat un sir
			free(((info *)node->data)->s);
		free(node->data);
		free(node);

		// cream noul nod pe care il adaugam in lista cu blocuri alocate
		info *new_node = malloc(sizeof(info));
		new_node->address = addr;
		new_node->dimension = heap->nr_bytes;
		new_node->s = NULL;
		int position = position_index(mem_alloc, addr);
		dll_add_nth_node(mem_alloc, position, (void *)new_node);
		free(new_node);

		if (frag == 1) {	// daca s-a fragmentat nodul
			*nr_fragmentation = *nr_fragmentation + 1;
			dl_list_t *list = exist_list(heap, new_dim);// cautam o lista
			if (list) {
				// exista o lista in care se poate adauga blocul ramas
				int pos = position_index(list, new_adr);
				info *another_node = malloc(sizeof(info));
				another_node->address = new_adr;
				another_node->dimension = new_dim;
				another_node->s = NULL;
				dll_add_nth_node(list, pos, (void *)another_node);
				free(another_node);
			} else {
				// se da realloc
				heap->nr_lists++;
				heap->memory = realloc(heap->memory,
									   heap->nr_lists * sizeof(dl_list_t *));
				DIE(!heap->memory, "malloc failed...");
				heap->capacity = realloc(heap->capacity,
										 heap->nr_lists * sizeof(int));
				// inseram o lista;
				add_new_list(heap, new_adr, new_dim);
			}
		}
	}
}

/*
	returneaza blocul de memorie care are adresa addr din lista suplimentara
	si actualizeaza pozitia acestuia
*/
dll_node_t *find_address(dl_list_t *mem_alloc, int addr, int *pos)
{
	dll_node_t *current = mem_alloc->head;
	int position = 0;
	while (current) {
		if (((info *)current->data)->address == addr) {
			*pos = position;
			return current;
		}
		current = current->next;
		position++;
	}
	return NULL;
}

// functie pentru comanda free
void free_command(sfl *heap, dl_list_t *mem_alloc, int *free_calls)
{
	int pos = 0;
	dll_node_t *n_free = find_address(mem_alloc, heap->address, &pos);
	if (n_free) {	// daca s-a gasit un bloc de adresa address
		*free_calls = *free_calls + 1;	// marim numarul de comenzi free
		dl_list_t *list = exist_list(heap, ((info *)n_free->data)->dimension);
		if (list) {
			// exista o lista in care se poate adauga blocul ramas
			int index = position_index(list, heap->address);
			// se creaza noul nod pe care il adaugam in lista de vectori
			info *another_node = malloc(sizeof(info));
			another_node->address = ((info *)n_free->data)->address;
			another_node->dimension = ((info *)n_free->data)->dimension;
			another_node->s = NULL;
			dll_add_nth_node(list, index, (void *)another_node);
			free(another_node);
		} else {
			// se da realloc
			heap->nr_lists++;
			heap->memory = realloc(heap->memory,
								   heap->nr_lists * sizeof(dl_list_t *));
			DIE(!heap->memory, "malloc failed...");
			heap->capacity = realloc(heap->capacity,
									 heap->nr_lists * sizeof(int));
			// inseram o lista
			add_new_list(heap, ((info *)n_free->data)->address,
						 ((info *)n_free->data)->dimension);
		}
		// stergem nodul din lista de blocuri de memorie alocata
		dll_node_t *free_block = ll_remove_nth_node(mem_alloc, pos);
		if (((info *)free_block->data)->s)	// daca s-a aocat un sir
			free(((info *)free_block->data)->s);
		free(free_block->data);
		free(free_block);
	} else {
		// daca nu s-a gasit un bloc de memorie se afiseaza un mesaj specific
		printf("Invalid free\n");
	}
}

// fuctie care elibereaza memoria pentru o lista
void ll_free(dl_list_t **list)
{
	dll_node_t *curr;

	if (!list || !*list)
		return;

	while ((*list)->size > 0) {
		// stergem si eliberam noduri rand pe rand
		curr = ll_remove_nth_node(*list, 0);
		if (curr) {
			if (((info *)curr->data)->s)
				free(((info *)curr->data)->s);
			free(curr->data);
		}
		free(curr);
	}

	free(*list);
	*list = NULL;
}

/*
	Functie care citeste parametrii comenzii write si returneaza sirul de
	caractere citit. Parametrul nr va actualiza numarul citit dupa sirul de
	caractere, iar al doilea parametru se va folosi pentru eliberarea memoriei
	pentru sirul de caractere.
*/
char *scan_string(int *nr, char **p)
{
	char *s = (char *)malloc(600 * sizeof(char));
	fgets(s, 600, stdin);	// se citeste restul liniei
	int start = -1, stop;	// unde incepe si unde se sfarseste sirul
	s[strlen(s) - 1] = '\0';
	for (unsigned int i = 0; i < strlen(s); i++) {
		if (s[i] == '"' && start == -1)
			start = i;
		else if (s[i] == '"')
			stop = i;
	}
	// actualizam numarul de caractere care se vor scrie
	*nr = atoi(s + stop + 1);
	s[stop + 1] = '\0';
	*p = s;	// inceputul sirului, pentru a elibera ulterior memoria
	return s + start;
}

/*
	returneaza primul bloc de memorie din lista de blocri alocate care incepe
	sau contine adresa data ca parametru si verifica in acelasi timp daca
	exista o zona de memorie continua se nr_bytes octeti
*/
dll_node_t *continuos_area(dl_list_t *mem_alloc, int nr_bytes, int address)
{
	dll_node_t *current = mem_alloc->head;
	while (current) {
		if (((info *)current->data)->address <= address &&
			address <= ((info *)current->data)->address +
			((info *)current->data)->dimension) {
			// am gasit blocul de memorie

			// adresa ultimului octet din bloc
			int x = ((info *)current->data)->address +
					((info *)current->data)->dimension - 1;

			dll_node_t *copy = current;
			// sa verific daca e continua memoria
			if (address + nr_bytes - 1 <= x)	// daca e suficient primul bloc
				return copy;

			// ne uitam la alte blocuri
			int total = nr_bytes - (x - address + 1);	// cati bytes ne raman
			while (total > 0) {
				if (current->next) {
					if (((info *)current->next->data)->address == x + 1) {
						current = current->next;
						// adresa ultimului octet din bloc
						x = ((info *)current->data)->address +
							((info *)current->data)->dimension - 1;
						total = total - ((info *)current->data)->dimension;
					} else {
						// nu e o zona continua de memorie
						break;
					}
				} else {
					// nu exista suficiente blocuri
					break;
				}
			}
			if (total <= 0) {
				// e bine, e o zona de memorie continua
				return copy;
			}

			// nu s-a gasit o zona buna de memorie
			return NULL;
		}
		current = current->next;
	}
	// daca nu s-a gasit nimic
	return NULL;
}

// functie care va scrie in nodurile din lista de blocuri alocate un sir
void write_in_memory(dl_list_t *mem_alloc, char *string, int n, int address)
{
	dll_node_t *curr = mem_alloc->head;
	while (curr) {
		if (((info *)curr->data)->address <= address &&
			address <= ((info *)curr->data)->address +
			((info *)curr->data)->dimension - 1) {
			// daca am gasit un nod bun

			// adresa ultimului octet din bloc
			int x = ((info *)curr->data)->address +
					((info *)curr->data)->dimension - 1;

			if (n <= x - address + 1) {	// este suficient primul bloc
				if (!((info *)curr->data)->s) {	// nu s-a alocat nimic
					// alocam memorie pentru sir
					int dim = ((info *)curr->data)->dimension;
					((info *)curr->data)->s = malloc(dim + 1);
					DIE(!((info *)curr->data)->s, "malloc failed...");
				}
				// copiem continutul sirului in nod
				memcpy(((info *)curr->data)->s, string, n);
				return;
			}

			// scriem in mai multe noduri
			if (!((info *)curr->data)->s) {	// nu s-a alocat nimic
				// alocam memorie
				int dim = ((info *)curr->data)->dimension;
				((info *)curr->data)->s = malloc(dim + 1);
				DIE(!((info *)curr->data)->s, "malloc failed...");
			}
			// copiem continutul sirului in nod
			memcpy(((info *)curr->data)->s, string, x - address + 1);

			// ne uitam la alte blocuri
			int total = n - (x - address + 1); // cat ne mai ramane de scris
			curr = curr->next;
			int index = x - address + 1;	// de unde copiem
			while (total > 0) {
				if (!((info *)curr->data)->s) {	// nu s-a alocat nimic
					int dim = ((info *)curr->data)->dimension;
					((info *)curr->data)->s = malloc(dim + 1);
					DIE(!((info *)curr->data)->s, "malloc failed...");
				}
				if (total > ((info *)curr->data)->dimension) {
					// mai ramand noduri in care copiem sirul
					int dim = ((info *)curr->data)->dimension;
					memcpy(((info *)curr->data)->s, string + index, dim);
					index = index + dim;
				} else {
					// ultimul nod in care copiem sirul
					memcpy(((info *)curr->data)->s, string + index, total);
					index = index + ((info *)curr->data)->dimension;
				}
				total = total - ((info *)curr->data)->dimension;
				curr = curr->next;
			}
			return;
		}
		curr = curr->next;
	}
}

// functie pentru comanda write
void write_command(dl_list_t *mem_alloc, int address, int nr_bytes, char *str,
				   int *dump)
{
	// calculam numarul de bytes care trebuie scrisi
	int minim = nr_bytes;
	if (nr_bytes > (int)strlen(str) - 2)
		minim = strlen(str) - 2;

	// verificam daca e o adresa continua de memorie
	dll_node_t *it = continuos_area(mem_alloc, minim, address);
	if (it) {
		// vom scrie continutui in noduri
		write_in_memory(mem_alloc, str + 1, minim, address);
	} else {
		// daca nu s-a gasit o zona continua afisam un mesaj si facem dump
		printf("Segmentation fault (core dumped)\n");
		*dump = 1;
	}
}

// functie care va citi din nodurile din lista de blocuri alocate un sir
void read_from_memory(dl_list_t *mem_alloc, int n, int address)
{
	dll_node_t *curr = mem_alloc->head;
	while (curr) {
		if (((info *)curr->data)->address <= address &&
			address <= ((info *)curr->data)->address +
			((info *)curr->data)->dimension - 1) {
			// am gasit un nod

			// adresa ultimului octet din bloc
			int x = ((info *)curr->data)->address +
					((info *)curr->data)->dimension - 1;

			// pozitia de unde se va afisa sirul
			int pos = address - ((info *)curr->data)->address;
			if (n <= x - address + 1) {	// daca citim doar din primul nod
				for (int i = pos; i < n + pos; i++)
					printf("%c", ((info *)curr->data)->s[i]);
				printf("\n");
				return;
			}

			// ne uitam la alte blocuri
			int dim = ((info *)curr->data)->dimension;
			for (int i = pos; i < dim; i++)
				printf("%c", ((info *)curr->data)->s[i]);

			// cat ne mai ramane de afisat
			int total = n - (dim - pos);
			curr = curr->next;
			while (total > 0) {
				if (total > ((info *)curr->data)->dimension) {
					dim = ((info *)curr->data)->dimension;
					for (int i = 0; i < dim; i++)
						printf("%c", ((info *)curr->data)->s[i]);
				} else {
					// ultimul nod din care trebuie sa afisam
					for (int i = 0; i < total; i++)
						printf("%c", ((info *)curr->data)->s[i]);
				}
				total = total - ((info *)curr->data)->dimension;
				curr = curr->next;
			}
			printf("\n");
			return;
		}
		curr = curr->next;
	}
}

// functie pentru comanda read
void read_command(dl_list_t *mem_alloc, int nr_bytes, int address, int *dump)
{
	// verificam daca e o adresa continua de memorie
	dll_node_t *read = continuos_area(mem_alloc, nr_bytes, address);
	if (read) {
		// afisam nr_bytes caractere din sir
		read_from_memory(mem_alloc, nr_bytes, address);
	} else {
		// nu avem ce citi, se va afisa un mesaj si se va face dump
		printf("Segmentation fault (core dumped)\n");
		*dump = 1;
	}
}

// functie care elibereaza memoria pentru structurile de date alocate
void free_the_sfl(sfl *heap, dl_list_t *mem_alloc)
{
	for (int i = 0; i < heap->nr_lists; i++)
		if (heap->memory[i])
			ll_free(&heap->memory[i]);

	free(heap->capacity);
	free(heap->memory);
	free(heap);
	ll_free(&mem_alloc);
}

int main(void)
{
	char *command = (char *)malloc(600 * sizeof(char));
	scanf("%s", command);

	sfl *heap = malloc(sizeof(sfl));
	DIE(!heap, "malloc failed...");
	dl_list_t *mem_alloc;	// lista dublu inlantuita cu blocuri alocate
	long long total_memory;
	int malloc_calls = 0, free_calls = 0, nr_fragmentation = 0, dump = 0;

	while (1) {
		if (strcmp(command, "INIT_HEAP") == 0) {
			scanf("%x%d%d%d", &heap->address, &heap->nr_lists, &heap->nr_bytes,
				  &heap->type);
			total_memory = heap->nr_lists * heap->nr_bytes;
			// initializez vectorul de liste, si lista cu blocurile alocate
			heap->memory = create_list_vector(heap);
			mem_alloc = dll_create(sizeof(info));

		} else if (strcmp(command, "MALLOC") == 0) {
			scanf("%d", &heap->nr_bytes);
			malloc_command(heap, mem_alloc, &malloc_calls, &nr_fragmentation);
		} else if (strcmp(command, "FREE") == 0) {
			scanf("%x", &heap->address);
			free_command(heap, mem_alloc, &free_calls);

		} else if (strcmp(command, "READ") == 0) {
			scanf("%x%d", &heap->address, &heap->nr_bytes);

			read_command(mem_alloc, heap->nr_bytes, heap->address, &dump);
			if (dump == 1) {	// se va face dump
				dump_print(heap, mem_alloc, total_memory, malloc_calls,
						   nr_fragmentation, free_calls);
				break;
			}
		} else if (strcmp(command, "WRITE") == 0) {
			scanf("%x", &heap->address);
			char *p;
			char *string = scan_string(&heap->nr_bytes, &p);
			write_command(mem_alloc, heap->address, heap->nr_bytes, string,
						  &dump);
			free(p);
			if (dump == 1) {	// se va face dump
				dump_print(heap, mem_alloc, total_memory, malloc_calls,
						   nr_fragmentation, free_calls);
				break;
			}
		} else if (strcmp(command, "DUMP_MEMORY") == 0) {
			dump_print(heap, mem_alloc, total_memory, malloc_calls,
					   nr_fragmentation, free_calls);
		} else if (strcmp(command, "DESTROY_HEAP") == 0) {
			break;
		}
		scanf("%s", command);
	}
	// eliberam memoria
	free_the_sfl(heap, mem_alloc);
	free(command);

	return 0;
}
