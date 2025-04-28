#include "test_translator.h"

int main(int argc, char *argv[])
{
    yyin = fopen(argv[1], "r");
    yyparse();
    return 0;
}