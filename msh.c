/*-
 * msh.c
 *
 * Minishell C source
 * Show how to use "obtain_order" input interface function
 *
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>			
#include <unistd.h>
#include <string.h>

#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define maxln_Com_Amb 105 /*Numero de caracteres maximo para comando las variables de ambiente*/

extern int obtain_order();		/* See parser.y for description */

struct command{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands 
  char ***argvv;
  // Store the I/O redirection
  char *filev[3];
  // Store if the command is executed in background or foreground
  int bg;
};

void free_command(struct command *cmd)
{
   if((*cmd).argvv != NULL){
     char **argv;
     for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
     {
      for (argv = *(*cmd).argvv; argv && *argv; argv++)
      {
        if(*argv){
           free(*argv);
           *argv = NULL;
        }
      }
     }
   }
   free((*cmd).args);
   int f;
   for(f=0;f < 3; f++)
   {
     free((*cmd).filev[f]);
   }
}

void store_command(char ***argvv, char *filev[3], int bg, struct command* cmd)
{
  int num_commands = 0;
  while(argvv[num_commands] != NULL){
    num_commands++;
  }

  int f;
  for(f=0;f < 3; f++)
  {
    if(filev[f] != NULL){
      (*cmd).filev[f] = (char *) malloc(strlen(filev[f]) * sizeof(char));
      strcpy((*cmd).filev[f], filev[f]);
    }
  }  

  (*cmd).bg = bg;
  (*cmd).num_commands = num_commands;
  (*cmd).argvv = (char ***) malloc((num_commands+1) * sizeof(char **));
  (*cmd).args = (int*) malloc(num_commands * sizeof(int));
  int i;
  for( i = 0; i < num_commands; i++){
    int args= 0;
    while( argvv[i][args] != NULL ){
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **) malloc((args+1) * sizeof(char *));
    int j;
    for (j=0; j<args; j++) {
       (*cmd).argvv[i][j] = (char *)malloc(strlen(argvv[i][j])*sizeof(char));
       strcpy((*cmd).argvv[i][j], argvv[i][j] );
    }
  }
}

