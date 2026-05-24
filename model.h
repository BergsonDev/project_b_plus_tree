#ifndef MODEL_H
#define MODEL_H

#include "dependencies.h"

int allProducts(Products * data, int capacidade);
int insertProduct(const char *name, const char *code, const char * brandName, int quantity);
int editProduct(int id, const char *name, const char *code, const char *brandName, int quantity);
int removeProduct(int id);
Products * searchProduct(int id);

#endif