#include <unistd.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <hibus.h>

#include "inetd.h"

#undef  DAEMON
//#define DAEMON

//extern void * start_wifi(void * args);

int main(void)
{
    char* etc_value = NULL;                     // get the configure file path from ENV
    char config_path[MAX_PATH];                 // configure file full path
    char library_path[MAX_PATH];                // storage path of libraries
    char library_full_path[MAX_PATH];           // the full path of loaded libraries
    char library_name[128];                     // item name of configure file, such as "lib0"
    int library_number = 0;                     // the number of libraries

    int ret = 0;
    int i = 0;
    int library_start = 0;                      // the number of loaded library, which runs ok
    pthread_attr_t thread_attr_start[MAX_DEVICE_NUM];
    pthread_t a_thread_start[MAX_DEVICE_NUM];
    
    void * library_handle[MAX_DEVICE_NUM];      // handle of loaded library
	char * library_error = NULL;                // the error message during loading
	void * (*start_function)(void *);           // start_function, the entry of library

#ifdef	DAEMON
    int pid = 0;

    pid = fork();
    if(pid < 0)    
        exit(1);  		        // fork error, son process quits
    else if(pid > 0) 	        // parent process quits
        exit(0);

    setsid();  
    pid = fork();
    if(pid > 0)
        exit(0); 		        // quits again. close terminal
    else if(pid < 0)    
        exit(1);                // fork error, son process quits

    for(i = 0; i < NOFILE; i++) // close all file
        close(i);

    chdir(WORKING_DIRECTORY);   // change working directory
    umask(0);					// reset mask
#endif

    memset(config_path, 0, MAX_PATH);
    memset(library_path, 0, MAX_PATH);

    // step 1: get library path from configure file
    if ((etc_value = getenv ("INETD_CFG_PATH")))
    {
        int len = strlen(etc_value);
        if (etc_value[len - 1] == '/')
            sprintf(config_path, "%s%s", etc_value, INETD_CONFIG_FILE);
        else
            sprintf(config_path, "%s/%s", etc_value, INETD_CONFIG_FILE);
    }
    else
        sprintf(config_path, "%s", INETD_CONFIG_FILE);

    if(GetValueFromEtcFile(config_path, "system", "library_path", library_path, ETC_MAXLINE) != ETC_OK)
    {
        fprintf(stderr, "INETD: read library path error, exit now!");
        exit(1);
    }

    if(GetIntValueFromEtcFile(config_path, "system", "library_num", &library_number) != ETC_OK)
    {
        fprintf(stderr, "INETD: read library number error, exit now!");
        exit(1);
    }

    if(library_number == 0)
    {
        fprintf(stderr, "INETD: the number of loaded library is 0, exit now!");
        exit(1);
    }

    if(library_number > MAX_DEVICE_NUM)
        fprintf(stderr, "INETD: the number of loaded library is over MAX_DEVICE_NUM, modify it!");
    library_number = (library_number < MAX_DEVICE_NUM)? library_number: MAX_DEVICE_NUM;

    fprintf(stderr, "INETD: read from configure file, library path is %s, number is %d\n", library_path, library_number);


    // step 2: start thread according to the configuration file
    for(i = 0; i < library_number; i++)
    {
        // get the library path
        memcpy(library_full_path, library_path, MAX_PATH);
        memset(library_name, 0, 128);

        strcat(library_full_path, "/lib");
        sprintf(library_name, "lib%d", i);

        if(GetValueFromEtcFile(config_path, "library", library_name, library_full_path + strlen(library_full_path), ETC_MAXLINE) != ETC_OK)
        {
            fprintf(stderr, "INETD: get %s name error, ignore it!", library_name);
            continue;
        }
        strcat(library_full_path, ".so");

        // the library is existence
        if((access(library_full_path, F_OK)) == -1)   
        {   
            fprintf(stderr, "INETD: library file %s does not exist, ignore it!", library_full_path);
            continue;
        }   


        // load library
        library_handle[i] = dlopen(library_full_path, RTLD_LAZY);  
        if(!library_handle[i]) 
        {
		    fprintf (stderr, "load %s error: %s\n", library_full_path, dlerror());
		    continue;
	    }

        start_function = (void * (*) (void *))dlsym(library_handle[i], "start_function");
	    if((library_error = dlerror()) != NULL)
        {
		    fprintf (stderr, "get start_function pointer error: %s\n", library_error);
		    continue;
        }

        
        // start library
        ret = pthread_attr_init(&thread_attr_start[i]);
        if(ret != 0)
        {
            fprintf(stderr, "INETD: Create Start thread attribute failed.\n");
            continue;
        }
        else
        {
            ret = pthread_attr_setdetachstate(&thread_attr_start[i], PTHREAD_CREATE_DETACHED);
            if(ret != 0)
            {
                (void)pthread_attr_destroy(&thread_attr_start[i]);
                fprintf(stderr, "INETD: Detach Start thread attribute failed.\n");
                continue;
            }
            else
            {
                ret = pthread_create(&a_thread_start[i], &thread_attr_start[i], start_function, (void *)NULL);
                if(ret != 0)
                {
                    (void)pthread_attr_destroy(&thread_attr_start[i]);
                    fprintf(stderr, "INETD: Start Start thread function failed.\n");
                    continue;
                }
            }
            (void)pthread_attr_destroy(&thread_attr_start[i]);
        }

        library_start ++;
        fprintf(stderr, "INETD: load library %s successfully\n", library_full_path);
    }

    if(library_start == 0)
    {
        fprintf(stderr, "INETD: no library is loaded, inetd daemon exit!!!\n");
        exit(1);
    }

    while(1)
    {
        sleep(10);
    }

    wait(NULL);

    for(i = 0; i < library_number; i++)
    {
        if(library_handle[i])
        {
            dlclose(library_handle[i]);
            library_handle[i] = NULL;
        }
    }

	return 0;
}
