#include "test_translator.h"

// hi
//v rebwerb

Node::Node() : nodeType(-1), left(NULL), right(NULL), attr(NULL), pro(0) {}
Node::Node(Node* node) : nodeType(node->nodeType), content(node->content), left(node->left), right(node->right), attr(node->attr), pro(node->pro) {}

Node *makeTermNode(string s)
{
    Node *node = new Node();
    node->nodeType = WORD_;
    node->content = s;
    return node;
}

Node *makeAndNode(Node* left, Node *right)
{
    Node *node = new Node();
    node->nodeType = AND_;
    node->content = "AND";
    node->left = left;
    node->right = right;
    return node;
}

Node *appendAndNode(Node* left, Node *right)
{
    Node *node = new Node();
    node->nodeType = AND_;
    node->content = "AND";
    node->left = left;
    node->right = appendNode(node, right);
    if(right->nodeType != WORD_) return right;
    else return node;
}

Node *appendNode(Node* left, Node *right)
{
    if(right->nodeType == WORD_) return right;
    if(right->left->nodeType == WORD_)
    {
        Node *temp = right->left;
        right->left = left;
        return temp;
    }
    return appendNode(left, right->left);
}

void get_str(string& s, Node* node)
{
    if(node->nodeType == WORD_)
    {
        s += node->content;
        return;
    }
    get_str(s, node->left);
    s += " AND ";
    get_str(s, node->right);
}

Node *makeTree(Node* left)
{
    Node *node = new Node();
    node->nodeType = TABLE_;
    node->content = "TABLE";
    node->right = NULL;
    node->left = left;
    return node;
}

Node *makeSelectTree(Node* left, Node *right)
{
    if(right->nodeType == JTABLE_) return joinTree(appendAndNode(left, right->attr), right->left, right->right);
    Node *node = new Node();
    node->nodeType = STABLE_;
    node->content = "STABLE";
    node->right = right;
    node->left = left;
    return node;
}

Node *get_table(Node *tree)
{
    if(tree->nodeType != PTABLE_) return tree;
    return get_table(tree->right);
}

Node *makeProjectTree(Node* left, Node* right)
{
    if(right->nodeType == UTABLE_) return unionTree(left, right->left, right->right);
    Node *node = new Node();
    node->nodeType = PTABLE_;
    node->content = "PTABLE";
    node->left = left;
    node->right = get_table(right);
    return node;
}

Node *gen_copy(Node *node)
{
    Node *new_node = new Node(node);
    if(node->left != NULL) new_node->left = gen_copy(node->left);
    if(node->right != NULL) new_node->right = gen_copy(node->right);
    return new_node;
}

Node *joinTree(Node* attr, Node* table1, Node* table2)
{
    Node *node = new Node();
    node->nodeType = JTABLE_;
    node->content = "JTABLE";
    node->right = table2;
    node->left = table1;
    node->attr = attr;
    return node;
}

Node *diffTree(Node* attr, Node* table1, Node* table2)
{
    Node *node = new Node();
    node->nodeType = DTABLE_;
    node->content = "DTABLE";
    if(attr)
    {
        node->right = makeSelectTree(gen_copy(attr), table2);
        node->left = makeSelectTree(gen_copy(attr), table1);
    }
    else
    {
        node->right = table2;
        node->left = table1;
    }
    return node;
}

Node *unionTree(Node* attr, Node* table1, Node* table2)
{
    Node *node = new Node();
    node->nodeType = UTABLE_;
    node->content = "UTABLE";
    if(attr)
    {
        node->right = makeProjectTree(gen_copy(attr), table2);
        node->left = makeProjectTree(gen_copy(attr), table1);
    }
    else
    {
        node->right = table2;
        node->left = table1;
    }
    return node;
}

Node *intersectTree(Node* table1, Node* table2)
{
    Node *node = new Node();
    node->nodeType = ITABLE_;
    node->content = "ITABLE";
    node->right = table2;
    node->left = table1;
    return node;
}

void printTable(Node* node)
{
    if(node == NULL) return;
    switch(node->nodeType) {
    case WORD_:
        cout << node->content;
        return;
    default:
        cout << "[ " << node->content << " :: { ";
        printTable(node->left);
        cout << " : ";
        printTable(node->right);
        cout << " } ] ";
        return;
    }
}

Node *parse(Node* node)
{
    Node *res = new Node();
    Node *temp = get_and(node->left, res);
    *temp = *(node->right);
    return res;
}

Node *get_and(Node* node, Node* res)
{
    Node* temp = res;
    if(node->left != NULL) temp = get_and(node->left, temp);
    if(node->right != NULL) temp = get_and(node->right, temp);
    if(node->nodeType != WORD_) return temp;
    temp->nodeType = STABLE_;
    temp->content = "STABLE";
    temp->pro = 1;
    temp->left = new Node(node);
    temp->right = new Node();
    return temp->right;
}

void revert(Node* node)
{
    if(node->nodeType == TABLE_) revert(node->left);
    else if(node->nodeType == STABLE_)
    {
        if(node->pro == 0) node = parse(node);
        cout << "SELECT [ ";
        revert(node->left);
        cout << " ] (";
        revert(node->right);
        cout << ")";
    }
    else if(node->nodeType == PTABLE_)
    {
        cout << "PROJECT [ ";
        revert(node->left);
        cout << " ] (";
        revert(node->right);
        cout << ")";
    }
    else if(node->nodeType == DTABLE_)
    {
        revert(node->left);
        cout << " DIFF ";
        revert(node->right);
    }
    else if(node->nodeType == UTABLE_)
    {
        revert(node->left);
        cout << " UNION ";
        revert(node->right);
    }
    else if(node->nodeType == JTABLE_)
    {
        revert(node->left);
        cout << " JOIN [ ";
        string s;
        get_str(s, node->attr);
        cout << s << " ] ";
        revert(node->right);
    }
    else if(node->nodeType == ITABLE_)
    {
        revert(node->left);
        cout << " INTERSECT ";
        revert(node->right);
    }
    else cout << node->content;
}