%{
    #include "test_translator.h"
    extern int yylex();
    void yyerror(char* s);
%}

%union 
{
    char* str;
    Node* node;
}

%token AND OR SELECT PROJECT JOIN UNION INTERSECT DIFF PROD LSQ RSQ LPAR RPAR NL
%token <str> WORD

%start result

%type<node>
    expression
    table
    term

%%

result :
        | result table
        { revert($2); }
        | result table NL
        { revert($2); cout << endl; }
        | result error NL
        {  }
;

table : term
        { $$ = makeTree($1); }
        | LPAR table RPAR
        { $$ = $2; }
        | SELECT LSQ expression RSQ LPAR table PROD table RPAR
        { $$ = joinTree($3, $6, $8); }
        | SELECT LSQ expression RSQ LPAR table JOIN LSQ expression RSQ table RPAR
        { $$ = joinTree(appendAndNode($3, $9), $6, $11); }
        | SELECT LSQ expression RSQ LPAR table DIFF table RPAR
        { $$ = diffTree($3, $6, $8); }
        | SELECT LSQ expression RSQ LPAR table RPAR
        { $$ = makeSelectTree($3, $6); }
        | PROJECT LSQ expression RSQ LPAR table RPAR
        { $$ = makeProjectTree($3, $6); }
        | PROJECT LSQ expression RSQ LPAR table UNION table RPAR
        { $$ = unionTree($3, $6, $8); }
        | table INTERSECT table
        { $$ = intersectTree($1, $3); }
        | table UNION table
        { $$ = unionTree(NULL, $1, $3); }
        | table DIFF table
        { $$ = diffTree(NULL, $1, $3); }
        | table JOIN LSQ expression RSQ table
        { $$ = joinTree($4, $1, $6); }
;

expression: term
            { $$ = $1; }
            | expression AND term
            { $$ = makeAndNode($1, $3); }
;

term:   WORD
        { $$ = makeTermNode($1); }
        | term OR WORD
        {
            string s = ($1)->content + " OR " + $3;
            $$ = makeTermNode(s);
        }
;

%%

/**
 * This function performs a specific task.
 * It takes in some parameters and returns a value.
 */

void yyerror(char* s)
{
    cout << "Error Detected : " << s << endl;
}