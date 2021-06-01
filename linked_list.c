#include "mipssim.h"

//* COMPLETE
struct node {
    int index_row;
    struct node* next;
};
typedef struct node node_t;

//* COMPLETE
struct Linked_list {
    node_t* head;
    node_t* last;
};
typedef struct Linked_list linkedlist;


//* COMPLETE
void print_content_list(linkedlist* list) {
    // if list is empty
    if (list->head == NULL && list->last == NULL) {
        printf("the list is empty\n");
    }

    node_t* temp = list->head;

    while(temp != NULL){
        printf("%d - ", temp->index_row);
        temp = temp->next;
    }
    printf("\n");
}

//* COMPLETE
node_t* create_node(int value_index) {

    node_t* pt = (node_t *) malloc(sizeof(node_t));
    pt->index_row = value_index;
    pt->next = NULL;
    return pt;
}

//* COMPLETE
void add_node(linkedlist* list, int value_node) {
    // if list is empty
    if (list->head == NULL && list->last == NULL) { // technically they must be at the same time
        node_t* new_node = create_node(value_node);
        list->head = new_node;
        list->last = new_node;

        return;
    }

    // if the linked list already has elements
    node_t* new_node = create_node(value_node);

    new_node->next = list->head;
    list->head = new_node;
}

//* COMPLETE
void delete_node(linkedlist* list, int value_node) {

    // * check if the given list is not empty
    if (list->head == NULL && list->last == NULL) {
        printf("could not remove node as list is empty\n");
        return;
    }

    // * check if there is only one element
    if (list->head == list->last) {
        if (list->head->index_row == value_node) {
            node_t* tofree = list->head;
            list->head = NULL;
            list->last = NULL;
            free(tofree);
            return;
        }
        printf("value to remove not in the list\n");
        return;
    }

    // * not it has at least two nodes
    node_t* temp = list->head;

    // in the case that we want to remove the first item of the list
    if (temp->index_row == value_node) {
        
        list->head = temp->next;
        free(temp);
        return;
    }

    // could remove the second to second to last node
    while (temp->next->next != NULL) {

        if (temp->next->index_row == value_node) {
            node_t* tofree = temp->next;
            temp->next = temp->next->next;
            free(tofree);
            return;
        }
        temp = temp->next;
    }

    // if this is executed you haven't found the value or it is the last one
    //if it is the last one
    if (list->last->index_row == value_node) {
        // you still have the pointer to the second last node
        node_t* tofree = list->last;
        list->last = temp;
        temp->next = NULL;
        free(tofree);
        return;
    }

    // at this point you haven't found the value
    printf("have not found value to remove in the list\n");

}

//* COMPLETE
int get_last_value(linkedlist* list) {
    if (list->head == NULL && list->last == NULL) {
        printf("the list is empty, there is no last value\n");
        return -1;
    }
    return list->last->index_row;
}

/*
// * this is called whenever we access memory and we found the data, so we set the given row as most recenetly used
//* COMPLETE
void update_LRU(int index_addr, int row_num) {

    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;

    // * in the case that the given row was already used in the past, it is already in the list, so we need to remove it
    delete_node(list_pt, row_num);

    // * then add it to the front
    add_node(list_pt, row_num);
}

//* COMPLETE
int get_num_row_to_evict(int index_addr) {
    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;

    int last_value = get_last_value(list_pt);

    update_LRU(index_addr, last_value); // because now that row is the most recently used

    return last_value;
}
*/
linkedlist* LRU_pt;

// * this is called whenever we access memory and we found the data, so we set the given row as most recenetly used
//* COMPLETE
void update_LRU(int index_addr, int row_num) {

    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;


    // * in the case that the given row was already used in the past, it is already in the list, so we need to remove it
    delete_node(list_pt, row_num);

    // * then add it to the front
    add_node(list_pt, row_num);


}


//* COMPLETE
int get_num_row_to_evict(int index_addr) {

    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;



    int last_value = get_last_value(list_pt);

    if (last_value == -1) {
        printf("you asked to evict a row when the row is free");
        assert(0);
    }

    update_LRU(index_addr, last_value); // because now that row is the most recently used



    return last_value;
}



int main() {
    
    int num_lists = 5;
    LRU_pt = (linkedlist*) malloc(sizeof(linkedlist) * num_lists);

    for (linkedlist *curr_list = LRU_pt; curr_list < LRU_pt + num_lists; curr_list++) {
        curr_list->head = NULL;
        curr_list->last = NULL;
    }

    int index = 0;
    int row_num = 7;




    add_node(LRU_pt, row_num);
    add_node(LRU_pt, row_num+1);
    add_node(LRU_pt, row_num+2);

    print_content_list(LRU_pt);

    get_num_row_to_evict(index);

    print_content_list(LRU_pt);


}