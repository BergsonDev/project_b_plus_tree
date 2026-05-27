#include "model.h"

#define DATA_FILE  "products.txt"
#define INDEX_FILE "index.dat"

#define BTREE_ORDER 4

/*
=========================================================
ESTRUTURA DO ÍNDICE PRIMÁRIO
=========================================================
*/

typedef struct BTreeNode {

    int numKeys;
    int leaf;

    int keys[BTREE_ORDER - 1];
    long offsets[BTREE_ORDER - 1];

    struct BTreeNode *children[BTREE_ORDER];

} BTreeNode;

/*
=========================================================
VARIÁVEIS GLOBAIS
=========================================================
*/

static BTreeNode *root = NULL;
static int next_id = 1;

/*
=========================================================
CRIAR NÓ
=========================================================
*/

static BTreeNode *createNode(int leaf)
{
    BTreeNode *node = malloc(sizeof(BTreeNode));

    node->leaf = leaf;
    node->numKeys = 0;

    for (int i = 0; i < BTREE_ORDER; i++)
        node->children[i] = NULL;

    return node;
}

/*
=========================================================
BUSCA NA ÁRVORE-B
=========================================================
*/

static long searchBTree(BTreeNode *node, int key)
{
    if (node == NULL)
        return -1;

    int i = 0;

    while (i < node->numKeys && key > node->keys[i])
        i++;

    if (i < node->numKeys && key == node->keys[i])
        return node->offsets[i];

    if (node->leaf)
        return -1;

    return searchBTree(node->children[i], key);
}

/*
=========================================================
SPLIT DO NÓ
=========================================================
*/

static void splitChild(BTreeNode *parent,
                       int index,
                       BTreeNode *child)
{
    BTreeNode *newNode = createNode(child->leaf);

    int mid = (BTREE_ORDER - 1) / 2;

    newNode->numKeys = mid;

    for (int j = 0; j < mid; j++) {

        newNode->keys[j] = child->keys[j + mid + 1];
        newNode->offsets[j] = child->offsets[j + mid + 1];
    }

    if (!child->leaf) {

        for (int j = 0; j <= mid; j++)
            newNode->children[j] = child->children[j + mid + 1];
    }

    child->numKeys = mid;

    for (int j = parent->numKeys;
         j >= index + 1;
         j--)
    {
        parent->children[j + 1] = parent->children[j];
    }

    parent->children[index + 1] = newNode;

    for (int j = parent->numKeys - 1;
         j >= index;
         j--)
    {
        parent->keys[j + 1] = parent->keys[j];
        parent->offsets[j + 1] = parent->offsets[j];
    }

    parent->keys[index] = child->keys[mid];
    parent->offsets[index] = child->offsets[mid];

    parent->numKeys++;
}

/*
=========================================================
INSERÇÃO EM NÓ NÃO CHEIO
=========================================================
*/

static void insertNonFull(BTreeNode *node,
                          int key,
                          long offset)
{
    int i = node->numKeys - 1;

    if (node->leaf) {

        while (i >= 0 && key < node->keys[i]) {

            node->keys[i + 1] = node->keys[i];
            node->offsets[i + 1] = node->offsets[i];

            i--;
        }

        node->keys[i + 1] = key;
        node->offsets[i + 1] = offset;

        node->numKeys++;
    }
    else {

        while (i >= 0 && key < node->keys[i])
            i--;

        i++;

        if (node->children[i]->numKeys == BTREE_ORDER - 1) {

            splitChild(node,
                       i,
                       node->children[i]);

            if (key > node->keys[i])
                i++;
        }

        insertNonFull(node->children[i],
                      key,
                      offset);
    }
}

/*
=========================================================
INSERÇÃO NA ÁRVORE-B
=========================================================
*/

static void insertBTree(int key,
                        long offset)
{
    if (root == NULL) {

        root = createNode(1);

        root->keys[0] = key;
        root->offsets[0] = offset;

        root->numKeys = 1;

        return;
    }

    if (root->numKeys == BTREE_ORDER - 1) {

        BTreeNode *newRoot = createNode(0);

        newRoot->children[0] = root;

        splitChild(newRoot,
                   0,
                   root);

        int i = 0;

        if (key > newRoot->keys[0])
            i++;

        insertNonFull(newRoot->children[i],
                      key,
                      offset);

        root = newRoot;
    }
    else {

        insertNonFull(root,
                      key,
                      offset);
    }
}

/*
=========================================================
PERSISTÊNCIA DA ÁRVORE
=========================================================
*/

static void saveNode(FILE *fp, BTreeNode *node)
{
    if (node == NULL)
        return;

    fwrite(node,
           sizeof(BTreeNode),
           1,
           fp);

    if (!node->leaf) {

        for (int i = 0; i <= node->numKeys; i++)
            saveNode(fp, node->children[i]);
    }
}

static void saveTree()
{
    FILE *fp = fopen(INDEX_FILE, "wb");

    if (fp == NULL)
        return;

    fwrite(&next_id,
           sizeof(int),
           1,
           fp);

    saveNode(fp, root);

    fclose(fp);
}

static BTreeNode *loadNode(FILE *fp)
{
    BTreeNode temp;

    if (fread(&temp,
              sizeof(BTreeNode),
              1,
              fp) != 1)
    {
        return NULL;
    }

    BTreeNode *node = createNode(temp.leaf);

    node->numKeys = temp.numKeys;

    for (int i = 0; i < BTREE_ORDER - 1; i++) {

        node->keys[i] = temp.keys[i];
        node->offsets[i] = temp.offsets[i];
    }

    if (!node->leaf) {

        for (int i = 0; i <= node->numKeys; i++)
            node->children[i] = loadNode(fp);
    }

    return node;
}

