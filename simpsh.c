#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>

int openFile(char *opt,int flags);
bool isComValid(int *fdArr, char *args[],int numFiles, int *ioe);
void exeCom(char *args[], int* ioe);
int numArg(char *args[]);
void printOpt(bool isVerbose, char *args[], int index);
void closeFds(int *fdArr, int numFiles);
int * fAlloc(int *fdArr, int numFiles, int *nFd);
int ctoi(char* numChar);


int main(int argc, char *argv[]){

  int *fdArr = NULL;
  int numFiles = 0;
  int nFd = 0;
  fdArr = fAlloc(fdArr,numFiles, &nFd);

  struct option longopts[] = { //all options
    {"append", no_argument, NULL, '/'},
    {"cloexec", no_argument, NULL, '0'},
    {"creat", no_argument, NULL, '1',},
    {"directory", no_argument, NULL, '2'},
    {"dsync", no_argument, NULL, '3'},
    {"excl", no_argument, NULL, '4'},
    {"nofollow", no_argument, NULL, '5'},
    {"nonblock", no_argument, NULL, '6'},
    {"rsync", no_argument, NULL, '7'},
    {"sync", no_argument, NULL, '8'},
    {"trunc", no_argument, NULL, '9'},

    {"rdonly", required_argument, NULL, 'r' },
    {"wronly", required_argument, NULL, 's' },
    {"rdwr", required_argument, NULL, 't' },
    {"command", no_argument, NULL, 'c' },
    {"verbose", no_argument, NULL, 'v' },
    {0, 0, 0, 0 },
  };

  int fileOpt[11] = {O_APPEND, O_CLOEXEC, O_CREAT, 
		     O_DIRECTORY, O_DSYNC, O_EXCL,
		     O_NOFOLLOW, O_NONBLOCK, O_RSYNC,
		     O_SYNC, O_TRUNC }; //array to hold file options

  int perOpt[3] = {O_RDONLY, O_WRONLY, O_RDWR}; //array for file permission

  //getOpt related
  int opt;
  int longindex;
  int prevInd = 1;

  //fork related
  int p_id;
  int numProcess = 0;

  //option indicators
  bool isVerbose = false;
  bool doWait = false;

  //file option related
  int flags = 0; //accumulator for flags of file opening options
  int fd;


  //command option temporary variables
  int ioe[3];
  bool isValid;


  //parent related variables
  int retCode = 0;

  while ((opt = getopt_long(argc, argv, "/0123456789r:s:t:cv", longopts, &longindex)) != -1){

    printOpt(isVerbose, argv, prevInd);
    switch(opt) {

      //FILE FLAG OPTIONS
    case '/':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      flags = flags | (fileOpt[ opt - 47]); //add the corresponding file option to flags
      break;

    case 'r':
    case 's':
    case 't':
      //check that array is not full
      if(numFiles + 1 >= nFd){
	fdArr = fAlloc(fdArr, numFiles, &nFd);
      }
      fd = openFile(argv[optind - 2],flags | perOpt[opt - 114]);
      //if failed to open, return code should be set to 1
      if(fd == -1){
	retCode = 1;
      }
      //else, number of files should be increased
      else{
	fdArr[numFiles] = fd;
	numFiles++;
      }
      flags = 0; //reset the flag
      break;

    case 'c':
      isValid = isComValid(fdArr, argv + optind, numFiles, ioe);

      if (!isValid){ //if command is not valid, set return code to 1 and ignore the command
	fprintf(stderr, "Skipping this command. \n");
	retCode = 1;
	break;
      }
      else{

      p_id = fork();
      if(p_id == 0){ //child

	exeCom(argv + optind, ioe);
	return 0;
      }
      else{ //parent
	optind += numArg(argv + optind);
	numProcess++;
      }
      

      }
      break;
    case 'v':
      isVerbose = true;
      break;
    default:
      printf("not an option\n");

    }

    prevInd = optind;
  } 


  return retCode;


}

