#include<stdio.h>
#include<syslog.h>

int main(int argc, char** argv)
{
    FILE *fp=NULL;

    openlog(NULL, 0, LOG_USER);
    if(argc != 3)
    {
        syslog(LOG_ERR, "insufficient arguments.\n");
        closelog();
        return 1;
    }

    if(NULL == (fp == fopen(argv[1], "w")))
    {
        syslog(LOG_ERR, "file cannot be opened.\n");
        closelog();
        return 1;
    }

    if(0 > fprintf(fp, "%s", argv[2]))
    {
        syslog(LOG_DEBUG, "Writing %s to %s.\n",argv[2],argv[1]);
        closelog();
    }

    closelog();
    fclose(fp);
    return 0;

}
