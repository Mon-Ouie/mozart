%{
/* Copyright (c) by Denys Duchier, April 1996, Universitaet des Saarlandes */
#include <stdio.h>
#include <string.h>
int start=0;
#define warning(msg) fprintf(stderr,"Warning at line %d (%d): %s\n",yylineno,start,msg)
#define error(msg) { \
  fprintf(stderr,"Error at line %d (%d): %s\n",yylineno,start,msg); \
  exit(-1) ; }
int space=0;
#define SPACE space++
#define TAB   {int n = 8 - (++space % 8); space += n; }
#define RESET space=0
#define FLUSH {if (space>0) printf("\\OzSpace{%d}",space); RESET; }
#define PUSH yy_push_state
#define POP  yy_pop_state()
int level=0;
#define INCR level++
#define DECR if (--level<=0) POP
#define OZCHAR {                           \
  FLUSH;                                   \
  if (yytext[0]=='\\') printf("\\OzBsl "); \
  else printf("\\OzChar\\%c",yytext[0]); }
#define OZKEYWORD { FLUSH; printf("\\OzKeyword{%s}",yytext); }
#define FLUSHECHO {FLUSH;ECHO;}
#define ENDPOP    {printf("}");POP;}
#define FLUSHPOP  {FLUSH;printf("}");POP;}
#define ESCAPE    {                                             \
  int c;                                                        \
  FLUSH;                                                        \
  while ((c=input())!='') {                                   \
    if (c==EOF) error("end of file in escape from Oz code");    \
    putchar(c); } }