int main(void)
{
  char ***argvv;
	//int command_counter;
	int num_commands;
	//int args_counter;
	char *filev[3];
	int bg;
	int ret;
  
	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);
  
  char HOME[maxln_Com_Amb];
  char PWD[maxln_Com_Amb];

  int tick = 0;
  char aux_comandos[20][maxln_Com_Amb];
  memset(aux_comandos,'\0',maxln_Com_Amb);
  
  getcwd(PWD,maxln_Com_Amb); /*Obteniendo la ruta actual y cargando en PWD*/
  
  /*Funciones para MyTime*/
  time_t comienzo, final;
  time(&comienzo);
  strcpy(HOME,PWD);
  
  /*Funciones para MyEstructure*/
  
	while (1) 
	{
    struct command cmd;
		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		num_commands = ret - 1;		/* Line */
		if (num_commands == 0) continue;	/* Empty line */
    store_command(argvv, filev, bg, &cmd);
   

    if(tick <= 19){
      strcpy(aux_comandos[tick], cmd.argvv[0][0]);
      for( int i = 0; i < cmd.num_commands; i++){
          for (int j=0; j< cmd.args[i]; j++) {
             if((i >= 0 && j >= 1) || (i >= 1 && j >= 0) ){
                strcat(aux_comandos[tick], " ");
                strcat(aux_comandos[tick], cmd.argvv[i][j]);
             }
          }
        if(i+1 != cmd.num_commands){
           strcat(aux_comandos[tick], " |");
        }
      }
      
		  if (cmd.filev[0] != NULL){
         strcat(aux_comandos[tick], " < ");  
         strcat(aux_comandos[tick], cmd.filev[0]);      
      } 

      if (cmd.filev[1] != NULL){
         strcat(aux_comandos[tick], " > ");  
         strcat(aux_comandos[tick], cmd.filev[1]);      
      }
      
      if (cmd.filev[2] != NULL){
         strcat(aux_comandos[tick], " >& ");  
         strcat(aux_comandos[tick], cmd.filev[2]);      
      }
                                              
      if(cmd.bg == 1){
        strcat(aux_comandos[tick], " &");      
      }
        
    }
    else{
      strcpy(aux_comandos[tick%20], cmd.argvv[0][0]);
      for( int i = 0; i < cmd.num_commands; i++){
          for (int j=0; j<cmd.args[i]; j++) {
             if(i != 0 && j != 0){
                strcat(aux_comandos[tick%20], " ");
                strcat(aux_comandos[tick%20], cmd.argvv[i][j]);
             }
          }
        if(i+1 != cmd.num_commands){
           strcat(aux_comandos[tick%20], "| ");
        }
      }
    }

      if(num_commands == 1){
      /*Comando EXIT*/
      if(strcmp(argvv[0][0], "exit") == 0){
          printf("Goodbye!\n");
          free_command(&cmd);
          exit(-1);
      }
      /*Comando MYTIME*/
      else if(strcmp(argvv[0][0], "mytime") == 0){
        time(&final);
        int diff_t, hr, min;
        diff_t = difftime(final, comienzo);
        hr = diff_t / (60*60);
        diff_t %= 60*60;
        min = diff_t / 60;
        diff_t %= 60;
        printf("Uptime: %d h. %d min. %d s.\n", hr,min,diff_t);
      }
      /*Comando MYCD*/
      else if(strcmp(argvv[0][0], "mycd") == 0){
        if(argvv[0][1] == NULL){
            chdir(HOME);
            printf("%s\n",getcwd(PWD,maxln_Com_Amb));/*En caso de cambio exitoso actualizar PWD*/
        }
        else{
            if(chdir(argvv[0][1])!=0) { /*La func chdir hace el cambio de directorio si regresa un valor diferente de cero la operacion no se pudo ejecutar con exito*/
              printf("Error! %s no existe o no se puede cambiar a este directorio\n",argvv[0][1]);
            }   
            else {
                printf("%s\n",getcwd(PWD,maxln_Com_Amb));/*En caso de cambio exitoso actualizar PWD*/
            }
        }
      } 
      /*Comando MYHISTORY*/
      else if(strcmp(argvv[0][0], "myhistory")==0){
        if(argvv[0][1] == NULL){
          if(tick < 20){
            for(int i = 0 ; i < tick; i++){
               printf("%d %s\n",i, aux_comandos[i]);
            }
          }
          else{
            for(int i = 19; i >= 0; i--){
              printf("%d %s\n",i, aux_comandos[i]);
            }
          }  
        }
        else{
          if((strlen(aux_comandos[atoi(argvv[0][1])])==0) || (atoi(argvv[0][1]) < 0) || (atoi(argvv[0][1]) > 20)){
            fprintf(stderr, "%s","ERROR: Command not found\n");
          }
          else{
            printf("Running command %s\n", argvv[0][1]);
          }
        }
      }
      else{  //Si es otro mandato
				int pid;
				int estado;
				pid = fork();
				switch(pid) {   
					case -1: /* error */
						fprintf(stderr, "%s","Error en el fork del mandato simple\n");
						exit(-1);

					case 0: /* hijo */
           // printf("Child %d\n",pid);
						if (filev[0] != NULL) {
							close(STDIN_FILENO);
							open(filev[0],O_RDONLY);
						}

						if (filev[1] != NULL) {
							close(STDOUT_FILENO);
							open(filev[1],O_CREAT|O_WRONLY,0666);
						}

						if (filev[2] != NULL) {
							close(STDERR_FILENO);
							open(filev[2],O_CREAT|O_WRONLY,0666);
						}
						execvp(argvv[0][0], argvv[0]);
						fprintf(stderr, "%s","Error en el execvp del mandato simple\n");
						exit(-1);
						
					default: /* padre */
						if(!bg){
              //printf("Wait child %d\n",pid);
							while (wait(&estado) != pid);
						}else printf("%d\n",pid);
		
				} //fin switch (1 mandato)
			} //fin if (si es comando interno o no)	
    }
    else if(num_commands==2){
			int pid;
			int estado;
			int fd[2];
			pipe(fd);
			pid = fork();
			switch(pid) {
				case -1: /* error */
					fprintf(stderr, "%s","Error en el fork del primer mandato\n");
					exit(-1);
					
				case 0: /* hijo1 */
					close(STDOUT_FILENO);
					dup(fd[1]);
					close(fd[0]);
					close(fd[1]);

					if (filev[0] != NULL) {
						close(STDIN_FILENO);
						open(filev[0],O_RDONLY);
					}

					if (filev[2] != NULL) {
						close(STDERR_FILENO);
						open(filev[2],O_CREAT|O_WRONLY,0666);
					}
					execvp(argvv[0][0], argvv[0]);
					fprintf(stderr, "%s","Error en el execvp del primer mandato\n");
					exit(-1);
					
				default: /* padre */
					pid = fork();
					switch(pid) {
						case -1: /* error */
							fprintf(stderr, "%s","Error en el fork del segundo mandato\n");
							exit(-1);
							
						case 0: /* hijo 2*/
							close(STDIN_FILENO);
							dup(fd[0]);
							close(fd[0]);
							close(fd[1]);

							if (filev[1] != NULL) {
								close(STDOUT_FILENO);
								open(filev[1],O_CREAT|O_WRONLY,0666);
							}

							if (filev[2] != NULL) {
								close(STDERR_FILENO);
								open(filev[2],O_CREAT|O_WRONLY,0666);
							}
							execvp(argvv[1][0], argvv[1]);
							fprintf(stderr, "%s","Error en el execvp del segundo mandato\n");
							exit(-1);
							
						default: /* padre */
							close(fd[0]);
							close(fd[1]);
							if(!bg){
								while (wait(&estado) != pid);
							}else printf("%d\n",pid);
					} //fin switch2 (2 mandatos)	
			} //fin switch1 (2 mandatso)
		}
    else if(num_commands==3){
			int pid;
			int estado;
			int fd[2], fd2[2];
			pipe(fd);
			pid = fork();
			switch(pid) {
				case -1: /* error */
					fprintf(stderr, "%s","Error en el fork del primer mandato\n");
					exit(-1);
					
				case 0: /* hijo1 */
					close(STDOUT_FILENO);
					dup(fd[1]);
					close(fd[0]);
					close(fd[1]);

					if (filev[0] != NULL) {
						close(STDIN_FILENO);
						open(filev[0],O_RDONLY);
					}

					if (filev[2] != NULL) {
						close(STDERR_FILENO);
						open(filev[2],O_CREAT|O_WRONLY,0666);
					}
					execvp(argvv[0][0], argvv[0]);
					fprintf(stderr, "%s","Error en el execvp del primer mandato\n");
					exit(-1);
					
				default: /* padre */
					pipe(fd2);
					pid = fork();
					switch(pid) {
						case -1: /* error */
							fprintf(stderr, "%s","Error en el fork del segundo mandato\n");
							exit(-1);
							
						case 0: /* hijo 2*/
							close(STDIN_FILENO);
							dup(fd[0]);
							close(STDOUT_FILENO);
							dup(fd2[1]);
							close(fd[0]);
							close(fd[1]);
							close(fd2[0]);
							close(fd2[1]);

							if (filev[2] != NULL) {
								close(STDERR_FILENO);
								open(filev[2],O_CREAT|O_WRONLY,0666);
							}
							execvp(argvv[1][0], argvv[1]);
							fprintf(stderr, "%s","Error en el execvp del segundo mandato\n");
							exit(-1);
							
						default: /* padre */
							close(fd[0]);
							close(fd[1]);
							pid = fork();
							switch(pid) {
								case -1: /* error */
									fprintf(stderr, "%s","Error en el fork del tercer mandato\n");
									exit(-1);
									
								case 0: /* hijo 3*/
									close(STDIN_FILENO);
									dup(fd2[0]);
									close(fd2[0]);
									close(fd2[1]);
									
									if (filev[1] != NULL) {
									close(STDOUT_FILENO);
									open(filev[1],O_CREAT|O_WRONLY,0666);
									}
									

									if (filev[2] != NULL) {
										close(STDERR_FILENO);
										open(filev[2],O_CREAT|O_WRONLY,0666);
									}
									execvp(argvv[2][0], argvv[2]);
									fprintf(stderr, "%s","Error en el execvp del tercer mandato\n");
									exit(-1);
									
								default: /* padre */
									close(fd2[0]);
									close(fd2[1]);
									if(!bg){
										while (wait(&estado) != pid);
									}else printf("%d\n",pid);
									
							} 
							
					} 
			} 	
		} 
    tick++;
	} //fin while 

	return 0;

} //end main