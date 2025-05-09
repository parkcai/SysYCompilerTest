#pragma once

#include <unordered_map>

using namespace std;

class StackFrame {
private:
    int currentPos;
    int lastCurrentPos;
    unordered_map<void *, int> stackFrame;
public:
    bool saved_ra;
    int length;
    int paramsLength;

    StackFrame() {
        saved_ra = false;
        length = 0;
        currentPos = 0;
        paramsLength = 0;
        lastCurrentPos = 0;
        stackFrame = {};
    }
    int find(void *ptr) {
        auto iter = stackFrame.find(ptr);
        if(iter != stackFrame.end())
            return iter->second;
        return -1;
    }
    void add(void *ptr, int size) {
        stackFrame[ptr] = currentPos;
        currentPos += size;
    }
    void insert(void *ptr, int pos) {
        stackFrame[ptr] = pos;
    }
    void addToLastFrame(void *ptr, int size) {
        stackFrame[ptr] = lastCurrentPos;
        lastCurrentPos += size;
    }
    void align() {
        length = (length / 16 + (int)(length % 16 > 0)) * 16;
        currentPos = paramsLength;
        lastCurrentPos = length;
    }
};