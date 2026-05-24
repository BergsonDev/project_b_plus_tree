#include "model.h"

/*
* `table_products` : 
*   - Local de armazenamento simples em vetor para armazenar produtos.
*
*
*/
static Products table_products[MAX_PRODUCT]; 
/*
* `total_products` : 
*   - Local de armazenamento para quantidades totais para controle de produtos, pode ser substituído
*
*
*/
static int      total_products = 0;
/*
* `total_products` : 
*   - Geração por auto incremento de id, por exemplo, se o primeiro produto é inserido, então o algo que identifique aquele produto vai ser o número 1, e será incrementado para o próximo produto. Caso tenha outro produto, vai ser adicionado como id 2 e será incrementado para o próximo número, o 3.
*  
* `Exemplo` : 
* |Produto|Identificador
*
* |Detergente Ype|1
*
* |Sabonete Phebo|2
*
* |Talco Barla|3
*
* |Farinha |4
*
* |???????|5
*
*/
static int      next_id      = 1;


/* --- Funções do Model (stubs — implemente sua lógica aqui) --- */


/**
 * @brief Lista todos os produtos baseados na memória presente
 * 
 * 
 * @param data Ponteiro para `Products` para controlar as informações registradas, nele será preenchido todas as informações que são da model Products
 * lembrando que é gerado uma cópia de vetores, por isso é passado e retorna a quantidade total de models
 * 
 * @param capacity inteiro para controlar o máximo de registros/informações possíveis
 *
 * @return int retorna a quantidade total de informações no vetor para controlar o `for` durante a listagem de produtos
 * 
 * @note observe que a lógica presente é baseado em junção de vetores, vai ser mudado para o controle de árvore b+, pois esta função deve listar tudo de projetos
 */

 
int allProducts(Products * data, int capacity){
    int copy = (total_products < capacity) ? total_products : capacity;
    memcpy(data, table_products, (size_t)copy * sizeof(Products));
    return copy;
}

/**
 * @brief Insere os produtos no vetor !! Deve ser modificada a lógica
 * 
 * 
 * @param name Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 * 
 * @param code Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 * 
 * @param brandName Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 *
 * @param brandName inteiro para adicionar a quantidade de produtos existentes
 *
 * @return int retorna o id que está sendo presente no produto, deve ser averiguado para a estrutura de dados em árvore B+
 * 
 * @note observe que a lógica presente é baseado na inserção de vetores, deve ser modificada também por conta da estrutura de dados
 */

int insertProduct(const char *name, const char *code, const char * brandName, int quantity){
    if (total_products >= MAX_PRODUCT) return -1;

    Products *p = &table_products[total_products];
    p->id = next_id++;
    strncpy(p->name,  name,  MAX_STR - 1);
    strncpy(p->code, code, MAX_STR - 1);
    strncpy(p->brandName, brandName, MAX_STR - 1);
    p->name[MAX_STR - 1]  = '\0';  /* garante terminação nula */
    p->code[MAX_STR - 1] = '\0';
    p->brandName[MAX_STR - 1] = '\0';
    p->quantity = quantity;
    total_products++;
    return p->id;
}

/**
 * @brief Remove os produtos no vetor !! Deve ser modificada a lógica
 * 
 * 
 * @param id Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 * 
 * @return int se foi removido ou não baseado no boolean int
 * 
 * @note observe que a lógica presente é baseado na inserção de vetores, deve ser modificada também por conta da estrutura de dados
 */

int removeProduct(int id){
    for (int i = 0; i < total_products; i++) {
        if (table_products[i].id == id) {
            /* Desloca elementos à esquerda — sem malloc/free */
            memmove(&table_products[i], &table_products[i + 1],
                    (size_t)(total_products - i - 1) * sizeof(Products));
            total_products--;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Editar o produtos no vetor por ID !! Deve ser modificada a lógica
 * 
 * 
 * @param id Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 * 
 * @param name Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 * 
 * @param name Ponteiro para Char constante avisa que o conteúdo não vai ser modificado na função
 *
 * @return int se foi removido ou não baseado no boolean int
 * 
 * @note observe que a lógica presente é baseado na inserção de vetores, deve ser modificada também por conta da estrutura de dados
 */

int editProduct(int id, const char *name, const char *code, const char *brandName, int quantity)
{
    for (int i = 0; i < total_products; i++) {
        if (table_products[i].id == id) {
            strncpy(table_products->name,  name,  MAX_STR - 1);
            strncpy(table_products->code, code, MAX_STR - 1);
            strncpy(table_products->brandName, brandName, MAX_STR - 1);
            table_products->name[MAX_STR - 1]  = '\0';  /* garante terminação nula */
            table_products->code[MAX_STR - 1] = '\0';
            table_products->brandName[MAX_STR - 1] = '\0';
            table_products->quantity = quantity;
            return 1;
        }
    }
    return 0;
}


Products * searchProduct(int id){
    for (int i = 0; i < total_products; i++) {
        if (table_products[i].id == id) {
            return &table_products[i];
        }
    }
    return NULL;
}