static void loadTree()
{
    FILE *fp = fopen(INDEX_FILE, "rb");

    if (fp == NULL)
        return;

    fread(&next_id,
          sizeof(int),
          1,
          fp);

    root = loadNode(fp);

    fclose(fp);
}

/*
=========================================================
INICIALIZAÇÃO
=========================================================
*/

static void initializeSystem()
{
    static int initialized = 0;

    if (!initialized) {

        loadTree();

        initialized = 1;
    }
}

/*
=========================================================
INSERIR PRODUTO
=========================================================
*/

int insertProduct(const char *name,
                  const char *code,
                  const char *brandName,
                  int quantity)
{
    initializeSystem();

    FILE *fp = fopen(DATA_FILE, "a+");

    if (fp == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);

    long offset = ftell(fp);

    int id = next_id++;

    fprintf(fp,
            "%d|%s|%s|%s|%d\n",
            id,
            name,
            code,
            brandName,
            quantity);

    fclose(fp);

    insertBTree(id, offset);

    saveTree();

    return id;
}

/*
=========================================================
BUSCAR PRODUTO
=========================================================
*/

Products * searchProduct(int id)
{
    initializeSystem();

    static Products p;

    long offset = searchBTree(root, id);

    if (offset == -1)
        return NULL;

    FILE *fp = fopen(DATA_FILE, "r");

    if (fp == NULL)
        return NULL;

    fseek(fp, offset, SEEK_SET);

    char line[512];

    fgets(line, sizeof(line), fp);

    fclose(fp);

    if (line[0] == '*')
        return NULL;

    char *token;

    token = strtok(line, "|");
    p.id = atoi(token);

    token = strtok(NULL, "|");
    strcpy(p.name, token);

    token = strtok(NULL, "|");
    strcpy(p.code, token);

    token = strtok(NULL, "|");
    strcpy(p.brandName, token);

    token = strtok(NULL, "|");
    p.quantity = atoi(token);

    return &p;
}

/*
=========================================================
TRAVESSIA ORDENADA
=========================================================
*/

static void traverseBTree(BTreeNode *node,
                          Products *data,
                          int *count,
                          int capacity)
{
    if (node == NULL)
        return;

    for (int i = 0; i < node->numKeys; i++) {

        if (!node->leaf)
            traverseBTree(node->children[i],
                          data,
                          count,
                          capacity);

        if (*count >= capacity)
            return;

        FILE *fp = fopen(DATA_FILE, "r");

        if (fp == NULL)
            return;

        fseek(fp, node->offsets[i], SEEK_SET);

        char line[512];

        fgets(line, sizeof(line), fp);

        fclose(fp);

        if (line[0] == '*')
            continue;

        Products p;

        char *token;

        token = strtok(line, "|");
        p.id = atoi(token);

        token = strtok(NULL, "|");
        strcpy(p.name, token);

        token = strtok(NULL, "|");
        strcpy(p.code, token);

        token = strtok(NULL, "|");
        strcpy(p.brandName, token);

        token = strtok(NULL, "|");
        p.quantity = atoi(token);

        data[*count] = p;

        (*count)++;
    }

    if (!node->leaf)
        traverseBTree(node->children[node->numKeys],
                      data,
                      count,
                      capacity);
}

/*
=========================================================
LISTAR TODOS
=========================================================
*/

int allProducts(Products *data,
                int capacidade)
{
    initializeSystem();

    int total = 0;

    traverseBTree(root,
                  data,
                  &total,
                  capacidade);

    return total;
}

/*
=========================================================
REMOVER PRODUTO
=========================================================
*/

int removeProduct(int id)
{
    initializeSystem();

    long offset = searchBTree(root, id);

    if (offset == -1)
        return 0;

    FILE *fp = fopen(DATA_FILE, "r+");

    if (fp == NULL)
        return 0;

    fseek(fp, offset, SEEK_SET);

    fputc('*', fp);

    fclose(fp);

    return 1;
}

/*
=========================================================
EDITAR PRODUTO
=========================================================
*/

int editProduct(int id,
                const char *name,
                const char *code,
                const char *brandName,
                int quantity)
{
    initializeSystem();

    long oldOffset = searchBTree(root, id);

    if (oldOffset == -1)
        return 0;

    FILE *fp = fopen(DATA_FILE, "r+");

    if (fp == NULL)
        return 0;

    fseek(fp, oldOffset, SEEK_SET);

    fputc('*', fp);

    fclose(fp);

    fp = fopen(DATA_FILE, "a+");

    if (fp == NULL)
        return 0;

    fseek(fp, 0, SEEK_END);

    long newOffset = ftell(fp);

    fprintf(fp,
            "%d|%s|%s|%s|%d\n",
            id,
            name,
            code,
            brandName,
            quantity);

    fclose(fp);

    updateOffset(root, id, newOffset);

    saveTree();

    return 1;
}

static int updateOffset(BTreeNode *node,
                        int key,
                        long newOffset)
{
    if (node == NULL)
        return 0;

    int i = 0;

    while (i < node->numKeys &&
           key > node->keys[i])
    {
        i++;
    }

    if (i < node->numKeys &&
        key == node->keys[i])
    {
        node->offsets[i] = newOffset;
        return 1;
    }

    if (node->leaf)
        return 0;

    return updateOffset(node->children[i],
                        key,
                        newOffset);
}