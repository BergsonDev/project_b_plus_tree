/*
 * ===========================================================================
 *  TEMPLATE GTK 4 — CRUD VISUAL EM C PURO (PADRÃO MVC)
 *  Compilador: GCC estrito (sem g++, sem extensões C++)
 *  Padrão:     C11 (-std=c11)
 *  Dependência: GTK 4
 *
 */

#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

// definido nas dependencias para facilitar o controle global
#define MAX_PRODUCT 256
#define MAX_STR     256

typedef struct {
    int  id;
    char name[MAX_STR];
    char code[MAX_STR];
    char brandName[MAX_STR];
    int quantity; 
} Products;
//

#endif