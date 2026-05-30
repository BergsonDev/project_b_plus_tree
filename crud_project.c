#include "model.h"

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
    GtkWidget *entry_name;
    GtkWidget *entry_code;
    GtkWidget *entry_brandName;
    GtkWidget *entry_quantity;

    /* Botões de ação (análogo a <button>) */
    GtkWidget *btn_inserir;
    GtkWidget *btn_editar;
    GtkWidget *btn_remover;

    /* Rótulo de status (análogo a um <p id="status">) */
    GtkWidget *label_status;

    /* Pesquisa por ID */
    GtkWidget *entry_search;    /* campo de texto para o ID */
    GtkWidget *btn_pesquisar;   /* botão de pesquisa */
    GtkWidget *label_resultado; /* exibe o resultado da consulta */

    /* ID do registro selecionado (-1 = nenhum selecionado) */
    int id_selecionado;
} AppWidgets;


/* ===========================================================================
 *  SEÇÃO 3 — CONTROLLER (Funções Auxiliares da View)
 *  Mantém a View sincronizada com o Model.
 * ===========================================================================*/

/*
 * viewListAllProducts:
 *   Recria todas as linhas do GtkListBox a partir do Model.
 *
 *   Analogia Web: é o equivalente a limpar um <ul> com innerHTML = ''
 *   e depois fazer um forEach para criar novos <li> com createElement().
 *
 *   PONTEIROS: gtk_widget_get_parent() retorna um ponteiro sem transferência
 *   de propriedade (não faça unref). gtk_list_box_remove_all() é GTK 4.8+;
 *   para versões anteriores usamos o loop abaixo.
 */
static void viewListAllProducts(AppWidgets *w)
{
    /* Remove todas as linhas existentes do GtkListBox */
    GtkWidget *filho;
    while ((filho = gtk_widget_get_first_child(w->lista_box)) != NULL) {
        gtk_list_box_remove(GTK_LIST_BOX(w->lista_box), filho);
    }

    /* Busca os dados no Model */
    Products buf[MAX_PRODUCT];
    int n = allProducts(buf, MAX_PRODUCT);

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
        char texto[MAX_STR * 2 + 32];
        snprintf(texto, sizeof(texto), "ID : %d/ Nome : %s/Código: %s / Marca : %s / Quantidade : %d",
                 buf[i].id, buf[i].name, buf[i].code, buf[i].brandName, buf[i].quantity);

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
static void viewClearForm(AppWidgets *w)
{
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_code), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_brandName), "");

    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(w->entry_quantity),
        0
    );

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
 * onBtnInsertClick:
 *   Lê os campos, chama o Model para inserir e atualiza a View.
 *   ➜ INJETE SUA LÓGICA DE VALIDAÇÃO E PERSISTÊNCIA AQUI.
 */
static void onBtnInsertClick(GtkWidget *widget, gpointer user_data)
{
    (void)widget; /* suprime warning de parâmetro não usado */
    AppWidgets *w = (AppWidgets *)user_data;

    /* gtk_editable_get_text retorna const char* — pertence ao widget, NÃO faça free() */
    const char *name      = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    const char *code      = gtk_editable_get_text(GTK_EDITABLE(w->entry_code));
    const char *brandName = gtk_editable_get_text(GTK_EDITABLE(w->entry_brandName));
    const int   quantity  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w->entry_quantity));

    /* --- PONTO DE INJEÇÃO: validação de input --- */
    if (name[0] == '\0' || code[0] == '\0' || brandName[0] == '\0') {
        view_set_status(w, "⚠ Preencha todos os campos.");
        return;
    }

    /* --- PONTO DE INJEÇÃO: persistência (DB, arquivo, API) --- */
    int novo_id = insertProduct(name, code, brandName, quantity);

    if (novo_id > 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Inserido com ID %d.", novo_id);
        view_set_status(w, msg);
        viewClearForm(w);
        viewListAllProducts(w);
    } else {
        view_set_status(w, "Erro ao inserir: capacidade máxima atingida.");
    }
}

/*
 * onBtnRemoveClick:
 *   Remove o registro com id_selecionado.
 *   ➜ INJETE SUA LÓGICA DE CONFIRMAÇÃO E PERSISTÊNCIA AQUI.
 */
static void onBtnRemoveClick(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppWidgets *w = (AppWidgets *)user_data;

    if (w->id_selecionado < 0) {
        view_set_status(w, "⚠ Nenhum item selecionado.");
        return;
    }

    /* --- PONTO DE INJEÇÃO: confirmação (ex.: GtkAlertDialog) --- */
    /* --- PONTO DE INJEÇÃO: persistência                       --- */
    int removido = removeProduct(w->id_selecionado);

    if (removido) {
        view_set_status(w, "Registro removido.");
        viewClearForm(w);
        viewListAllProducts(w);
    } else {
        view_set_status(w, "Registro não encontrado.");
    }
}

/*
 * onBtnUpdateClick:
 *   Atualiza o registro selecionado com os valores dos campos.
 *   ➜ INJETE SUA LÓGICA DE VALIDAÇÃO E PERSISTÊNCIA AQUI.
 */
