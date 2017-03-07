#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NF_MAX 64
#define DEP_MAX 204800

char Buf[DEP_MAX];

typedef struct {
    int num;
    int line_l;
    char *line[NF_MAX];
} RL;

typedef struct {
    int n;
    char *array[DEP_MAX];
} OL;

typedef struct {      // eg: -12AAAAAAAAAAAA
    int digit;        // digit = 12
    int diglen;       // diglen = 2 (the length of string "12")
} DL;                 // Digit length


void Usage ()
{
    char* Usage =
    "Usage: ./pilefilt <in.mpileupfile> <out.mpileupfile>\n" ;
    fprintf(stderr, "%s", Usage);
}


void linesplit ( char *bufline, RL *readline )
{
    int pword_l;
    char *pword = strtok(bufline, "\t");

    while ( pword ) {
        pword_l = strlen(pword);
        readline->line[readline->num] = (char *)malloc(pword_l + 4);
        strcpy(readline->line[readline->num++], pword); 
        readline->line_l += pword_l;
        pword = strtok(NULL, "\t\n");
    }
}

void destory ( RL *readline, char *out )
{
    for ( int i=0; i < readline->num; ++i ) {
        free(readline->line[i]);
    }
    memset(readline, 0, sizeof(RL)); free(out);
}

DL GetDigit ( const char *inseq )
{
    char digit[5] = {'\0'};
    char *pdigit = digit;
    DL seqdig;

    for ( ++inseq; isdigit(*inseq); ++inseq )
        *pdigit++ = *inseq;
    seqdig.digit = atoi(digit); seqdig.diglen = strlen(digit);
    return seqdig;
}


char *filtbase ( RL *pile )
{
    char *a, *a_s, *b, *o, *c, *d, *d_s;
    DL dig;
    int dep = atoi(pile->line[3]);
    b = pile->line[4];  c = pile->line[5];

    a = a_s = (char *)malloc((pile->line_l) * sizeof(char));
    d = d_s = (char *)malloc((pile->line_l) * sizeof(char));
    o = (char *)malloc((pile->line_l + NF_MAX)* sizeof(char));

    if ( dep == 0 ) {
        sprintf(o, "%s\t%s\t%s\t%d\n",\
                   pile->line[0], pile->line[1], pile->line[2], dep);
        free(a_s); free(d_s); a_s = d_s = NULL;
        return o;
    }

    for ( b; *b != '\0'; ++b ) {
        if ( *b == 46 || *b == 44 ) { *a++ = *b; *d++ = *c++; }         // [ . , ]
        else if ( *b == 94 ) { b += 2; --dep; c++; }                    // [ ^ ]
        else if ( *b == 36 ) { --a; --dep; --d; }                       // [ $ ]
        else if ( *b == 42 || *b == 78 || *b == 110 )  { --dep; c++; }  // [ * N n ]
        else if ( *b == 43 || *b == 45 ) {                              // [ - + ]
            dig = GetDigit(b); 
            strncpy(a, b, (dig.digit + dig.diglen + 1)); 
            a += (dig.digit + dig.diglen + 1); b += (dig.digit + dig.diglen); 
            *d++ = *c++;
        }
        else { *a++ = *b; *d++ = *c++; }
    } *a = *d = '\0';

    if (dep == 0) {
        sprintf(o, "%s\t%s\t%s\t%d\n", \
                   pile->line[0], pile->line[1], pile->line[2], dep);
    } else {
        sprintf(o, "%s\t%s\t%s\t%d\t%s\t%s\n", \
                   pile->line[0], pile->line[1], pile->line[2], dep, a_s, d_s);
    }
    free(a_s); free(d_s); a_s = d_s = NULL;
    return o;
}

void write ( char *name, OL *out )
{
    FILE *fp = fopen(name, "a");
    for ( int i =0; i < out->n; ++i ) {
        fputs(out->array[i], fp);
    }
    for ( int j=0; j < out->n; ++j ) {
        free(out->array[j]);
    }
    out->n = 0; fclose(fp);
}

int main ( int argc, char **argv )
{
    char *f_line;
    time_t start, end;
    RL readline; memset(&readline, 0, sizeof(RL));
    OL outarray; memset(&outarray, 0, sizeof(OL));
    FILE *fp = fopen( argv[1], "r"); 

    if ( argc != 3 ) {
        Usage(); exit(-1);
    }
    if ( !fp ) {
        fprintf( stderr, "[Err::%s::%d] Failed to open %s\n", \
                         __func__, __LINE__, argv[1] ); exit (-1);
    }
    setvbuf (fp, NULL, _IOFBF, BUFSIZ); time(&start);

    while ( fgets(Buf, DEP_MAX, fp) ) {
        linesplit( Buf, &readline );  f_line = filtbase(&readline); 

        if ( outarray.n == DEP_MAX )  
            write(argv[2], &outarray);
	    {
            outarray.array[outarray.n] = (char *) malloc((readline.line_l + NF_MAX) * sizeof(char));
            memcpy(outarray.array[outarray.n], f_line, (readline.line_l + NF_MAX)); ++outarray.n;
            destory (&readline, f_line);
        }
    } time(&end);
    write(argv[2], &outarray);
    printf ("Time Consum is: %.2lf (s)\n", difftime(end, start));
}
