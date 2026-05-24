/*
 * ===========================================================================
 *  TEMPLATE GTK 4 — CRUD VISUAL EM C PURO (PADRÃO MVC)
 *  Compilador: GCC estrito (sem g++, sem extensões C++)
 *  Padrão:     C11 (-std=c11)
 *  Dependência: GTK 4
 *
 *  ANALOGIA WEB RÁPIDA:
 *  ┌─────────────────────────────────────────────────────┐
 *  │  GTK / C               │  Web (HTML/CSS/JS)         │
 *  ├─────────────────────────────────────────────────────│
 *  │  GtkWidget*            │  Elemento DOM (HTMLElement) │
 *  │  GtkApplication        │  O próprio navegador/runtime│
 *  │  GtkWindow             │  <body> / janela do browser │
 *  │  GtkBox                │  <div style="display:flex"> │
 *  │  GtkButton             │  <button>                   │
 *  │  GtkEntry              │  <input type="text">        │
 *  │  GtkLabel              │  <span> / <label>           │
 *  │  GtkListBox            │  <ul> / <table>             │
 *  │  GtkListBoxRow         │  <li> / <tr>                │
 *  │  GtkScrolledWindow     │  overflow: scroll           │
 *  │  g_signal_connect()    │  addEventListener()         │
 *  │  activate (sinal)      │  DOMContentLoaded           │
 *  │  clicked (sinal)       │  click event                │
 *  └─────────────────────────────────────────────────────┘
 *
 *  CICLO DE VIDA DOS WIDGETS (análogo ao DOM):
 *  - gtk_application_window_new() → document.createElement() + appendChild()
 *  - gtk_widget_set_visible(FALSE) → elemento.style.display = 'none'
 *  - g_object_unref() → elemento.remove() / GC do JS (mas EXPLÍCITO em C)
 *
 *  COMPILAÇÃO (Linux e MSYS2/MinGW no Windows 11):
 *  gcc $(pkg-config --cflags gtk4) -o crud_gtk4 crud_gtk4.c $(pkg-config --libs gtk4) -std=c11 -Wall -Wextra
 * ===========================================================================
 */

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

/* ===========================================================================
 *  SEÇÃO 1 — MODEL
 *  Representa os dados da aplicação. Substitua por sua struct real e funções
 *  de persistência (arquivo, SQLite, rede, etc.).
 *  Analogia Web: é o "estado" (state) da aplicação React/Vue, ou o objeto
 *  JSON que viria de uma API REST.
 * ===========================================================================*/

#define MAX_REGISTROS 256
#define MAX_CAMPO     128

typedef struct {
    int  id;
    char nome[MAX_CAMPO];
    char email[MAX_CAMPO];
} Registro;

/* Armazenamento em memória — substitua por chamadas ao seu backend */
static Registro tabela[MAX_REGISTROS];
static int      total_registros = 0;
static int      proximo_id      = 1;

/* --- Funções do Model (stubs — implemente sua lógica aqui) --- */

/*
 * model_listar_todos:
 *   Preenche 'saida' com os registros existentes e retorna a quantidade.
 *   PONTEIROS: 'saida' aponta para memória já alocada pelo chamador.
 *              Nunca faça free() aqui; quem aloca, desaloca.
 */
static int model_listar_todos(Registro *saida, int capacidade)
{
    int copiados = (total_registros < capacidade) ? total_registros : capacidade;
    memcpy(saida, tabela, (size_t)copiados * sizeof(Registro));
    return copiados;
}

/*
 * model_inserir:
 *   Adiciona um novo registro. Retorna o ID gerado ou -1 em caso de erro.
 *   PONTEIROS: nome e email são strings literais ou buffers do chamador;
 *              copiamos com strncpy para evitar dependência do ponteiro externo.
 */
static int model_inserir(const char *nome, const char *email)
{
    if (total_registros >= MAX_REGISTROS) return -1;

    Registro *r = &tabela[total_registros];
    r->id = proximo_id++;
    strncpy(r->nome,  nome,  MAX_CAMPO - 1);
    strncpy(r->email, email, MAX_CAMPO - 1);
    r->nome[MAX_CAMPO - 1]  = '\0';  /* garante terminação nula */
    r->email[MAX_CAMPO - 1] = '\0';
    total_registros++;
    return r->id;
}

