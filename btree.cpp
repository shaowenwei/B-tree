#include "btree.h"
#include "bnode.h"
#include "bnode_inner.h"
#include "bnode_leaf.h"

#include <cassert>

using namespace std;

const int LEAF_ORDER = BTREE_LEAF_SIZE/2;
const int INNER_ORDER = (BTREE_FANOUT-1)/2;

Btree::Btree() : root(new Bnode_leaf), size(0) {
    // Fill in here if needed
}

Btree::~Btree() {
    // Don't forget to deallocate memory
}

// find which leaf_node should be inserted in
Bnode_leaf* Btree::find_leaf_node(Bnode* root, VALUETYPE value){
    assert(root);
    Bnode* current = root;

    Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);

    // Have not reached a leaf node yet
    while (inner) {
        int find_index = inner->find_value_gt(value);
        current = inner->getChild(find_index);
        inner = dynamic_cast<Bnode_inner*>(current);
    }

    Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
    return leaf;
}

bool Btree::insert(VALUETYPE value) {
    if(search(value) != nullptr){
        return false;
    }
    size++; // if insert successfully

    if(size <= BTREE_LEAF_SIZE) // start stage
    {
        Bnode_leaf* cur = dynamic_cast<Bnode_leaf*>(root);
        cur->insert(value);
        return true;
    }

    Bnode_leaf* leaf = find_leaf_node(root, value); // leaf node where should be inserted in
    int NumVal = leaf->getNumValues();
    if(NumVal < BTREE_LEAF_SIZE){
        leaf->insert(value);
    }
    else{
        Bnode_leaf* new_leaf = leaf->split(value); // new_leaf after split(latter one)
        VALUETYPE head = new_leaf->get(0); // value which should be pop up to parent
        Bnode_inner* pre_node = leaf->parent;// parent node
        while(1){
            if(pre_node->getNumChildren() < BTREE_FANOUT){ // if not parent node isn't overflow
                int idx = pre_node->insert(head);
                pre_node->insert(new_leaf, idx + 1); // insert new_leaf as a child
                break;
            }
            else{ //if parent is overflow
                int new_head = -1; //the value should be push up to parent
                Bnode_inner* new_node = pre_node->split(new_head, head, new_leaf);
                if(pre_node->parent == nullptr){ // if parent is the root
                    Bnode_inner* new_root = new Bnode_inner;
                    new_root->insert(new_head);
                    new_root->insert(pre_node, 0);
                    new_root->insert(new_node, 1);
                    new_node->parent = new_root;
                    pre_node->parent = new_root;
                    root = new_root;
                    break;
                }
                head = new_head;
                pre_node = new_node->parent;
            }
        }
    }
    return true;
}

