%{
    #include "test_translator.h"
    #include "y.tab.h"
%}

%%

"SELECT"                      { return SELECT;                             }
"PROJECT"                     { return PROJECT;                            }
"JOIN"                        { return JOIN;                               }
"UNION"                       { return UNION;                              }
"DIFF"                        { return DIFF;                               }
"INTERSECT"                   { return INTERSECT;                          }
"PROD"                        { return PROD;                               }
"AND"                         { return AND;                                }
"OR"                          { return OR;                                 }
[a-zA-Z0-9][a-zA-Z0-9\.]*     { yylval.str=strdup(yytext); return WORD;    }
"*"                           { yylval.str=strdup(yytext); return WORD;    }
"["                           { return LSQ;                                }
"]"                           { return RSQ;                                }
"("                           { return LPAR;                               }
")"                           { return RPAR;                               }
[\n]                          { return NL;                                 }
[ \t]                         {                                            }

%%

int yywrap()
{
    return 1;
}