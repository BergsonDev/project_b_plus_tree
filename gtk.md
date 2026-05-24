# 📘 Guia de Engenharia e Arquitetura GTK 4 para C Puro
### Foco em Sistemas Operacionais, Padrão MVC e Transição Web-to-Desktop

Este documento serve como um guia técnico avançado para engenheiros de sistemas que possuem sólidos conhecimentos em **C puro** (ponteiros, gerenciamento de memória, estruturas de dados) e experiência no ecossistema **Web (HTML/CSS/JS)**, mas estão ingressando no desenvolvimento de interfaces gráficas nativas no ecossistema de Sistemas Operacionais através do **GTK 4**.

---

## 1. Mapeamento Mental: Paradigma Web vs. Arquitetura GTK 4

A transição da Web para o desenvolvimento desktop nativo com GTK 4 torna-se trivial quando mapeamos os conceitos de gerenciamento de estado, renderização e árvore de componentes:

| Conceito Web (HTML/CSS/JS) | Equivalente no GTK 4 | Descrição Técnica no Nível de SO |
| :--- | :--- | :--- |
| **Tags HTML** (`<div>`, `<input>`) | **Widgets** (`GtkBox`, `GtkEntry`) | Estruturas (`structs`) que representam regiões de desenho na tela. |
| **DOM** (Árvore de Elementos) | **Widget Tree / Hierarchy** | Organização hierárquica onde containers possuem ponteiros para os filhos. |
| **Event Listeners** (`onclick`) | **Signals & Callbacks** | Sistema de interrupção via software baseado no barramento `GSignal`. |
| **Event Object** (`e.target`) | **`gpointer user_data`** | Ponteiro genérico (`void*`) para passar contextos de memória arbitrários. |
| **Estilização CSS** | **GtkCssProvider** | Mecanismo de parsing de CSS para customização dos nós de renderização. |

---

## 2. A Engenharia Interna do GTK: GObject e C Puro

O GTK 4 é escrito estritamente em **C puro**, mas simula Orientação a Objetos através de uma biblioteca base chamada **GObject**. 

### Herança via Inclusão de Estruturas
Como o C não possui a palavra-chave `extends`, o GObject simula herança colocando a `struct` pai como o **primeiro membro** da `struct` filho.

```c
// Representação conceitual interna do GTK
struct _GtkWidget {
    GObject parent_instance;
    // Dados privados do widget básico
};

struct _GtkButton {
    GtkWidget parent_instance; // Permite o "cast" seguro de ponteiros
    // Dados privados do botão
};

```

Graças a essa disposição na memória, um ponteiro para `GtkButton` aponta exatamente para o mesmo endereço de memória onde começa sua representação como `GtkWidget`.

### Macros de Conversão de Tipo (Casts)

Para garantir a tipagem estrita do C sem disparar avisos do compilador (`gcc`), o GTK 4 utiliza macros em maiúsculo. Elas fazem o cast e, em modo de depuração, checam se o ponteiro na memória é realmente válido:

```c
GtkWidget *botao = gtk_button_new_with_label("Salvar");

// Maneira correta de passar o botão para uma função que espera uma janela ou widget genérico
gtk_widget_set_sensitive(botao, FALSE); 
// Se uma função exigir um tipo específico (ex: GtkButton):
// gtk_button_set_label(GTK_BUTTON(botao), "Enviar");

```

---

## 3. Gerenciamento de Memória no GTK 4 (Reference Counting)

Diferente do C tradicional onde cada `malloc()` exige um `free()`, o GTK 4 implementa um modelo híbrido baseado em **Contagem de Referências** (Reference Counting) e **Propriedade Hierárquica**.

1. **Floating Reference (Referência Flutuante):** Quando você cria um widget (ex: `GtkWidget *input = gtk_entry_new();`), o GTK o aloca na Heap com uma referência inicial "flutuante".
2. **Transferência de Posse (Ownership):** Ao anexar esse widget a um container (ex: `gtk_box_append(GTK_BOX(box), input);`), o container consome a referência flutuante e assume a custódia da memória daquele ponteiro.
3. **Desalocação em Cascata:** Quando a janela principal (`GtkApplicationWindow`) for destruída pelo Sistema Operacional ou pelo usuário, ela percorrerá sua árvore de ponteiros chamando `g_object_unref()` recursivamente. **Você não deve dar `free()` em widgets que foram anexados à tela.**

---

## 4. O Padrão MVC Desacoplado no GTK 4

Para garantir que a lógica de controle (Input/Output, persistência, regras de negócio) não se misture com a interface visual (Widgets, Janelas), o GTK utiliza o parâmetro final de conexão de sinais: o `gpointer user_data`.

Ele funciona exatamente como um `void*`. Nós encapsulamos todo o contexto do nosso programa (Ponteiros da interface + Ponteiros das funções de dados) em uma estrutura customizada e a injetamos nos gatilhos de clique.

---

