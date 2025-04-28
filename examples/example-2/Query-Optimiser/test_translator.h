#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include <bits/stdc++.h>
using namespace std;

#define WORD_ 101
#define AND_ 102
#define OR_ 103
#define STABLE_ 104
#define PTABLE_ 105
#define TABLE_ 106
#define DTABLE_ 107
#define UTABLE_ 108
#define JTABLE_ 109
#define ITABLE_ 110

class Node {
    public:
        int nodeType;
        string content;
        Node* left;
        Node* right;
        Node* attr; // Join only
        int pro;
        Node();
        Node(Node*);
};

Node *makeTermNode(string);
Node *makeAndNode(Node*, Node*);
Node *appendAndNode(Node*, Node*);
Node *makeTree(Node*);
Node *makeSelectTree(Node*, Node*);
Node *makeProjectTree(Node*, Node*);
Node *joinTree(Node*, Node*, Node*);
Node *diffTree(Node*, Node*, Node*);
Node *unionTree(Node*, Node*, Node*);
Node *intersectTree(Node*, Node*);
void printTable(Node*);
void revert(Node*);
Node *parse(Node*);
Node *get_and(Node*, Node*);
Node *get_table(Node*);
void get_str(string&, Node*);
Node *gen_copy(Node*);
Node *appendNode(Node*, Node*);

extern FILE *yyin;
extern int yyparse();

#endif