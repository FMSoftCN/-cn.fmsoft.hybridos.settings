/*#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
*/

#include <unistd.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <hibus.h>

#include "inetd.h"

#undef  DAEMON
//#define DAEMON

//extern void * start_wifi(void * args);

int main(void)
{
    char* etc_value = NULL;
    char library_path[MAX_PATH];
    char config_path[MAX_PATH];
//    int ret = 0;
//    int i = 0;
//    pthread_attr_t thread_attr_start[MAX_DEVICE_NUM];
//    pthread_t a_thread_start[MAX_DEVICE_NUM];

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

    if ((etc_value = getenv ("INETD_CFG_PATH")))
    {
        int len = strlen(etc_value);
        if (etc_value[len-1] == '/')
            sprintf(config_path, "%s%s", etc_value, INETD_CONFIG_FILE);
        else
            sprintf(config_path, "%s/%s", etc_value, INETD_CONFIG_FILE);
    }
    else
        sprintf(config_path, "%s", INETD_CONFIG_FILE);

    if(GetValueFromEtcFile(config_path, "system", "library_path", library_path, ETC_MAXLINE) == ETC_OK)
    {
        printf("%s\n", library_path);
    }



/*
    // start thread according to the configuration file
    for(i = 0; i < MAX_DEVICE_NUM; i++)
    {
        ret = pthread_attr_init(&thread_attr_start[i]);
        if(ret != 0)
        {
            printf("INETD: Create Start thread attribute failed.\n");
            continue;
        }
        else
        {
            ret = pthread_attr_setdetachstate(&thread_attr_start[i], PTHREAD_CREATE_DETACHED);
            if(ret != 0)
            {
                printf("INETD: Detach Start thread attribute failed.\n");
                continue;
            }
            else
            {
                ret = pthread_create(&a_thread_start[i], &thread_attr_start[i], start_wifi, (void *)NULL);
                if(ret != 0)
                {
                    printf("INETD: Start Start thread function failed.\n");
                    continue;
                }
            }
            (void)pthread_attr_destroy(&thread_attr_start[i]);
        }
    }
*/
    while(1)
    {
        sleep(10);
    }

	return 0;
}
