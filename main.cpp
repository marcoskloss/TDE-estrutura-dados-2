#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SALES_CSV_LINE_SIZE 91
#define INDEX_LINE_SIZE 15
#define ORDER_INDEX_LINE_SIZE 18
#define DATA_LINE_SIZE 93
#define INDEX_GROUP_QNT 2
#define ID_INDEX_KEY_SIZE 6
#define ORDER_ID_KEY_SIZE 9
#define MAX_ADDRESSES_PER_NODE 2500

#define CREATE_INDEXES 0

typedef struct reg {
    char region[29];
    char item_type[16];
    char sales_channel[8];
    float profit;
    unsigned int id;
    unsigned int order_id;
    int date;
    char priority;
} DATA_REG;

typedef struct idx_tree_node {
    int date;
    int* addresses;
    int address_qnt;
    struct idx_tree_node* r;
    struct idx_tree_node* l;
} IDX_TREE_NODE;

typedef struct linked_list_address {
    int address;
    struct linked_list_address* next;
} LINKED_LIST_ADDRESS;

typedef struct idx_linked_list_node {
    char priority;
    LINKED_LIST_ADDRESS* addresses;
    struct idx_linked_list_node* next;
} IDX_LINKED_LIST_NODE;

char *itoa(int num, char *str, int _)
{
    if(str == NULL)
    {
        return NULL;
    }
    sprintf(str, "%d", num);
    return str;
}

void write_int(unsigned int id, int chars_count, FILE* f) {
    int i;
    char *aux = (char *) malloc(sizeof(char) * chars_count);
    itoa(id, aux, 10);

    size_t aux_size = strlen(aux);
    size_t diff = chars_count - aux_size;
    for (i=0; i<diff; i++) {
        fprintf(f, "%c", '0');
    }
    fprintf(f, "%d", id);
}

void write_profit(float profit, FILE* f) {
    int chars_count = 10, i;
    char aux[10];
    sprintf(aux, "%.2f", profit);

    size_t aux_size = strlen(aux);
    size_t diff = chars_count - aux_size;
    for (i=0; i<diff; i++) {
        fprintf(f, "%c", '0');
    }
    fprintf(f, "%s", aux);
}

void write_data_file(DATA_REG* r, FILE* f) {
    write_int(r->id, 6, f);
    fprintf(f, ",");
    fprintf(f, "%-28s", r->region);
    fprintf(f, ",");
    fprintf(f, "%-15s", r->item_type);
    fprintf(f, ",");
    fprintf(f, "%-7s", r->sales_channel);
    fprintf(f, ",");
    fprintf(f, "%c", r->priority);
    fprintf(f, ",");
    write_int(r->date, 8, f);
    fprintf(f, ",");
    write_int(r->order_id, 9, f);
    fprintf(f, ",");
    write_profit(r->profit, f);
    fprintf(f, "\n");
}

