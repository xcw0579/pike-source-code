/*
 * $Id: lexer.h,v 1.20 2000/08/07 12:13:37 grubba Exp $
 *
 * Lexical analyzer template.
 * Based on lex.c 1.62
 *
 * Henrik Grubbström 1999-02-20
 */

#ifndef SHIFT
#error Internal error: SHIFT not defined
#endif

/*
 * Definitions
 */
#if (SHIFT == 0)

#define LOOK() EXTRACT_UCHAR(lex.pos)
#define GETC() EXTRACT_UCHAR(lex.pos++)
#define SKIP() lex.pos++
#define GOBBLE(c) (LOOK()==c?(lex.pos++,1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') lex.pos++; }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) lex.pos++; }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) lex.pos++; }while(0)

#define READBUF(X) do {				\
  register int C;				\
  buf = lex.pos;				\
  while((C = LOOK()) && (X))			\
    lex.pos++;					\
  len = (size_t)(lex.pos - buf);		\
} while(0)

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)
#define ISWORD(X) ((len == strlen(X)) && !MEMCMP(buf,X,strlen(X)))

/*
 * Function renaming
 */

#define yylex yylex0
#define low_yylex low_yylex0
#define lex_atoi atoi
#define lex_strtol STRTOL
#define lex_strtod my_strtod
#define lex_isidchar isidchar

#else /* SHIFT != 0 */

#define LOOK() INDEX_CHARP(lex.pos,0,SHIFT)
#define GETC() ((lex.pos+=(1<<SHIFT)),INDEX_CHARP(lex.pos-(1<<SHIFT),0,SHIFT))
#define SKIP() (lex.pos += (1<<SHIFT))
#define GOBBLE(c) (LOOK()==c?((lex.pos+=(1<<SHIFT)),1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') lex.pos += (1<<SHIFT); }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) lex.pos += (1<<SHIFT); }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) lex.pos += (1<<SHIFT); }while(0)

#define READBUF(X) do {				\
  register int C;				\
  buf = lex.pos;				\
  while((C = LOOK()) && (X))			\
    lex.pos += (1<<SHIFT);			\
  len = (size_t)((lex.pos - buf) >> SHIFT);	\
} while(0)

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)

#define ISWORD(X) ((len == strlen(X)) && low_isword(buf, X, strlen(X)))

/* Function renaming */
#if (SHIFT == 1)

#define low_isword low_isword1
#define char_const char_const1
#define readstring readstring1
#define yylex yylex1
#define low_yylex low_yylex1
#define lex_atoi lex_atoi1
#define lex_strtol lex_strtol1
#define lex_strtod lex_strtod1

#else /* SHIFT != 1 */

#define low_isword low_isword2
#define char_const char_const2
#define readstring readstring2
#define yylex yylex2
#define low_yylex low_yylex2
#define lex_atoi lex_atoi2
#define lex_strtol lex_strtol2
#define lex_strtod lex_strtod2

#endif /* SHIFT == 1 */


#define lex_isidchar(X) ((((unsigned) X)>=256) || isidchar(X))

static int low_isword(char *buf, char *X, size_t len)
{
  while(len--) {
    if (INDEX_CHARP(buf, len, SHIFT) != ((unsigned char *)X)[len]) {
      return 0;
    }
  }
  return 1;
}

static int lex_atoi(char *buf)
{
  /* NOTE: Cuts at 63 digits */
  char buff[64];
  int i=0;
  int c;

  while(((c = INDEX_CHARP(buf, i, SHIFT))>='0') && (c <= '9') && (i < 63)) {
    buff[i++] = c;
  }
  buff[i] = 0;
  return atoi(buff);
}

static long lex_strtol(char *buf, char **end, int base)
{
  PCHARP foo;
  long ret;
  ret=STRTOL_PCHARP(MKPCHARP(buf,SHIFT),&foo,base);
  if(end) end[0]=(char *)foo.ptr;
  return ret;
}

