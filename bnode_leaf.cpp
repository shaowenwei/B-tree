#include "bnode_leaf.h"
#include <vector>

using namespace std;

Bnode_leaf::~Bnode_leaf() {
    // Remember to deallocate memory!!

}

VALUETYPE Bnode_leaf::merge(Bnode_leaf* rhs) {
    assert(num_values + rhs->getNumValues() < BTREE_LEAF_SIZE);

    VALUETYPE retVal = rhs->get(0);

    Bnode_leaf* save = next;
    next = next->next;
    if (next) next->prev = this;

    for (int i = 0; i < rhs->getNumValues(); ++i)
        insert(rhs->getData(i));

    rhs->clear();
    return retVal;
}

VALUETYPE Bnode_leaf::redistribute(Bnode_leaf* rhs) {
    assert(num_values + rhs->getNumValues() < BTREE_LEAF_SIZE);
    vector<Data*> all_values(values, values + num_values);

    int rhs_num = rhs->getNumValues();
    for(int i = 0; i != rhs_num; ++i){
        all_values.push_back(rhs->getData(i));
    }
    clear();
    rhs->clear();

    for (int i = 0; i < all_values.size()/2; ++i)
        insert(all_values[i]);

    for (int i = all_values.size()/2; i < all_values.size(); ++i)
        rhs->insert(all_values[i]);

    VALUETYPE res = all_values[all_values.size()/2]->value;

    return res;
}

Bnode_leaf* Bnode_leaf::split(VALUETYPE insert_value) {
    assert(num_values == BTREE_LEAF_SIZE);

    // Populate an intermediate array with all the values/children before splitting - makes this simpler
    vector<Data*> all_values(values, values + num_values);

    Bnode_leaf* split_node = new Bnode_leaf;
    clear();

    for (int i = 0; i < BTREE_LEAF_SIZE/2; ++i)
        insert(all_values[i]);

    VALUETYPE output_val = all_values[BTREE_LEAF_SIZE/2]->value;

    for (int i = BTREE_LEAF_SIZE/2; i < all_values.size(); ++i)
        split_node->insert(all_values[i]);

    if(insert_value < output_val){
        insert(insert_value);
    }
    else{
        split_node->insert(insert_value);
    }

    // I like to do the asserts :)
    assert(num_values == BTREE_LEAF_SIZE/2);
    assert(split_node->getNumValues() == BTREE_LEAF_SIZE/2 + 1);

    split_node->parent = parent; // they are siblings

    split_node->prev = this; 
    split_node->next = this->next;
    this->next = split_node;


    return split_node;
}


