#include "machine.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "add_efun.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_efuns.h"


#include "discdate.c"
#include "stardate.c"
#include "debug.c"
#include "spider.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#define MAX_PARSE_RECURSE 128

void do_html_parse(struct lpc_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args);

void f_parse_accessed_database(INT32 args)
{
  int cnum=0, i, num=0;

  struct array *arg;
  if(args != 1)
    error("Wrong number of arguments to parse_accessed_database(string)\n");

  push_string(make_shared_string("\n"));
  f_explode(2);
  arg = sp[-1].u.array;
  arg->refs++;
  /* The initial string is gone, but the array is there now. */
  pop_n_elems(args); 

  for (i = 0; i < arg->size; i++)
  {
    int j=0,k;
    char *s;
    s=(char *)(SHORT_ITEM(arg)[i].string->str);
    k=(SHORT_ITEM(arg)[i].string->len);
    for(j=k; j>0 && s[j-1]!=':'; j--);
    if(j>0)
    {
      push_string(make_shared_binary_string(s, j-1));
      k=atoi(s+j);
      if(k>cnum)
	cnum=k;
      push_int(k);
      num++;
    }
  }
  free_array(arg);
  f_aggregate_mapping(num*2);
  push_int(cnum);
  f_aggregate(2);
}


#if 0
// This function leaks a lot of memory, but it ends with either an
// exec() or an exit().  This could be a problem on systems with louse memory
// management, though. I know of none.
// Thanks to David Byers for the bugfix.
#endif

#if 0
void f_exece(INT32 args)
{
  int i=0, environ_size=0;
  struct array *arg;
  struct array *env;
  char **envv, **argv;
  extern char **environ;
    
  /*
   *  Check that the arguments given are
   *  1. A string (the program to execute)
   *  2. An array (argv)
   *  3. An array (extra environment variables)
   */
    
  if (args < 3 ||
      sp[-args].type  != T_STRING ||
      sp[1-args].type != T_ARRAY  ||
      sp[2-args].type != T_ARRAY)
    error("Illegal argument(s) to exece().\n");

  environ_size = 0;
  while (environ[environ_size])
    environ_size += 1;

  arg = sp[1-args].u.array;
  env = sp[2-args].u.array;

  /*
   * Allocate argv and envv arrays
   * In argv, make room for program name and NULL
   * In envv, make room for NULL
   */

  argv = (char **)xalloc((2 + arg->size)*sizeof(char **));
  envv = (char **)xalloc((1 + env->size + environ_size)*sizeof(char **));

  /*
   * Build argv
   */

  argv[0] = strdup((char *)(sp[-args].u.string->str));
  if (arg->array_type == T_STRING)
  {
    for (i = 0; i < arg->size; i++)
      argv[1+i] = strdup((char *)(SHORT_ITEM(arg)[i].string->str));
  }
  else if (arg->array_type == T_MIXED)
  {
    for (i = 0; i < arg->size; i++)
      if (ITEM(arg)[i].type == T_STRING)
	argv[1+i] = strdup((char *)(ITEM(arg)[i].u.string->str));
      else
	argv[1+i] = strdup("bad_argument");
  }
  else
    error("Wrong type argv send to exece");

  argv[1+i] = NULL;

  /*
   * Build envv
   * 1. Copy environ
   * 2. Copy arguments
   */

  for (i = 0; i < environ_size; i++)
    envv[i] = environ[i];

  if (arg->array_type == T_STRING)
  {
    for (i = 0; i < env->size; i++)
      envv[environ_size+i] =
	strdup((char *)(SHORT_ITEM(env)[i].string->str));
  }
  else if (env->array_type == T_MIXED)
  {
    for (i = 0; i < env->size; i++)
      if (ITEM(env)[i].type == T_STRING)
	envv[environ_size+i] =
	  strdup((char *)(ITEM(env)[i].u.string->str));
      else
	envv[environ_size+i] = strdup("bad=argument");
  }
  else
    error("Wrong type envv send to exece");

  envv[environ_size + i] = NULL;

  set_close_on_exec(0,0);
  set_close_on_exec(1,0);
  set_close_on_exec(2,0);

  execve(argv[0], argv, envv);
  exit(0);
}
#endif
void f_parse_html(INT32 args)
{
  struct lpc_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
   
  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    error("Bad argument(s) to parse_html.\n");

  ss=sp[-args].u.string;
  sp[-args].type=T_INT;

  single=sp[1-args].u.mapping; 
  cont=sp[2-args].u.mapping; 
  cont->refs++;
  single->refs++;

  if (args>3)
  {
    f_aggregate(args-3);
    extra_args=sp[-1].u.array;
    extra_args->refs++;
    pop_stack();
  }
  else extra_args=NULL;

  pop_n_elems(3);

  strings=0;
  do_html_parse(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args);

  if (extra_args) free_array(extra_args);

  free_mapping(cont);
  free_mapping(single);
  f_aggregate(strings);
  f_implode(1);
}

