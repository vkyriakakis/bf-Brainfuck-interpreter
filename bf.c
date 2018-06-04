#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define MEM_SIZE 2048
#define MEMFAIL ((void*)-1)

struct node {
	char command;
	struct node *next;
};
typedef struct node commNode;

void freeStack(commNode *stack);
int interpretSource(char *memory, char *source, size_t sourceLen);
int jumpLoopEnd(char *source, size_t sourceLen, size_t currComm);
int jumpLoopStart(char *source, size_t sourceLen, size_t currComm);
int checkMatchingBracks(char *source, size_t sourceLen);
commNode *stackInit(void);
commNode *push(commNode *stack, char symbol);
commNode *pop(commNode *stack);
char *loadSource(char *filename, size_t *length);
size_t getFileSize(int fd);

int main(int argc, char *argv[]) {
	char memory[MEM_SIZE] = {0}; //Initialized to zero
	char *source;
	size_t sourceLen;

	if (argc < 2) {
		fprintf(stderr, "bf: No source file was given!\n");
		return(1);
	}

	source = loadSource(argv[1], &sourceLen);
	if (source == MEMFAIL) {
		return(1);
	}

	//Check for matching brackets
	if (checkMatchingBracks(source, sourceLen)) {
		munmap(source, sourceLen);
		return(1);
	}

	if (interpretSource(memory, source, sourceLen) < 0) {
		munmap(source, sourceLen);
		return(1);
	}

	if (munmap(source, sourceLen) < 0) {
		return(1);
	}

	return(0);
}

int interpretSource(char *memory, char *source, size_t sourceLen) {
	size_t pos = 0; //Points to the current cell in memory
	int temp;

	for (size_t k = 0 ; k < sourceLen ; k++) {
		switch(source[k]) {
			case '>':
				if (pos < MEM_SIZE-1) {
					pos++;
				}
				else {
					fprintf(stderr, "bf: Error at ch:%lu: Passed maximum cell position.\n", k);
					return(-1);
				}
				break;
			case '<':
				if (pos > 0) {
					pos--;
				}
				else {
					fprintf(stderr, "bf: Error at ch:%lu: Passed minimum cell position.\n", k);
					return(-1);
				}
				break;
			case '+':
				memory[pos] += 1;
				break;
			case '-':
				memory[pos] -= 1;
				break;
			case '.':
				putchar(memory[pos]);
				break;
			case ',':
				memory[pos] = getchar();
				break;
			case '[':
				if (!memory[pos]) {
					temp = jumpLoopEnd(source, sourceLen, k);
					if (temp < 0) {
						return(-1);
					}
					k = temp;
				}
				break;
			case ']':
				if (memory[pos]) {
					temp = jumpLoopStart(source, sourceLen, k);
					if (temp < 0) {
						return(-1);
					}
					k = temp;
				}
				break;
			default:
				break;
		}
	}

	return(0);
}

int checkMatchingBracks(char *source, size_t sourceLen) {
	commNode *bracketStack, *temp;
	size_t k;
	size_t lastLeft;

	bracketStack = stackInit();

	for (k = 0 ; k < sourceLen ; k++) {
		if (source[k] == '[') {
			temp = push(bracketStack, '[');
			if (!temp) {
				fprintf(stderr, "bf: Memory allocation error!\n");
				freeStack(bracketStack);
				return(-1);
			}
			bracketStack = temp;
		}
		else if (source[k] == ']') {
			if (!bracketStack) {
				fprintf(stderr, "bf: Error at ch:%lu: ']' without matching '['.\n", k);
				freeStack(bracketStack);
				return(1);
			}
			bracketStack = pop(bracketStack);
		}
	}
	//All brackets matched
	if (!bracketStack) {
		return(0); //return new position
	}
	//else it got to the end without finding something
	fprintf(stderr, "bf: Error at ch:%lu: '[' without matching ']'.\n", k-1);
	freeStack(bracketStack);

	return(1);
}

int jumpLoopEnd(char *source, size_t sourceLen, size_t currComm) {
	commNode *bracketStack, *temp;
	size_t k;

	bracketStack = stackInit();

	for (k = currComm ; k < sourceLen ; k++) {
		if (source[k] == '[') {
			temp = push(bracketStack, '[');
			if (!temp) {
				fprintf(stderr, "bf: Memory allocation error!\n");
				freeStack(bracketStack);
				return(-1);
			}
			bracketStack = temp;
		}
		else if (source[k] == ']') {
			bracketStack = pop(bracketStack);
			if (!bracketStack) {
				break;
			}
		}
	}

	freeStack(bracketStack);

	return(k);
}

int jumpLoopStart(char *source, size_t sourceLen, size_t currComm) {
	commNode *bracketStack, *temp;
	long k;

	bracketStack = stackInit();

	for (k = currComm ; k >= 0 ; k--) {
		if (source[k] == ']') {
			temp = push(bracketStack, ']');
			if (!temp) {
				fprintf(stderr, "bf: Memory allocation error!\n");
				freeStack(bracketStack);
				return(-1);
			}
			bracketStack = temp;
		}
		else if (source[k] == '[') {
			bracketStack = pop(bracketStack);
		}
		//if stack emptied, all brackets matched
		if (!bracketStack) {
			break;
		}
	}

	freeStack(bracketStack);

	return(k);
}

commNode *stackInit(void) {
	return(NULL);
}

commNode *push(commNode *stack, char symbol) {
	commNode *new;

	new = malloc(sizeof(commNode));
	if (!new) {
		return(NULL);
	}

	new->next = stack;
	return(new);
}

commNode *pop(commNode *stack) {
	commNode *newHead;

	newHead = stack->next;
	free(stack);

	return(newHead);
}

void freeStack(commNode *stack) {
	commNode *curr, *next;

	for (curr = stack ; curr != NULL ; curr = next) {
		next = curr->next;
		free(curr);
	}
}

//File loading
char *loadSource(char *filename, size_t *length) {
	char *source;
	size_t fileSize;
	int fd;

	fd = open(filename, O_RDONLY, 0);
	if (fd < 0) {
		if (errno == ENOENT) {
			fprintf(stderr, "bf: The file \"%s\" does not exist!\n", filename);
		}
		return(MEMFAIL);
	}

	*length = getFileSize(fd);
	if (fileSize < 0) {
		return(MEMFAIL);
	}
	else if (*length == 0) {
		fprintf(stderr, "bf: An empty source file was given!\n");
		return(MEMFAIL);
	}
	//MAP_PRIVATE gia na mhn perasoun oi allages ston disko
	source = mmap(NULL, *length, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (source == MEMFAIL) {
		return(MEMFAIL);
	}
	close(fd);

	return(source);
}

size_t getFileSize(int fd) {
	size_t size;

	size = lseek(fd, 0, SEEK_END);
	if (size == -1) {
		return(-1);
	}

	return(size);
}