#define STARTFWD { FLUSH; printf("\\OzFwd{"); PUSH(QUOTED); }
#define STARTBWD { FLUSH; printf("\\OzBwd{"); PUSH(QUOTED); }
#define STARTSTRING { FLUSH; printf("\\OzString{"); PUSH(STRING); }
#define START start=yylineno
void banner() {
  printf("%s","%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
  printf("%s","%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
  printf("%s","%                                                                  %\n");
  printf("%s","%    This file has been generated by raw2tex from a .raw file      %\n");
  printf("%s","%           raw2tex (flex) by Denys Duchier, April 1996            %\n");
  printf("%s","%                   DON'T EDIT THIS FILE !                         %\n");
  printf("%s","%                                                                  %\n");
  printf("%s","%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
  printf("%s","%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
}
#define OZECHAR {                            \
  printf("\\OzBsl ");                         \
  if (yytext[1]=='\\') printf("\\OzBsl ");   \
  else if (index("{}$&#^_%~'`\"",yytext[1])) \
    printf("\\OzChar\\%c",yytext[1]);        \
  else                                       \
    putchar(yytext[1]); }
#define ECHOIDENT(s) {                       \
  char* s__ = s;                             \
  while(*s__) {                              \
    if (*s__ == '_') printf("\\OzChar\\_");  \
    else putchar(*s__);                      \
    s__ ++; }                                \
}
#define FLUSHECHOIDENT { FLUSH; ECHOIDENT(yytext); }
%}
IDENT           [a-zA-Z0-9_]+
KEYWORD         (proc|fun|local|declare|"if"|or|dis|choice|"case"|then|"else"|elseif|of|elseof|elsecase|end|"class"|meth|from|with|attr|feat|self|true|false|touch|div|mod|andthen|orelse|thread|in|condis|not|try|catch|finally|raise|lock|skip|fail|unit|prop)
CKEYWORD        ("asm"|"auto"|"break"|"char"|"case"|"const"|"continue"|"default"|"do"|"double"|"else"|"enum"|"extern"|"float"|"for"|"goto"|"if"|"inline"|"int"|"long"|"register"|"return"|"short"|"signed"|"sizeof"|"static"|"struct"|"switch"|"typedef"|"typeof"|"union"|"unsigned"|"void"|"volatile"|"while"|"all"|"except"|"exception"|"raise"|"raises"|"reraise"|"throw"|"try"|"catch"|"class"|"classof"|"delete"|"dynamic"|"friend"|"headof"|"new"|"operator"|"overload"|"private"|"protected"|"public"|"this"|"template"|"virtual")
PSEUDOCKEYWORD  ({CKEYWORD}|"repeat"|"until"|"end"|"then")
COPERATOR       ("->"|"<<"|">>"|"<="|">="|"!="|"||"|"..."|"*="|"<<="|">>="|"^="|"|="|"~"|"*"|"^"|"|"|"->*"|"/"|"<"|">"|"&&"|"%="|"&="|"{"|"}"|"&"|"%"|"--"|".*"|"?"|":"|"="|","|"."|";"|"!"|"-"|"+"|"/="|"=="|"++"|"+="|"-="|"("|")"|"["|"]"|"::")
BLANKLINES      ([ \t]*\n)*
%option stack
%option main
%option yylineno
%x TEX DISPLAY CDISPLAY QUOTED STRING COMMENT EOLCOMMENT INLINE INDEX ENTRY SEE SKIP INL CINL PSEUDOCDISPLAY PSEUDOCINL
%%
        banner(); BEGIN(TEX);

<TEX>"%".*              ECHO;
<TEX>"\\begin{ozdisplay""*"?"}"{BLANKLINES}     {
  printf("\\begin{oz2texdisplay}");
  RESET; START; BEGIN(DISPLAY); }
<TEX>"\\begin{cdisplay}"{BLANKLINES}            {
  printf("\\begin{c2texdisplay}");
  RESET; START; BEGIN(CDISPLAY); }
<TEX>"\\begin{pseudocdisplay}"{BLANKLINES}              {
  printf("\\begin{c2texdisplay}");
  RESET; START; BEGIN(PSEUDOCDISPLAY); }
<TEX>"\\begin{codedisplay}"[ \t\n]*"{oz}"{BLANKLINES}   {
  printf("\\begin{oz2texdisplay}");
  RESET;START;BEGIN(DISPLAY); }
<TEX>"\\begin{codedisplay}"[ \t\n]*"{c}"{BLANKLINES}    {
  printf("\\begin{c2texdisplay}");
  RESET;START;BEGIN(CDISPLAY); }
<TEX>"\\begin{codedisplay}"[ \t\n]*"{c++}"{BLANKLINES}  {
  printf("\\begin{c2texdisplay}");
  RESET;START;BEGIN(CDISPLAY); }
<TEX>"\\begin{codedisplay}"[ \t\n]*"{pseudoc}"{BLANKLINES}      {
  printf("\\begin{c2texdisplay}");
  RESET;START;BEGIN(PSEUDOCDISPLAY); }
<TEX>"\\begin{codedisplay}"[ \t\n]*"{pseudoc++}"{BLANKLINES}    {
  printf("\\begin{c2texdisplay}");
  RESET;START;BEGIN(PSEUDOCDISPLAY); }
<TEX>"\\?"              {
  printf("\\OzInline{"); RESET; START; PUSH(INLINE); }
<TEX>"\\codeinline"[ \t\n]*"{oz}"[ \t\n]*"{"    {
  printf("\\OzInline{");RESET;START;PUSH(INL);level=1;}
<TEX>"\\codeinline"[ \t\n]*"{c}"[ \t\n]*"{"             {
  printf("\\CInline{");RESET;START;PUSH(CINL);level=1;}
<TEX>"\\codeinline"[ \t\n]*"{c++}"[ \t\n]*"{"           {
  printf("\\CInline{");RESET;START;PUSH(CINL);level=1;}
<TEX>"\\codeinline"[ \t\n]*"{pseudoc}"[ \t\n]*"{"               {
  printf("\\CInline{");RESET;START;PUSH(PSEUDOCINL);level=1;}
<TEX>"\\codeinline"[ \t\n]*"{pseudoc++}"[ \t\n]*"{"             {
  printf("\\CInline{");RESET;START;PUSH(PSEUDOCINL);level=1;}
<TEX>"\\ozindex{"       {
  printf("\\index{");    RESET; START; PUSH(INDEX);  }
<TEX>%.*\n              ECHO;
<TEX>"\\".              ECHO;
<TEX>.                  ECHO;
<TEX>\n                 ECHO;

<DISPLAY>^"@H@"         ;
<DISPLAY>^"@D@".*\n     ;
<DISPLAY>[ \t\n]*"\\end{ozdisplay""*"?"}"       {
  printf("\\end{oz2texdisplay}");
  BEGIN(TEX); }
<CDISPLAY>[ \t\n]*"\\end{cdisplay}"     {
  printf("\\end{c2texdisplay}");
  BEGIN(TEX); }
<PSEUDOCDISPLAY>[ \t\n]*"\\end{pseudocdisplay}" {
  printf("\\end{c2texdisplay}");
  BEGIN(TEX); }
<DISPLAY>[ \t\n]*"\\end{codedisplay}"   {
  printf("\\end{oz2texdisplay}");
  BEGIN(TEX); }
<CDISPLAY,PSEUDOCDISPLAY>[ \t\n]*"\\end{codedisplay}"   {
  printf("\\end{c2texdisplay}");
  BEGIN(TEX); }
<CDISPLAY,PSEUDOCDISPLAY>^"#"[ \t]*{IDENT}      {
  char* s=yytext;
  space=1;
loop:
  switch (*++s) {
  case ' ' : SPACE; goto loop;
  case '\t': TAB;   goto loop;
  default: break;
  }
  space--;
  printf("\\OzMacro{%d}{",space);
  ECHOIDENT(s);
  printf("}");
  RESET;
}
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>      ESCAPE;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>"/*"   { FLUSH; printf("\\OzComment{"); PUSH(COMMENT); }
<CDISPLAY,PSEUDOCDISPLAY>"//"   { FLUSH; printf("\\OzEolComment{"); PUSH(EOLCOMMENT); }
<DISPLAY>"%"    { FLUSH; printf("\\OzEolComment{"); PUSH(EOLCOMMENT); }
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>" "    SPACE;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>\t     TAB;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>\n     { RESET; printf("\\OzEol\n"); }
<CDISPLAY,PSEUDOCDISPLAY>{COPERATOR}    {
  char *s = yytext;
  FLUSH;
  printf("\\OzOp{");
  while (*s) {
    if (*s=='%' || *s=='{' || *s=='}' || *s=='^' || *s=='&' || *s=='~') putchar('\\');
    putchar(*s++); }
  putchar('}');
}
<DISPLAY>"&"[\"\']      {
  FLUSH;
  printf("\\OzChar\\&\\OzChar\\%c ",yytext[1]); }
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>[\\{}$&#^_%~]  OZCHAR;
<DISPLAY>{KEYWORD}                              OZKEYWORD;
<CDISPLAY>{CKEYWORD}                            OZKEYWORD;
<PSEUDOCDISPLAY>{PSEUDOCKEYWORD}                OZKEYWORD;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>{IDENT}        FLUSHECHOIDENT;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>\'             STARTFWD;
<DISPLAY>\`             STARTBWD;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>\"             STARTSTRING;
<DISPLAY,CDISPLAY,PSEUDOCDISPLAY>.              FLUSHECHO;

<QUOTED>\\.             OZECHAR;
<QUOTED>[{}$&#^_%~]     OZCHAR;
<QUOTED>[\'\`]          { FLUSH; printf("}"); POP; }
<QUOTED>" "             SPACE;
<QUOTED>\t              TAB;
<QUOTED>.               FLUSHECHO;
<QUOTED>\n              {
  SPACE; warning("newline in quoted symbol (ignored)"); }

<STRING>\\.     OZECHAR;
<STRING>\"      FLUSHPOP;
<STRING>" "     SPACE;
<STRING>\t      TAB;
<STRING>\n      { FLUSH; printf("\\OzEol\n"); }
<STRING>[{}$&#^_%~\'\`] OZCHAR;
<STRING>.       FLUSHECHO;

<COMMENT>"*/"   FLUSHPOP;
<COMMENT>[\\{}$&#^_%~]  OZCHAR;
<COMMENT>" "    SPACE;
<COMMENT>\t     TAB;
<COMMENT>\n     { RESET; printf("\\OzEol\n"); }
<COMMENT>.      FLUSHECHO;

<EOLCOMMENT>\n  ENDPOP;RESET;
<EOLCOMMENT>[\\{}$&#^_%~]       OZCHAR;
<EOLCOMMENT>" " SPACE;
<EOLCOMMENT>\t  TAB;
<EOLCOMMENT>.   FLUSHECHO;

<INLINE>"?"     ENDPOP;
<INLINE>       ESCAPE;
<INLINE>"/*"    { FLUSH; printf("\\OzComment{"); PUSH(COMMENT); }
<INLINE>"%"     { FLUSH; printf("\\OzEolComment{"); PUSH(EOLCOMMENT); }
<INLINE>" "     SPACE;
<INLINE>\t      TAB;
<INLINE>\n      {
  SPACE; warning("newline in inline Oz code (ignored)"); }
<INLINE>[\\{}$&#^_%~]   OZCHAR;
<INLINE>{KEYWORD}       OZKEYWORD;
<INLINE>{IDENT}         FLUSHECHOIDENT;
<INLINE>\'      STARTFWD;
<INLINE>\`      STARTBWD;
<INLINE>\"      STARTSTRING;
<INLINE>.       FLUSHECHO;

<INL,CINL,PSEUDOCINL>  ESCAPE;
<INL,CINL,PSEUDOCINL>"}"        if (--level<=0) {ENDPOP;} else {FLUSH;printf("\\OzChar\\}");};
<INL,CINL,PSEUDOCINL>"{"        {level++;FLUSH;printf("\\OzChar\\{");}
<INL,CINL,PSEUDOCINL>"/*"       {FLUSH;printf("\\OzComment{");PUSH(COMMENT);}
<INL>"%"        {FLUSH;printf("OzEolComment{");PUSH(EOLCOMMENT);}
<CINL,PSEUDOCINL>"//"   {FLUSH;printf("OzEolComment{");PUSH(EOLCOMMENT);}
<INL,CINL,PSEUDOCINL>" "        SPACE;
<INL,CINL,PSEUDOCINL>\t TAB;
<INL,CINL,PSEUDOCINL>\n {
  SPACE;warning("newinline in inlined code (ignored)"); }
<CINL,PSEUDOCINL>{COPERATOR}    {
  char *s = yytext;
  FLUSH;
  printf("\\OzOp{");
  while (*s) {
    if (*s=='%' || *s=='{' || *s=='}' || *s=='^' || *s=='&' || *s=='~') putchar('\\');
    putchar(*s++); }
  putchar('}');
}
<INL,CINL,PSEUDOCINL>[\\{}$&#^_%~]      OZCHAR;
<INL>{KEYWORD}                          OZKEYWORD;
<CINL>{CKEYWORD}                        OZKEYWORD;
<PSEUDOCINL>{PSEUDOCKEYWORD}            OZKEYWORD;
<INL,CINL,PSEUDOCINL>{IDENT}            FLUSHECHOIDENT;
<INL,CINL,PSEUDOCINL>\'                 STARTFWD;
<INL>\`                                 STARTBWD;
<INL,CINL,PSEUDOCINL>\"                 STARTSTRING;
<INL,CINL,PSEUDOCINL>.                  FLUSHECHO;


<INDEX>[^!|\}]*[!|\}]   {
  int n=yyleng-1; char*s=yytext;
  RESET;
  while (n-->0) putchar(*s++);
  printf("@\\OzBox{");
  yyless(0);
  BEGIN(ENTRY); }
<INDEX>.        error("unparseable \\ozindex");

<ENTRY>" "      SPACE;
<ENTRY>\t       TAB;
<ENTRY>\n       {
  SPACE; warning("newline in index entry (ignored)"); }
<ENTRY>"!"      {printf("}!");BEGIN(INDEX);}
<ENTRY>"}"      {printf("}}");POP;}
<ENTRY>"|see{"  {printf("}|see{\\OzBox{");BEGIN(SEE);}
<ENTRY>"|"      {printf("}|"); level=1; BEGIN(SKIP);}
<ENTRY>[\\$&#^_~]       OZCHAR;
<ENTRY>{KEYWORD}        OZKEYWORD;
<ENTRY>{IDENT}          FLUSHECHOIDENT;
<ENTRY>\'       STARTFWD;
<ENTRY>\`       STARTBWD;
<ENTRY>\"       STARTSTRING;
<ENTRY>.        FLUSHECHO;

<SEE>"}"        { FLUSH; printf("}}"); level=1; BEGIN(SKIP); }
<SEE>" "        SPACE;
<SEE>\t         TAB;
<SEE>\n         {
  SPACE; warning("newline in index/see (ignored)"); }
<SEE>[\\$&#^_~] OZCHAR;
<SEE>{KEYWORD}  OZKEYWORD;
<SEE>{IDENT}    FLUSHECHOIDENT;
<SEE>\'         STARTFWD;
<SEE>\`         STARTBWD;
<SEE>\"         STARTSTRING;
<SEE>.          FLUSHECHO;

<SKIP>%.*\n     ECHO;
<SKIP>\\.       ECHO;
<SKIP>"{"       ECHO;INCR;
<SKIP>"}"       ECHO;DECR;
<SKIP>\n        ECHO;
<SKIP>.         ECHO;
%%
