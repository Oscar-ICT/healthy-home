#include <cstddef>

// pragma once says only include once no matter how many times it is included in other files
#pragma once

// All decision tree's need nodes, here is the node class
class Node {
    public:
        int depth;
        bool predicted_class;
        int left_child; // index into the Node array cause memory contraints
        int right_child; // -1 = no child

        int feature_index;
        float threshold;

        // Determining if node is a leaf function
        bool isLeaf() const;
    private:
    protected:
};

bool Node::isLeaf() const {
    if ((left_child == -1) && (right_child == -1)){
        return true;
    }
    return false;
};

class DecisionTreeClassifier {
public:
    DecisionTreeClassifier(const Node* nodes, size_t nodeCount);

    int predict(const float* features) const;

private:
    const Node* nodes;
    size_t nodeCount;
};

// This is for traversing the tree
int DecisionTreeClassifier::predict(const float* features) const{
    int current_index = 0;
    while (!nodes[current_index].isLeaf()) {
        if (features[nodes[current_index].feature_index] <= nodes[current_index].threshold){
            current_index = nodes[current_index].left_child;
        }
        else{
            current_index = nodes[current_index].right_child;
        }
    }
    return nodes[current_index].predicted_class;
};

// Equivilant to the def __init__(x) self.x = x in python
DecisionTreeClassifier::DecisionTreeClassifier(const Node* nodes, size_t nodeCount)
    : nodes(nodes), nodeCount(nodeCount)
{
};