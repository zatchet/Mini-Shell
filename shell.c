#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "vect.h"
#include <assert.h>
#include <string.h>

// reads in a line of stdin and tokenizes
// void -> vect
vect_t* tokenize_input() {
	
	// allocate space for input string
	char *inp = (char*) calloc(SHELL_MAX_INPUT_LEN, sizeof(char));
	size_t buf_size = SHELL_MAX_INPUT_LEN;
	
	// read and check for end of line
	if (getline(&inp, &buf_size, stdin) == -1) {
		printf("Bye bye.\n");
		exit(0);
		return;	
	}
	
	vect_t *a = tokenize(inp);
	free(inp);
	return a;
}

// prepends "/bin/" to given string if not already present
// str -> str
void binify(char *c) {
	char *binable = calloc(5, sizeof(char));
	strncpy(binable, c, 5);

	if (strcmp(binable, "/bin/")==0) {
		free(binable);
		return;
	}

	else {
		// free memory
		free(binable);
		char bin[5] = "/bin/";
		size_t len = 5;
		memmove(c + len, c, strlen(c) + 1);	
		memcpy(c, bin, len);
		return;
	}

}

int main(int argc, char **argv) {

	printf("Welcome to mini-shell.\n");

  	while(1) {
		// prompt user
		printf("shell $ ");	
		// tokenize input
		vect_t *tokens = tokenize_input();

		// if user enters space, tab, or newline, reprompt
		if (vect_size(tokens) == 0) {
			continue;	
		}		

		char **data = (char **) malloc(sizeof(char*) * (vect_size(tokens) + 1));

		// prepate argv array
		for (int i = 0; i < vect_size(tokens); i ++) {
			data[i] = vect_get_copy(tokens, i);
		}

		// check for exit keyword
		if (strcmp(data[0], "exit") == 0) {
			
			// free args
			for (int i = 0; i <= vect_size(tokens); i++) {
				free(data[i]);
			}

			free(data);
			vect_delete(tokens);
			break;
		}

		binify(data[0]);
		// ensure null terminated array
		data[vect_size(tokens)] = NULL;
		
		// if child, execute system call
		if (fork() == 0) {

			execvp(data[0], data, NULL);

			// if we reach here, it wasn't a valid command
			printf("No such file or directory\n");

			// free all data
			for (int i = 0; i <= vect_size(tokens); i ++) {
				free(data[i]);
		       	}

			free(data);
			vect_delete(tokens);
			exit(1);
		} else {
			wait(NULL);
		}

		// free all data
		for (int i = 0; i <= vect_size(tokens); i ++) {
			free(data[i]);
		}	
		free(data);
		vect_delete(tokens);
	}

	printf("Bye bye.\n");

  	return 0;
}

