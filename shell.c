#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "vect.h"
#include "shell.h"
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>

vect_t **prev_sequence;
char* prev;

/*
 * Reads in a line from stdin and tokenizes it into a vector
 */
vect_t* tokenize_input() {
	
	// allocate string that will hold stdin input
	char *inp = (char*) calloc(SHELL_MAX_INPUT_LEN, sizeof(char));
	
	// set buf size to max input length
	size_t buf_size = SHELL_MAX_INPUT_LEN;
	
	// read and check for end of line
	if (getline(&inp, &buf_size, stdin) == -1) {
		printf("Bye bye.\n");
		exit(0);
	}

	// if the input command is not prev, we set prev to the input command
	if (strcmp(inp, "prev\n") != 0) {
		if (prev != NULL) {
			free(prev);
			prev = malloc(strlen(inp) + 1);
		}
		strcpy(prev, inp);	
	}
	
	// tokenize the input
	vect_t *a = tokenize(inp);

	free(inp);
	return a;
}

/*
 * Null terminate a given list of strings
 */
char** null_terminate(char ** data, int size) {
	assert(data != NULL);

	// reallocate size to make it bigger so we can fit in null
	char** new_data = realloc(data, (size + 1) * sizeof(char*));

	// set last element to null
	new_data[size] = NULL;

	return new_data;
}

/*
 * Given an input token list returns a list of commands (list of vectors)
 */
vect_t **sequencer(vect_t* tokens) {
	// allocate proper size for list of vectors to be returned
	vect_t **commands = (vect_t **) malloc(sizeof(vect_t) * vect_size(tokens));

	int vect_idx = 0;
	commands[0] = vect_new();
	for (int i = 0; i < vect_size(tokens); i ++) {
		// when we reach a semi-colon we start a new command
		if (strcmp(vect_get(tokens, i), ";") == 0) {
			vect_idx++;
			commands[vect_idx] = vect_new();
		} 
		// if we have not reached a semi-colon we keep adding to the current command vector
		else {
			vect_add(commands[vect_idx], vect_get(tokens, i));
		
		}
	}
	
	return commands;

}

/* 
 * Correctly frees a list of vectors
 */
void free_sequences(vect_t **sequences) {
	int i = 0;
	while(sequences[i] != NULL) {
		vect_delete(sequences[i]);
		i++;
	}
	free(sequences);
}

/*
 * Helper method for the "source" command. Iterates through previously set stdin.
 */
void exec_source() {
        while (getc(stdin) != EOF) {
        	fseek(stdin, -1, SEEK_CUR);
                execute_sequences(sequencer(tokenize_input()));
        }
}

/*
 * Checks if a specified symbol is in a vector. If it is, return its index in the vector,
 * otherwise return -1 if it is not present.
 */
int symbol_in_command(vect_t *command, const char* symbol) {
	for (int j = 0; j < vect_size(command); j++) {
		if (strcmp(vect_get(command, j), symbol) == 0) {
			return j;
		}
	}
	return -1;
}

/*
 * Given a vector of commands we return a list of vectors
 * containing a subset of the vector passed in
 */
vect_t** isolate_cmd(vect_t* cmd, int start_idx, int end_idx) {
	vect_t** locmd = (vect_t**) malloc(sizeof(vect_t) * vect_size(cmd));
	locmd[0] = vect_new();
	for (int i = start_idx; i < end_idx; i++) {
		vect_add(locmd[0], vect_get(cmd, i));
	}
	return locmd;
}

/*
 * Redirects input or output as specified by the char passed in. Should be one
 * of "<" or ">"
 */
void redirect(vect_t* sequence, int index, const char* symbol) {
	if (fork() == 0 && index + 1 != vect_size(sequence)) {
		// redirecting in
		if (strcmp(symbol, "<") == 0) {
			close(0);
			assert(open(sequence->data[index + 1], O_RDONLY) == 0);
		}
		// redirecting out
		else {
			close(1);
			assert(open(sequence->data[index + 1], O_WRONLY | O_CREAT, 0644) == 1);
		}
		vect_t **locmd = isolate_cmd(sequence, 0, index);
		execute_sequences(locmd);
		free_sequences(locmd);	
		exit(0);
	}
	else {
		wait(NULL);
		return;
	}
}

/*
 * Helper function for piping process.
 */