/*
 * model_remover:
 *   Remove pelo ID. Compacta o array (sem buracos).
 *   Retorna 1 se removeu, 0 se não encontrou.
 */
static int model_remover(int id)
{
    for (int i = 0; i < total_registros; i++) {
        if (tabela[i].id == id) {
            /* Desloca elementos à esquerda — sem malloc/free */
            memmove(&tabela[i], &tabela[i + 1],
                    (size_t)(total_registros - i - 1) * sizeof(Registro));
            total_registros--;
            return 1;
        }
    }
    return 0;
}

/*
 * model_editar:
 *   Atualiza nome e email de um registro existente pelo ID.
 *   Retorna 1 se editou, 0 se não encontrou.
 */
static int model_editar(int id, const char *novo_nome, const char *novo_email)
{
    for (int i = 0; i < total_registros; i++) {
        if (tabela[i].id == id) {
            strncpy(tabela[i].nome,  novo_nome,  MAX_CAMPO - 1);
            strncpy(tabela[i].email, novo_email, MAX_CAMPO - 1);
            tabela[i].nome[MAX_CAMPO - 1]  = '\0';
            tabela[i].email[MAX_CAMPO - 1] = '\0';
            return 1;
        }
    }
    return 0;
}


/* ===========================================================================
 *  SEÇÃO 2 — VIEW STATE
 *  Agrupa todos os ponteiros de widgets que precisam ser acessados entre
 *  callbacks. Análogo ao "ref" do React ou ao objeto de estado de um
 *  componente JS.
 *
 *  GERENCIAMENTO DE MEMÓRIA:
 *  Todos esses ponteiros são propriedade do GTK. Você NÃO faz free() deles
 *  manualmente — o GTK destrói os widgets filhos quando o pai é destruído.
 *  A regra de ouro: só chame g_object_unref() em objetos que VOCÊ criou
 *  fora da hierarquia de widgets (ex.: GListStore, GtkStringList).
 * ===========================================================================*/

typedef struct {
    /* Janela principal */
    GtkWidget *janela;

    /* Lista (análogo a uma <table> ou <ul>) */
    GtkWidget *lista_box;        /* GtkListBox — o container */

    /* Campos de formulário (análogo a <input type="text">) */
    GtkWidget *entry_nome;
    GtkWidget *entry_email;

    /* Botões de ação (análogo a <button>) */
    GtkWidget *btn_inserir;
    GtkWidget *btn_editar;
    GtkWidget *btn_remover;

    /* Rótulo de status (análogo a um <p id="status">) */
    GtkWidget *label_status;

    /* ID do registro selecionado (-1 = nenhum selecionado) */
    int id_selecionado;
} AppWidgets;


/* ===========================================================================
 *  SEÇÃO 3 — CONTROLLER (Funções Auxiliares da View)
 *  Mantém a View sincronizada com o Model.
 * ===========================================================================*/

/*
 * view_atualizar_lista:
 *   Recria todas as linhas do GtkListBox a partir do Model.
 *
 *   Analogia Web: é o equivalente a limpar um <ul> com innerHTML = ''
 *   e depois fazer um forEach para criar novos <li> com createElement().
 *
 *   PONTEIROS: gtk_widget_get_parent() retorna um ponteiro sem transferência
 *   de propriedade (não faça unref). gtk_list_box_remove_all() é GTK 4.8+;
 *   para versões anteriores usamos o loop abaixo.
 */
