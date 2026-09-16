#include <cstddef>
#include <cstdint>
#include <string>

#include "decision_tree.h"
#pragma once


// This is a template
template<size_t MaxNodes, size_t NumFeatures>
class DecisionTreeTrainer {
    bool addSample(float *features, uint8_t label)
    {
    // ADD ONE SAMPLE (features + label) TO THE INTERNAL STORAGE ARRAY, 
    // IF ROOM REMAINS. MUST RETURN bool (success/fail)
    }

    size_t sampleCount() const
    {
    // RETURN HOW MANY SAMPLES HAVE BEEN ADDED SO FAR. MUST RETURN size_t
    }

    float calculateEntropy(uint8_t* labels, size_t count)
    {
    // GIVEN A SET OF SAMPLE INDICES, COMPUTE SHANNON ENTROPY BASED ON THE LABEL 
    // DISTRIBUTION AMONG THEM. MUST RETURN float
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
    }

    struct SplitResult {
        size_t feature_index;
        float threshold;
        float gain;
    };
    // GIVEN A SET OF SAMPLE INDICES, TRY EVERY FEATURE AND CANDIDATE THRESHOLD, 
    // RETURN WHICHEVER (feature_index, threshold) GIVES MAX INFORMATION GAIN. MUST RETURN 
    // SOME STRUCT/PAIR OF (feature_index, threshold, gain)
    

    const Node* nodes() const
    {

    }

    size_t train(size_t minSamplesToSplit, size_t maxDepth)
    {

    }

    int buildNode(const float* allFeatures, const uint8_t* allLabels, const size_t* sampleIndices, size_t sampleIndexCount, size_t currentDepth, size_t minSamplesToSplit, size_t maxDepth)
    {

    }
};