static void onBtnUpdateClick(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppWidgets *w = (AppWidgets *)user_data;

    if (w->id_selecionado < 0) {
        view_set_status(w, "Selecione um item na lista para editar.");
        return;
    }

    const char *name      = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    const char *code      = gtk_editable_get_text(GTK_EDITABLE(w->entry_code));
    const char *brandName = gtk_editable_get_text(GTK_EDITABLE(w->entry_brandName));
    const int   quantity  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w->entry_quantity));

    /* --- PONTO DE INJEÇÃO: validação e persistência --- */
    if (name[0] == '\0' || code[0] == '\0' || brandName[0] == '\0') {
        view_set_status(w, "⚠ Preencha todos os campos para editar.");
        return;
    }

    int ok = editProduct(w->id_selecionado, name, code, brandName, quantity);

    if (ok) {
        view_set_status(w, "Registro atualizado.");
        viewClearForm(w);
        viewListAllProducts(w);
    } else {
        view_set_status(w, "Falha ao editar: ID não encontrado.");
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
    Products buf[MAX_PRODUCT];
    int n = allProducts(buf, MAX_PRODUCT);
    for (int i = 0; i < n; i++) {
        if (buf[i].id == id) {
            gtk_editable_set_text(GTK_EDITABLE(w->entry_name),      buf[i].name);
            gtk_editable_set_text(GTK_EDITABLE(w->entry_code),      buf[i].code);
            gtk_editable_set_text(GTK_EDITABLE(w->entry_brandName), buf[i].brandName);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->entry_quantity), buf[i].quantity);
            break;
        }
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Selecionado: ID %d", id);
    view_set_status(w, msg);
}

/*
 * onBtnSearchClick:
 *   Lê o ID digitado em entry_search, chama searchProduct() e exibe
 *   o resultado em label_resultado.
 *
 *   PONTEIROS: searchProduct() retorna Products* apontando para dentro
 *   da estrutura interna do Model — NÃO chame free() nesse ponteiro.
 *   A propriedade permanece com o Model.
 *
 *   Efeito colateral útil: se a peça for encontrado, o formulário
 *   principal já é preenchido automaticamente, permitindo editar/remover
 *   na sequência sem redigitar os dados.
 */
static void onBtnSearchClick(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppWidgets *w = (AppWidgets *)user_data;

    const char *texto = gtk_editable_get_text(GTK_EDITABLE(w->entry_search));

    if (texto[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(w->label_resultado),
                           "Digite um ID para pesquisar.");
        return;
    }

    /* strtol é preferível a atoi: detecta erros de conversão */
    int id = (int)strtol(texto, NULL, 10);

    Products *p = searchProduct(id);

    if (p != NULL) {
        char msg[MAX_STR * 4 + 64];
        snprintf(msg, sizeof(msg),
                 "✔ ID: %d | Nome: %s | Código: %s | Marca: %s | Qtd: %d",
                 p->id, p->name, p->code, p->brandName, p->quantity);
        gtk_label_set_text(GTK_LABEL(w->label_resultado), msg);

        /* Preenche o formulário principal para edição imediata */
        gtk_editable_set_text(GTK_EDITABLE(w->entry_name),      p->name);
        gtk_editable_set_text(GTK_EDITABLE(w->entry_code),      p->code);
        gtk_editable_set_text(GTK_EDITABLE(w->entry_brandName), p->brandName);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->entry_quantity), p->quantity);
        w->id_selecionado = p->id;
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "Nenhuma peça com ID %d.", id);
        gtk_label_set_text(GTK_LABEL(w->label_resultado), msg);
    }
}


/* ===========================================================================
 *  SEÇÃO 5 — CONSTRUÇÃO DA INTERFACE (View Builder)
 *  Aqui montamos o layout, análogo a escrever o HTML e o CSS inline.
 *  No GTK, o layout é feito via código C; em projetos maiores você pode
 *  usar arquivos .ui (XML) processados pelo GtkBuilder — análogo ao JSX.
 * ===========================================================================*/


