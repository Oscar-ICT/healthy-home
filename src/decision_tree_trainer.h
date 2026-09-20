#include <cstddef>
#include <cstdint>
#include <string>
#include <cmath>

#include "decision_tree.h"
#pragma once

/*
short press is for changing states while in training
long press is just for the finalisation of gathering training data
*/

// This is a template
template<size_t MaxNodes, size_t NumFeatures>
class DecisionTreeTrainer {
private:
    float sampleFeatures[MaxNodes * NumFeatures];
    uint8_t sampleLabels[MaxNodes];
    size_t sample_count = 0;

    Node node_storage[MaxNodes];
    size_t node_count = 0;

    struct SplitResult {
        size_t feature_index;
        float threshold;
        float gain;
    };

    float calculateEntropy(uint8_t* labels, size_t count)
    {
    // GIVEN A SET OF SAMPLE INDICES, COMPUTE SHANNON ENTROPY BASED ON THE LABEL 
    // DISTRIBUTION AMONG THEM. MUST RETURN float
    // Now this one is mathy, and all thanks to claude shannons information theory (wiki page for entropy is not a nice read lol)
        size_t zeros_count = 0;
        size_t ones_count = 0;    

        for (size_t i = 0; i < count; i++){
            if (labels[i] == 0){
                zeros_count = zeros_count + 1;
            }
            else
            {
                ones_count = ones_count + 1;
            }

        }

        // After we have counted all of the zeros and ones we convert those into probabilities
        // pretty simpel probability equation of p_i = count_i / total_count SIMPLES
        float probability_for_zero = static_cast<float>(zeros_count) / count;
        float probability_for_one = static_cast<float>(ones_count) / count;

        float shannon_entropy = 0.0f;

        // but the entropy's equation is ---> entropy = -Σ (p_i * log2(p_i))
        // the negative sum, for each class, of that class's probability multiplied by log base two of that class's probability
        if (probability_for_zero != 0) {
            shannon_entropy = shannon_entropy - (probability_for_zero * log2(probability_for_zero));
        }

        if (probability_for_one != 0) {
            shannon_entropy = shannon_entropy - (probability_for_one * log2(probability_for_one));
        }

        return shannon_entropy;
    }

    float calculateInformationGain(
        const float* allFeatures,     // full stored feature data
        const uint8_t* allLabels,     // full stored labels
        const size_t* sampleIndices,  // which samples belong to this node
        size_t sampleIndexCount,      // how many indices
        size_t featureIndex,          // which feature to test
        float threshold               // where to split
        )
    {
    // GIVEN A PARENT SET OF INDICES AND A PROPOSED SPLIT (FEATURE + THRESHOLD), 
    // COMPUTE PARENT ENTROPY MINUS WEIGHTED CHILD ENTROPY. MUST RETURN float
    // basically run calculate entropy and then subtract the weighted sum of the two children to the parents entropy to figure the gain
        uint8_t parentLabels[MaxNodes];
        for (size_t i = 0; i < sampleIndexCount; i++) {
            parentLabels[i] = allLabels[sampleIndices[i]];
        }
        float parent_entropy = calculateEntropy(parentLabels, sampleIndexCount); 
        
        size_t leftIndices[MaxNodes];
        size_t rightIndices[MaxNodes];
        size_t leftCount = 0;
        size_t rightCount = 0;
        
        // Split into left and right and return the entropy of the two children
        for (size_t i = 0; i < sampleIndexCount; i++){
            float currentValue = allFeatures[sampleIndices[i] * NumFeatures + featureIndex];
            if (currentValue <= threshold){
                leftIndices[leftCount] = sampleIndices[i];
                leftCount = leftCount + 1;
            }
            else{
                rightIndices[rightCount] = sampleIndices[i];
                rightCount = rightCount + 1;
            }
        }

        // For the right
        uint8_t rightLabels[MaxNodes];
        for (size_t i = 0; i < rightCount; i++) {
            rightLabels[i] = allLabels[rightIndices[i]];
        }
        // aannnd for the left
        uint8_t leftLabels[MaxNodes];
        for (size_t i = 0; i < leftCount; i++) {
            leftLabels[i] = allLabels[leftIndices[i]];
        }

        // Calculating the childrens information entorpy
        float right_entropy = calculateEntropy(rightLabels, rightCount);
        float left_entropy = calculateEntropy(leftLabels, leftCount);
        
        // Now we have actually gathered the values here is the information gain calculation
        float weightedChildEntropy = (static_cast<float>(leftCount) / sampleIndexCount) * left_entropy +
         (static_cast<float>(rightCount) / sampleIndexCount) * right_entropy;
        // THis is the actual infomration gain
        return parent_entropy - weightedChildEntropy;

    }

    SplitResult findBestSplit(
        const float* allFeatures,
        const uint8_t* allLabels,
        const size_t* sampleIndices,
        size_t sampleIndexCount
        )
    {
    // GIVEN A SET OF SAMPLE INDICES, TRY EVERY FEATURE AND CANDIDATE THRESHOLD, 
    // RETURN WHICHEVER (feature_index, threshold) GIVES MAX INFORMATION GAIN. MUST RETURN 
    // SOME STRUCT/PAIR OF (feature_index, threshold, gain)
        float bestGain = -1.0f;
        SplitResult best = {0, 0.0f, -1.0f};;

        for (size_t feature_index = 0; feature_index < NumFeatures; feature_index++){
            for (size_t j = 0; j < sampleIndexCount; j++){
                // Start info entropying here?
                float currentThreshold = allFeatures[sampleIndices[j] * NumFeatures + feature_index];
                float currentGain = calculateInformationGain(allFeatures, allLabels, sampleIndices, sampleIndexCount, feature_index, currentThreshold);
                if (currentGain > bestGain){
                    bestGain = currentGain;

                    best.feature_index = feature_index;
                    best.threshold = currentThreshold;
                    best.gain = currentGain;
                }
            }
        }
        return best;
    }

