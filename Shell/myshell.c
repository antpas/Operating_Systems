#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

char ** inputRead()
{

  int bufferSize = 600;
  char input[bufferSize];
  int wordNum = 0;
  int charNum = 0;
  int i;
  int flag = 0;

  //fgets(input, sizeof(input), stdin);
  //Read from input stream
  if(fgets(input, sizeof(input), stdin) == NULL){
    printf("\n");
    exit(0);
  }
  int length = strlen(input);

  //Dynamically allocate string array
  char **parseChars = malloc(length * sizeof(char *));
  for (i = 0; i < length; ++i) {
      parseChars[i] = (char *)malloc(length+1);
  }
  //Custom Tokenize State Machine
  for(i = 0; i < length; i++)
  {
    if(!isspace(input[i]))
    {
      if(input[i] == '<' || input[i] == '>'
         || input[i] == '|' || input[i] == '&')
      { 
        charNum = 0;
        parseChars[wordNum][charNum] = input[i];
        if(!isspace(input[i+1]))
          wordNum++;
      }
      else //if regular token
      {
        parseChars[wordNum][charNum] = input[i];
        charNum++;
        if(input[i+1] == '<' || input[i+1] == '>' 
           || input[i+1] == '|' || input[i+1] == '&')
        {
          wordNum++;
        }
      }
    }
    else //is space
    {
      if(!isspace(input[i-1]) && i != 0){
        wordNum++;
        charNum = 0;
      }
    }
  }
    parseChars[wordNum] = NULL;
    return parseChars;
}

char ***alloc_array(int x, int y) {
  char ***a = calloc(x, sizeof(char **));
  int i;
  for(i = 0; i != x; i++) {
      a[i] = calloc(y, sizeof(char *));
  }
  return a;
}

void organizeAndExecute(char **parseChars){
  char ***cmds = alloc_array(20,20);
  int lengthArray = 0;
  int background = 0;
  int redirectOut = 0;
  int redirectIn = 0;
  int i = 0, j = 0, count = 0;
    
  //Check for special characters
  lengthArray = 0;
  while(parseChars[lengthArray] != NULL){
    lengthArray = lengthArray + 1;
  }
  
  for(i=0;i<lengthArray;i++){
    if(strcmp("|", parseChars[i]) == 0){
      count = count +  1;
      j=0;
    }
    else if(strcmp("&", parseChars[i]) == 0){
      //i = i + 1;
      background = 1;
    }
    else if(strcmp(">", parseChars[i]) == 0){
      //cmds[count][j] = NULL;
      count = count + 1;
      j = 0;
      cmds[count][j] = parseChars[i];
      cmds[count][j+1] = parseChars[i+1];
      cmds[count][j+2] = NULL;
      redirectOut = 1;
      //count = count + 1;
      j =0;
      i = i + 1;
      continue;
    }
    else if(strcmp("<", parseChars[i]) == 0){
      cmds[count][j] = NULL;
      //count = count + 1;
      //j = 0;
      cmds[count][j] = parseChars[i+1];
      redirectIn = 1;
      //count = count + 1;
      j =0;
      i = i + 1;
      continue;
    }
    else{
      cmds[count][j] = parseChars[i];
      j = j + 1;

    }
    if(i+1>lengthArray){
      cmds[count][j] = parseChars[i];
    }
  }

  executeCommands(cmds, background, redirectOut, redirectIn);
}
static void redirect(int oldp, int newp){

  if(oldp != newp){
    if(dup2(oldp,newp) != -1){
      close(oldp);
    }
    else{
      exit(EXIT_FAILURE);
    }
  }
}

static void run(char **cmd, int input, int out){
  redirect(input, STDIN_FILENO);
  redirect(out, STDOUT_FILENO);

  if(execvp(cmd[0], cmd) == -1) {
      perror(cmd[0]);
  }
  //exit(0);
}

int executeCommands(char ***cmds, int background, int redirectOut, int redirectIn){

  pid_t pid;
  int p[2];
  int input = 0;
  const int READ = 0;
  const int WRITE = 1;
  int i = 0, j, status, temp;

  while (cmds[i][0] != NULL)
  {

    int p[2];
    pid_t pid;

    if(strcmp(cmds[i][0], ">") == 0 || strcmp(cmds[i][0], "<") == 0){
      i++;
      continue;
    }

    if(pipe(p) == -1){
      exit(EXIT_FAILURE);
    }
    else if((pid = fork()) == -1){
      exit(EXIT_FAILURE);
    }
    else if(pid == 0) //Child
    {
      // > Logic for unlimited output redirects
      if(cmds[i+1][0] != NULL && strcmp(cmds[i+1][0], ">") == 0){
          temp = i + 1;
          while(strcmp(cmds[temp][0], ">") == 0){
            p[1] = creat(cmds[temp][1] , 0644);
            temp++;
            if(cmds[temp][0] == NULL){
              break;
            }
          }
          run(cmds[i], input, p[1]);
      }

      else if(cmds[i+1][0] != NULL && strcmp(cmds[i+1][0], "<") == 0){
          p[0] = open(cmds[i+1][1], O_RDONLY);
          run(cmds[i], input, p[0]);
      }
      
      else{
        if(cmds[i+1][0] != NULL)
          run(cmds[i], input, p[1]);
        else{
          run(cmds[i], input, STDOUT_FILENO);
        }
        exit(EXIT_FAILURE);
      }
    }
    else{ //Parent

      if(background == 0){
        waitpid(pid, &status, WUNTRACED);
        wait(NULL);
      }
      else if(background == 1)
      {
        //waitpid(fork,&status,WUNTRACED);
      }

      close(p[1]);
      input = p[0];
      i++;
    }
  }

  if(background == 1){
    printf("[%d] %d\n", getppid(), getpid());
  }
  background = 0;
}

int main(int argc, char *argv[]) {
   
  char **parseChars;
  char ***cmds;
  int i = 0;
  if (argc <= 1) {
    argv[1] = "l";
  }
    printf("my_shell\n");
  
  
  while(1){
    if(strcmp(argv[1],"n") != 0){
      printf("shell:");
    }
    cmds = alloc_array(20,20);
    parseChars = inputRead();
    organizeAndExecute(parseChars);
  }

  free(parseChars);
  free(cmds);
  printf("\n");

  return 0;
}