int openFile(char *opt, int flags){ //opens the file (if not invalid)

  int oStatus;

  mode_t wrAll = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  if (optarg[0] == '-' && optarg[1] == '-'){
    fprintf(stderr, "No argument passed for %s. Skipping this option.\n", opt);
    optind--;
    return -1;
  }
  oStatus = open(optarg, flags, wrAll);
  if (oStatus == -1){
    perror(optarg);
    fprintf(stderr, "Failed to open the file. Skipping this option.\n");
    return -1;
  }


  return oStatus;

}


bool isComValid(int *fdArr, char *args[], int numFiles, int *ioe){

  //check argument exists
  int i;
  for(i = 0;i < 3;i++){
    if (args[i] == NULL){
      fprintf(stderr, "Invalid number of arguments. \n");
      return false;
    }

    //check that each argument is a number
    int fd;
    fd = ctoi(args[i]);
    if(fd < 0){
      return false;
    }
    //check that resulting fd is within range
    if (fd >= numFiles || fdArr[i] == -1){ 
      fprintf(stderr, "Invalid file descriptor. Either the file descriptor is invalid (they must be between 0 and %d.), or is already closed.\n", numFiles - 1);
      return false;
    }
    //assign fd to appropriate place
    ioe[i] = fdArr[fd];
  }

  //check that command exists
  if (args[3] == NULL){ //check that command exists
    fprintf(stderr, "Must have name of the command. \n");
    return false;
  }
  

  return true;

}

void exeCom(char *args[], int *ioe){

  int l;
  for(l = 4; args[l] != NULL; l++){ //set one after last element to null
    if (args[l][0] == '-' && args[l][1] == '-'){
      args[l] = NULL;
      break;
    }
    else{
      continue;
    }
  }

  int i;
  for (i = 0; i < 3; i++){
    close(i);
    dup(ioe[i]);
    close(ioe[i]);
  }

  execvp(args[3], (args + 3));


}



int numArg(char *args[]){

  int i;
  int nArgs = 0;
  for (i = 0; args[i] != NULL; i++){
    if (args[i][0] == '-' && args[i][1] == '-'){
      return nArgs;
    }

    else{
      nArgs++;
    }

  }
  return nArgs;
}

void printOpt(bool isVerbose, char *args[], int index){

  if (isVerbose == 0){
    return;
  }
  int numPrint = numArg(args + index + 1);
  numPrint++; //
  int i;
  for (i = 0; i < numPrint; i++){

    if(i + 1 == numPrint){
      printf("%s\n", args[index + i]);
    }
    else{
      printf("%s ", args[index + i]);
    }
  }

}

void closeFds(int *fdArr, int numFiles){

  int i;
  for (i = 0; i < numFiles; i++){
    if(fdArr[i] != -1){
      if(close(fdArr[i]) == -1){
	fprintf(stderr, "Error closing file descriptor %d", i);
	perror(NULL);
	fprintf(stderr, "\n");
      }
    }
  }

  free(fdArr);


}
int *fAlloc(int *fdArr, int numFiles, int *nFd){

  if(fdArr == NULL){ //malloc
    fdArr = (int*)malloc((*nFd + 100) * sizeof(int));
    if(fdArr == NULL){
      fprintf(stderr, "Failed to allocate memory for the file descriptors.");
      closeFds(fdArr, numFiles);
      _exit(1);
    }
    else{
      *nFd += 100;
    }
  }


    else{ //realloc
      int *temp;
      temp = (int*)realloc(fdArr,(*nFd + 100) * sizeof(int));
      if(temp == NULL){
	fprintf(stderr, "Failed to reallocate memory for the file descriptors.\n");
	closeFds(fdArr, numFiles);
	_exit(1);
      }
      else{
	*nFd += 100;
	fdArr = temp;
      }



    }

  return fdArr;
}

int ctoi(char *numChar){

  int digits = 0;
  int i;
  //check number of digits, and make sure each character is a number
  for(i = 0; numChar[i] != '\0'; i++){
      if ((int)numChar[i] - 48 < 0 || (int)numChar[i] - 48 > 9)
	{
	  fprintf(stderr, "File descriptor must be a number.\n");
	  return -1;
	}
      digits++;
    }
    
    int fd = 0;
    int mult = 1;
    int k;
    for (k = digits - 1; k >=0; k--){ //convert characters to digits
      fd += ((int) numChar[k] - 48) * mult;
      mult *= 10;
    }

    return fd;

}