int push_parsed_tag(char *s,int len)
{
  int i,j,elems=0;
  int haskey;

  for (i=haskey=0; i<len&&s[i]!='>';)
  {
    for (; i<len&&s[i]!='>' && isspace(s[i]); i++); /* skip space */
    if (!(i<len && s[i]!='>')) break;
    /* check wordlen */
    for (j=i; i<len&&s[i]!='>' && !isspace(s[i]) && s[i]!='='; i++); 
    if (j<i) 
    {
      push_string(make_shared_binary_string(s+j,i-j));
      f_lower_case(1);
    }
    else
      push_string(make_shared_string(""));

    for (;i<len&&s[i]!='>'&&isspace(s[i]);i++); /* skip space */

    if (i>=len || s[i]!='=') 
    {
      assign_svalue_no_free(sp,sp-1); sp++;
      elems++;
      continue;
    }
    i++;
    if (i>=len || s[i]=='>' || (s[i]=='"' && i+1 >= len))
    {
      assign_svalue_no_free(sp,sp-1); sp++;
      elems++;
      continue;
    }
    if (s[i]=='"')
    {
      j=++i;
      for (;i<len && s[i]!='"' ;i++); /* end quote */
      push_string(make_shared_binary_string(s+j,i-j));
      elems++;
      if (i<len) i++;
      continue;
    }
    for (j=i;i<len&&s[i]!='>'&&!isspace(s[i]);i++); /* check wordlen */
    push_string(make_shared_binary_string(s+j,i-j));
    elems++;
  }
  f_aggregate_mapping(elems*2);
  return i+(i<len);
}

INLINE int tagsequal(char *s,char *t,int len)
{
  while (--len) if (tolower(*(t++))!=tolower(*(s++))) return 1;
  return 0;
}


int find_endtag(struct lpc_string *tag,char *s,int len,int *aftertag)
{
  int i,lend,j;
  for (i=j=0; i<len; i++)
  {
    for (; i<len&&s[i]!='<'; i++);
    if (i>=len) break;
    j=i++;
    if (i>=len) break;
    if (s[i]=='/')
    {
      if (i+tag->len+1<len&&
	  !tagsequal(s+i+1,tag->str,tag->len)&&
	  (isspace(s[i+1+tag->len])||s[i+1+tag->len]=='>'))
	break;
      continue;
    }
    if (i+tag->len<len&&
	!tagsequal(s+i,tag->str,tag->len)&&
	(isspace(s[i+tag->len])||s[i+tag->len]=='>')) /* oop, recurse */
    {
      find_endtag(tag,s+i+tag->len,len-i-tag->len,&lend);
      j=i+=lend;
    }
  }
  if (i>=len) *aftertag=len,j=i; /* no end */
  else
  {
    for (;i<len&&s[i]!='>'; i++);
    *aftertag=i+(i<len);
  }
  return j;
}

