// check if that was the only element in the list
        if (temp->next == NULL) {
            list->head = NULL;
            list->last = NULL;
            free(temp);
            return;
        }