static void view_atualizar_lista(AppWidgets *w)
{
    /* Remove todas as linhas existentes do GtkListBox */
    GtkWidget *filho;
    while ((filho = gtk_widget_get_first_child(w->lista_box)) != NULL) {
        gtk_list_box_remove(GTK_LIST_BOX(w->lista_box), filho);
    }

    /* Busca os dados no Model */
    Registro buf[MAX_REGISTROS];
    int n = model_listar_todos(buf, MAX_REGISTROS);

    /* Cria uma linha GTK para cada registro */
    for (int i = 0; i < n; i++) {
        /*
         * GtkBox horizontal — análogo a <li style="display:flex">
         * gtk_box_new() aloca internamente; o GTK é dono desta memória.
         */
        GtkWidget *linha = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(linha, 8);
        gtk_widget_set_margin_end(linha, 8);
        gtk_widget_set_margin_top(linha, 4);
        gtk_widget_set_margin_bottom(linha, 4);

        /* Formata o texto da linha */
        char texto[MAX_CAMPO * 2 + 32];
        snprintf(texto, sizeof(texto), "[ID: %d]  %s  —  %s",
                 buf[i].id, buf[i].nome, buf[i].email);

        /* GtkLabel — análogo a <span> */
        GtkWidget *lbl = gtk_label_new(texto);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f); /* alinhar à esquerda */
        gtk_widget_set_hexpand(lbl, TRUE);           /* flex: 1 */
        gtk_box_append(GTK_BOX(linha), lbl);

        /*
         * GtkListBoxRow — wrapper obrigatório, análogo ao <li>
         * Usamos g_object_set_data() para associar o ID ao widget,
         * como se fosse um atributo data-id="N" no HTML.
         */
        GtkWidget *row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), linha);

        /*
         * g_object_set_data() — armazena um inteiro arbitrário no widget.
         * PONTEIROS: GINT_TO_POINTER converte int → void* sem alocar heap.
         * O dado pertence ao ciclo de vida do widget; sem free() necessário.
         */
        g_object_set_data(G_OBJECT(row), "registro-id",
                          GINT_TO_POINTER(buf[i].id));

        gtk_list_box_append(GTK_LIST_BOX(w->lista_box), row);
    }
}

/* Define o texto do label de status (análogo a statusEl.textContent = msg) */
static void view_set_status(AppWidgets *w, const char *msg)
{
    gtk_label_set_text(GTK_LABEL(w->label_status), msg);
}

/* Limpa os campos de entrada (análogo a form.reset()) */
static void view_limpar_form(AppWidgets *w)
{
    gtk_editable_set_text(GTK_EDITABLE(w->entry_nome),  "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_email), "");
    w->id_selecionado = -1;
}


/* ===========================================================================
 *  SEÇÃO 4 — CALLBACKS (Controladores de Eventos)
 *  Analogia Web: são os handlers passados para addEventListener().
 *
 *  Assinatura padrão GTK para sinais de botão:
 *    void callback(GtkWidget *widget, gpointer user_data)
 *    - widget:    o próprio botão que disparou o evento (como 'event.target')
 *    - user_data: dado extra passado no g_signal_connect() (nosso AppWidgets*)
 *
 *  PONTEIROS: user_data é void*; sempre faça cast explícito antes de usar.
 * ===========================================================================*/

/*
 * on_btn_inserir_clicked:
 *   Lê os campos, chama o Model para inserir e atualiza a View.
 *   ➜ INJETE SUA LÓGICA DE VALIDAÇÃO E PERSISTÊNCIA AQUI.
 */
static void on_btn_inserir_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget; /* suprime warning de parâmetro não usado */
    AppWidgets *w = (AppWidgets *)user_data;

    /* gtk_editable_get_text retorna const char* — pertence ao widget, NÃO faça free() */
    const char *nome  = gtk_editable_get_text(GTK_EDITABLE(w->entry_nome));
    const char *email = gtk_editable_get_text(GTK_EDITABLE(w->entry_email));

    /* --- PONTO DE INJEÇÃO: validação de input --- */
    if (nome[0] == '\0' || email[0] == '\0') {
        view_set_status(w, "⚠ Preencha todos os campos.");
        return;
    }

    /* --- PONTO DE INJEÇÃO: persistência (DB, arquivo, API) --- */
    int novo_id = model_inserir(nome, email);

    if (novo_id > 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "✔ Inserido com ID %d.", novo_id);
        view_set_status(w, msg);
        view_limpar_form(w);
        view_atualizar_lista(w);
    } else {
        view_set_status(w, "✖ Erro ao inserir: capacidade máxima atingida.");
    }
}