bool Btree::remove(VALUETYPE value) {
    if(search(value) == nullptr) // if value does not exist
    {
        return false;
    }
    size--; // if remove successfully;

    Bnode_leaf* leaf = find_leaf_node(root, value);
    int DOM = -1;  //1:merge, 0:distribute
    Bnode_leaf* left;
    Bnode_leaf* right;
    if(leaf->getNumValues() - 1 >= BTREE_LEAF_SIZE/2) //if leaf can be removed
    {
        leaf->remove(value);
    }
    else // can not be removed directly
    {
        if(leaf->next == nullptr) // rightmost
        {
            left = leaf->prev;
            right = leaf;
            if(left->getNumValues() >= BTREE_LEAF_SIZE/2 + 1)
                DOM = 0;
            else
                DOM = 1;
        }
        else if(leaf->prev == nullptr) // leftmost
        {
            left = leaf;
            right = leaf->next;
            if(right->getNumValues() >= BTREE_LEAF_SIZE/2 + 1)
                DOM = 0;
            else
                DOM = 1;
        }
        else // middle
        {
            if(leaf->next->getNumValues() >= BTREE_LEAF_SIZE/2 + 1)
            {
                DOM = 0;
                left = leaf;
                right = leaf->next;
            }
            else if(leaf->prev->getNumValues() >= BTREE_LEAF_SIZE/2 + 1)
            {
                DOM = 0;
                left = leaf->prev;
                right = leaf;
            }
            else //merge
            {
                DOM = 1;
                left = leaf;
                right = leaf->next;
            }
        }
        if(DOM == 0) //redistribute
        { 
            VALUETYPE res = left->redistribute(right);
            modify_ancestor(res, right);
        }
        else //merge
        { 
            Bnode_inner* right_parent = right->parent;
            VALUETYPE ret = left->merge(right);
            int val_idx = right_parent->find_value(ret);
            right_parent->remove_value(val_idx);
            int idx = right_parent->find_child(right);
            right_parent->remove_child(idx);
            if(left->parent != right_parent) // is not sibling
            {
                modify_ancestor(left->next->get(0), left->next);
            }
            Bnode_inner* left_inner = right_parent;
            Bnode_inner* right_inner;
            while(right_parent->getNumValues() < (BTREE_FANOUT - 1)/2) // when right parent is not half full
            {
                Bnode_inner* pop = right_parent->parent;
                if(pop == nullptr) // if reach root
                {
                    if(right_parent->getNumValues() == 0) // if root is empty
                    {
                        right_parent->clear();
                        right_parent->remove_child(0);
                        left_inner->parent = nullptr;
                        root = left_inner;
                    }
                    break;
                }
                int p_idx = pop->find_child(right_parent);
                int pop_chd = pop->getNumChildren();

                if(p_idx == 0) //if is leftmost
                {
                    left_inner =  right_parent;
                    right_inner = dynamic_cast<Bnode_inner*>(pop->getChild(p_idx + 1));
                    if(right_inner->getNumValues() > (BTREE_FANOUT - 1)/2)
                    {
                        DOM = 0;//redistribute
                    }
                    else DOM = 1; // merge
                }
                else if(p_idx == pop_chd - 1) // is rightmost
                {
                    left_inner =  dynamic_cast<Bnode_inner*>(pop->getChild(p_idx - 1));
                    right_inner = right_parent;
                    if(left_inner->getNumValues() > (BTREE_FANOUT - 1)/2)
                    {
                        DOM = 0;//redistribute
                    }
                    else DOM = 1; //merge
                }
                else //middle
                {
                    Bnode_inner* v1 = dynamic_cast<Bnode_inner*>(pop->getChild(p_idx + 1));
                    Bnode_inner* v2 = dynamic_cast<Bnode_inner*>(pop->getChild(p_idx - 1));
                    if(v1->getNumValues() > (BTREE_FANOUT - 1)/2)
                    {
                        DOM = 0;
                        left_inner = right_parent;
                        right_inner = v1;
                    }
                    else if(v2->getNumValues() > (BTREE_FANOUT - 1)/2)
                    {
                        DOM = 0;
                        left_inner = v2;
                        right_inner = right_parent;
                    }
                    else{
                        DOM = 1; // merge
                        left_inner = right_parent;
                        right_inner = v1;
                    }
                }
                if(DOM == 0) // redistribution
                {
                    VALUETYPE res_dis = left_inner->redistribute(right_inner, right_inner->parent->find_child(right_inner) - 1);
                    modify_ancestor(res_dis, right_inner);
                    break; // get out of loop
                }
                else // merge
                {
                    VALUETYPE res_dis = left_inner->merge(right_inner, right_inner->parent->find_child(right_inner) - 1);
                    int val_idx = left_inner->parent->find_value(res_dis);
                    left_inner->parent->replace_value(res_dis, val_idx);
                    int idx = left_inner->parent->find_child(right);
                    left_inner->parent->remove_child(idx);
                    right_parent = left_inner->parent;
                }
            }
        }
    }
    return true;
}

vector<Data*> Btree::search_range(VALUETYPE begin, VALUETYPE end) {
    vector<Data*> returnValues;
    Bnode_leaf* leaf_begin = find_leaf_node(root, begin);
    int find = 0;

    while(find == 0)
    {
        int num_val = leaf_begin->getNumValues();
        for(int i = 0; i != num_val; ++i){
            VALUETYPE tmp = leaf_begin->get(i); 
            if(tmp >= begin && tmp <= end)
                returnValues.push_back(leaf_begin->getData(i));
            if(tmp > end){
                find = 1;
                break;
            }
        }
        leaf_begin = leaf_begin->next;
        if(leaf_begin == nullptr)
            find = 1;
    }

    return returnValues;
}

//
// Given code
//
Data* Btree::search(VALUETYPE value) {
    assert(root);
    Bnode* current = root;

    // Have not reached a leaf node yet
    Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
    // A dynamic cast <T> will return a nullptr if the given input is polymorphically a T
    //                    will return a upcasted pointer to a T* if given input is polymorphically a T
    while (inner) {
        int find_index = inner->find_value_gt(value);
        current = inner->getChild(find_index);
        inner = dynamic_cast<Bnode_inner*>(current);
    }

    // Found a leaf node
    Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
    assert(leaf);
    for (int i = 0; i < leaf->getNumValues(); ++i) {
        if (leaf->get(i) > value)    return nullptr; // passed the possible location
        if (leaf->get(i) == value)   return leaf->getData(i);
    }

    // reached past the possible values - not here
    return nullptr;
}

void modify_ancestor(int val, Bnode* rhs){
    Bnode_inner* par = rhs->parent;
    while(par->get(0) > val){
        par = par->parent;
    }
    int idx = par->find_value_gt(val);
    par->replace_value(val, idx-1);
}








