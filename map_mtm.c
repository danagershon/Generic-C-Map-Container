#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map_mtm.h"

// structs

typedef struct node_t{
    MapDataElement data;
    MapKeyElement key;
    struct node_t *next;
}*Node;

struct Map_t{
    Node iterator;
    // number of keys in the map
    int size;
    // pointer to a dummy node which is the first node of the list of key-data
    // elements
    Node first;
    copyMapDataElements copy_data;
    copyMapKeyElements copy_key;
    freeMapDataElements free_data;
    freeMapKeyElements free_key;
    compareMapKeyElements compare_keys;
};

// additional functions declarations

/**
* freeNode: deallocating all the fields of node (key and data) using supplied
 *          free_data, free_key functions
* @param map - target map which stores the deallocating functions
* @param node - the node to be deallocated
*/
static void freeNode(Map map,Node node);

/**
* freeList: iterating through the list and deallocating every node
* @param map - target map from which to delete the list of key-data elements
* @param list - the list of nodes which hold the key-data elements
*/
static void freeList(Map map,Node list);

/**
* createNewList: creating a list with "length" number of nodes
* @param length - number of nodes to create for the new list
* @return
*    pointer to the first node of the list if creating the list was successful
*    NULL if memory allocation of a node failed
*/
static Node createNewList(int length);

/**
* copyList: create a copy of the list from src_map and put it as the list
 *          of new_map
* @param src_map - the map whose list is copied
* @param new_map - the map that the list is copied to
* @return
*    MAP_OUT_OF_MEMORY if any memory allocation fails
*    MAP_SUCCESS if copying the list is successful
*/
static MapResult copyList(Map src_map,Map new_map);

/**
* findPrevNode: searching for the node that should be previous to "element"
*               whilst maintaining the lexicographic order of the map, and
*               storing the previous node address in prev_node
* @param map - the map that holds the list of key-data elements
* @param prev_node - a variable to store the address of the previous node
* @param element - the key element which we search for it's lexicographical
*                  previous node
* @return
*    true if the element exists in the map
*    false if the element does not exist in the map
*/
static bool findPrevNode(Map map,Node *prev_node,MapKeyElement element);

/**
* createNewNode: allocating dynamic memory for a new node and it's fields
 *               and copying the key-data elements into the node's fields
* @param map - the map that holds pointers to the copy and free functions of
*              key and data elements
* @param keyElement - pointer to the key element to be copied into the new node
* @param dataElement - pointer to the data element to be copied into the new
*                      node
* @return
*    pointer to the new mode if creating the node was successful
*    NULL if memory allocation failed
*/
static Node createNewNode(Map map,MapKeyElement keyElement,
                          MapDataElement dataElement);

/**
* insertNewNode: inserting (in an existing list) new_node after prev_node
* @param prev_node - pointer to the lexicographical previous node of new_node
*                    in the list
* @param new_node - pointer to the node that should be inserted after prev_node
*/
static void insertNewNode(Node prev_node,Node new_node);

/**
* initializeMap: initialize the fields of the map with given parameters
* @param map - target map to initialize it's fields
* @param copyDataElement - pointer to the data element copying function
* @param copyKeyElement - pointer to the key element copying function
* @param freeDataElement - pointer to the data element deallocating function
* @param freeKeyElement - pointer to the key element deallocating function
* @param compareKeyElements - pointer to the key comparing function
* @param dummy_first - pointer to the previously allocated dummy first node
*/
static void initializeMap(Map map,copyMapDataElements copyDataElement,
                          copyMapKeyElements copyKeyElement,
                          freeMapDataElements freeDataElement,
                          freeMapKeyElements freeKeyElement,
                          compareMapKeyElements compareKeyElements,
                          Node dummy_first);

// functions implementations

static void freeNode(Map map,Node node)
{
    map->free_data(node->data);
    map->free_key(node->key);
    free(node);
}

static void freeList(Map map,Node list)
{
    Node cur_node = list,temp;

    while(cur_node != NULL){
        temp = cur_node;
        cur_node = cur_node->next;
        freeNode(map,temp);
    }
}

static Node createNewList(int length)
{
    Node new_list_head = NULL;

    while(length--){
        Node new_node = malloc(sizeof(*new_node));
        if(!new_node){
            free(new_list_head);
            return NULL;
        }
        new_node->next = new_list_head;
        new_list_head = new_node;
    }

    return new_list_head;
}

static MapResult copyList(Map src_map,Map new_map)
{
    if(!src_map || !new_map){
        return MAP_NULL_ARGUMENT;
    }

    Node new_list_head = createNewList(src_map->size);
    if(!new_list_head){
        return MAP_OUT_OF_MEMORY;
    }

    Node new_list_cur = new_list_head;
    Node src_list_cur = src_map->first->next;

    while(src_list_cur != NULL){
        new_list_cur->data = src_map->copy_data(src_list_cur->data);
        new_list_cur->key = src_map->copy_key(src_list_cur->key);
        if(!(new_list_cur->data) || !(new_list_cur->key)){
            freeList(src_map,new_list_head);
            return MAP_OUT_OF_MEMORY;
        }
        src_list_cur = src_list_cur->next;
        new_list_cur = new_list_cur->next;
    }

    new_map->first->next = new_list_head;
    return MAP_SUCCESS;
}

static bool findPrevNode(Map map,Node *prev_node,MapKeyElement element)
{
    Node cur_node = map->first,next;
    bool was_found = false;

    while(cur_node != NULL){
        next = cur_node->next;
        if(!next){
            was_found = false;
            break;
        }
        int result = map->compare_keys(element,next->key);
        if(result == 0){
            was_found = true;
            break;
        }
        if(result < 0){
            was_found = false;
            break;
        }
        cur_node = next;
    }

    *prev_node = cur_node;
    return was_found;
}