/*  Carrega CSS externo (fora do arquivo principal) */
static void applyCss()
{
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_path(provider, "main.css");

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "App com Ícone");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    gtk_window_set_icon_name(GTK_WINDOW(window), "com.meuapp.icone");

    gtk_window_present(GTK_WINDOW(window));
}


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
 *       │   ├── GtkEntry (name)
 *       │   ├── GtkEntry (code)
 *       │   ├── GtkEntry (brandName)
 *       │   └── GtkSpinButton (quantity)
 *       ├── GtkBox horizontal (botões CRUD)
 *       │   ├── GtkButton (Inserir)
 *       │   ├── GtkButton (Editar)
 *       │   └── GtkButton (Remover)
 *       ├── GtkSeparator
 *       ├── GtkLabel ("Pesquisa por ID")
 *       ├── GtkBox horizontal (pesquisa)
 *       │   ├── GtkEntry (entry_search)
 *       │   └── GtkButton (btn_pesquisar)
 *       ├── GtkLabel (label_resultado)
 *       ├── GtkSeparator
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

    (void)user_data;

    /* ----- Janela principal ----- */
    w->janela = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(w->janela), "Controle de Peças em Árvore B em C com GTK 4 — Template MVC");
    gtk_window_set_default_size(GTK_WINDOW(w->janela), 700, 580);

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
                         "<span size='large' weight='bold'>Controle de Peças</span>");
    gtk_widget_set_halign(lbl_titulo, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), lbl_titulo);

    /* ----- Separador (análogo a <hr>) ----- */
    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Área rolável + lista ----- */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_size_request(scroll, -1, 200);
    gtk_box_append(GTK_BOX(vbox_raiz), scroll);

    w->lista_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(w->lista_box),
                                    GTK_SELECTION_SINGLE);
    gtk_list_box_set_show_separators(GTK_LIST_BOX(w->lista_box), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), w->lista_box);

    g_signal_connect(w->lista_box, "row-selected",
                     G_CALLBACK(on_row_selected), w);

    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Rótulo do formulário ----- */
    GtkWidget *lbl_form = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_form), "<b>Formulário</b>");
    gtk_widget_set_halign(lbl_form, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), lbl_form);

    /* ----- Linha do formulário (campos lado a lado) ----- */
    GtkWidget *hbox_form = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox_raiz), hbox_form);

    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name), "Nome");
    gtk_widget_set_hexpand(w->entry_name, TRUE);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_name);

    w->entry_code = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_code), "Código / Série");
    gtk_widget_set_hexpand(w->entry_code, TRUE);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_code);

    w->entry_brandName = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_brandName), "Marca");
    gtk_widget_set_hexpand(w->entry_brandName, TRUE);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_brandName);

    w->entry_quantity = gtk_spin_button_new_with_range(0, 999, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w->entry_quantity), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w->entry_quantity), 0);
    gtk_box_append(GTK_BOX(hbox_form), w->entry_quantity);

    /* ----- Linha de botões CRUD ----- */
    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox_raiz), hbox_btns);

    w->btn_inserir = gtk_button_new_with_label("Inserir");
    gtk_widget_add_css_class(w->btn_inserir, "btn-successfull");
    gtk_widget_set_hexpand(w->btn_inserir, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_inserir);

    w->btn_editar = gtk_button_new_with_label("✏ Editar");
    gtk_widget_add_css_class(w->btn_editar, "btn-update");
    gtk_widget_set_hexpand(w->btn_editar, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_editar);

    w->btn_remover = gtk_button_new_with_label("🗑 Remover");
    gtk_widget_add_css_class(w->btn_remover, "destructive-action");
    gtk_widget_set_hexpand(w->btn_remover, TRUE);
    gtk_box_append(GTK_BOX(hbox_btns), w->btn_remover);

    g_signal_connect(w->btn_inserir, "clicked",
                     G_CALLBACK(onBtnInsertClick), w);
    g_signal_connect(w->btn_editar,  "clicked",
                     G_CALLBACK(onBtnUpdateClick),  w);
    g_signal_connect(w->btn_remover, "clicked",
                     G_CALLBACK(onBtnRemoveClick), w);

    /* ----- Separador antes da pesquisa ----- */
    gtk_box_append(GTK_BOX(vbox_raiz), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ----- Seção de pesquisa por ID ----- */
    /*
     * Análogo a um <section id="search"> com um <input> + <button> + <p>.
     *
     * entry_search  → campo de texto onde o usuário digita o ID.
     * btn_pesquisar → dispara onBtnSearchClick.
     * label_resultado → exibe a peça encontrado (ou mensagem de erro).
     *
     * PONTEIROS: searchProduct() retorna Products* apontando para dentro
     * do Model — . O formulário principal é preenchido
     * automaticamente quando a peça é encontrado.
     */
    GtkWidget *lbl_search = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_search), "<b>Pesquisa por ID</b>");
    gtk_widget_set_halign(lbl_search, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), lbl_search);

    GtkWidget *hbox_search = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox_raiz), hbox_search);

    w->entry_search = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_search), "ID da peça");
    gtk_widget_set_hexpand(w->entry_search, TRUE);
    gtk_box_append(GTK_BOX(hbox_search), w->entry_search);

    w->btn_pesquisar = gtk_button_new_with_label("Pesquisar pelo ID");
    gtk_box_append(GTK_BOX(hbox_search), w->btn_pesquisar);

    g_signal_connect(w->btn_pesquisar, "clicked",
                     G_CALLBACK(onBtnSearchClick), w);

    w->label_resultado = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->label_resultado), TRUE);
    gtk_widget_set_halign(w->label_resultado, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox_raiz), w->label_resultado);

    /* ----- Separador antes do status ----- */
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
     */
    g_object_set_data_full(G_OBJECT(w->janela), "app-widgets", w, g_free);

    /* Popula a lista inicial e exibe a janela */
    viewListAllProducts(w);
    applyCss();
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
