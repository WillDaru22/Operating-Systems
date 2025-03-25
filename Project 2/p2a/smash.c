/* * * * * * * * * * * 
 * smash shell
 * Created by William Wilson
 *
 * I pity the poor soul who has to look through and try to figure out this mess
 *
 *
 * ACKNOWLEDGEMENTS - These probably shouldn't be looked at seriously given the state of this
 * ================
 * https://riptutorial.com/c/example/8274/get-lines-from-a-file-using-getline--  - Help with understanding how to use getline()
 * https://www.geeksforgeeks.org/fork-system-call/  - Help with understanding fork()
 * https://stackoverflow.com/questions/12752647/trouble-with-fork-and-execve - More help with understanding fork()
 * https://www.geeksforgeeks.org/linked-list-set-1-introduction/ - Refresher on linked lists
 * https://www.geeksforgeeks.org/g-fact-66/ - Some help with realloc()
 * https://stackoverflow.com/questions/6417158/c-how-to-free-nodes-in-the-linked-list  - Freeing a linked list
 * https://stackoverflow.com/questions/58146750/how-do-i-check-if-a-string-contains-a-certain-character  - checking a string for a certain character
 * https://www.geeksforgeeks.org/linked-list-set-2-inserting-a-node/  - Help with inserting into a linked list
 * https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish  - help with wait()
 * https://stackoverflow.com/questions/632846/clearing-a-char-array-c  - Help with clearing a char array
 *
 * * * * * * * * * * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct Node {  //for storing path data
    char* data;
    struct Node* next;
};

/* Variable Declarations
 */
char error_message[30] = "An error has occurred\n";
char *buffer = NULL;
size_t buffsize;
ssize_t line_size;
//char line[128];
char *storage = NULL;
char *place = NULL;
char *parallel = NULL;
//char** temp = NULL;  //temp storage of realloc
struct Node* head = NULL;  //start of linked list for storing paths
struct Node** list_start = NULL;  //pointer to the start of the linked list
pid_t child;
pid_t parent;
int status = 0;
char *filepath = NULL;
char searchpath[128];
FILE *fp;
const char *exitstr = "exit";
const char *cdstr = "cd";
const char *pathstr = "path";
char** exargs = NULL;  //arguments for exec
int argcnt = 0;

/* This is a helper method to add a node to a linked list
 *
 */
void addNode(struct Node** start, char* new_path) {
    struct Node* new = (struct Node*)malloc(sizeof(struct Node));
    if(new == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
	exit(1);
    }
    //printf("malloced path\n");
    struct Node *prev = *start;
    new->data = new_path;
    new->next = NULL;
    if(prev == NULL) {
        
    }
    while(prev->next != NULL) {
        prev = prev->next;
    }
    prev->next = new;
    return;
}

/* This is a helper method to remove a specific node with the specified content as its data
 *
 */
void removeNode(struct Node *prev,char* content) {
    struct Node *temp = NULL;
    if(strcmp(prev->data,content) == 0) {
        temp = prev;
	prev = prev->next;
	free(temp);
	return;
    }
    while(prev->next != NULL) {
        if(strcmp(prev->next->data,content) != 0) {
	    prev = prev->next;
	}
	else {
            temp = prev->next;
	    prev->next = prev->next->next;
	    free(temp);
	    return;
	}
    }
    //write(STDERR_FILENO, error_message, strlen(error_message));
    //exit(1);    
}

/* This is a helper method that empties and frees a linked list
 *
 */
void freeList(struct Node* start) {
    struct Node* temp = NULL;

    while(start != NULL) {
        temp = start;
	start = start->next;
	free(temp);
    }
}

/* This is a helper method that frees a 2D dynamic array
 *
 */
void freeArray(char** array) {
    for(int f = 0; f < 256; f++) {
            //printf("trying to free\n");
	    free(array[f]);
	    //printf("freed\n"); //debug statement
	    array[f] = NULL;
    }
    free(array);
    array = NULL;
}

