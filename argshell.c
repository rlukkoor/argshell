#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

extern char ** get_args();

    int			fd0 = -1;	// file descriptor for "<"
    int 		fd1 = -1;	// file descriptor for ">", ">>", ">&", ">>&"
    bool		error_redirection = false;

char curr_dir [1024];

void 
exec(char **argv)	// executes the commands
{
	int    		status;
    pid_t  		pid;
    

    if (*argv == NULL)
     	return;

    if ((pid = fork()) < 0) {     // fork a child process
        printf("Forking child process failed\n");
        exit(1);
    }
    else if (pid == 0) {          // for the child process:

    	if(fd0 > 0) {       	// if "<" is used
    		dup2(fd0,0);
    	}
    	if(fd1 > 0 && !error_redirection) {		// if ">" or ">>" is used
    		dup2(fd1,1);
    	}
    	if(error_redirection) { 		// if ">&" or ">>&" is used
    		dup2(fd1,1);
    		dup2(fd1,2);
    	}
        if (execvp(*argv, argv) < 0) {     // execute the command
               printf("Exec failed\n");
               exit(1);
        }
    }

	if(fd0 > 0) {		// close fd	& reset fd0          
    	close(fd0);
    	fd0 = -1;
    }
	if(fd1 > 0) {	    // close fd & reset fd1           	
		close(fd1);
		fd1 = -1;
	}
    else {                                  // for the parent:      
        while (wait(&status) != pid);       // wait for completion      // put in ";" section        
    }
}

int
main()
{
    int         i;
    char **     args;
    int 		z;
    char  		*argv_buf[1024];
    getcwd(curr_dir, 1024);

    while (1) {
		printf ("Command ('exit' to quit): ");
		args = get_args();
		for (i = 0; args[i] != NULL; i++) {
		    printf ("Argument %d: %s\n", i, args[i]);
		}

		if (args[0] == NULL) {
		    printf ("No arguments on line!\n");
		    continue;
		} else if ( !strcmp (args[0], "exit")) {
		    printf ("Exiting...\n");
		    break;
		}

		argv_buf[0] = NULL;
		int buf_index = 0;
			for (z = 0; args[z] != NULL ; z++) {	// iterate through arguments
				if(strcmp(args[z], ";") == 0){		// looks for ";"
					argv_buf[buf_index] = NULL;		// set argv_buf to NULL
					exec(argv_buf);				// execute arg_v buffer
					buf_index = 0;
					argv_buf[0] = NULL;
					continue;
				}

				if(strcmp(args[z], "cd") == 0){		// looks for "cd"
					if((args[z+1]) == NULL || strcmp(args[z+1], ";") == 0) {	// see if next argument is NULL or ";"
						chdir(curr_dir);		// go to current working directory
					}
					if((args[z+1]) == NULL){	// break if NULL
						break;
					}
					if(strcmp(args[z+1], ";") == 0){	// increment args and continue if ";"
						z++;
						continue;
					}
					if(chdir(args[z+1]) < 0) {		// checks if the path is valid
						perror("chdir: No such file or directory\n"); 
						break;
					}
					z++;	// go to next argument
					continue;
				}

				if(strcmp(args[z], "<") == 0){		// look for "<"
                	if((fd0 = open(args[z+1], O_RDONLY)) < 0){	// open the file after "<"
                		perror("Cannot open file\n"); 	
                		exit(0);
                	}      
					z++;	// go to next argument
					continue;
				}

				int open_flags = 0;
				error_redirection = false;
                if ((strcmp(args[z], ">") == 0) || (strcmp(args[z], ">&") == 0)) {		// look for ">" or ">&"
                	open_flags = O_WRONLY|O_TRUNC|O_CREAT;								// sets appropriate flags
            	}
                if ((strcmp(args[z], ">>") == 0) || (strcmp(args[z], ">>&") == 0)) {	// look for ">>" or ">>&"
                	open_flags = O_WRONLY|O_CREAT|O_APPEND;								// sets appropriate flags
                }

                if((strcmp(args[z], ">&") == 0) || (strcmp(args[z], ">>&") == 0) || (strcmp(args[z], "|&") == 0)) {		// set error redirection to true     																		
                	error_redirection = true;											// if ">&" or ">>&" is used
                }

                if (open_flags != 0) {        
					if((fd1 = open(args[z+1], open_flags, 0777)) < 0) {		// open file in file descriptor fd1
	                	perror("Cannot open file\n"); 
	                	exit(0);
	                }	                
					z++;	// go to next argument
					continue;
				}

				if(strcmp(args[z], "|") == 0){		// look for "|"
					int pfd[2];
					pid_t pipe0, pipe1;
					if(pipe(pfd) < 0) {
						perror("Cannot start pipe\n"); 
						break;	
					}

					if ((pipe0 = fork()) < 0) {     // fork a child process
        				printf("Forking child process failed\n");
        				exit(1);
    				}

					if(pipe0 == 0) {
						dup2(pfd[1], 1);
						close(pfd[1]);
						for(int m = 0; m < buf_index; m++) {
							if(execvp(argv_buf[0], NULL) < 0) {
								printf("Exec failed\n");
								exit(0);
							}
						}
					}
					else { 		// parent is executing
						if ((pipe1 = fork()) < 0) {     // fork another child process
	        				printf("Forking child process failed\n");
	        				exit(1);
    					}
    					if (pipe1 == 0) {
    						dup2(pfd[0], 0);
    						close(pfd[0]);  
    						if(execvp(args[z+1], NULL) < 0) {
								printf("Exec failed\n");
								exit(0);
							}
							else {
								wait(NULL);		// parent waits for children
								wait(NULL);
							}
    					}
					}
				}
				argv_buf[buf_index++] = args[z]; // add arguments to buffer
			}
			
			argv_buf[buf_index] = NULL;	
			exec(argv_buf);		// execute commands in buffer
			argv_buf[0] = NULL;		// set first element of buffer to NULL
			buf_index = 0;			
	}
}