static double lex_strtod(char *buf, char **end)
{
  PCHARP foo;
  double ret;
  ret=STRTOD_PCHARP(MKPCHARP(buf,SHIFT),&foo);
  if(end) end[0]=(char *)foo.ptr;
  return ret;
}

#endif /* SHIFT == 0 */


/*** Lexical analyzing ***/

static int char_const(void)
{
  int c;
  switch(c=GETC())
  {
    case 0:
      lex.pos -= (1<<SHIFT);
      yyerror("Unexpected end of file\n");
      return 0;
      
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      c-='0';
      while(LOOK()>='0' && LOOK()<='8')
	c=c*8+(GETC()-'0');
      return c;
      
    case 'r': return '\r';
    case 'n': return '\n';
    case 't': return '\t';
    case 'b': return '\b';
      
    case '\n':
      lex.current_line++;
      return '\n';
      
    case 'x':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*16+GETC()-'0';
	    continue;
	    
	  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    c=c*16+GETC()-'a'+10;
	    continue;
	    
	  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    c=c*16+GETC()-'A'+10;
	    continue;
	}
	break;
      }
      break;

    case 'd':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*10+GETC()-'0';
	    continue;
	}
	break;
      }
      break;
  }
  return c;
}

static struct pike_string *readstring(void)
{
  int c;
  struct string_builder tmp;
  init_string_builder(&tmp,0);
  
  while(1)
  {
    switch(c=GETC())
    {
    case 0:
      lex.pos -= (1<<SHIFT);
      yyerror("End of file in string.");
      break;
      
    case '\n':
      lex.current_line++;
      yyerror("Newline in string.");
      break;
      
    case '\\':
      string_builder_putchar(&tmp,char_const());
      continue;
      
    case '"':
      break;
      
    default:
      string_builder_putchar(&tmp,c);
      continue;
    }
    break;
  }
  return finish_string_builder(&tmp);
}



int yylex(YYSTYPE *yylval)
#if LEXDEBUG>4
{
  int t;
  int low_yylex(YYSTYPE *);
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX:\n");
#endif /* LEXDEBUG>8 */

  t=low_yylex(yylval);
  if(t<256)
  {
    fprintf(stderr,"YYLEX: '%c' (%d) at %s:%d\n",t,t,lex.current_file->str,lex.current_line);
  }else{
    fprintf(stderr,"YYLEX: %s (%d) at %s:%d\n",low_get_f_name(t,0),t,lex.current_file->str,lex.current_line);
  }
  return t;
}

