2,/^extern int yylineno;$/c\
static int start_cond = 0;\
#define BEGIN start_cond =
/^struct yysvf \*yyestate;$/,/^extern struct yysvf yysvec/d
/^# define YYNEWLINE /,/^int nstr;/d
/^while((nstr = yylook()/,/^if(yywrap()) /d
/^case -1:$/,/^} return(0); }/c\
	default: return(0);\
} }
/^# define YYTYPE unsigned short/c\
# define YYTYPE int
/^unsigned char yymatch\[\] = {/c\
char yymatch[] = {
/^unsigned char yyextra\[\] = {/c\
char yyextra[] = {
/^struct yysvf *yybgin = yysvec+1;$/d
/^int yylineno /,$d
