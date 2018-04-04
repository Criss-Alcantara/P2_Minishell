/* -
 * msh.c
 *
 * Minishell C source
 * Show how to use "obtain_order" input interface function
 *
 * THIS FILE IS TO BE MODIFIED
  */

#include <stddef.h>			/*  NULL  */
#include <stdio.h>			/*  setbuf, printf  */
#include <stdlib.h>			
#include <unistd.h>
#include <string.h>

#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define max_comandos 105
#define max_history 21 /*  numero maximo de comandos que se guardan en el historial */
                       /*  myhistory muestra 'maxhistory'-1 comandos anteriores */

extern int obtain_order();		/*  See parser.y for description  */

struct command{
  /*  Store the number of commands in argvv */
  int num_commands;
  /*  Store the number of arguments of each command */
  int *args;
  /*  Store the commands  */
  char ***argvv;
  /*  Store the I/O redirection */
  char *filev[3];
  /*  Store if the command is executed in background or foreground */
  int bg;
};

void free_command(struct command *cmd){
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
     (*cmd).filev[f] = NULL;
   }
}

void store_command(char ***argvv, char *filev[3], int bg, struct command* cmd){
  int num_commands = 0;
  while(argvv[num_commands] != NULL){
    num_commands++;
  }
  int f;
  for(f=0;f < 3; f++)
  {
    if(filev[f] != NULL){
      (*cmd).filev[f] = (char *) calloc(strlen(filev[f]), sizeof(char));
      strcpy((*cmd).filev[f], filev[f]);
    }
  }
  (*cmd).bg = bg;
  (*cmd).num_commands = num_commands;
  (*cmd).argvv = (char ***) calloc((num_commands+1), sizeof(char **));
  (*cmd).args = (int*) calloc(num_commands, sizeof(int));
  int i;
  for( i = 0; i < num_commands; i++){
    int args= 0;
    while( argvv[i][args] != NULL ){
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **) calloc((args+1), sizeof(char *));
    int j;
    for (j=0; j<args; j++) {
       (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
       strcpy((*cmd).argvv[i][j], argvv[i][j] );
    }
  }
}

void ejecutar_comando (char ***argvv, char *filev[3], int bg, int num_commands);

void MYTIME(int diff_t);

void MYCD(char ***argvv, char *HOME, char *PWD);

void MYHISTORY(char ***argvv, struct command comandos[max_history], int command_counter, char *HOME, char *PWD, time_t comienzo, time_t Final);

int main(void)
{
  char ***argvv;
  int command_counter; /*  guarda el nuero total de comandos introducidos en minishell */
  int num_commands;
  char *filev[3];
  int bg;
  int ret;
  
  setbuf(stdout, NULL);			/*  Unbuffered  */
  setbuf(stdin, NULL);
  
  char HOME[max_comandos];
  char PWD[max_comandos];
  int i;

  getcwd(PWD,max_comandos); /* Obteniendo la ruta actual y cargando en PWD */
  strcpy(HOME,PWD);
  
  /* Funciones para MyTime */
  time_t comienzo, Final;
  time(&comienzo);

  struct command comandos [max_history];
  for (i = 0; i< max_history; i++){ /*  Inicializar con nulos  */
    memset ((void *) (&comandos [i]),'\0',sizeof (comandos [i]));
  }
  
  command_counter = -1;
  
  while (1) {
      fprintf(stderr, "%s", "msh> ");	/*  Prompt  */
      ret = obtain_order(&argvv, filev, &bg);
      if (ret == 0) break;		/*  EOF  */
      if (ret == -1) continue;	/*  Syntax error  */
      num_commands = ret - 1;		/*  Line  */
      if (num_commands == 0) continue;	/*  Empty line  */

      command_counter++;
      if (command_counter < max_history){ /* Solo se guardan los 'max_history' ultimos comandos introducidos  */
        store_command(argvv, filev, bg, &comandos [command_counter]);
      }
      else{ /*  Si se han introducido mas de 'max_history' comandos  */
        for (i=0; i < (max_history -1); i++){
          comandos [i] = comandos [i+1]; /*  Se elimina el comando mas antiguo  */
        }
        store_command(argvv, filev, bg, &comandos [max_history-1]); /*  Se inserta el comando nuevo  */
      }
      if(num_commands == 1){
        /* Comando EXIT */
        if(strcmp(argvv[0][0], "exit") == 0){
          printf("Goodbye!\n");
          int liberar = (command_counter <= (max_history - 1))? command_counter : (max_history-1);
          for (i=0; i <= liberar; i++){
            free_command(&comandos [i]);
          } 
          return 0;
        }
        /* Comando MYTIME */
        else if(strcmp(argvv[0][0], "mytime") == 0){
          time(&Final);
          int diff_t;
          diff_t = difftime(Final, comienzo);
          MYTIME(diff_t);
        }
        /* Comando MYCD */
        else if(strcmp(argvv[0][0], "mycd") == 0){
          MYCD(argvv, HOME, PWD);
        } 
        /* Comando MYHISTORY */
        else if(strcmp(argvv[0][0], "myhistory")==0){
        MYHISTORY(argvv, comandos, command_counter, HOME, PWD, comienzo, Final);
        }
        else{  /* Si es otro mandato simple */
          ejecutar_comando(argvv, filev, bg, num_commands);
        } /* Fin if (si es comando interno o no) */
      }
      else{  /*  si num_comandos es 2 o 3 */
        ejecutar_comando(argvv, filev, bg, num_commands);         
      } /* Fin if 	 */
    } /* Fin while */

  /* Liberar la memoria con free_command */
  int liberar = (command_counter <= max_history - 1)? command_counter : max_history-1;
  
  for (i=0; i <= liberar; i++){
    free_command(&comandos [i]);
  } 
  
  return 0;
} /* end main */

void MYTIME(int diff_t){
  int hr, min;
  hr = diff_t / (60*60);
  diff_t %= 60*60;
  min = diff_t / 60;
  diff_t %= 60;
  printf("Uptime: %d h. %d min. %d s.\n", hr,min,diff_t);
}
 
void MYCD(char ***argvv, char *HOME, char *PWD){
  if(argvv[0][1] == NULL){
    chdir(HOME);
    printf("%s\n",getcwd(PWD,max_comandos));/* Si el cambio es correcto, se actualiza PWD */
  }
  else{
    if(chdir(argvv[0][1])!=0) { /* La funcion Chdir cambia el directorio, si regresa 0 la operacion se realizo con exito, en caso contrario retorna un valor diferente */
      printf("Error! %s no existe o no se puede cambiar a este directorio\n",argvv[0][1]);
    }   
    else {
      printf("%s\n",getcwd(PWD,max_comandos));/* Si el cambio es correcto, se actualiza PWD */
    }
  }
}

void MYHISTORY(char ***argvv, struct command comandos[max_history], int command_counter, char *HOME, char *PWD, time_t comienzo, time_t Final){
  int i,j,k;
  if(argvv[0][1] == NULL){ /*  myhistory sin argumento */
    int imprimir = (command_counter < max_history)? command_counter : max_history - 1;
    for (i = 0; i < imprimir; i++){
      printf ("%d",i);
      for (j = 0; j < comandos[i].num_commands; j++){
        for (k = 0; k < comandos[i].args[j] ; k++){
          printf(" %s", comandos[i].argvv[j][k]);
        }
        if (j+1 != comandos[i].num_commands) printf (" |");
      }
      if (comandos[i].filev[0] != NULL) printf(" < %s", comandos[i].filev[0]);/*  IN  */
      if (comandos[i].filev[1] != NULL) printf(" > %s", comandos[i].filev[1]);/*  OUT  */
      if (comandos[i].filev[2] != NULL) printf(" >& %s", comandos[i].filev[2]);/*  ERR  */
      if (comandos[i].bg) printf(" &");
      printf ("\n");
    }            
  }
  else{ /*  myhistory con argumento */       
    if((comandos[atoi(argvv[0][1])].num_commands == 0) || (atoi(argvv[0][1]) < 0) || (atoi(argvv[0][1]) > 20)){
      fprintf(stderr, "%s","ERROR: Command not found\n");
    }
    else{      
      if(strcmp(comandos[atoi(argvv[0][1])].argvv[0][0], "mytime") == 0){
        time(&Final);
        int diff_t;
        diff_t = difftime(Final, comienzo);
        printf("Running command %s\n", argvv[0][1]);
        MYTIME(diff_t);
      }
      else if(strcmp(comandos[atoi(argvv[0][1])].argvv[0][0], "mycd") == 0){
        printf("Running command %s\n", argvv[0][1]);
        MYCD(comandos[atoi (argvv[0][1])].argvv, HOME, PWD);
      }
      else if(strcmp(comandos[atoi(argvv[0][1])].argvv[0][0], "myhistory")==0){
        /* Evitar llamadas recursivas a si mismo */
        if((atoi (argvv[0][1]) == (max_history-1)) || 
           (atoi (argvv[0][1]) == command_counter) ||
           (atoi (argvv[0][1]) <= atoi(comandos[atoi (argvv[0][1])].argvv[0][1]))){
          fprintf(stderr, "%s","ERROR: Command not found\n");
        }
        else{
          printf("Running command %s\n", argvv[0][1]);
          MYHISTORY(comandos[atoi (argvv[0][1])].argvv, comandos, command_counter, HOME, PWD, comienzo, Final);
        }
      }
      else{
        printf("Running command %s\n", argvv[0][1]);
        ejecutar_comando(comandos[atoi (argvv[0][1])].argvv,
                         comandos[atoi (argvv[0][1])].filev,
                         comandos[atoi (argvv[0][1])].bg,
                         comandos[atoi (argvv[0][1])].num_commands);
      }  
    }
  }        
}
             
void ejecutar_comando(char ***argvv, char *filev[3], int bg, int num_commands){
  if (num_commands==1){
    int pid;
		int estado;
		pid = fork();
		switch(pid) {
		  case -1: /*  error  */
        perror("Error en el fork del mandato simple");
				exit(-1);
      case 0: /*  Primer hijo  */
        printf("Child %d\n",getpid());
        if (filev[0] != NULL) {
          close(STDIN_FILENO);
          open(filev[0],O_RDONLY);
        }
        if (filev[1] != NULL) {
		      close(STDOUT_FILENO);
          open(filev[1],O_CREAT|O_WRONLY,0700);
        }
        if (filev[2] != NULL) {
          close(STDERR_FILENO);
          open(filev[2],O_CREAT|O_WRONLY,0700);
        }
        execvp(argvv[0][0], argvv[0]);
        perror("Error en el execvp del mandato simple");
        exit(-1);
        default: /*  Padre  */
          if(!bg){
		        while (wait(&estado) != pid);
            printf("Wait child %d\n",pid);
          }
				} /* Fin switch */
  }/* Fin del if del Mandato Simple */
  else if(num_commands==2){
		  int pid;
			int estado;
			int fd[2];
			pipe(fd);
			pid = fork();
			switch(pid) {
				case -1: /*  error  */
					perror("Error en el fork del primer mandato");
					exit (-1);		
				case 0: /* Primer hijo  */
          printf("Child %d\n",getpid());
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
						open(filev[2],O_CREAT|O_WRONLY,0700);
					}
					execvp(argvv[0][0], argvv[0]);
				  perror("Error en el execvp del primer mandato");
					exit(-1);
				default: /*  Padre  */
					pid = fork();
					switch(pid) {
						case -1: /*  error  */
							perror("Error en el fork del segundo mandato");
							exit (-1);
						case 0: /*  Segundo hijo */
							close(STDIN_FILENO);
							dup(fd[0]);
							close(fd[0]);
							close(fd[1]);
							if (filev[1] != NULL) {
								close(STDOUT_FILENO);
								open(filev[1],O_CREAT|O_WRONLY,0700);
							}
							if (filev[2] != NULL) {
								close(STDERR_FILENO);
								open(filev[2],O_CREAT|O_WRONLY,0700);
							}
							execvp(argvv[1][0], argvv[1]);
							perror("Error en el execvp del segundo mandato");
							exit(-1);
						default: /*  Padre  */
							close(fd[0]);
							close(fd[1]);
							if(!bg){
								while (wait(&estado) != pid);
                printf("Wait child %d\n",pid);
							}
					} /* Fin del Segundo switch */
			} /* Fin del Primer switch */
  } /* Fin del if de 2 Mandatos */
  else if(num_commands==3){
			int pid;
			int estado;
			int fd[2], fd2[2];
			pipe(fd);
			pid = fork();
			switch(pid) {
				case -1: /*  error  */
					perror("Error en el fork del primer mandato");
					exit (-1);
				case 0: /* Primer hijo */
          printf("Child %d\n",getpid());
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
						open(filev[2],O_CREAT|O_WRONLY,0700);
					}
					execvp(argvv[0][0], argvv[0]);
					perror("Error en el execvp del primer mandato");
					exit(-1);
				default: /*  Padre  */
					pipe(fd2);
					pid = fork();
					switch(pid) {
						case -1: /*  error  */
							perror("Error en el fork del segundo mandato");
							exit (-1);
						case 0: /* Segundo Hijo */
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
								open(filev[2],O_CREAT|O_WRONLY,0700);
							}
							execvp(argvv[1][0], argvv[1]);
              perror("Error en el execvp del segundo mandato");
							exit(-1);
						default: /*  Padre  */
							close(fd[0]);
							close(fd[1]);
							pid = fork();
							switch(pid) {
								case -1: /*  error  */
                  perror("Error en el fork del tercer mandato");
									exit (-1);
								case 0: /* Tercer hijo */
									close(STDIN_FILENO);
									dup(fd2[0]);
									close(fd2[0]);
									close(fd2[1]);
									if (filev[1] != NULL) {
									close(STDOUT_FILENO);
									open(filev[1],O_CREAT|O_WRONLY,0700);
									}
									if (filev[2] != NULL) {
										close(STDERR_FILENO);
										open(filev[2],O_CREAT|O_WRONLY,0700);
									}
									execvp(argvv[2][0], argvv[2]);
                  perror("Error en el execvp del tercer mandato");
									exit(-1);
								default: /*  Padre  */
									close(fd2[0]);
									close(fd2[1]);
									if(!bg){
										while (wait(&estado) != pid);
                    printf("Wait child %d\n",pid);
									}
							} /* Fin del Tercer switch */
					} /* Fin del Segundo switch */
			} /* Fin del Primer switch */
  } /* Fin del if de 3 Mandatos */
  else{
    fprintf(stderr, "%s","Error en la cantidad de mandatos en secuencia, se limita a 3\n");
  }
}/* Fin */
