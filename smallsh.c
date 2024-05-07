#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

void cd_(char* arg);
void exit_(char* arg);
char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);
char bgpid[20];
char status[20];
int child(char **words_, size_t num_words);
void handle_SIGINT(int signum);
struct sigaction sa, old_action;
struct sigaction sa_2, old_action_2;
void handle_SIGINT(int sig);


void handle_SIGINT(int sig){
	// signal handler does nothing
}


int main(int argc, char *argv[])
{


  sa_2.sa_handler = SIG_IGN;
  sa_2.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa_2, &old_action_2);

  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTSTP, &sa, &old_action);

  FILE *input = stdin;
  char *input_fn = "(stdin)";
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  char *line = NULL;
  size_t n = 0;
  for (;;) {
 
     char *ps1 = getenv("PS1");
     if (ps1 == NULL) {
	fprintf(stdout, "");
     } else {fprintf(stderr, "%s", ps1);}
     
//prompt:;
    /* TODO: Manage background processes */

     pid_t my_pid = getpid();
     pid_t pgid = getpgid(my_pid);

     int status;
     pid_t result;

     do {
	result = waitpid(-pgid, &status, WNOHANG | WUNTRACED);
	if (result > 0) {
	    if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		fprintf(stderr, "Child process %d done. Exit status %d.\n", result, exit_status);
	    } else if (WIFSIGNALED(status)) {
		fprintf(stderr, "Child process %d done. Signaled %d.\n", result, WTERMSIG(status));
	    } else if (WIFSTOPPED(status)) {
		fprintf(stderr, "Child process %d stopped. Continuing.\n", result);
		kill(result, SIGCONT);	
	    }
	} else if (result == -1 && errno != ECHILD) {
	    perror("waitpid failed");
	    exit(1);
	}
      } while (result > 0);



    /* TODO: prompt */
    if (input == stdin) {

    }

     sa_2.sa_handler = handle_SIGINT;
     sa_2.sa_flags = 0;
     sigemptyset(&sa.sa_mask);
     sigaction(SIGINT, &sa_2, NULL); 

    ssize_t line_len = getline(&line, &n, input);
    if (line_len < 0) {
	if (errno == EINTR) {
	   clearerr(input);
	   fprintf(stderr, "\n");
	   continue;
	}
	else if (feof(input)) {
	  int exit_code = 0;
	  exit(exit_code);
	}
	else err(1, "%s", input_fn);
    }

    sa_2.sa_handler = SIG_IGN;
    sa_2.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa_2, NULL);


    if (line_len == 1){
	    continue;
    }
    size_t nwords = wordsplit(line);
    if (nwords == 0) continue; 

   for (size_t i = 0; i < nwords; ++i) {
      /*fprintf(stderr, "Word %zu: %s  -->  ", i, words[i]);*/
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
     /* fprintf(stderr, "%s\n", words[i]);*/
    }

    if (strcmp(words[0], "exit") == 0){
	  	if (nwords > 2){
	    fprintf(stdout, "smallsh: exit: too many arguments\n");
	} else if (nwords == 1) {
		int exit_code = 0;
		exit(exit_code);
	} else {
	exit_(words[1]);
	}
    } else if (strcmp(words[0], "cd") == 0){
	if (nwords > 2){
	fprintf(stdout, "smallsh: cd: too many arguements\n");
	} else if (nwords == 1) {
  	    char empty[1];
	    cd_(empty);
	} else {
	  cd_(words[1]); 
	}
    }
     else {
	child(words, nwords);
    }


   /* for (size_t i = 0; i < nwords; ++i) {
      fprintf(stderr, "Word %zu: %s  -->  ", i, words[i]);
      printf("we got here ");
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
      fprintf(stderr, "%s\n", words[i]);
    }*/
  }
}

char *words[MAX_WORDS] = {0};

/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char **start, char **end)
{
  static char *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = NULL;
  *end = NULL;
  char *s = strchr(word, '$');
  if (s) {
    char *c = strchr("$!?}", s[1]);
    if (c) {
      ret = *c;
      *start = s;
      *end = s + 2;
    }
    else if (s[1] == '{') {
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = '{';
        *start = s;
        *end = e + 1;
	}
      else {
	ret = '[';
	*start = s;
	*end = s + 2;
      }
    } 
  }
  prev = *end;
  return ret;
}