/* Helper function to use access
 *
 */
int checkFile(char *file) {
    if(access(file, X_OK) != -1) {
       filepath = file;
       return 0;
    }
    else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return -1;
    }
}

/* Main method
 *
 */
    int main(int argc, char* argv[]) {

    int loop = 1;
    if(argc == 1) {  //interactive mode
	if((head = (struct Node*)malloc(sizeof(struct Node))) == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);     
	}
	head->data = "/bin/";  //default path
        head->next = NULL;
	list_start = &head;
	//printf("entering\n");
	while(loop == 1) {
            if((exargs = malloc(256 * sizeof(char*))) == NULL) {
                //printf("Failed malloc\n");
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            for(int i = 0; i < 256; i++) {
                if((exargs[i] = malloc(256 * sizeof(char))) == NULL) {
                    //printf("Failed malloc\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
            }
            printf("smash> ");
	    fflush(stdout);
            getline(&buffer,&buffsize,stdin);
            //strcpy(line,buffer);
                    while((storage = strsep(&buffer," \t\n;")) != NULL && (strcmp(storage,"") != 0)) {
                        strcpy(exargs[argcnt], storage);
		        argcnt++;
                    }
	            //printf("copied\n");
		    if(exargs == NULL) {
		        break;
		    }
		    //printf("attempting to check 0\n");  //debug statement
                    exargs[argcnt] = NULL;
		    //printf("nulled\n");  //debug statement
                    if(strcmp(exargs[0],exitstr) == 0) {
                        freeArray(exargs);
                        exit(0);
                    }
                    else if(strcmp(exargs[0],cdstr) == 0) {
                        //printf("changing dir\n");  //debug statement
			if(argcnt != 2) {
                            //printf("wrong args\n");
			    //printf("%d\n",argcnt);  //debug statement
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
			else if(chdir(exargs[1]) != 0) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            
                        }
			//printf("%s\n",exargs[1]);
		        //printf("directory changed\n");  //debug statement
                    }
		    else if(strcmp(exargs[0],pathstr) == 0) {
			if(argcnt == 1) {
			    write(STDERR_FILENO, error_message, strlen(error_message));
			}
			else if(strcmp(exargs[1],"add") == 0) {
                            if(argcnt != 3) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
                            }
			    else {
			    //printf("attempting add\n");
                                addNode(list_start,exargs[2]);
			    //printf("added\n");
			    }
		        }
		        else if(strcmp(exargs[1],"remove") == 0) {
		            if(argcnt != 3) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
			    }
			    else {
			        removeNode(head,exargs[2]);
			    }
		        }
		        else if(strcmp(exargs[1],"clear") == 0) {
		            if(argcnt != 2) {
			        write(STDERR_FILENO, error_message, strlen(error_message));
			    }
			    else {
                                freeList(head);
				head = NULL;
			    }
		        }
		        else {
                            write(STDERR_FILENO, error_message, strlen(error_message));
		        }
		    }
		    else {
			if(head != NULL) {
			    strcat(searchpath,head->data);
			}
			strcat(searchpath,exargs[0]);
			//printf("%s",searchpath);
                        if(checkFile(searchpath) != -1) {
			    child = fork();
	                    if(child == 0) {
		            
	                        if(execv(filepath,exargs) == -1) {  //calls to execute the command
	                            write(STDERR_FILENO, error_message, strlen(error_message));
	                        }
	                    }
			    else if(child < 0) {
			        write(STDERR_FILENO, error_message, strlen(error_message));
			    }
			    else {
			    }
	                    while((parent = wait(&status)) > 0);
			}
		    }
		    argcnt = 0;
		    memset(&searchpath[0], 0, sizeof(searchpath));
		    freeArray(exargs);
		    //buffer = NULL;
	    //}
	    //free(storage);
        }
    }
    /*else if(argc == 2) {  //TODO batch mode
        //printf("Batch mode coming soon\n");
	fp = fopen(argv[2],"r");
	if(fp == NULL) {
	    write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
	}
        if((head = (struct Node*)malloc(sizeof(struct Node))) == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            //exit(1);     
	}
	head->data = "/bin";  //default path
        head->next = NULL;
	list_start = &head;
	while(loop == 1) {
            printf(" smash>");
            while((line_size = getline(&buffer,&buffsize,fp)) != -1) {
                strcpy(line,buffer);
                while((storage = strsep(&buffer,";")) != NULL && (strcmp(storage,"") != 0)) {
		    //printf("semicolon sep\n");  //debug statement
		    //printf("%s",storage);  //debug statement
	            while((parallel = strsep(&storage,"&")) != NULL && (strcmp(parallel,"") != 0)) {
                        
                        //printf("ampersand sep\n"); //debug statement
                        while((place = strsep(&parallel," \t\n")) != NULL && (strcmp(place,"") != 0)) {  //separates input and stores each piece in the array exargs
                            //printf("whitespace parsed\n");  //debug statement
                            argcnt++;
                            //printf("counted\n");  //debug statement
                            if((exargs = realloc(exargs, argcnt * sizeof(char*))) == NULL) {  //reallocates space for the new argument to be entered
                                printf("Error reallocing\n");  //debug statement
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
                            }
                            //printf("reallocated\n");  //debug statement
                            if((exargs[argcnt-1] = realloc(exargs[argcnt-1],strlen(place) * sizeof(char))) == NULL) {
                                printf("Error allocating space for string in array\n");  //debug statement
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
                            }
                            //printf("allocated space for string\n");  //debug statement
                            exargs[argcnt-1] = strcpy(exargs[argcnt-1], place);
                        }
			if(exargs == NULL) {
			    break;
			}
                        exargs[argcnt] = NULL;
                        if(strcmp(exargs[0],exitstr) == 0) {
                            free(exargs);
                            exit(0);
                        }
                        else if(strcmp(exargs[0],cdstr) == 0) {
                            printf("changing dir\n");  //debug statement
			    if(argcnt != 2) {
                                //printf("%d\n",argcnt);  //debug statement
				//printf("wrong args\n");
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
                            }
			    else if(chdir(exargs[1]) != 0) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                
                            }
		            //printf("directory changed\n");  //debug statement
                        }
		        else if(strcmp(exargs[0],pathstr) == 0) {
                            if(argcnt == 1) {
			        write(STDERR_FILENO, error_message, strlen(error_message));
			    }
			    else if(strcmp(exargs[1],"add") == 0) {
                                if(argcnt != 3) {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    
                                }
                                addNode(list_start,exargs[2]);
		            }
		            else if(strcmp(exargs[1],"remove") == 0) {
		                if(argcnt != 3) {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    
			        }
			        removeNode(head,exargs[2]);
		            }
		            else if(strcmp(exargs[1],"clear") == 0) {
		                if(argcnt != 2) {
			            write(STDERR_FILENO, error_message, strlen(error_message));
			        }
                                freeList(head);
		            }
		            else {
                                write(STDERR_FILENO, error_message, strlen(error_message));
		            }
		        }
		        else {
                            child = fork();
	                    if(child == 0) {
	  	                //char* name = "/bin/ls";
		                //char** argv_temp = malloc(2*sizeof(char*));
		                //argv_temp[0] = "ls";
		                //argv_temp[1] = NULL;
	                        if(execv(exargs[0],exargs) == -1) {  //calls to execute the command
	                            write(STDERR_FILENO, error_message, strlen(error_message));
	                        }
	                    }
	                    else {
                    
	                    }
		        }
		        //Cleanup and ready for next part of the loop
		        //printf("About to free array\n");  //debug statement
                        freeArray(exargs,argcnt);
		        argcnt = 0;
		        //printf("Freed array\n");  //debug statement
		    //free(place);
		    }
		    //free(parallel);
		}
		//free(storage);
	    }
	}
    }*/
    else {
	write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}