void sales_csv_line_to_struct(char line[SALES_CSV_LINE_SIZE], DATA_REG* r, int id) {
    int comma_pointer = 0, prev_comma_pointer = 0, i;
    r->id = id;

    prev_comma_pointer = comma_pointer;
    for (i=prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// region
    strncpy(r->region, &line[prev_comma_pointer], sizeof(char) * (comma_pointer - prev_comma_pointer));
    r->region[comma_pointer - prev_comma_pointer] = '\0';

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// item type
    strncpy(r->item_type, &line[prev_comma_pointer+1], sizeof(char) * (comma_pointer - prev_comma_pointer));
    r->item_type[comma_pointer - prev_comma_pointer - 1] = '\0';

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// sales channel
    strncpy(r->sales_channel, &line[prev_comma_pointer+1], sizeof(char) * (comma_pointer - prev_comma_pointer));
    r->sales_channel[comma_pointer - prev_comma_pointer - 1] = '\0';

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// priority
    r->priority = line[comma_pointer-1];

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// date
    char str_date[9];
    strncpy(str_date, &line[prev_comma_pointer+1], sizeof(char) * (comma_pointer - prev_comma_pointer));
    str_date[8] = '\0';
    r->date = atoi(str_date);

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// order id
    char str_order_id[10];
    strncpy(str_order_id, &line[prev_comma_pointer+1], sizeof(char) * (comma_pointer - prev_comma_pointer));
    str_order_id[9] = '\0';
    r->order_id = atoi(str_order_id);

    prev_comma_pointer = comma_pointer;
    for (i= prev_comma_pointer + 1; line[i] != ',' && line[i] != '\n' && line[i] != '\r'; i++) {}
    comma_pointer = i;

// profit
    char str_profit[11];
    strncpy(str_profit, &line[prev_comma_pointer+1], sizeof(char) * (comma_pointer - prev_comma_pointer));
    str_profit[10] = '\0';
    r->profit = strtod(str_profit, NULL); // str -> double
}

void read_line_and_key_order_index_file(FILE* f, char buff[ORDER_INDEX_LINE_SIZE], int* key) {
    char key_str[10];
    fgets(buff, ORDER_INDEX_LINE_SIZE, f);
    strncpy(key_str, buff, 4);
    key_str[9] = '\0';
    *key = atoi(key_str);
}

void bubblesort_order_id_index(FILE* f) {
    char buffer[ORDER_INDEX_LINE_SIZE];
    char next_buffer[ORDER_INDEX_LINE_SIZE];
    int key, next_key;
    int start_p, size, qnt, i;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    qnt = (size / ORDER_INDEX_LINE_SIZE);

    int sort = 0;
    do {
        sort = 0;
        for (i=0; i < qnt - 1; i++) {
            fseek(f, (ORDER_INDEX_LINE_SIZE) * i, SEEK_SET);

            start_p = ftell(f);
            read_line_and_key_order_index_file(f, buffer, &key);
            read_line_and_key_order_index_file(f, next_buffer, &next_key);

            if (next_key < key) {
                fseek(f, start_p, SEEK_SET);
                fputs(next_buffer, f);

                fseek(f, (start_p + ORDER_INDEX_LINE_SIZE), SEEK_SET);
                fputs(buffer, f);

                sort = 1;
            }
        }
    } while (sort);
}

void data_txt_line_to_struct(char line[DATA_LINE_SIZE], DATA_REG* r) {
    char id_str[7];
    strncpy(id_str, line, sizeof(char) * 6);
    id_str[6] = '\0';
    r->id = atoi(id_str);

    strncpy(r->region, &line[7], sizeof(char) * 28);
    r->region[28] = '\0';

    strncpy(r->item_type, &line[36], sizeof(char) * 15);
    r->item_type[15] = '\0';

    strncpy(r->sales_channel, &line[52], sizeof(char) * 7);
    r->sales_channel[7] = '\0';

    r->priority = line[60];

    char date_str[9];
    strncpy(date_str, &line[62], sizeof(char) * 8);
    date_str[8] = '\0';
    r->date = atoi(date_str);

    char order_id[10];
    strncpy(order_id, &line[71], sizeof(char) * 9);
    order_id[9] = '\0';
    r->order_id = atoi(order_id);

    char profit_str[11];
    strncpy(profit_str, &line[81], sizeof(char) * 10);
    profit_str[10] = '\0';
    r->profit = strtod(profit_str, NULL); // str -> double
}

void create_data_file()
{
	printf("#### CRIANDO ARQUIVO DE DADOS ####\n\n");
    FILE* f_csv = fopen("sales.csv", "rt");
    FILE* f_data = fopen("data.txt", "wt");

    if (f_csv == NULL) {
        printf("create_data_file error: sales.csv\n");
        return;
    }
    if (f_data == NULL) {
        printf("create_data_file error: data.txt\n");
        return;
    }

    char buff[SALES_CSV_LINE_SIZE];
    DATA_REG r;
    int id_counter = 1;

    while(fgets(buff, SALES_CSV_LINE_SIZE, f_csv)) {
        sales_csv_line_to_struct(buff, &r, id_counter);
        id_counter += 1;

        write_data_file(&r, f_data);
    }
    fclose(f_data);
    fclose(f_csv);
}

void create_order_id_index_file() {
	printf("#### CRIANDO ARQUIVO DE INDICE NA CHAVE ORDER_ID ####\n\n");
	
    FILE* f_data = fopen("data.txt", "rt");
    FILE* f_index = fopen("order_id_index.txt", "w+t");

    if (f_data == NULL) {
        printf("Erro ao abrir arquivo data.txt\n");
        return;
    }
    if (f_index == NULL) {
        printf("Erro ao abrir arquivo index.txt\n");
        return;
    }

    char buff[DATA_LINE_SIZE];
    int address = 0;

    DATA_REG r;

    while (fgets(buff, sizeof(buff), f_data) != NULL) {
        data_txt_line_to_struct(buff, &r);

        write_int(r.order_id, 9, f_index);
        fprintf(f_index, ",");
        write_int(address, 6, f_index);
        fprintf(f_index, "\n");

        address += 1;
    }

    bubblesort_order_id_index(f_index);

    fclose(f_data);
    fclose(f_index);
}

void create_id_index_file() {
	printf("#### CRIANDO ARQUIVO DE INDICE NA CHAVE ID ####\n\n");
	
    FILE* f_data = fopen("data.txt", "rt");
    FILE* f_index = fopen("id_index.txt", "w+t");

    if (f_data == NULL) {
        printf("Erro ao abrir arquivo data.txt\n");
        return;
    }
    if (f_index == NULL) {
        printf("Erro ao abrir arquivo index.txt\n");
        return;
    }

    char buff[DATA_LINE_SIZE];
    char index_buff[INDEX_LINE_SIZE];
    int address = 0;

    DATA_REG r;

    while (fgets(buff, sizeof(buff), f_data) != NULL) {
        data_txt_line_to_struct(buff, &r);

        write_int(r.id, 6, f_index);
        fprintf(f_index, ",");
        write_int(address, 6, f_index);
        fprintf(f_index, "\n");

        address += INDEX_GROUP_QNT + 1;
        fseek(f_data, (DATA_LINE_SIZE) * (INDEX_GROUP_QNT), SEEK_CUR);
    }

    fclose(f_data);
    fclose(f_index);
}

void new_tree_node(IDX_TREE_NODE** node, int init_sub_nodes) {
    if ((*node) != NULL) return;
    (*node) = (IDX_TREE_NODE*) malloc(sizeof(IDX_TREE_NODE));
    (*node)->addresses = (int*) malloc(sizeof(int) * MAX_ADDRESSES_PER_NODE);
    (*node)->address_qnt = 0;
    (*node)->l = NULL;
    (*node)->r = NULL;
    (*node)->date = 0;

    if (init_sub_nodes) {
        new_tree_node(&(*node)->l, 0);
        new_tree_node(&(*node)->r, 0);
    }
}

void add_address_to_node(IDX_TREE_NODE* node, int address) {
    node->addresses[node->address_qnt] = address;
    node->address_qnt += 1;
}

void insert_tree_node(IDX_TREE_NODE** root, DATA_REG r, int address) {
    if ((*root) == NULL) {
        new_tree_node(&(*root), 1);
        add_address_to_node((*root), address);
        (*root)->date = r.date;
        return;
    }

    IDX_TREE_NODE* aux = (*root);

    while (aux->date != 0) {
        if (r.date < aux->date) {
            aux = aux->l;
        } else if (r.date > aux->date) {
            aux = aux->r;
        } else { // r.date == aux->date
            break;
        }
    }

    if (aux->date == 0) {
        new_tree_node(&(aux->l), 1);
        new_tree_node(&(aux->r), 1);
        aux->date = r.date;
    }

    add_address_to_node(aux, address);
}

void create_tree_index(IDX_TREE_NODE** root) {
    FILE* f = fopen("data.txt", "rt");
    char buffer[DATA_LINE_SIZE];
    DATA_REG r;

    int address = 0;

    while(fgets(buffer, sizeof(buffer), f) != NULL) {
        data_txt_line_to_struct(buffer, &r);
        insert_tree_node(&(*root), r, address);
        address += 1;
    }
}

void new_linked_list_node(IDX_LINKED_LIST_NODE** node, int init_next_node) {
    if ((*node) != NULL) return;
    (*node) = (IDX_LINKED_LIST_NODE*) malloc(sizeof(IDX_LINKED_LIST_NODE));
    (*node)->priority = '\0';
    (*node)->next = NULL;
    (*node)->addresses = (LINKED_LIST_ADDRESS*) malloc(sizeof(LINKED_LIST_ADDRESS));
    (*node)->addresses->address = -1;
    (*node)->addresses->next = NULL;
    if (init_next_node) {
        new_linked_list_node(&(*node)->next, 0);
    }
}

void add_address_to_linked_list_node(IDX_LINKED_LIST_NODE** node, int address) {
    if ((*node)->addresses->address == -1) {
        (*node)->addresses->address = address;

        (*node)->addresses->next = (LINKED_LIST_ADDRESS*) malloc(sizeof(LINKED_LIST_ADDRESS));
        (*node)->addresses->next->address = -1;
    } else {
        LINKED_LIST_ADDRESS* aux = (*node)->addresses;
        while (aux->address != -1) {
            aux = aux->next;
        }
        aux->address = address;
        aux->next = (LINKED_LIST_ADDRESS*) malloc(sizeof(LINKED_LIST_ADDRESS));
        aux->next->address = -1;
    }
}

void insert_linked_list_node(IDX_LINKED_LIST_NODE ** root, DATA_REG r, int address) {
    if ((*root) == NULL) {
        new_linked_list_node(&(*root), 1);
        add_address_to_linked_list_node(&(*root), address);
        (*root)->priority = r.priority;
        return;
    }

    IDX_LINKED_LIST_NODE* aux = (*root);
    while(aux->priority != '\0') {
        if (r.priority == aux->priority) break;
        aux = aux->next;
    }

    if (aux->priority == '\0') {
        new_linked_list_node(&(aux->next), 1);
        aux->priority = r.priority;
    }
    add_address_to_linked_list_node(&aux, address);
}

void create_linked_list_index(IDX_LINKED_LIST_NODE** root) {
    FILE* f = fopen("data.txt", "rt");
    char buffer[DATA_LINE_SIZE];
    DATA_REG r;

    int address = 0;

    while(fgets(buffer, sizeof(buffer), f) != NULL) {
        data_txt_line_to_struct(buffer, &r);
        insert_linked_list_node(&(*root), r, address);
        address += 1;
    }
}

int file_binary_search(FILE* f, int register_size, int key_size, int search_key) {
    int f_size;
    fseek(f, 0, SEEK_END);
    f_size = ftell(f);
    rewind(f);

    char* buff = (char*) malloc(sizeof(char) * register_size);
    int key = -1, address, prev_key;
    double middle, bottom = -1;
    double top = (f_size / register_size - 1);
    char* str_key = (char*) malloc(sizeof(char) * (key_size + 1));

    while (bottom < top) {
        middle = ceil((top + bottom) / 2);

        fseek(f, (register_size) * (middle), SEEK_SET);
        fgets(buff, register_size, f);

        strncpy(str_key, buff, key_size);
        str_key[key_size] = '\0';

        prev_key = key;
        key = atoi(str_key);

        if (prev_key == key) break;

        if (search_key < key) {
            top = middle;
        } else if (search_key > key) {
            bottom = middle;
        } else {
            break;
        }
    }
    free(str_key);

    if (key != search_key) {
        return -1;
    }

    char str_addr[7];
    strncpy(str_addr, &buff[key_size+1], 6);
    str_addr[6] = '\0';

    address = atoi(str_addr);

    return address;
}

void print_data_register(DATA_REG* r) {
    printf("id: %d\n", r->id);
    printf("Item Type: %s\n", r->item_type);
    printf("Priority: %c\n", r->priority);
    printf("Profit: %.2f\n", r->profit);
    printf("Region: %s\n", r->region);
    printf("Sales Channel: %s\n", r->sales_channel);
    printf("Date: %d\n", r->date);
    printf("OrderID: %d\n\n", r->order_id);
}

int normalize_id_index_search_key(int key) {
    if (key == 1 || (key-1) % 3 == 0) return key;
    while (1) {
        key -= 1;
        if (key == 1 || (key-1) % 3 == 0) break;
    }
    return key;
}

void data_binary_search_by_id_index(int key) {
    printf("\n\t..--=== (indice em arquivo) Buscando o registro com id = %d\n\n", key);


    FILE* f = fopen("id_index.txt", "rt");
    FILE* f_data = fopen("data.txt", "rt");
    DATA_REG r;
    int n_key = normalize_id_index_search_key(key);
    int address = file_binary_search(f, INDEX_LINE_SIZE, ID_INDEX_KEY_SIZE, n_key);

    if (address == -1) {
        printf("Registro de chave %d nao encontrado\n", key);
        return;
    }

    char buff[DATA_LINE_SIZE];
    int tries = 3;
    while (tries != 0) {
        fseek(f_data, DATA_LINE_SIZE * address, SEEK_SET);
        fgets(buff, DATA_LINE_SIZE, f_data);
        data_txt_line_to_struct(buff, &r);
        if (r.id == key) break;
        tries -= 1;
        address += 1;
    }

    if (tries == 0) {
        printf("Registro de chave %d nao encontrado\n", key);
        return;
    }

    print_data_register(&r);

    fclose(f);
    fclose(f_data);
}

void data_search_by_order_id_index(int key) {
    printf("\n\t..--=== (indice em arquivo) Buscando o registro com order id = %d\n\n", key);


    FILE* f = fopen("order_id_index.txt", "rt");
    FILE* f_data = fopen("data.txt", "rt");
    DATA_REG r;
    int address = file_binary_search(f, ORDER_INDEX_LINE_SIZE, ORDER_ID_KEY_SIZE, key);

    if (address == -1) {
        printf("Registro de chave %d nao encontrado\n", key);
        return;
    }

    char buff[DATA_LINE_SIZE];
    fseek(f_data, DATA_LINE_SIZE * address, SEEK_SET);
    fgets(buff, DATA_LINE_SIZE, f_data);

    data_txt_line_to_struct(buff, &r);
    print_data_register(&r);

    fclose(f);
    fclose(f_data);
}

void data_binary_search_tree_index(int date, IDX_TREE_NODE* root) {
    printf("\n\t..--=== (indice tree em memoria) Buscando todos os registros com data = %d\n\n", date);

    IDX_TREE_NODE* aux = root;
    FILE* f = fopen("data.txt", "rt");

    while (aux->date != 0) {
        if (date < aux->date) {
            aux = aux->l;
        } else if (date > aux->date) {
            aux = aux->r;
        } else {
            break;
        }
    }

    if (aux->date == 0) {
        printf("Registro de data %d nao encontrado\n", date);
        return;
    }

    int i, address;
    DATA_REG r;
    char buff[DATA_LINE_SIZE];

    for (i = 0; i < aux->address_qnt; i++) {
        address = aux->addresses[i];

        fseek(f, DATA_LINE_SIZE * address, SEEK_SET);
        fgets(buff, DATA_LINE_SIZE, f);

        data_txt_line_to_struct(buff, &r);
        print_data_register(&r);
    }
    fclose(f);
}

void data_search_linked_list_index(char priority, IDX_LINKED_LIST_NODE* root) {
    printf("\n\t..--=== (indice linked list em memoria) Buscando todos os registros com prioridade = %c\n\n", priority);

    FILE* f = fopen("data.txt", "rt");
    IDX_LINKED_LIST_NODE* aux = root;

    while(aux->priority != '\0') {
        if (aux->priority == priority) break;
        aux = aux->next;
    }

    if (aux->priority == '\0') {
        printf("Registro com prioridade %c nao encontrado\n", priority);
        return;
    }

    DATA_REG r;
    LINKED_LIST_ADDRESS* addr = aux->addresses;
    char buff[DATA_LINE_SIZE];

    while(addr->address != -1) {
        fseek(f, DATA_LINE_SIZE * addr->address, SEEK_SET);
        fgets(buff, DATA_LINE_SIZE, f);

        data_txt_line_to_struct(buff, &r);
        print_data_register(&r);

        addr = addr->next;
    }
}

int main() {
		
	/*
	 * 0 - indice linked list em memoria, busca por prioridade
	 * 1 - indice tree em memoria, busca por data
	 * 2 - indice em arquivo esparso, busca por id
	 * 3 - indice em arquivo, busca por id do pedido
	 * */

	int MENU = 0;
	
    if (CREATE_INDEXES) {
        create_data_file();
        create_id_index_file();
        create_order_id_index_file();
    }

    IDX_TREE_NODE* root = NULL;
    IDX_LINKED_LIST_NODE* list_root = NULL;

    create_linked_list_index(&list_root);
    create_tree_index(&root);

    if (MENU == 0) {
        data_search_linked_list_index('H', list_root);
        data_search_linked_list_index('L', list_root);
        data_search_linked_list_index('M', list_root);
        data_search_linked_list_index('C', list_root);

        data_search_linked_list_index('X', list_root);
        data_search_linked_list_index('Y', list_root);
        data_search_linked_list_index('Z', list_root);
    }

    if (MENU == 1) {
        data_binary_search_tree_index(20160306, root);
        data_binary_search_tree_index(20100418, root);
        data_binary_search_tree_index(20150108, root);
        data_binary_search_tree_index(20140119, root);
        data_binary_search_tree_index(20190426, root);
        data_binary_search_tree_index(20120303, root);
        data_binary_search_tree_index(20121124, root);
        data_binary_search_tree_index(20110318, root);

        data_binary_search_tree_index(20050318, root);
        data_binary_search_tree_index(20040112, root);
        data_binary_search_tree_index(20221102, root);
        data_binary_search_tree_index(20010826, root);
    }

    if (MENU == 2) {
        data_binary_search_by_id_index(1);
        data_binary_search_by_id_index(2);
        data_binary_search_by_id_index(3);
        data_binary_search_by_id_index(4);
        data_binary_search_by_id_index(5);
        data_binary_search_by_id_index(6);
        data_binary_search_by_id_index(7);
        data_binary_search_by_id_index(8);
        data_binary_search_by_id_index(29);
        data_binary_search_by_id_index(30);
        data_binary_search_by_id_index(9);
        data_binary_search_by_id_index(123);
        data_binary_search_by_id_index(933);
        data_binary_search_by_id_index(9999);
        data_binary_search_by_id_index(10000);
        
        data_binary_search_by_id_index(10001);
        data_binary_search_by_id_index(10003);
        data_binary_search_by_id_index(10004);
    }

    if (MENU == 3) {
        data_search_by_order_id_index(309317338);
        data_search_by_order_id_index(380507028);
        data_search_by_order_id_index(387733113);
        data_search_by_order_id_index(504055583);
        data_search_by_order_id_index(598814380);
        data_search_by_order_id_index(954955518);
        data_search_by_order_id_index(970755660);
	data_search_by_order_id_index(999787032);
		
	data_search_by_order_id_index(999345433); /* BUG */
        data_search_by_order_id_index(421321);
        data_search_by_order_id_index(123123);
    }

    return 0;
}