void piper(vect_t* sequence, int pipe_idx) {
	// Fork child A
	if (fork() == 0) {
		// In child A create pipe
		int fds[2];
		assert(pipe(fds) == 0);

		int read_fd = fds[0];
		int write_fd = fds[1];
		
		// Fork child B
		int child_pid = fork();
		if (child_pid > 0) {
			// close write end of pipe since we won't use it here
			assert(close(write_fd) != -1);

			// close stdin
			assert(close(0) != -1);

			//redirect read end of pipe to stdin
			assert(dup(read_fd) == 0);

			vect_t** cmd = isolate_cmd(sequence, pipe_idx + 1, vect_size(sequence));
			
			// execute command 1
			execute_sequences(cmd);
			free_sequences(cmd);

			// wait for child B
			wait(NULL);
			exit(0);

		} else {
			// close read end of pipe since we won't use it here
			assert(close(read_fd) != -1);

			// close stdout
			assert(close(1) != -1);

			// redirect write end of pipe to stdout
			assert(dup(write_fd) == 1);

			vect_t **cmd = isolate_cmd(sequence, 0, pipe_idx);
			
			// execute command 2
			execute_sequences(cmd);
			free_sequences(cmd);
			exit(0);
		}
		exit(0);
                
	} else {
		// wait for child A
		wait(NULL);
		return;
	}
}


/*
 * Executes a line of commands (list of vectors). Returns 1 on exit (good) and 0 upon failure (should never return 0).
 */
int execute_sequences(vect_t** sequences) {	
	
	// iterate through sequences
	int i = 0;
	while(sequences[i] != NULL) {
		// If we have been given an empty command continue and reprompt
		if (vect_size(sequences[i]) == 0) {
			i++;
			continue;
		}
		
		// look for a redirection of input symbol
		int redirin_idx = symbol_in_command(sequences[i], "<");	
		if (redirin_idx >= 0) {
			redirect(sequences[i], redirin_idx, "<");
			i++;
			continue;
		}

		// look for a redirection of output symbol
		int redirout_idx = symbol_in_command(sequences[i], ">");
		if (redirout_idx >= 0) {
			redirect(sequences[i], redirout_idx, ">");
			i++;
			continue;
		}

		// look for a pipe symbol
		int pipe_idx = symbol_in_command(sequences[i], "|");
		if (pipe_idx >= 0) {
			piper(sequences[i], pipe_idx);
			i++;
			continue;
		}
		
		// check if client has asked for prev
		if (strcmp(sequences[i]->data[0], "prev") == 0) {
			printf(prev);
			execute_sequences(prev_sequence);
			i++;
			continue;
		}

		// check if client has asked to exit
		if (strcmp(sequences[i]->data[0], "exit") == 0) {
			return 1;
			printf("SHOULD NOT REACH HERE.\n");
		}

		// check if user has asked for help
		if (strcmp(sequences[i]->data[0], "help") == 0) {
			printf(HELP_MANUAL);
			i++;
			continue;
		}

		// check if user is looking to change directories
		if (strcmp(sequences[i]->data[0], "cd") == 0) {
			chdir(sequences[i]->data[1]);
			i++;
			continue;
		}

		// check if user is looking to use a script as source of commands
		if (strcmp(sequences[i]->data[0], "source") == 0) {			
			if (fork() == 0) {
				close(0);
				assert(open(sequences[i]->data[1], O_RDONLY) == 0);
				exec_source();
				exit(0);
			} else {
				wait(NULL);
				i++;
				continue;
			}
		}

		// if none of the above cases, we try to execute what has been input
		if (fork() == 0) {
			sequences[i]->data = null_terminate(sequences[i]->data, vect_size(sequences[i]));
			execvp(sequences[i]->data[0], sequences[i]->data, NULL);

			// if we reach here, the exec call was unsuccesful, and the child process lives on 
			printf("No such file or directory\n");
			exit(1);
			break;
		} else {
			wait(NULL);
			i++;
		}
	}

	return 0;
}

/*
 * Return a copy of a list of vectors
 */
vect_t **copy_lovect(vect_t** source) {
	
	assert(source != NULL);
	vect_t **dest = (vect_t**) malloc(sizeof(vect_t*) * 20);

	int i = 0;
	while (source[i] != NULL) {
		dest[i] = vect_new();
		for (int j = 0; j < vect_size(source[i]); j++) {
			vect_add(dest[i], vect_get(source[i], j));
		}
		i++;
	}

	return dest;
}

/*
 * Main method acts as controller of shell program.
 */
int main(int argc, char **argv) {
	printf("Welcome to mini-shell.\n");
	
	prev = malloc(sizeof(char*));

	while(1) {
		printf("shell $ ");

		// retrieve initial input and tokenize
		vect_t* tokens = tokenize_input();	
		
		// make list of commands
		vect_t** sequences = sequencer(tokens);
		
		// we no longer need the initial input
		vect_delete(tokens);
		
		// if execute_sequences returns 1 then we were prompted to exit
		if (execute_sequences(sequences) == 1) {
			free_sequences(sequences);
			break;
		}

		if (prev_sequence != NULL) {
			free_sequences(prev_sequence);
		}

		prev_sequence = copy_lovect(sequences);		
		
		free_sequences(sequences);
	}

	free(prev);
	printf("Bye bye.\n");
  	return 0;

}