void do_html_parse(struct lpc_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args)
{
  int i,j,k,l,m,len,last;
  unsigned char *s;
  struct svalue sval1,sval2;
  struct lpc_string *ss2;

  if (!ss->len)
  {
    free_string(ss);
    return;
  }

  if (!recurse_left)
  {
    push_string(ss);
    (*strings)++;
    return;
  }

  s=(unsigned char *)ss->str;
  len=ss->len;

  last=0;
  for (i=0; i<len-1;)
  {
    if (s[i]=='<')
    {
      /* skip all spaces */
      i++;
      for (j=i; j<len && s[j]!='>' && !isspace(s[j]); j++);

      if (j==len) break; /* end of string */

      sval2.type=T_STRING;
      sval2.subtype=-1;		/* ? */
      sval2.u.string=make_shared_binary_string((char *)s+i,j-i);

      mapping_index_no_free(&sval1,single,&sval2);
      if (sval1.type==T_STRING)
      {
	assign_svalue_no_free(sp++,&sval1);
	free_svalue(&sval1);
	(*strings)++;
	find_endtag(sval2.u.string,s+j,len-j,&l);
	free_svalue(&sval2);
	i=last=j+=l;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	assign_svalue_no_free(sp++,&sval2);
	k=push_parsed_tag(s+j,len-j); 
	if (extra_args)
	{
	  extra_args->refs++;
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,2+(extra_args?extra_args->size:0));
	if (sp[-1].type==T_STRING)
	{
	  free_svalue(&sval1);
	  free_svalue(&sval2);
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();
	  if (last!=i-1)
	  { 
	    push_string(make_shared_binary_string(s+last,i-last-1)); 
	    (*strings)++; 
	  }
	  i=last=j+k;
	  do_html_parse(ss2,cont,single,strings,recurse_left-1,extra_args);
	  continue;
	}
	pop_stack();
      }

      free_svalue(&sval1);
      mapping_index_no_free(&sval1,cont,&sval2);
      if (sval1.type==T_STRING)
      {
	assign_svalue_no_free(sp++,&sval1);
	free_svalue(&sval1);
	(*strings)++;
	find_endtag(sval2.u.string,s+j,len-j,&l);
	free_svalue(&sval2);
	i=last=j+=l;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	assign_svalue_no_free(sp++,&sval2);
	m=j+(k=push_parsed_tag(s+j,len-j)); 
	k=find_endtag(sval2.u.string,s+m,len-m,&l);
	push_string(make_shared_binary_string(s+m,k));
	m+=l;
	if (extra_args)
	{
	  extra_args->refs++;
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));
	if (sp[-1].type==T_STRING)
	{
	  free_svalue(&sval1);
	  free_svalue(&sval2);
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();
	  if (last!=i-1)
	  { 
	    push_string(make_shared_binary_string(s+last,i-last-1)); 
	    (*strings)++; 
	  }
	  i=last=j=m;
	  do_html_parse(ss2,cont,single,strings,recurse_left-1,extra_args);
	  continue;
	}
	pop_stack();
      }
      free_svalue(&sval1);
      free_svalue(&sval2);
      i=j;
    }
    else
      i++;
  }

  if (last==0)
  {
    push_string(ss);
    (*strings)++;
  }
  else if (last<len)
  {
    push_string(make_shared_binary_string(s+last,len-last));  
    free_string(ss);
    (*strings)++;
  }
  else
  {
    free_string(ss);
  }
}

static int does_match(char *s,int len,char *m,int mlen)
{
  int i,j;
  for (i=j=0; i<mlen&&j<len; )
    switch (m[i])
    {
     case '?': 
      j++; i++; 
      break;
     case '*': 
      i++;
      if (i==mlen) return 1;	/* slut */
      for (;j<len;j++)
	if (does_match(s+j,len-j,m+i,mlen-i)) return 1;
      return 0;
     default: 
      if (m[i]!=s[j]) return 0;
      j++; i++;
    }
  if (i==mlen && j==len) return 1;
  return 0;
}

void f_do_match(INT32 args)
{
  struct lpc_string *match,*str;
  struct array *a;
  int i;
  struct svalue *sval;

  if (args<2||
      sp[1-args].type!=T_STRING||
      (sp[-args].type!=T_STRING &&
       sp[-args].type!=T_ARRAY))
    error("Illegal arguments to do_match\n");

  copy_shared_string(match,sp[1-args].u.string);
  pop_n_elems(args-1);

  if (sp[-1].type==T_ARRAY)
  {
    a=sp[-1].u.array;
    a->refs++;
    pop_stack();
    i=a->size;
    if (!i) { push_int(0); return; }
    push_array_items(a);
    while (i--)
    {
      match->refs++;
      push_string(match);
      f_do_match(2);
      if (sp[-1].type!=T_INT)
      {
	sval=sp-1;
	sp--;
	pop_n_elems(i);
	sp[0]=*sval;
	sp++;
	free_string(match);
	return;
      }
      pop_stack();
    }
    push_int(0);
    free_string(match);
    return;
  }
  copy_shared_string(str,sp[-1].u.string);
  pop_stack();
   
  if (does_match(str->str,str->len,match->str,match->len)) 
  {
    push_string(str); 
    free_string(match); 
    return; 
  }
  push_int(0);
  free_string(str);
  free_string(match);
}