static int low_yylex(YYSTYPE *yylval)
#endif /* LEXDEBUG>4 */
{
  INT32 c;
  size_t len;
  char *buf;

#ifdef __CHECKER__
  MEMSET((char *)yylval,0,sizeof(YYSTYPE));
#endif
#ifdef MALLOC_DEBUG
  check_sfltable();
#endif

  while(1)
  {
    switch(c = GETC())
    {
    case 0:
      lex.pos -= (1<<SHIFT);
#ifdef TOK_LEX_EOF
      return TOK_LEX_EOF;
#else /* !TOK_LEX_EOF */
      return 0;
#endif /* TOK_LEX_EOF */

    case '\n':
      lex.current_line++;
      continue;

    case 0x1b: case 0x9b:	/* ESC or CSI */
      /* Assume ANSI/DEC escape sequence.
       * Format supported:
       * 	<ESC>[\040-\077]+[\100-\177]
       * or
       * 	<CSI>[\040-\077]*[\100-\177]
       */
      while ((c = LOOK()) && (c == ((c & 0x1f)|0x20))) {
	SKIP();
      }
      if (c == ((c & 0x3f)|0x40)) {
	SKIP();
      } else {
	/* FIXME: Warning here? */
      }
      continue;

    case '#':
      SKIPSPACE();
      READBUF(C!=' ' && C!='\t' && C!='\n');

      switch(len>0?INDEX_CHARP(buf, 0, SHIFT):0)
      {
	char *p;
	
      case 'l':
	if(!ISWORD("line")) goto badhash;
	READBUF(C!=' ' && C!='\t' && C!='\n');
	
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	lex.current_line=lex_atoi(buf)-1;
	SKIPSPACE();
	if(GOBBLE('"'))
	{
	  struct pike_string *tmp=readstring();
	  free_string(lex.current_file);
	  lex.current_file=tmp;
	}
	break;
	
      case 'e':
	if(ISWORD("error"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  yyerror(buf);
	  break;
	}
	goto badhash;

      case 'p':
	/* FIXME: Support #pike */
	if(ISWORD("pragma"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  if (ISWORD("all_inline"))
	  {
	    lex.pragmas |= ID_INLINE;
	  }
	  else if (ISWORD("all_nomask"))
	  {
	    lex.pragmas |= ID_NOMASK;
	  }
	  else if (ISWORD("strict_types"))
	  {
	    lex.pragmas |= ID_STRICT_TYPES;
	  }
	  break;
	}
	
      badhash:
	/* FIXME: This doesn't look all that safe...
	 * buf isn't NUL-terminated, and it won't work on wide strings.
	 * /grubba 1999-02-20
	 */
	if (strlen(buf) < 1024) {
	  my_yyerror("Unknown preprocessor directive #%s.",buf);
	} else {
	  my_yyerror("Unknown preprocessor directive.");
	}
	SKIPUPTO('\n');
	continue;
      }
      continue;

    case ' ':
    case '\t':
      continue;

    case '\'':
      switch(c=GETC())
      {
      case 0:
	lex.pos -= (1<<SHIFT);
	yyerror("Unexpected end of file\n");
	break;

	case '\\':
	  c = char_const();
      }
      if(!GOBBLE('\''))
	yyerror("Unterminated character constant.");
      debug_malloc_pass( yylval->n=mkintnode(c) );
      return TOK_NUMBER;
	
    case '"':
    {
      struct pike_string *s=readstring();
      yylval->n=mkstrnode(s);
      free_string(s);
      return TOK_STRING;
    }
  
    case ':':
      if(GOBBLE(':')) return TOK_COLON_COLON;
      return c;

    case '.':
      if(GOBBLE('.'))
      {
	if(GOBBLE('.')) return TOK_DOT_DOT_DOT;
	return TOK_DOT_DOT;
      }
      return c;
  
    case '0':
    {
      int base = 0;
      
      if(GOBBLE('b') || GOBBLE('B'))
      {
	base = 2;
	goto read_based_number;
      }
      else if(GOBBLE('x') || GOBBLE('X'))
      {
	struct svalue sval;
	base = 16;
      read_based_number:
	sval.type = PIKE_T_INT;
	sval.subtype = NUMBER_NUMBER;
	sval.u.integer = 0;
	wide_string_to_svalue_inumber(&sval,
				      lex.pos,
				      (void **)&lex.pos,
				      base,
				      0,
				      SHIFT);
	dmalloc_touch_svalue(&sval);
	yylval->n = mksvaluenode(&sval);
	free_svalue(&sval);
	return TOK_NUMBER;
      }
    }
  
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      char *p1, *p2;
      double f;
      long l;
      struct svalue sval;

      lex.pos -= (1<<SHIFT);
      if(INDEX_CHARP(lex.pos, 0, SHIFT)=='0')
	for(l=1;INDEX_CHARP(lex.pos, l, SHIFT)<='9' &&
	      INDEX_CHARP(lex.pos, l, SHIFT)>='0';l++)
	  if(INDEX_CHARP(lex.pos, l, SHIFT)>='8')
	    yyerror("Illegal octal number.");

      f=lex_strtod(lex.pos, &p1);

      sval.type = PIKE_T_INT;
      sval.subtype = NUMBER_NUMBER;
      sval.u.integer = 0;      

      wide_string_to_svalue_inumber(&sval,
				    lex.pos,
				    (void **)&p2,
				    0,
				    0,
				    SHIFT);
      dmalloc_touch_svalue(&sval);
      yylval->n = mksvaluenode(&sval);
      free_svalue(&sval);

      if(p1>p2)
      {
	debug_malloc_touch(yylval->n);
	free_node(yylval->n);
	lex.pos=p1;
	yylval->fnum=(FLOAT_TYPE)f;
	return TOK_FLOAT;
      }else{
	debug_malloc_touch(yylval->n);
	lex.pos=p2;
	return TOK_NUMBER;
      }
  
    case '-':
      if(GOBBLE('=')) return TOK_SUB_EQ;
      if(GOBBLE('>')) return TOK_ARROW;
      if(GOBBLE('-')) return TOK_DEC;
      return '-';
  
    case '+':
      if(GOBBLE('=')) return TOK_ADD_EQ;
      if(GOBBLE('+')) return TOK_INC;
      return '+';
  
    case '&':
      if(GOBBLE('=')) return TOK_AND_EQ;
      if(GOBBLE('&')) return TOK_LAND;
      return '&';
  
    case '|':
      if(GOBBLE('=')) return TOK_OR_EQ;
      if(GOBBLE('|')) return TOK_LOR;
      return '|';

    case '^':
      if(GOBBLE('=')) return TOK_XOR_EQ;
      return '^';
  
    case '*':
      if(GOBBLE('=')) return TOK_MULT_EQ;
      return '*';

    case '%':
      if(GOBBLE('=')) return TOK_MOD_EQ;
      return '%';
  
    case '/':
      if(GOBBLE('=')) return TOK_DIV_EQ;
      return '/';
  
    case '=':
      if(GOBBLE('=')) return TOK_EQ;
      return '=';
  
    case '<':
      if(GOBBLE('<'))
      {
	if(GOBBLE('=')) return TOK_LSH_EQ;
	return TOK_LSH;
      }
      if(GOBBLE('=')) return TOK_LE;
      return '<';
  
    case '>':
      if(GOBBLE(')')) return TOK_MULTISET_END;
      if(GOBBLE('=')) return TOK_GE;
      if(GOBBLE('>'))
      {
	if(GOBBLE('=')) return TOK_RSH_EQ;
	return TOK_RSH;
      }
      return '>';

    case '!':
      if(GOBBLE('=')) return TOK_NE;
      return TOK_NOT;

    case '(':
      if(GOBBLE('<')) return TOK_MULTISET_START;
      return '(';

    case ']':
    case '?':
    case ',':
    case '~':
    case '@':
    case ')':
    case '[':
    case '{':
    case ';':
    case '}': return c;

    case '`':
    {
      char *tmp;
      int offset=2;
      if(GOBBLE('`')) offset--;
      if(GOBBLE('`')) offset--;
      
      switch(GETC())
      {
      case '/': tmp="```/"; break;
      case '%': tmp="```%"; break;
      case '*': tmp="```*"; break;
      case '&': tmp="```&"; break;
      case '|': tmp="```|"; break;
      case '^': tmp="```^"; break;
      case '~': tmp="```~"; break;
      case '+':
	if(GOBBLE('=')) { tmp="```+="; break; }
	tmp="```+";
	break;
      case '<':
	if(GOBBLE('<')) { tmp="```<<"; break; }
	if(GOBBLE('=')) { tmp="```<="; break; }
	tmp="```<";
	break;

      case '>':
	if(GOBBLE('>')) { tmp="```>>"; break; }
	if(GOBBLE('=')) { tmp="```>="; break; }
	tmp="```>";
	break;

      case '!':
	if(GOBBLE('=')) { tmp="```!="; break; }
	tmp="```!";
	break;

      case '=':
	if(GOBBLE('=')) { tmp="```=="; break; }
	tmp="```=";
	break;

      case '(':
	if(GOBBLE(')')) 
	{
	  tmp="```()";
	  break;
	}
	yyerror("Illegal ` identifier.");
	tmp="``";
	break;
	
      case '-':
	if(GOBBLE('>'))
	{
	  tmp="```->";
	  if(GOBBLE('=')) tmp="```->=";
	}else{
	  tmp="```-";
	}
	break;

      case '[':
	if(GOBBLE(']'))
	{
	  tmp="```[]";
	  if(GOBBLE('=')) tmp="```[]=";
	  break;
	}

      default:
	yyerror("Illegal ` identifier.");
	lex.pos -= (1<<SHIFT);
	tmp="``";
	break;
      }

      {
	struct pike_string *s=make_shared_string(tmp+offset);
	yylval->n=mkstrnode(s);
	free_string(s);
	return TOK_IDENTIFIER;
      }
    }

  
    default:
      if(lex_isidchar(c))
      {
	struct pike_string *s;
	lex.pos -= (1<<SHIFT);
	READBUF(lex_isidchar(C));

	yylval->number=lex.current_line;

	if(len>1 && len<10)
	{
	  /* NOTE: TWO_CHAR() will generate false positives with wide strings,
	   * but that doesn't matter, since ISWORD() will fix it.
	   */
	  switch(TWO_CHAR(INDEX_CHARP(buf, 0, SHIFT),
			  INDEX_CHARP(buf, 1, SHIFT)))
	  {
	  case TWO_CHAR('a','r'):
	    if(ISWORD("array")) return TOK_ARRAY_ID;
	  break;
	  case TWO_CHAR('b','r'):
	    if(ISWORD("break")) return TOK_BREAK;
	  break;
	  case TWO_CHAR('c','a'):
	    if(ISWORD("case")) return TOK_CASE;
	    if(ISWORD("catch")) return TOK_CATCH;
	  break;
	  case TWO_CHAR('c','l'):
	    if(ISWORD("class")) return TOK_CLASS;
	  break;
	  case TWO_CHAR('c','o'):
	    if(ISWORD("constant")) return TOK_CONSTANT;
	    if(ISWORD("continue")) return TOK_CONTINUE;
	  break;
	  case TWO_CHAR('d','e'):
	    if(ISWORD("default")) return TOK_DEFAULT;
	  break;
	  case TWO_CHAR('d','o'):
	    if(ISWORD("do")) return TOK_DO;
	  break;
	  case TWO_CHAR('e','l'):
	    if(ISWORD("else")) return TOK_ELSE;
	  break;
	  case TWO_CHAR('e','x'):
	    if(ISWORD("extern")) return TOK_EXTERN;
	  break;
	  case TWO_CHAR('f','i'):
	    if(ISWORD("final")) return TOK_FINAL_ID;
	  break;
	  case TWO_CHAR('f','l'):
	    if(ISWORD("float")) return TOK_FLOAT_ID;
	  break;
	  case TWO_CHAR('f','o'):
	    if(ISWORD("for")) return TOK_FOR;
	    if(ISWORD("foreach")) return TOK_FOREACH;
	  break;
	  case TWO_CHAR('f','u'):
	    if(ISWORD("function")) return TOK_FUNCTION_ID;
	  break;
	  case TWO_CHAR('g','a'):
	    if(ISWORD("gauge")) return TOK_GAUGE;
	  break;
	  case TWO_CHAR('i','f'):
	    if(ISWORD("if")) return TOK_IF;
	  break;
	  case TWO_CHAR('i','m'):
	    if(ISWORD("import")) return TOK_IMPORT;
	  break;
	  case TWO_CHAR('i','n'):
	    if(ISWORD("int")) return TOK_INT_ID;
	    if(ISWORD("inherit")) return TOK_INHERIT;
	    if(ISWORD("inline")) return TOK_INLINE;
	  break;
	  case TWO_CHAR('l','a'):
	    if(ISWORD("lambda")) return TOK_LAMBDA;
	  break;
	  case TWO_CHAR('l','o'):
	    if(ISWORD("local")) return TOK_LOCAL_ID;
	  break;
	  case TWO_CHAR('m','a'):
	    if(ISWORD("mapping")) return TOK_MAPPING_ID;
	  break;
	  case TWO_CHAR('m','i'):
	    if(ISWORD("mixed")) return TOK_MIXED_ID;
	  break;
	  case TWO_CHAR('m','u'):
	    if(ISWORD("multiset")) return TOK_MULTISET_ID;
	  break;
	  case TWO_CHAR('n','o'):
	    if(ISWORD("nomask")) return TOK_NO_MASK;
	  break;
	  case TWO_CHAR('o','b'):
	    if(ISWORD("object")) return TOK_OBJECT_ID;
	  break;
	  case TWO_CHAR('o','p'):
	    if(ISWORD("optional")) return TOK_OPTIONAL;
	  break;
	  case TWO_CHAR('p','r'):
	    if(ISWORD("program")) return TOK_PROGRAM_ID;
	    if(ISWORD("predef")) return TOK_PREDEF;
	    if(ISWORD("private")) return TOK_PRIVATE;
	    if(ISWORD("protected")) return TOK_PROTECTED;
	    break;
	  break;
	  case TWO_CHAR('p','u'):
	    if(ISWORD("public")) return TOK_PUBLIC;
	  break;
	  case TWO_CHAR('r','e'):
	    if(ISWORD("return")) return TOK_RETURN;
	  break;
	  case TWO_CHAR('s','s'):
	    if(ISWORD("sscanf")) return TOK_SSCANF;
	  break;
	  case TWO_CHAR('s','t'):
	    if(ISWORD("string")) return TOK_STRING_ID;
	    if(ISWORD("static")) return TOK_STATIC;
	  break;
	  case TWO_CHAR('s','w'):
	    if(ISWORD("switch")) return TOK_SWITCH;
	  break;
	  case TWO_CHAR('t','y'):
	    if(ISWORD("typeof")) return TOK_TYPEOF;
	  break;
	  case TWO_CHAR('v','a'):
	    if(ISWORD("variant")) return TOK_VARIANT;
	  break;
	  case TWO_CHAR('v','o'):
	    if(ISWORD("void")) return TOK_VOID_ID;
	  break;
	  case TWO_CHAR('w','h'):
	    if(ISWORD("while")) return TOK_WHILE;
	  break;
	  }
	}
	{
#if (SHIFT == 0)
	  struct pike_string *tmp = make_shared_binary_string(buf, len);
#else /* SHIFT != 0 */
#if (SHIFT == 1)
	  struct pike_string *tmp = make_shared_binary_string1((p_wchar1 *)buf,
							       len);
#else /* SHIFT != 1 */
	  struct pike_string *tmp = make_shared_binary_string2((p_wchar2 *)buf,
							       len);
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */
	  yylval->n=mkstrnode(tmp);
	  free_string(tmp);
	  return TOK_IDENTIFIER;
	}
#if 0
      }else if (c == (c & 0x9f)) {
	/* Control character in one of the ranges \000-\037 or \200-\237 */
	/* FIXME: Warning here? */
	/* Ignore */
#endif /* 0 */
      }else{
	char buff[100];
	if ((c > 31) && (c < 256)) {
	  sprintf(buff, "Illegal character (hex %02x) '%c'", c, c);
	} else {
	  sprintf(buff, "Illegal character (hex %02x)", c);
	}
	yyerror(buff);
	return ' ';
      }
    }
    }
  }
}

/*
 * Clear the defines for the next pass
 */

#undef LOOK
#undef GETC
#undef SKIP
#undef GOBBLE
#undef SKIPSPACE
#undef SKIPWHITE
#undef SKIPUPTO
#undef READBUF
#undef TWO_CHAR
#undef ISWORD

#undef low_isword
#undef char_const
#undef readstring
#undef yylex
#undef low_yylex
#undef lex_atoi
#undef lex_strtol
#undef lex_strtod
#undef lex_isidchar
