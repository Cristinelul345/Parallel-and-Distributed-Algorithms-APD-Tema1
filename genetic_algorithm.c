#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function_parallel(const sack_object *objects, individual *generation, int start, int object_count, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = start; i < object_count; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}


void mergeTwo(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;

	for (int i = start; i < end; i++) {
		if (end == iB || (iA < mid && cmpfunc(&source[iA],&source[iB]) < 0)) {
			destination[i] = source[iA];
			iA++;
		} else {
			destination[i] = source[iB];
			iB++;
		}
	}
}

void copy_back(individual* source, individual* destination, int size) {
	for(int i = 0; i < size; i++) {
		source[i] = destination[i];
	}
}

void general_merge(individual* source, int merges, int size, individual* destination) {

	for(int i = 1; i <= merges - 1; i++) {
		mergeTwo(source, 0, i * size / merges, (i + 1) * size / merges, destination);
		copy_back(source, destination, (i + 1) * size / merges);
	}	
}

void merge(individual* source, int size, int merges) {

	individual* destination = malloc(size * sizeof(individual));
	if(!destination) exit(1);

	int start; int step;
	switch(merges) {
		case 1:
			return;
		default:
			general_merge(source, merges, size, destination);
			return;
	}
}


void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = 0; i < object_count; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}


Generic wrap(void* elem) {
	Generic g;
	g.elem = elem;
	return g;
}

void* thread_fct(void* x) {
	Generic* args = (Generic*) x;
	int count, cursor;

	int newstart, newend;
	int P = *((int*) args[0].elem);
	const sack_object* objects = (const sack_object*) args[1].elem;
	int obj_count = *((int*) args[2].elem);
	int generations_count = *((int*) args[3].elem);
	int sack_capacity = *((int*) args[4].elem);
	int start = *((int*) args[5].elem);
	int end = *((int*) args[6].elem);
	int id = *((int*) args[7].elem);

	pthread_barrier_t* barrier = (pthread_barrier_t*) args[8].elem;
	individual** current_generation_addr = (individual**) args[9].elem;
	individual** next_generation_addr = (individual**) args[10].elem;
	individual** tmp_addr = (individual**) args[11].elem;


	individual* current_generation = *current_generation_addr;
	individual* next_generation = *next_generation_addr;
	individual* tmp = *tmp_addr;

	for (int i = start; i < end; ++i) {

		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(obj_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = obj_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(obj_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = obj_count;
	}

	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {
		cursor = 0;

		pthread_barrier_wait(barrier);

		current_generation = *current_generation_addr;
		next_generation = *next_generation_addr;
		tmp = *tmp_addr;

		for (int i = start; i < end; ++i) {
			current_generation[i].index = i;
		}

		compute_fitness_function_parallel(objects, current_generation, start, end, sack_capacity);

		int crt_count = end - start;
		qsort(current_generation + start, crt_count, sizeof(individual), cmpfunc);
		pthread_barrier_wait(barrier);
		
		if(start == 0) {
			merge(current_generation, obj_count, P);
		}	
		
		pthread_barrier_wait(barrier);

		count = obj_count * 3 / 10;
		newstart = start * 3 / 10;
		newend = end * 3 / 10;
		for (int i = newstart; i < newend; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		cursor = count;

		count = obj_count * 2 / 10;
		newstart = start * 2 / 10;
		newend = end * 2 / 10;
		for (int i = newstart; i < newend; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);

			copy_individual(current_generation + i + count, next_generation + cursor + count + i);
			mutate_bit_string_2(next_generation + cursor + count + i, k);
		}
		cursor += 2*count;

		newstart = start * 2 / 10;
		newend = end * 2 / 10;

		count = obj_count * 3 / 10;


		if(count % 2 == 1) --count;

		if(id == 0) {
			if (count % 2 == 1) {
				copy_individual(current_generation + obj_count - 1, next_generation + cursor + count - 1);
			}
		}
		newstart = id * (double) count / P;
		if( (id+1) * (double) count / P < count)  {
			newend = (id+1) * (double) count/P;
		} else newend = count;

		if(newstart % 2 != 0) newstart++;

		for (int i = newstart; i < newend; i += 2) {
			crossover(current_generation + i, next_generation + cursor + i, k);

		}
		pthread_barrier_wait(barrier);

		if(start == 0) {
			*tmp_addr = *next_generation_addr;
			*next_generation_addr = *current_generation_addr;
			*current_generation_addr = *tmp_addr;
			if (k % 5 == 0) {
				print_best_fitness(*current_generation_addr);
			}
		}

	}

	if(start == 0) {
		compute_fitness_function(objects, current_generation, obj_count, sack_capacity);
		qsort(current_generation, obj_count, sizeof(individual), cmpfunc);
		print_best_fitness(current_generation);
	}
}

void run_genetic_algorithm(int P, const sack_object *objects, int object_count, int generations_count, int sack_capacity)
{

	pthread_barrier_t barrier;
	individual *current_generation;
	individual *next_generation;
	individual *tmp;

	pthread_barrier_init(&barrier, NULL, P);

	current_generation = (individual*) calloc(object_count, sizeof(individual));
	next_generation = (individual*) calloc(object_count, sizeof(individual));
	tmp = NULL;

	pthread_t thread[P];
	int ids[P];

	for(int i=0; i<P; i++) {
		ids[i] = i;

		Generic* args = calloc(12, sizeof(Generic));
		args[0] = wrap(&P);
		args[1] = wrap((void*) objects);
		args[2] = wrap(&object_count);
		args[3] = wrap(&generations_count);
		args[4] = wrap(&sack_capacity);
		int* start = malloc(sizeof(int));
		*start = i * (double) object_count / P;;
		int* end = malloc(sizeof(int));
		if( (i+1) * (double) object_count / P < object_count)  {
			*end = (i+1) * (double) object_count/P;
		} else *end = object_count;
		int* j = malloc(sizeof(int));
		*j = i;

		args[5] = wrap(start);
		args[6] = wrap(end);
		args[7] = wrap(j);

		args[8] = wrap(&barrier);
		args[9] = wrap(&current_generation);
		args[10] = wrap(&next_generation);
		args[11] = wrap(&tmp);

		pthread_create(thread + i, NULL, thread_fct, args);
	}

	for(int i = 0; i < P; i++) {
		pthread_join(*(thread + i), NULL);
	}

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	pthread_barrier_destroy(&barrier);
	// free resources
	free(current_generation);
	free(next_generation);
}