static Node createNewNode(Map map,MapKeyElement keyElement,
                          MapDataElement dataElement)
{
    Node new_node = malloc(sizeof(*new_node));
    if(!new_node){
        return NULL;
    }
    new_node->key = map->copy_key(keyElement);
    new_node->data = map->copy_data(dataElement);
    if(!(new_node->key) || !(new_node->data)){
        freeNode(map,new_node);
        return NULL;
    }

    return new_node;
}

static void insertNewNode(Node prev_node,Node new_node)
{
    new_node->next = prev_node->next;
    prev_node->next = new_node;
}

static void initializeMap(Map map,copyMapDataElements copyDataElement,
                          copyMapKeyElements copyKeyElement,
                          freeMapDataElements freeDataElement,
                          freeMapKeyElements freeKeyElement,
                          compareMapKeyElements compareKeyElements,
                          Node dummy_first)
{
    map->copy_data = copyDataElement;
    map->copy_key = copyKeyElement;
    map->free_data = freeDataElement;
    map->free_key = freeKeyElement;
    map->compare_keys = compareKeyElements;
    map->size = 0;
    map->first = dummy_first;
    map->first->key = NULL;
    map->first->data = NULL;
    map->first->next = NULL;
    map->iterator = map->first;
}

Map mapCreate(copyMapDataElements copyDataElement,
              copyMapKeyElements copyKeyElement,
              freeMapDataElements freeDataElement,
              freeMapKeyElements freeKeyElement,
              compareMapKeyElements compareKeyElements)
{
    if(!copyDataElement || !copyKeyElement || !freeDataElement ||
       !freeKeyElement || !compareKeyElements){
        return NULL;
    }

    Map map = malloc(sizeof(*map));
    if(!map){
        return NULL;
    }
    Node dummy_first = malloc(sizeof(*dummy_first));
    if(!dummy_first){
        free(map);
        return NULL;
    }

    initializeMap(map,copyDataElement,copyKeyElement,freeDataElement,
                  freeKeyElement,compareKeyElements,dummy_first);

    return map;
}

void mapDestroy(Map map)
{
    if(!map){
        return;
    }
    mapClear(map);
    free(map->first);
    free(map);
}

Map mapCopy(Map map)
{
    if(!map){
        return NULL;
    }

    Map map_copy = mapCreate(map->copy_data,map->copy_key,map->free_data,
                             map->free_key,map->compare_keys);
    if(!map_copy){
        return NULL;
    }

    map_copy->size = map->size;

    if(map->size != 0){
        int error_code = copyList(map,map_copy);
        if(error_code != MAP_SUCCESS){
            mapDestroy(map_copy);
            return NULL;
        }
    }

    map->iterator = NULL;
    map_copy->iterator = NULL;

    return map_copy;
}

int mapGetSize(Map map)
{
    if(!map){
        return -1;
    }
    return map->size;
}

bool mapContains(Map map,MapKeyElement element)
{
    if(!map || !element){
        return false;
    }

    Node prev_node = NULL;

    if(findPrevNode(map,&prev_node,element)){
        return true;
    }else{
        return false;
    }
}

MapResult mapPut(Map map,MapKeyElement keyElement,MapDataElement dataElement)
{
    if(!map || !keyElement || !dataElement){
        return MAP_NULL_ARGUMENT;
    }

    Node prev_node = NULL;

    if(findPrevNode(map,&prev_node,keyElement)){
        map->free_data(prev_node->next->data);
        prev_node->next->data = map->copy_data(dataElement);
        if(!(prev_node->next->data)){
            return MAP_OUT_OF_MEMORY;
        }
    }else{
        Node new_node = createNewNode(map,keyElement,dataElement);
        if(!new_node){
            return MAP_OUT_OF_MEMORY;
        }
        insertNewNode(prev_node,new_node);
        map->size += 1;
    }

    map->iterator = NULL;

    return MAP_SUCCESS;
}

MapDataElement mapGet(Map map, MapKeyElement keyElement)
{
    if(!map || !keyElement){
        return NULL;
    }

    Node prev_node = NULL;
    if(!findPrevNode(map,&prev_node,keyElement)){
        return NULL;
    }

    return prev_node->next->data;
}

MapResult mapRemove(Map map, MapKeyElement keyElement)
{
    if(!map || !keyElement){
        return MAP_NULL_ARGUMENT;
    }

    Node prev_node = NULL;
    if(!findPrevNode(map,&prev_node,keyElement)){
        return MAP_ITEM_DOES_NOT_EXIST;
    }
    Node node_to_remove = prev_node->next;
    prev_node->next = node_to_remove->next;
    freeNode(map,node_to_remove);

    map->size -= 1;
    map->iterator = NULL;

    return MAP_SUCCESS;
}

MapKeyElement mapGetFirst(Map map)
{
    if(!map || !(map->first->next)){
        return NULL;
    }
    map->iterator = map->first->next;
    return map->iterator->key;
}

MapKeyElement mapGetNext(Map map)
{
    if(!map || !(map->iterator)){
        return NULL;
    }

    if(!(map->iterator->next)){
        return NULL;
    }else{
        map->iterator = map->iterator->next;
    }

    return map->iterator->key;
}

MapResult mapClear(Map map)
{
    if(!map){
        return MAP_NULL_ARGUMENT;
    }

    freeList(map,map->first->next);
    map->first->next = NULL;
    map->size = 0;

    return MAP_SUCCESS;
}
