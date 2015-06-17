#include<stdio.h>
#include<stdlib.h>
#include "avl.h"

// An AVL tree node
struct node
{
    char *key; int length;
    struct node *left;
    struct node *right;
    int height;
};

#ifndef dmax
#define dmax(x, y)	((x>y)?x:y)
#endif

// A utility function to get height of the tree
int height(struct node *N)
{
    if (N == NULL)
        return 0;
    return N->height;
}

/* Helper function that allocates a new node with the given key and
    NULL left and right pointers. */
struct node* newNode(char *key, int length)
{
    struct node* node = (struct node*)
                        malloc(sizeof(struct node));
    node->key   = key;
    node->length = length;
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;  // new node is initially added at leaf
    return(node);
}
 
// A utility function to right rotate subtree rooted with y
// See the diagram given above.
struct node *rightRotate(struct node *y)
{
    struct node *x = y->left;
    if (x== NULL)
    	return y;
    struct node *T2 = x->right;
 
    // Perform rotation
    x->right = y;
    y->left = T2;
 
    // Update heights
    y->height = dmax(height(y->left), height(y->right))+1;
    x->height = dmax(height(x->left), height(x->right))+1;
 
    // Return new root
    return x;
}
 
// A utility function to left rotate subtree rooted with x
// See the diagram given above.
struct node *leftRotate(struct node *x)
{
    struct node *y = x->right;
    if (y==NULL)
    	return x;
    struct node *T2 = y->left;
 
    // Perform rotation
    y->left = x;
    x->right = T2;
 
    //  Update heights
    x->height = dmax(height(x->left), height(x->right))+1;
    y->height = dmax(height(y->left), height(y->right))+1;
 
    // Return new root
    return y;
}
 
// Get Balance factor of node N
int getBalance(struct node *N)
{
    if (N == NULL)
        return 0;
    return height(N->left) - height(N->right);
}
 
struct node* insert(struct node* node, char *key, int length)
{
    /* 1.  Perform the normal BST rotation */
    if (node == NULL)
        return(newNode(key, length));
 
    if (key < node->key)
        node->left  = insert(node->left, key, length);
    else
        node->right = insert(node->right, key, length);
 
    /* 2. Update height of this ancestor node */
    node->height = dmax(height(node->left), height(node->right)) + 1;
 
    /* 3. Get the balance factor of this ancestor node to check whether
       this node became unbalanced */
    int balance = getBalance(node);
 
    // If this node becomes unbalanced, then there are 4 cases
 
    // Left Left Case
    if (balance > 1 && key < node->left->key)
        return rightRotate(node);
 
    // Right Right Case
    if (balance < -1 && key > node->right->key)
        return leftRotate(node);
 
    // Left Right Case
    if (balance > 1 && key > node->left->key)
    {
        node->left =  leftRotate(node->left);
        return rightRotate(node);
    }
 
    // Right Left Case
    if (balance < -1 && key < node->right->key)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
 
    /* return the (unchanged) node pointer */
    return node;
}
 
// A utility function to print preorder traversal of the tree.
// The function also prints height of every node
void preOrder(struct node *root)
{
	printf("PREORDER:");
    if(root != NULL)
    {
        printf("%p ", root->key);
        preOrder(root->left);
        preOrder(root->right);
    }
}

struct node *find(struct node *root, char *ptr)
{
	if (root == NULL)
		return NULL;
	//printf("%p\n", ptr);
	if ((ptr == root->key) ||
		((ptr >= root->key) && (ptr < root->key + root->length)))
		return root;
	if (ptr < root->key)
		return find(root->left, ptr);
	return find(root->right, ptr);
}

struct node *root = NULL;
void putBasePtr(void *ptr, int size)
{
	root = insert(root, (char*)ptr, size);
};

//  Base pointer for objects
void *getBasePtr(void *ptr)
{
	struct node *n = find(root, (char*)ptr);
	if (n == NULL)
	{
		//preOrder(root);
		return ptr;
	}
	return n->key;
};

void adjBasePtrSize(void *base, int newsize)
{
	struct node *n = find(root, (char*)base);
	n->length = newsize;
};