/*
 * on_btn_remover_clicked:
 *   Remove o registro com id_selecionado.
 *   ➜ INJETE SUA LÓGICA DE CONFIRMAÇÃO E PERSISTÊNCIA AQUI.
 */
static void on_btn_remover_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppWidgets *w = (AppWidgets *)user_data;

    if (w->id_selecionado < 0) {
        view_set_status(w, "⚠ Nenhum item selecionado.");
        return;
    }

    /* --- PONTO DE INJEÇÃO: confirmação (ex.: GtkAlertDialog) --- */
    /* --- PONTO DE INJEÇÃO: persistência                       --- */
    int removido = model_remover(w->id_selecionado);

    if (removido) {
        view_set_status(w, "✔ Registro removido.");
        view_limpar_form(w);
        view_atualizar_lista(w);
    } else {
        view_set_status(w, "✖ Registro não encontrado.");
    }
}

/*
 * on_btn_editar_clicked:
 *   Atualiza o registro selecionado com os valores dos campos.
 *   ➜ INJETE SUA LÓGICA DE VALIDAÇÃO E PERSISTÊNCIA AQUI.
 */
static void on_btn_editar_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppWidgets *w = (AppWidgets *)user_data;

    if (w->id_selecionado < 0) {
        view_set_status(w, "⚠ Selecione um item na lista para editar.");
        return;
    }

    const char *nome  = gtk_editable_get_text(GTK_EDITABLE(w->entry_nome));
    const char *email = gtk_editable_get_text(GTK_EDITABLE(w->entry_email));

    /* --- PONTO DE INJEÇÃO: validação e persistência --- */
    if (nome[0] == '\0' || email[0] == '\0') {
        view_set_status(w, "⚠ Preencha todos os campos para editar.");
        return;
    }

    int ok = model_editar(w->id_selecionado, nome, email);

    if (ok) {
        view_set_status(w, "✔ Registro atualizado.");
        view_limpar_form(w);
        view_atualizar_lista(w);
    } else {
        view_set_status(w, "✖ Falha ao editar: ID não encontrado.");
    }
}

/*
 * on_row_selected:
 *   Disparado quando o usuário clica em uma linha da lista.
 *   Análogo a: listaEl.addEventListener('click', e => { ... })
 *
 *   Assinatura específica do sinal "row-selected" do GtkListBox:
 *     void handler(GtkListBox*, GtkListBoxRow*, gpointer)
 */