#if !HAVE_INT_TIMEZONE
int _tz;
#else
extern long int timezone;
#endif



void f_localtime(INT32 args)
{
  struct tm *tm;
  time_t t;
  if (args<1||
      sp[-1].type!=T_INT)
    error("Illegal argument to localtime");
  t=sp[-1].u.integer;
  tm=localtime(&t);
  pop_stack();
  push_string(make_shared_string("sec"));
  push_int(tm->tm_sec);
  push_string(make_shared_string("min"));
  push_int(tm->tm_min);
  push_string(make_shared_string("hour"));
  push_int(tm->tm_hour);

  push_string(make_shared_string("mday"));
  push_int(tm->tm_mday);
  push_string(make_shared_string("mon"));
  push_int(tm->tm_mon);
  push_string(make_shared_string("year"));
  push_int(tm->tm_year);

  push_string(make_shared_string("wday"));
  push_int(tm->tm_wday);
  push_string(make_shared_string("yday"));
  push_int(tm->tm_yday);
  push_string(make_shared_string("isdst"));
  push_int(tm->tm_isdst);

  push_string(make_shared_string("timezone"));
#if !HAVE_INT_TIMEZONE
  push_int(_tz);
#else
  push_int(timezone);
#endif
  f_aggregate_mapping(20);
}

void f_do_setuid(INT32 args)
{
  if(!args)
    error("Set uid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set uid to _what_?\n");

  setuid(sp[-1].u.integer);
  pop_n_elems(args-1);
}

void f_do_setgid(INT32 args)
{
  if(!args)
    error("Set gid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set gid to _what_?\n");

  setgid(sp[-1].u.integer);
  pop_n_elems(args-1);
}


void f_timezone(INT32 args)
{
  pop_n_elems(args);
#if !HAVE_INT_TIMEZONE
  push_int(_tz);
#else
  push_int(timezone);
#endif
}

#ifdef HAVE_SYSLOG

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_SYSLOG_H
#ifndef HAVE_SYSLOG_H
#include <sys/syslog.h>
#endif
#endif


#ifndef LOG_PID
#define LOG_PID 0
#endif
#ifndef LOG_AUTH
#define LOG_AUTH 0
#endif
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV 0
#endif
#ifndef LOG_CRON
#define LOG_CRON 0
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON 0
#endif
#ifndef LOG_KERN
#define LOG_KERN 0
#endif
#ifndef LOG_LOCAL0
#define LOG_LOCAL0 0
#endif
#ifndef LOG_LOCAL1
#define LOG_LOCAL1 0
#endif
#ifndef LOG_LOCAL2
#define LOG_LOCAL2 0
#endif
#ifndef LOG_LOCAL3
#define LOG_LOCAL3 0
#endif
#ifndef LOG_LOCAL4
#define LOG_LOCAL4 0
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 0
#endif
#ifndef LOG_LOCAL6
#define LOG_LOCAL6 0
#endif
#ifndef LOG_LOCAL7
#define LOG_LOCAL7 0
#endif
#ifndef LOG_LPR
#define LOG_LPR 0
#endif
#ifndef LOG_MAIL
#define LOG_MAIL 0
#endif
#ifndef LOG_NEWS
#define LOG_NEWS 0
#endif
#ifndef LOG_SYSLOG
#define LOG_SYSLOG 0
#endif
#ifndef LOG_USER
#define LOG_USER 0
#endif
#ifndef LOG_UUCP
#define LOG_UUCP 0
#endif
#ifndef LOG_CONS
#define LOG_CONS 0
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0
#endif
#ifndef LOG_PERROR
#define LOG_PERROR 0
#endif
void f_openlog(INT32 args)
{
  int option=0, facility=0, i;
  if(args < 3)
    error("Wrong number of arguments to openlog(string,int,int)\n");
  if(sp[-args].type != T_STRING
     &&sp[-args+1].type != T_INT
     &&sp[-args+2].type != T_INT)
    error("Wrong type of arguments to openlog(string,int,int)\n");

  i=sp[-args+1].u.integer;
  
  if(i & (1<<0)) option |= LOG_CONS;
  if(i & (1<<1)) option |= LOG_NDELAY;
  if(i & (1<<2)) option |= LOG_PERROR;
  if(i & (1<<3)) option |= LOG_PID;

  i=sp[-args+2].u.integer;

  if(i & (1<<0)) facility |= LOG_AUTH; /* Don't use */
  if(i & (1<<1)) facility |= LOG_AUTHPRIV;
  if(i & (1<<2)) facility |= LOG_CRON;
  if(i & (1<<3)) facility |= LOG_DAEMON;
  if(i & (1<<4)) facility |= LOG_KERN;
  if(i & (1<<5)) facility |= LOG_LOCAL0;
  if(i & (1<<6)) facility |= LOG_LOCAL1;
  if(i & (1<<7)) facility |= LOG_LOCAL2;
  if(i & (1<<8)) facility |= LOG_LOCAL3;
  if(i & (1<<9)) facility |= LOG_LOCAL4;
  if(i & (1<<10)) facility |= LOG_LOCAL5;
  if(i & (1<<11)) facility |= LOG_LOCAL6;
  if(i & (1<<12)) facility |= LOG_LOCAL7;
  if(i & (1<<13)) facility |= LOG_LPR;
  if(i & (1<<14)) facility |= LOG_MAIL;
  if(i & (1<<15)) facility |= LOG_NEWS;
  if(i & (1<<16)) facility |= LOG_SYSLOG;
  if(i & (1<<17)) facility |= LOG_USER;
  if(i & (1<<18)) facility |= LOG_UUCP;
  
  openlog((char *)sp[-args].u.string->str, option, facility);
}