## 5. Molde Prático: Arquitetura de um CRUD Desacoplado

Abaixo está uma estrutura modular completa. Para manter a fidelidade ao C puro e facilitar a compilação direta proposta, o código está dividido logicamente em seções que podem ficar em arquivos separados (`model.h`, `view.c`, `main.c`) ou unificadas.

### 5.1. O Model (Lógica de Controle e Dados Puro)

*Nesta camada, não há nenhuma menção a bibliotecas gráficas ou GTK. É C puro para Sistemas Operacionais.*

```c
// ============================================================================
// CAMADA: MODEL (Lógica de Dados Pura - Sem dependências visuais)
// ============================================================================
#ifndef MODEL_H
#define MODEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 100

typedef struct {
    int id;
    char nome[MAX_STR];
    char detalhe[MAX_STR];
} ItemData;

// Funções de controle que você implementará após desenhar a GUI
void model_inserir_dados(int id, const char* nome, const char* detalhe) {
    printf("[MODEL IO] Inserindo no Banco/Arquivo -> ID: %d | Nome: %s\n", id, nome);
    // Sua lógica de controle de memória, escrita em disco ou IPC do SO entra aqui.
}

void model_deletar_dados(int id) {
    printf("[MODEL IO] Removendo registro físico -> ID: %d\n", id);
}

void model_editar_dados(int id, const char* novo_nome, const char* novo_detalhe) {
    printf("[MODEL IO] Editando registro -> ID: %d | Novo Nome: %s\n", id, nuevo_nome);
}

#endif

```

### 5.2. O Controller e a View (Camada de Interface GTK 4)

```c
// ============================================================================
// CAMADA: VIEW & CONTROLLER (GTK 4 Interface)
// ============================================================================
#include <gtk/gtk.h>
#include "model.h" // Conexão com o contrato de dados

// Estrutura de Contexto (Une a View ao Controller sem acoplamento rígido)
typedef struct {
    GtkWidget *entry_id;
    GtkWidget *entry_nome;
    GtkWidget *entry_detalhe;
    GtkWidget *list_box;     // Equivale a uma <ul> ou tabela de listagem
} AppContext;

// --- CALLBACKS (CONTROLLER) ---

// Gatilho: Inserir (Equivale ao submit de um formulário Web)
static void on_btn_inserir_clicked(GtkButton *btn, gpointer user_data) {
    AppContext *ctx = (AppContext *)user_data;

    // Extração das strings dos inputs (GtkEntry) -> Equivalente ao .value do JS
    const char *id_str = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_id));
    const char *nome = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_nome));
    const char *detalhe = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_detalhe));

    int id = atoi(id_str);

    // Injeção direta na lógica de controle (Desacoplada)
    model_inserir_dados(id, nome, detalhe);

    // Atualização Visual na Lista (View) -> Criando um novo item de lista dinamicamente
    char label_formatada[MAX_STR * 2];
    snprintf(label_formatada, sizeof(label_formatada), "ID: %d - %s (%s)", id, nome, detalhe);
    
    GtkWidget *novo_item = gtk_label_new(label_formatada);
    // Definindo o alinhamento para a esquerda (0.0 = esquerda, 0.5 = centro)
    gtk_label_set_xalign(GTK_LABEL(novo_item), 0.0); 
    
    gtk_list_box_append(GTK_LIST_BOX(ctx->list_box), novo_item);
}

// Gatilho: Remover Item Selecionado
static void on_btn_remover_clicked(GtkButton *btn, gpointer user_data) {
    AppContext *ctx = (AppContext *)user_data;
    
    // Obtém a linha selecionada na lista gráfica
    GtkListBoxRow *linha_selecionada = gtk_list_box_get_selected_row(GTK_LIST_BOX(ctx->list_box));
    
    if (linha_selecionada != NULL) {
        printf("[CONTROLLER] Item selecionado identificado na UI. Disparando remoção lógica.\n");
        
        // Chamar a função lógica passando o ID (Aqui simplificado, chamando exclusão simbólica)
        model_deletar_dados(-1); 

        // Remove o componente visual do DOM do GTK
        gtk_list_box_remove(GTK_LIST_BOX(ctx->list_box), GTK_WIDGET(linha_selecionada));
    } else {
        g_print("Nenhum item selecionado na lista para remoção.\n");
    }
}

// Gatilho: Editar Registro
static void on_btn_editar_clicked(GtkButton *btn, gpointer user_data) {
    AppContext *ctx = (AppContext *)user_data;
    
    const char *id_str = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_id));
    const char *nome = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_nome));
    const char *detalhe = gtk_editable_get_text(GTK_EDITABLE(ctx->entry_detalhe));

    // Delegação lógica do controle
    model_editar_dados(atoi(id_str), nome, detalhe);
}

// --- CONSTRUTOR DA INTERFACE (VIEW) ---

static void on_app_activate(GtkApplication *app, gpointer user_data) {
    // Alocação do contexto técnico para tráfego de ponteiros
    AppContext *ctx = g_new0(AppContext, 1);

    // 1. Definição da Janela Principal (Tag <body>)
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Sistema OS - Módulo CRUD Nativo");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 450);

    // 2. Layout Principal Flexível Vertical (Equivale a uma div com flex-direction: column)
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_all(main_box, 15);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // ==========================================
    // ÁREA DO FORMULÁRIO (Inputs)
    // ==========================================
    GtkWidget *form_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    ctx->entry_id = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_id), "ID (Int)");
    
    ctx->entry_nome = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_nome), "Nome do Item");
    
    ctx->entry_detalhe = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_detalhe), "Metadados / Detalhes");

    gtk_box_append(GTK_BOX(form_box), ctx->entry_id);
    gtk_box_append(GTK_BOX(form_box), ctx->entry_nome);
    gtk_box_append(GTK_BOX(form_box), ctx->entry_detalhe);
    gtk_box_append(GTK_BOX(main_box), form_box);

    // ==========================================
    // ÁREA DE BOTÕES DE AÇÃO
    // ==========================================
    GtkWidget *actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    GtkWidget *btn_inserir = gtk_button_new_with_label("Inserir Registro");
    GtkWidget *btn_editar  = gtk_button_new_with_label("Salvar Edição");
    GtkWidget *btn_remover = gtk_button_new_with_label("Remover Selecionado");

    gtk_box_append(GTK_BOX(actions_box), btn_inserir);
    gtk_box_append(GTK_BOX(actions_box), btn_editar);
    gtk_box_append(GTK_BOX(actions_box), btn_remover);
    gtk_box_append(GTK_BOX(main_box), actions_box);

    // ==========================================
    // ÁREA DE LISTAGEM (O Visualizador do CRUD)
    // ==========================================
    GtkWidget *label_lista = gtk_label_new("Registros Ativos:");
    gtk_label_set_xalign(GTK_LABEL(label_lista), 0.0);
    gtk_box_append(GTK_BOX(main_box), label_lista);

    // Componente de rolagem caso a listagem exceda o tamanho da janela
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_window, TRUE); // Ocupa o espaço vertical disponível
    
    ctx->list_box = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), ctx->list_box);
    gtk_box_append(GTK_BOX(main_box), scrolled_window);

    // ==========================================
    // ASSINATURA DE EVENTOS (Signals / Event Listeners)
    // ==========================================
    g_signal_connect(btn_inserir, "clicked", G_CALLBACK(on_btn_inserir_clicked), ctx);
    g_signal_connect(btn_editar,  "clicked", G_CALLBACK(on_btn_editar_clicked), ctx);
    g_signal_connect(btn_remover, "clicked", G_CALLBACK(on_btn_remover_clicked), ctx);

    // Liberar o contexto da Heap de forma limpa quando a janela morrer
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_free), ctx);

    gtk_window_present(GTK_WINDOW(window));
}

// ============================================================================
// PONTO DE ENTRADA DO SISTEMA OPERACIONAL
// ============================================================================
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("br.uf.crudsystem.purec", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    
    // Inicia o loop infinito de processamento de eventos do SO
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

```