static void on_row_selected(GtkListBox *lista, GtkListBoxRow *row,
                             gpointer user_data)
{
    (void)lista;
    AppWidgets *w = (AppWidgets *)user_data;

    if (row == NULL) {
        /* Seleção limpa (usuário clicou em área vazia) */
        w->id_selecionado = -1;
        view_set_status(w, "Seleção cancelada.");
        return;
    }

    /*
     * Recupera o ID que armazenamos via g_object_set_data().
     * GPOINTER_TO_INT faz o cast de void* → int sem alocação.
     */
    int id = GPOINTER_TO_INT(
                 g_object_get_data(G_OBJECT(row), "registro-id"));
    w->id_selecionado = id;

    /* Preenche os campos de formulário com os dados do registro selecionado */
    Registro buf[MAX_REGISTROS];
    int n = model_listar_todos(buf, MAX_REGISTROS);
    for (int i = 0; i < n; i++) {
        if (buf[i].id == id) {
            gtk_editable_set_text(GTK_EDITABLE(w->entry_nome),  buf[i].nome);
            gtk_editable_set_text(GTK_EDITABLE(w->entry_email), buf[i].email);
            break;
        }
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Selecionado: ID %d", id);
    view_set_status(w, msg);
}


/* ===========================================================================
 *  SEÇÃO 5 — CONSTRUÇÃO DA INTERFACE (View Builder)
 *  Aqui montamos o layout, análogo a escrever o HTML e o CSS inline.
 *  No GTK, o layout é feito via código C; em projetos maiores você pode
 *  usar arquivos .ui (XML) processados pelo GtkBuilder — análogo ao JSX.
 * ===========================================================================*/

/*
 * build_ui:
 *   Constrói toda a hierarquia de widgets e conecta os sinais.
 *   Chamado UMA vez pelo sinal "activate" da GtkApplication.
 *
 *   HIERARQUIA (análogo ao DOM):
 *
 *   GtkApplicationWindow (janela)
 *   └── GtkBox vertical (raiz — análogo a <body>)
 *       ├── GtkLabel (título)
 *       ├── GtkScrolledWindow (scroll)
 *       │   └── GtkListBox (lista de registros)
 *       ├── GtkSeparator
 *       ├── GtkBox horizontal (formulário)
 *       │   ├── GtkEntry (nome)
 *       │   └── GtkEntry (email)
 *       ├── GtkBox horizontal (botões)
 *       │   ├── GtkButton (Inserir)
 *       │   ├── GtkButton (Editar)
 *       │   └── GtkButton (Remover)
 *       └── GtkLabel (status)
 */
static void build_ui(GtkApplication *app, gpointer user_data)
{
    /*
     * AppWidgets vive na pilha (stack) desta função, mas como a GtkApplication
     * mantém a janela viva, precisamos alocar no heap para que os callbacks
     * possam acessar os ponteiros depois que build_ui retornar.
     *
     * MEMÓRIA: alocamos com g_new0 (zero-inicializado, equivale a calloc).
     * Liberamos em on_janela_destroy() para evitar leak.
     */
    AppWidgets *w = g_new0(AppWidgets, 1);
    w->id_selecionado = -1;

    /* O user_data aqui é NULL (não passamos nada no g_signal_connect abaixo) */
    (void)user_data;

    /* ----- Janela principal ----- */
    w->janela = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(w->janela), "CRUD GTK 4 — Template MVC");
    gtk_window_set_default_size(GTK_WINDOW(w->janela), 700, 520);

    /* ----- Container raiz vertical (análogo a <div class="flex flex-col">) ----- */
    GtkWidget *vbox_raiz = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox_raiz, 16);
    gtk_widget_set_margin_end(vbox_raiz, 16);
    gtk_widget_set_margin_top(vbox_raiz, 12);
    gtk_widget_set_margin_bottom(vbox_raiz, 12);
    gtk_window_set_child(GTK_WINDOW(w->janela), vbox_raiz);

    /* ----- Título ----- */
    GtkWidget *lbl_titulo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_titulo),
                         "<span size='large' weight='bold'>📋 Gerenciador de Registros</span>");
    gtk_widget_set_halign(lbl_titulo, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), lbl_titulo);

    /* ----- Separador (análogo a <hr>) ----- */
    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Área rolável + lista ----- */
    /*
     * GtkScrolledWindow — análogo a <div style="overflow-y: scroll; height: 250px">
     * gtk_widget_set_vexpand(TRUE) → flex-grow: 1 (ocupa espaço disponível)
     */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_size_request(scroll, -1, 200); /* altura mínima */
    gtk_box_append(GTK_BOX(vbox_raiz), scroll);

    /*
     * GtkListBox — análogo a <ul> com seleção de linha.
     * GTK_SELECTION_SINGLE = só um item selecionável por vez.
     */
    w->lista_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(w->lista_box),
                                    GTK_SELECTION_SINGLE);
    gtk_list_box_set_show_separators(GTK_LIST_BOX(w->lista_box), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), w->lista_box);

    /* Conecta o sinal de seleção de linha — análogo a addEventListener('click') */
    g_signal_connect(w->lista_box, "row-selected",
                     G_CALLBACK(on_row_selected), w);

    /* Separador visual antes do formulário */
    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Rótulo do formulário ----- */
    GtkWidget *lbl_form = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_form), "<b>Formulário</b>");
    gtk_widget_set_halign(lbl_form, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), lbl_form);

    /* ----- Linha do formulário (campos lado a lado) ----- */
    GtkWidget *hbox_form = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox_raiz), hbox_form);

    /* Campo Nome (análogo a <input type="text" placeholder="Nome"> */
    w->entry_nome = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_nome), "Nome");
    gtk_widget_set_hexpand(w->entry_nome, TRUE);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_nome);

    /* Campo E-mail */
    w->entry_email = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_email), "E-mail");
    gtk_widget_set_hexpand(w->entry_email, TRUE);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_email);

    /* ----- Linha de botões ----- */
    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox_raiz), hbox_btns);

    /* Botão Inserir */
    w->btn_inserir = gtk_button_new_with_label("➕ Inserir");
    gtk_widget_add_css_class(w->btn_inserir, "suggested-action"); /* cor de destaque */
    gtk_widget_set_hexpand(w->btn_inserir, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_inserir);

    /* Botão Editar */
    w->btn_editar = gtk_button_new_with_label("✏ Editar");
    gtk_widget_set_hexpand(w->btn_editar, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_editar);

    /* Botão Remover */
    w->btn_remover = gtk_button_new_with_label("🗑 Remover");
    gtk_widget_add_css_class(w->btn_remover, "destructive-action"); /* cor vermelha */
    gtk_widget_set_hexpand(w->btn_remover, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_remover);

    /*
     * Conecta os sinais dos botões.
     * g_signal_connect(objeto, "nome-do-sinal", G_CALLBACK(handler), dado_extra)
     *   ↕ equivalente JS:
     * objeto.addEventListener("nome-do-sinal", handler)
     *   (o 'dado_extra' é como fechar sobre variáveis com uma closure JS)
     *
     * PONTEIROS: passamos 'w' como user_data — os callbacks receberão este
     * ponteiro. Ele deve viver enquanto a janela existir (ver on_janela_destroy).
     */
    g_signal_connect(w->btn_inserir, "clicked",
                     G_CALLBACK(on_btn_inserir_clicked), w);
    g_signal_connect(w->btn_editar,  "clicked",
                     G_CALLBACK(on_btn_editar_clicked),  w);
    g_signal_connect(w->btn_remover, "clicked",
                     G_CALLBACK(on_btn_remover_clicked), w);

    /* Separador antes do status */
    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Label de status (análogo a <p id="status-bar">) ----- */
    w->label_status = gtk_label_new("Pronto.");
    gtk_widget_set_halign(w->label_status, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), w->label_status);

    /*
     * Libera AppWidgets quando a janela for destruída.
     *
     * g_object_set_data_full() associa 'w' à janela e registra 'g_free'
     * como destrutor — chamado automaticamente quando a janela é destruída.
     *
     * MEMÓRIA: isso substitui um callback on_destroy manual e garante
     * que não haverá leak mesmo que o usuário feche a janela pelo 'X'.
     * Análogo ao padrão RAII em C++ ou ao finally de um try/finally JS.
     */
    g_object_set_data_full(G_OBJECT(w->janela), "app-widgets", w, g_free);

    /* Popula a lista inicial e exibe a janela */
    view_atualizar_lista(w);
    gtk_widget_set_visible(w->janela, TRUE);
}