void f_syslog(INT32 args)
{
  int pri=0, i;

  if(args < 2)
    error("Wrong number of arguments to syslog(int, string)\n");
  if(sp[-args].type != T_INT ||
     sp[-args+1].type != T_STRING)
    error("Wrong type of arguments to syslog(int, string)\n");

  reference_shared_string(sp[-args+1].u.string);
  push_string(sp[-args+1].u.string);
  push_string(make_shared_string("%"));
  push_string(make_shared_string("%%"));
  f_replace(3);

  i=sp[-args].u.integer;
  if(i & (1<<0)) pri |= LOG_EMERG;
  if(i & (1<<1)) pri |= LOG_ALERT;
  if(i & (1<<2)) pri |= LOG_CRIT;
  if(i & (1<<3)) pri |= LOG_ERR;
  if(i & (1<<4)) pri |= LOG_WARNING;
  if(i & (1<<5)) pri |= LOG_NOTICE;
  if(i & (1<<6)) pri |= LOG_INFO;
  if(i & (1<<6)) pri |= LOG_DEBUG;
  
  syslog(pri, (char *)sp[-1].u.string->str,"%s","%s","%s","%s","%s","%s",
	 "%s","%s","%s","%s");
  pop_n_elems(args);
}

void f_closelog(INT32 args)
{
  closelog();
  pop_n_elems(args);
}
#endif

#ifdef HAVE_PERROR
void f_real_perror(INT32 args)
{
  pop_n_elems(args);
  perror(NULL);
}
#else
void f_real_perror(INT32 args)
{
  pop_n_elems(args);
}
#endif

