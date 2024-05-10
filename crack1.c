#include <stdio.h>
#include <string.h>
#include <crypt.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>


#define MAX_PASSWORDS 15000000
#define MAX_PASSWORD_LENGTH 128
#define MAX_PASSWORDS2 3
char **password_list;    
char **hash_list;
int count = 0;

int j = 0;
int npasswd;
int nhash;

int flag =0;

pthread_mutex_t espera_thread = PTHREAD_MUTEX_INITIALIZER;

int compara(){

    // The password hash from the shadow file (user-provided example)
    char *shadow_hash;
    char salt[12];
    //char *new_hash;
    
    struct crypt_data crypt_data;
    

    pthread_mutex_lock(&espera_thread);

    shadow_hash = hash_list[j];    
    //printf("%s\n",shadow_hash);
    // Extracting the salt from the shadow_hash, it includes "$1$" and ends before the second "$"
    strncpy(salt, shadow_hash, 11);
    salt[11] = '\0';  // Ensure null termination
    
    j++;
    
    pthread_mutex_unlock(&espera_thread);

    for (int i=0; i<npasswd; i++) {
	// em multithread use crypt_r(), pois crypt() não é threadsafe.
	char *new_hash = crypt_r(password_list[i], salt, &crypt_data);
	if (strcmp(shadow_hash, new_hash) == 0) {
	    printf("Password found: %s\n", password_list[i]);
	    i = npasswd;
	    flag = 1;
	    
	}
    }
    
    if(flag == 0){
       printf("Password not found!\n");
    }
}


void *consumidor(void *p){
	int id = (intptr_t) p;

	while(1){
		compara();
		//printf("consumidor %d analisou.\n", id);
		fflush(stdout);
		if(j == MAX_PASSWORDS2){break;}
		//usleep(500000);
	}
}



int loadpasswd(const char* filename) {
    char passwd[MAX_PASSWORD_LENGTH];
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen(): ");
        return -1;
    }
    password_list = malloc(MAX_PASSWORDS * sizeof(char*));

    int i = 0;
    while (i < MAX_PASSWORDS && fgets(passwd, MAX_PASSWORD_LENGTH, file) != NULL) {
        passwd[strcspn(passwd, "\n")] = 0;  // Remove newline
        password_list[i] = strdup(passwd);
        i++;
    }

    fclose(file);
    return i;
}


int loadhash(const char* filename) {
    char hash[MAX_PASSWORD_LENGTH];
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen(): ");
        return -1;
    }
    hash_list = malloc(MAX_PASSWORDS2 * sizeof(char*));

    int i = 0;
    while (i < MAX_PASSWORDS2 && fgets(hash, MAX_PASSWORD_LENGTH, file) != NULL) {
        hash[strcspn(hash, "\n")] = 0;  // Remove newline
        hash_list[i] = strdup(hash);
        i++;
    }

    fclose(file);
    return i;
}



int main(int argc, char* argv[]) {

    // Creating threads
    pthread_t *cons;
    int *ids;
    
    char *arq = "hashes.txt";
    long int k, nc, i, m;

 
    if(argc < 3) {
        printf("Usage: %s <threads> <dict file>\n", argv[0]);
        return 1;
    }

    npasswd = loadpasswd(argv[2]);
    nhash = loadhash(argv[3]); //argv[1]
    
    /*Cria array com os números de identificação das threads. */
    nc = atoi(argv[1]);
    ids = (int *)malloc(nc * sizeof(int));
    for(i=0; i<nc; i++){
        ids[i] = i;
    }
    
     /* Cria diversas threads consumidoras */
    cons = (pthread_t *)malloc(nc * sizeof(pthread_t));
    for(k=0; k<nc; k++){
        if(pthread_create(&cons[k], NULL, consumidor, (void*)&ids[k])) {
            fprintf(stderr, "Error creating thread\n");
            return 2;
        }
    }
    
    for(m=0; m<nc; m++){
    	pthread_join(cons[m], NULL);
    }


    //pthread_join(cons, NULL);
    
    //getchar();

   
    return 0;
}