/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);
  while (c) {
    if (c == '!'){
	    if (bgpid[0] == '\0') {
	    build_str("", NULL);
	    } else{
	    build_str(bgpid, NULL);}}
    else if (c == '$') {
	    pid_t pid = getpid();
	    char pid_str[20];
	    sprintf(pid_str, "%d", pid);
	    build_str(pid_str, NULL);}
    else if (c == '?'){ 
	    if (status[0] == '\0'){
	      build_str("0", NULL);
	    } else {
	    build_str(status, NULL);
    }}
    else if (c == '}') {
	build_str("$}", NULL);}

    else if (c == '[') {
	build_str("${", NULL);
    }
    else if (c == '{') {
      char param_str[100];
      int param_len = (end - 1) - (start + 2);
      strncpy(param_str, start + 2, param_len);
      param_str[param_len] = '\0';
      char *var = getenv(param_str);
     /* printf("param str: %s\n", param_str); 
      printf("getenv: %s\n", var); */
      if (var == NULL) build_str("", NULL);
      else build_str(var, NULL);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}



void exit_(char *status_){
    char *endptr;

    long int stat_num = strtol(status_, &endptr, 10);
    if (status_[0] == '\0' || *endptr != '\0') {
	fprintf(stderr, "smallsh: exit: %s is not a number\n", status_);
    } else {	
	exit(stat_num);
    }	
}


void cd_(char *dir_){
    if (dir_[0] == '\0'){
	char *new_dir = getenv("HOME");
	int result = chdir(new_dir);
	if (result == -1){
	fprintf(stderr, "smallsh: cd: Failed to change directory to %s\n", new_dir);
	} /* else {
	  printf("successful cd to %s\n", new_dir);
	}*/
    } else {
        int result = chdir(dir_);
	if (result == -1){
	fprintf(stderr, "smallsh: cd: Failed to change directory to %s\n", dir_);
	}/* else {
	printf("successful cd to %s\n", dir_);
	}*/
    }

   
}


int child(char **words_, size_t num_words){

	int background = 0;
	if (num_words > 0 && strcmp(words_[num_words - 1], "&") == 0){
		background = 1;
	}

	pid_t child_pid;
	child_pid = fork();

	if (child_pid == -1){
	    fprintf(stderr, "fork failed");
	    exit(1);}
	else if (child_pid == 0) {
	/* printf("child successful! \n");*/	
	
	if (sigaction(SIGTSTP, &old_action, NULL) == -1){
		perror("sigaction could not reset sigtstp");
		return 1;
	}

	if (sigaction(SIGINT, &old_action_2, NULL) == -1){
		perror("sigaction could not reset sigint");
		return 1;
	}
	    

	char *child_arg[num_words+1];
	int child_arg_i = 0;

      	int fd, i = 0;
	while (i < num_words) {
		background = 0;
	    if (strcmp(words_[i], "<") == 0) {
		fd = open(words_[i+1], O_RDONLY);
		if (fd == -1) {
		   perror("Could not open input file");
		   exit(EXIT_FAILURE);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
		i += 2;
	    } else if (strcmp(words_[i], ">") == 0){	
		fd = open(words_[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (fd == -1) {
		   perror("Could not open outputfile");
		   exit(EXIT_FAILURE);
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
		i += 2;
	    } else if (strcmp(words_[i], ">>") == 0){
		fd = open(words_[i+1], O_WRONLY| O_CREAT | O_APPEND, 0777);
		if (fd == -1) {
		   perror("Could not open outputfile");
		   exit(EXIT_FAILURE);
		} 
		dup2(fd, STDOUT_FILENO);
		close(fd);
		i += 2;
	    }/* else if ((strcmp(words_[i], "&") == 0) && (i == (num_words - 1))){
		printf("found the &\n");
		background = 1;
		printf("bg = %d\n", background);
		i += 1;
	    }*/else { 
		    if ((strcmp(words_[i], "&") == 0) && (i == (num_words - 1))){
			i++;
		        continue;} 
		    else {
		    child_arg[child_arg_i] = words_[i];
		    child_arg_i++;
		    i++; }
	    }

	}

	child_arg[child_arg_i] = NULL;
/*	printf("Child arguments:\n");
	for (i = 0; child_arg[i] != NULL; i++) {
        	printf("%s\n", child_arg[i]);
    } */

	    execvp(child_arg[0], child_arg);
	    fprintf(stderr, "smallsh: no such file/directory \n");
	    exit(1);
	} else {
	  /* printf("bg = %d\n", background); */
	   if (background == 0){
	   int _status_;
	   pid_t result_;
	   result_ = waitpid(child_pid, &_status_, WUNTRACED);

	   if (WIFEXITED(_status_)){
		int exit_status = WEXITSTATUS(_status_);
		/*fprintf(stdout, "Child process exited with status %d\n", exit_status);*/
		sprintf(status, "%d", exit_status);
	   } else if (WIFSTOPPED(_status_)) {
		kill(result_, SIGCONT);
	        fprintf(stderr, "Child process %d stopped. Continuing.\n", result_);
		int sig_num = WSTOPSIG(_status_);
		char fg_str[10];
		sprintf(fg_str, status, sig_num); 
		snprintf(bgpid, sizeof(bgpid), "%d", result_);
	   } else if (WIFSIGNALED(_status_)){
		/*fprintf(stderr, "Child process %d done. Signaled %d.\n", result_, WTERMSIG(_status_));*/
		int exit_status = WTERMSIG(_status_);
		exit_status = exit_status + 128;
		sprintf(status, "%d", exit_status);
	   } } else {
		snprintf(bgpid, sizeof(bgpid), "%d", child_pid);

	   }
	   return 0;
	}
}