/* ===========================================================================
 *  SEÇÃO 6 — ENTRY POINT
 *  GtkApplication gerencia o loop de eventos principal.
 *  Análogo ao `document.addEventListener('DOMContentLoaded', ...)` + event loop
 *  do navegador.
 * ===========================================================================*/

int main(int argc, char *argv[])
{
    /*
     * GtkApplication — análogo ao objeto global 'window' do navegador.
     * O ID reverso (com ponto) é convenção Linux/freedesktop.
     * G_APPLICATION_DEFAULT_FLAGS = sem flags especiais (GTK 4.6+).
     */
    GtkApplication *app = gtk_application_new("br.com.meuapp.crud",
                                              G_APPLICATION_DEFAULT_FLAGS);

    /*
     * Conecta o sinal "activate" ao nosso construtor de UI.
     * "activate" dispara quando a aplicação inicia — é o DOMContentLoaded do GTK.
     */
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);

    /*
     * Inicia o loop de eventos e bloqueia até a janela ser fechada.
     * Análogo ao event loop do Node.js / navegador.
     *
     * g_application_run() retorna o código de saída (0 = sucesso).
     */
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    /*
     * g_object_unref() decrementa o contador de referências da aplicação.
     * Quando chega a zero, a memória é liberada pelo GLib.
     * NUNCA chame isso antes de g_application_run() terminar.
     */
    g_object_unref(app);

    return status;
}