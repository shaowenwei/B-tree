#include "bnode_inner.h"
#include "bnode_leaf.h"
#include <vector>
#include <iostream>
using namespace std;

VALUETYPE Bnode_inner::merge(Bnode_inner* rhs, int parent_idx) {
    cout<<"IN INNER MERGE. "<<endl;
    assert(rhs->parent == parent); // can only merge siblings
    //assert(rhs->num_values > 0);
    int numval = rhs->getNumValues();
    int numChd = rhs->getNumChildren();
    // cout<<"rhs-numval: "<<numval<<endl;
    // cout<<"rhs-numChd: "<<numChd<<endl;
    // cout<<"lhs-numval: "<<getNumValues()<<endl;
    // cout<<"lhs-numChd: "<<getNumChildren()<<endl;
    int lhs_num = getNumChildren();
    //remove_value(num_values - 1);
    //remove_child(num_children - 1);

    Bnode* tmp = rhs->getChild(0);
    //Bnode_inner* inner = dynamic_cast<Bnode_inner*>(tmp);
    VALUETYPE val;
    while(1)
    {
        Bnode_leaf* inner = dynamic_cast<Bnode_leaf*>(tmp);
        if(inner){
            Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(tmp);
            val = leaf->get(0);
            break;
        }
        else
        {   
            cout<<"inner"<<endl;
            tmp = dynamic_cast<Bnode_inner*>(tmp)->getChild(0);
        }

    }
    // if(inner){
    //     tmp = dynamic_cast<Bnode_inner*>(tmp);
    //     val = inner->get(0);
    // }
    // else{
    //     Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(tmp);
    //     val = leaf->get(0);
    // }

    cout<<"val:"<<val<<endl;
    insert(val);
    // cout<<"2-lhs-numval: "<<getNumValues()<<endl;
    // cout<<"2-lhs-numChd: "<<getNumChildren()<<endl;

    for(int i = 0; i < numval; ++i){
        insert(rhs->get(i));
    }
    // cout<<"3-lhs-numval: "<<getNumValues()<<endl;
    // cout<<"3-lhs-numChd: "<<getNumChildren()<<endl;
    for(int i = 0; i < numChd; ++i){
        insert(rhs->getChild(i), lhs_num + i);
        rhs->getChild(i)->parent = this;
    }
    // cout<<"4-lhs-numval: "<<getNumValues()<<endl;
    cout<<"4-lhs-numChd: "<<getNumChildren()<<endl;
    rhs->clear();
    return parent->values[parent_idx];

}

VALUETYPE Bnode_inner::redistribute(Bnode_inner* rhs, int parent_idx) {
    assert(rhs->parent == parent); // inner node redistribution should only happen with siblings
    assert(parent_idx >= 0);
    assert(parent_idx < parent->getNumValues());

    vector<VALUETYPE> all_values(values, values + num_values);
    vector<Bnode*> all_children(children, children + num_children);

    int rhs_num = rhs->getNumValues();
    //cout<<"rhs_num"<<rhs_num<<endl;
    int rhs_chd = rhs->getNumChildren();
    //cout<<"rhs_chd"<<rhs_chd<<endl;
    int lhs_num = getNumValues();
    //cout<<"lhs_num"<<lhs_num<<endl;
    int lhs_chd = getNumChildren();
    //cout<<"lhs_chd"<<lhs_chd<<endl;
    VALUETYPE value;
    Bnode_inner* test = dynamic_cast<Bnode_inner*>(rhs->getChild(0));
    if(test) value = test->get(0);
    else
    {
        Bnode_leaf* leaf_test = dynamic_cast<Bnode_leaf*>(rhs->getChild(0));
        value = leaf_test->get(0);
    }
    int temp;
    if(rhs_chd > lhs_chd) temp = 0;
    else temp = 1;
    //cout<<"temp: "<<temp<<endl;
    

    for(int i = 0; i != rhs_num; ++i){
        all_values.push_back(rhs->get(i));
    }
    for(int i = 0; i != rhs_chd; ++i){
        all_children.push_back(rhs->getChild(i));
    }

    clear();
    rhs->clear();

    for (int i = 0; i < all_values.size()/2; ++i)
        insert(all_values[i]);

    for (int i = all_values.size()/2; i < all_values.size(); ++i)
        rhs->insert(all_values[i]);

    for (int i = 0; i < all_children.size()/2; ++i){
        insert(all_children[i], i);
        all_children[i]->parent = this;
    }

    for (int i = all_children.size()/2; i < all_children.size(); ++i){
        rhs->insert(all_children[i], i - all_children.size()/2);
        all_children[i]->parent = rhs;
    }
    //cout<<"rep"<<endl;

    // cout<<"rhs_num"<<rhs->getNumValues()<<endl;
    // cout<<"rhs_chd"<<rhs->getNumChildren()<<endl;
    
    // cout<<"lhs_num"<<getNumValues()<<endl;
    // cout<<"lhs_chd"<<getNumChildren()<<endl;

    if(temp == 0)
        replace_value(value, getNumValues() - 1);
    else
        rhs->replace_value(value, rhs->getNumValues() - rhs_num - 1);
    VALUETYPE res = all_values[all_values.size()/2];
    if(rhs->parent == parent){
        int idx = parent->find_child(rhs);
        parent->replace_value(res, idx - 1);
    }
    return res;
}


Bnode_inner* Bnode_inner::split(VALUETYPE& output_val, VALUETYPE insert_value, Bnode* insert_node) {
    assert(num_values == BTREE_FANOUT-1); // only split when it's full!

    // Populate an intermediate array with all the values/children before splitting - makes this simpler
    vector<VALUETYPE> all_values(values, values + num_values);
    vector<Bnode*> all_children(children, children + num_children);

    // Insert the value that created the split
    int ins_idx = find_value_gt(insert_value);
    all_values.insert(all_values.begin()+ins_idx, insert_value);
    all_children.insert(all_children.begin()+ins_idx+1, insert_node);

    // Do the actual split into another node
    Bnode_inner* split_node = new Bnode_inner;

    assert(all_values.size() == BTREE_FANOUT);
    assert(all_children.size() == BTREE_FANOUT+1);

    // Give the first BTREE_FANOUT/2 values to this bnode
    clear();
    for (int i = 0; i < (BTREE_FANOUT - 1)/2; ++i)
        insert(all_values[i]);
    for (int i = 0, idx = 0; i < (BTREE_FANOUT/2) + 1; ++i, ++idx) {
        insert(all_children[i], idx);
        all_children[i] -> parent = this;
    }

    // Middle value should be pushed to parent
    output_val = all_values[BTREE_FANOUT/2];

    // Give the last BTREE/2 values to the new bnode
    for (int i = (BTREE_FANOUT + 1)/2; i < all_values.size(); ++i)
        split_node->insert(all_values[i]);
    for (int i = (BTREE_FANOUT/2) + 1, idx = 0; i < all_children.size(); ++i, ++idx) {
        split_node->insert(all_children[i], idx);
        all_children[i] -> parent = split_node;
    }

    // I like to do the asserts :)
    assert(num_values == BTREE_FANOUT/2);
    assert(num_children == num_values+1);
    assert(split_node->getNumValues() == BTREE_FANOUT/2);
    assert(split_node->getNumChildren() == num_values + 1);

    split_node->parent = parent; // they are siblings
    //cout<<"///////"<<parent->getNumChildren()<<endl;

    return split_node;
}