    int buildNode(
        const float* allFeatures,
        const uint8_t* allLabels,
        const size_t* sampleIndices,
        size_t sampleIndexCount,
        size_t currentDepth,
        size_t minSamplesToSplit,
        size_t maxDepth
        )
    {
    // GIVEN A SET OF SAMPLE INDICES + CURRENT DEPTH, DECIDE: STOP AND 
    // MAKE A LEAF (via majority label), OR SPLIT USING findBestSplit AND RECURSE 
    // INTO LEFT/RIGHT SUBSETS. MUST RETURN int (the index in the nodes array where 
    // this node was placed)
    bool isPure = (sampleIndexCount > 0);
    uint8_t firstLabel = allLabels[sampleIndices[0]];
    for (size_t i = 1; i < sampleIndexCount; i++){
        if (allLabels[sampleIndices[i]] != firstLabel){
            isPure = false;
        }
    }
    bool shouldBeLeaf = (currentDepth >= maxDepth) || (sampleIndexCount < minSamplesToSplit) || (isPure);
    if(shouldBeLeaf){
        size_t zerosCount = 0;
        size_t onesCount = 0;
        for (size_t i = 0; i < sampleIndexCount; i++){
            if (allLabels[sampleIndices[i]] == 0){
                zerosCount = zerosCount + 1;
            }
            else{
                onesCount = onesCount + 1;}}
        
        uint8_t majorityLabel = (onesCount > zerosCount) ? 1 : 0;
        node_storage[node_count].depth = currentDepth;
        node_storage[node_count].predicted_class = majorityLabel;
        node_storage[node_count].left_child = -1;
        node_storage[node_count].right_child = -1;

        int thisNodeIndex = node_count;
        node_count = node_count + 1;
        return thisNodeIndex;
    }
    else{
        SplitResult split = findBestSplit(allFeatures, allLabels, sampleIndices, sampleIndexCount);
        size_t leftIndices[MaxNodes];
        size_t rightIndices[MaxNodes];
        size_t leftCount = 0;
        size_t rightCount = 0;

        for (size_t i = 0; i < sampleIndexCount; i++){
            float value = allFeatures[sampleIndices[i] * NumFeatures + split.feature_index];
            if (value <= split.threshold){
                leftIndices[leftCount] = sampleIndices[i];
                leftCount = leftCount + 1;
            }
            else{
                rightIndices[rightCount] = sampleIndices[i];
                rightCount = rightCount + 1;
            }
        }
        int leftChildIndex = buildNode(allFeatures, allLabels, leftIndices, leftCount, currentDepth + 1, minSamplesToSplit, maxDepth);
        int rightChildIndex = buildNode(allFeatures, allLabels, rightIndices, rightCount, currentDepth + 1, minSamplesToSplit, maxDepth);

        node_storage[node_count].depth = currentDepth;
        node_storage[node_count].feature_index = split.feature_index;
        node_storage[node_count].threshold = split.threshold;
        node_storage[node_count].left_child = leftChildIndex;
        node_storage[node_count].right_child = rightChildIndex;

        int thisNodeIndex = node_count;
        node_count = node_count + 1;
        return thisNodeIndex;
    }}

public:
    bool addSample(float* features, uint8_t label)
    {
    // ADD ONE SAMPLE (features + label) TO THE INTERNAL STORAGE ARRAY, 
    // IF ROOM REMAINS. MUST RETURN bool (success/fail)
    if (sample_count >= MaxNodes){
        return false;
    }
    else
    {
        for (size_t i = 0; i < NumFeatures; i++) {
            sampleFeatures[sample_count * NumFeatures + i] = features[i];            
        }

        sampleLabels[sample_count] = label;        
        sample_count = sample_count + 1;
        return true;
    }

    }

    size_t sampleCount() const
    {
        // RETURN HOW MANY SAMPLES HAVE BEEN ADDED SO FAR. MUST RETURN size_t
        return sample_count;
    }

    size_t train(size_t minSamplesToSplit, size_t maxDepth)
    {
    // ENTRY POINT: RESET nodeCount, CALL buildNode ON THE FULL SET OF SAMPLE INDICES 
    // STARTING AT DEPTH 0. MUST RETURN size_t (total nodes used)
    node_count = 0;
    
    size_t allIndices[MaxNodes];
    for (size_t i = 0; i < sample_count; i++){
        allIndices[i] = i;
    }
    buildNode(sampleFeatures, sampleLabels, allIndices, sample_count, 0, minSamplesToSplit, maxDepth);

    return node_count;
    }

    const Node* nodes() const
    {
    // RETURN A POINTER TO THE INTERNAL NODE ARRAY (READ-ONLY) SO DecisionTreeClassifier 
    // CAN USE IT. MUST RETURN const Node*
    return node_storage;
    }
};