void f_get_all_active_fd(INT32 args)
{
  int i,fds;
  struct stat foo;
  pop_n_elems(args);
  for (i=fds=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
    if (!fstat(i,&foo))
    {
      push_int(i);
      fds++;
    }
  f_aggregate(fds);
}

void f_fd_info(INT32 args)
{
  static char buf[256];
  int i;
  struct stat foo;

  if (args<1||
      sp[-args].type!=T_INT)
    error("Illegal argument to fd_info\n");
  i=sp[-args].u.integer;
  pop_n_elems(args);
  if (fstat(i,&foo))
  {
    push_string(make_shared_string("non-open filedescriptor"));
    return;
  }
  sprintf(buf,"%o,%ld,%d,%ld",
	  (unsigned int)foo.st_mode,
	  (long)foo.st_size,
	  (int)foo.st_dev,
	  (long)foo.st_ino);
  push_string(make_shared_string(buf));
}

struct lpc_string *fd_marks[MAX_OPEN_FILEDESCRIPTORS];

void f_mark_fd(INT32 args)
{
  int fd;
  struct lpc_string *s;
  if (args<1 
      || sp[-args].type!=T_INT 
      || (args>2 && sp[-args+1].type!=T_STRING))
    error("Illegal argument(s) to mark_fd(int,void|string)\n");
  fd=sp[-args].u.integer;
  if(fd>MAX_OPEN_FILEDESCRIPTORS || fd < 0)
    error("Fd must be in the range 0 to %d\n", MAX_OPEN_FILEDESCRIPTORS);
  if (args<2)
  {
    int len;
    struct sockaddr_in addr;
    char *tmp;
    char buf[20];

    pop_stack();
    if(fd_marks[fd])
    {
      fd_marks[fd]->refs++;
      push_string(fd_marks[fd]);
    } else {
      push_int(0);
    }

    len=sizeof(addr);
    if(! getsockname(fd, (struct sockaddr *) &addr, &len))
    {
      tmp=inet_ntoa(addr.sin_addr);
      push_string(make_shared_string(" Local:"));
      push_string(make_shared_string(tmp));
      sprintf(buf,".%d",(int)(ntohs(addr.sin_port)));
      push_string(make_shared_string(buf));
      f_sum(4);
    }

    if(! getpeername(fd, (struct sockaddr *) &addr, &len))
    {
      push_string(make_shared_string(" Remote:"));
      tmp=inet_ntoa(addr.sin_addr);
      push_string(make_shared_string(tmp));
      sprintf(buf,".%d",(int)(ntohs(addr.sin_port)));
      push_string(make_shared_string(buf));
      f_sum(4);
    }

    return;
  }
  s=sp[-args+1].u.string;
  s->refs++;
  if(fd_marks[fd])
    free_string(fd_marks[fd]);
  fd_marks[fd]=s;
  pop_n_elems(args);
  push_int(0);
}

void init_spider_efuns(void) 
{

  add_efun("parse_accessed_database", f_parse_accessed_database,
	   "function(string:array)", OPT_TRY_OPTIMIZE);

  add_efun("_string_debug", f__string_debug, "function(void|mixed:string)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_dump_string_table", f__dump_string_table, 
	   "function(:array(array))",  OPT_EXTERNAL_DEPEND);

  add_efun("_num_dest_objects", f__num_dest_objects, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_arrays", f__num_arrays, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_objects", f__num_objects, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_mappings", f__num_mappings, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_dump_obj_table", f__dump_obj_table, "function(:array(array))", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("parse_html",f_parse_html,
	   "function(string,mapping(string:function(string,mapping(string:string),mixed ...:string)),mapping(string:function(string,mapping(string:string),string,mixed ...:string)),mixed ...:string)",
	   0);

  add_efun("do_match",f_do_match,
	   "function(string|array(string),string:string)",OPT_TRY_OPTIMIZE);

  add_efun("localtime",f_localtime,
	   "function(int:mapping(string:mixed))",OPT_EXTERNAL_DEPEND);

  add_efun("real_perror",f_real_perror, "function(:void)",OPT_EXTERNAL_DEPEND);

#ifdef HAVE_SYSLOG
  add_efun("openlog", f_openlog, "function(string,int,int:void)", 0);
  add_efun("syslog", f_syslog, "function(int,string:void)", 0);
  add_efun("closelog", f_closelog, "function(:void)", 0);
#endif

  add_efun("discdate", f_discdate, "function(int:array)", 0);
  add_efun("stardate", f_stardate, "function(int,void|int:int)", 0);

  add_efun("setuid", f_do_setuid, "function(int:void)", 0);
  add_efun("setgid", f_do_setgid, "function(int:void)", 0);
  add_efun("timezone",f_timezone,"function(:int)",0);
#if 0
  add_efun("exece",f_exece,"function(string,array(string),array(string):int)",
	   OPT_SIDE_EFFECT);
#endif
  add_efun("get_all_active_fd",f_get_all_active_fd,"function(:array(int))",0);
  add_efun("fd_info",f_fd_info,"function(int:string)",0);
  add_efun("mark_fd",f_mark_fd,"function(int,void|mixed:mixed)",0);

  /* timezone() needs */
  { 
    time_t foo;
    struct tm *g;
    /* Purify wants */
    foo=(time_t)0; 

    g=localtime(&foo);
#if !HAVE_INT_TIMEZONE
    _tz = g->tm_gmtoff;
#endif
  }
}

void init_spider_programs() {}

void exit_spider(void)
{
  int i;
  for(i=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
    if(fd_marks[i])
      free_string(fd_marks[i]);
}