---

## 6. Pipeline de Compilação Universal Única com GCC

Para garantir tempo zero de setup e evitar a perda de foco no compilador, utilizamos a ferramenta utilitária `pkg-config` injetada de forma nativa no comando do **`gcc`**. Ele resolve automaticamente as flags de inclusão de cabeçalhos (`-I`) e de vinculação das bibliotecas estáticas/dinâmicas (`-l`).

### Comando Único (Linux Terminal / Windows MSYS2 UCRT64)

Abra o terminal apropriado do seu sistema operacional e execute:

```bash
gcc main.c -o modulo_crud `pkg-config --cflags --libs gtk4`

```

### Onde obter o compilador `gcc` e `gtk4` de acordo com o SO:

* **Windows 11:** Baixe o **MSYS2**. Abra o terminal **MSYS2 UCRT64** e instale a toolchain com:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gtk4 mingw-w64-ucrt-x86_64-pkg-config

```


* **Manjaro / Arch Linux:**
```bash
sudo pacman -S base-devel gtk4 pkgconf

```


* **Fedora Linux:**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install gtk4-devel pkgconf-pkg-config

```


* **Debian / Ubuntu Linux:**
```bash
sudo apt update
sudo apt install build-essential libgtk-4-dev pkg-config

```



---

## 7. Próximos Passos para o Desenvolvimento do Projeto

Com a interface rodando e as funções `model_*` isoladas:

1. Você pode alterar a função `model_inserir_dados` em C para salvar as structs em uma tabela hash ou árvore AVL alocada dinamicamente.
2. É possível acoplar persistência por arquivos de blocos (`fwrite`/`fread`) simulando sistemas de arquivos de S.O.
3. A interface continuará funcionando de forma idêntica e sem travamentos, pois os callbacks agem de forma puramente síncrona com o disparo da engine gráfica.
