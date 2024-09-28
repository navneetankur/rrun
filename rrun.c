#include <time.h>

//# url: https://gist.github.com/navneetankur/2cfcae0f8803afca0ed0ae9cdcca63e3/rrun.c
//# ghgid: 2cfcae0f8803afca0ed0ae9cdcca63e3
#define CACHE_DIR "/home/navn/bin/cache/rrun/"
/* #define RUST_STDLIB_PATH_ARG "link-args=-Wl,-rpath,/home/navn/nonssd/rust/rustup/toolchains/nightly-x86_64-unknown-linux-gnu/lib" */
#define RUST_STDLIB_PATH_ARG "link-args=-Wl,-rpath,"
/* #define RUST_LIB_DIR "/home/navn/nonssd/rust/rustup/toolchains/nightly-x86_64-unknown-linux-gnu/lib" */
#define RUST_LIB_DIR "/home/navn/bin/lib/rust"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>

typedef enum {false, true} bool;
//
bool rust_lib_is_modified(struct timespec bin_time);
bool timespce_lt(struct timespec lhs, struct timespec rhs);
int mkpath(char* file_path, mode_t mode);
//free the return value.
char * read_file(const char *filename, int * length);

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("script?\n");
		return 0;
	}
	int length;
	char * script_path = argv[1];
	char * absolute_script_path = realpath(script_path, NULL);
	char * script_text = read_file(script_path, &length);
	char * script_name = strrchr(script_path, '/');
	if(script_name == NULL) {
		script_name = script_path;
	}
	else {
		script_name += 1;
	}
	free(script_text);
	script_text = NULL;

	char * bin_file_path = malloc(sizeof(char) * (strlen(CACHE_DIR) + strlen(absolute_script_path) + 1)); //1 for \0

	if(bin_file_path){
		strcpy(bin_file_path, CACHE_DIR);
		strcat(bin_file_path,absolute_script_path);
	}
	else {
		perror("malloc failed.");
		exit(1);
	}
	struct stat script_stat, bin_stat;
	if(stat(script_path, &script_stat)==-1) {
		perror("stat");
		exit(1);
	}
	int stat_ret = stat(bin_file_path, &bin_stat);

	//if stat failed. or bin was modified before script or stdlib.
	if(
			stat_ret != 0 || 
			timespce_lt(bin_stat.st_mtim, script_stat.st_mtim) ||
			rust_lib_is_modified(bin_stat.st_mtim)
	  ) {
		//create directory for bin.
		if(mkpath(bin_file_path, 0755) != 0) {
			printf("failed creating dir. %s", bin_file_path);
			exit(1);
		}
		int child_pid = fork();
		if (child_pid == 0) {
			//inside child
			
			char * rust_std_lib_path_args = malloc(sizeof(char) * (strlen(RUST_STDLIB_PATH_ARG) + strlen(RUST_LIB_DIR) + 1));
			strcpy(rust_std_lib_path_args, RUST_STDLIB_PATH_ARG);
			strcat(rust_std_lib_path_args, RUST_LIB_DIR);
			//file doesn't exist. Create it.
			char * kargs[11];
			kargs[0] = "rustc";
			kargs[1] = script_path;
			kargs[2] = "-C";
			kargs[3] = "prefer-dynamic";
			kargs[4] = "-L";
			kargs[5] = RUST_LIB_DIR;
			kargs[6] = "-C";
			kargs[7] = rust_std_lib_path_args;
			kargs[8] = "-o";
			kargs[9] = bin_file_path;
			kargs[10] = NULL;

			execvp(kargs[0], kargs);

			//shouldn't be reached.
			perror(kargs[0]);
			free(absolute_script_path);
			absolute_script_path = NULL;
			free(rust_std_lib_path_args);
			rust_std_lib_path_args = NULL;
			free(bin_file_path);
			bin_file_path = NULL;
			return 1;
		}
		//in parent
		wait(NULL);
	}

	//try running the program
	char **jargs = malloc(sizeof(char *) * argc);
	for(int i=0,j=1; j<argc; i++,j++) { //argv[0] is rrun so ignore it.
		jargs[i] = argv[j];
	}
	jargs[argc - 1] = NULL;
	execvp(bin_file_path, jargs);

	//shouldn't be reached.
	perror(bin_file_path);

	free(bin_file_path);
	bin_file_path = NULL;
	free(absolute_script_path);
	absolute_script_path = NULL;
	free(jargs);
	jargs = NULL;
}

bool rust_lib_is_modified(struct timespec bin_time) {
	DIR *dp = opendir(RUST_LIB_DIR);
	bool retval = false;
	if (dp == NULL) {
		printf("can't open dir %s\n", RUST_LIB_DIR);
		return retval;
	}
	const int rust_lib_len = strlen(RUST_LIB_DIR);
	char *filepath = malloc(sizeof(char) * rust_lib_len + 256 + 2);
	char *filepath_name = filepath + rust_lib_len;
	strcpy(filepath, RUST_LIB_DIR);
	*filepath_name = '/';
	filepath_name ++;
	while (true) {
		struct dirent *entry = readdir(dp);
		if (entry == NULL) {
			retval = false;
			break; //all entries checked. No modification.
		}
		/* const char * filename = entry->d_name; */
		strcpy(filepath_name, entry->d_name);
		struct stat lib_stat;
		/* if(stat(filename, &lib_stat)==-1) { */
		if(stat(filepath, &lib_stat)!=0) {
			printf("%s\n", filepath);
			perror("stat");
			continue;
		}
		if (timespce_lt(bin_time, lib_stat.st_mtim)) {
			retval = true; //bin is modified before lib. Need rebuild.
			break;
		}
	}
	closedir(dp);
	return retval;
}

//Returns true if lhs happened first -- lhs will be "lower".
bool timespce_lt(struct timespec lhs, struct timespec rhs) {
    if (lhs.tv_sec == rhs.tv_sec)
        return lhs.tv_nsec < rhs.tv_nsec;
    else
        return lhs.tv_sec < rhs.tv_sec;
}
int mkpath(char* file_path, mode_t mode) {
    assert(file_path && *file_path);
    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}
//free the return value.
char * read_file(const char *filename, int * length) {
	char * buffer = 0;
	FILE * file = fopen (filename, "rb");
	int len;

	if(file) {
		fseek (file, 0, SEEK_END);
		len = ftell (file);
		fseek (file, 0, SEEK_SET);
		buffer = malloc (len + 1);
		if (buffer)
		{
			fread (buffer, 1, len, file);
		}
		fclose (file);
		buffer[len] = '\0';
		*length = len;
	}
	else {
		*length = -1;
	}
	return buffer;
}
