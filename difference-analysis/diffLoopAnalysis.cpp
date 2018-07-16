#include <cstdio>
#include <iostream>
#include <set>
#include <queue>
#include <algorithm>
#include <cstring>
#include <map>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Type.h"

#define EXTRA_ITERATION 4

using namespace llvm;
using namespace std;

void printResult(map<Value*, pair<Value*, Value*>> intervalMap);
void traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    int &reachedCount,
    map<Value*, pair<Value*, Value*>> &intervalMap);
map<Value*, pair<Value*, Value*>> initVars(BasicBlock *BB);
pair<Value*, Value*> compareIntervals(pair<Value*, Value*> p1, pair<Value*, Value*> p2);
bool reachFixedPoint(
    map<Value*, pair<Value*, Value*>> oldMap,
    map<Value*, pair<Value*, Value*>> newMap);

int main(int argc, char **argv)
{
    // Read the IR file.
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    Module *M = ParseIRFile(argv[1], Err, Context);
    if (M == nullptr)
    {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    map<Value*, Value*> valMap;
    map<Value*, pair<Value*, Value*>> intervalMap;

    int blkCount = 1;
    int reachedCount = 0;
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            intervalMap = initVars(BB);
            traverseCFG(BB, blkCount, reachedCount, intervalMap);
        }
    return 0;
}

int multiMin(int a, int b, int c, int d) {
    int array[4] = {a, b, c, d};
    sort(array, array + 4);
    return array[0];
}

int multiMax(int a, int b, int c, int d) {
    int array[4] = {a, b, c, d};
    sort(array, array + 4);
    return array[3];
}

void updateVars(Instruction &I, map<Value*, pair<Value*, Value*>> &intervalMap)
{
    IntegerType *globalType = Type::getInt32Ty(getGlobalContext());
    if (isa<LoadInst>(&I)) {
        Value *inst = dyn_cast<Value>(&I);
        Value *op = I.getOperand(0);
        auto iter = intervalMap.find(op);
        pair<Value*, pair<Value*, Value*>> newPair = make_pair(inst, iter->second);
        intervalMap.insert(newPair);
    }

    if (isa<StoreInst>(&I)) {
        Value *from = I.getOperand(0);
        Value *to = I.getOperand(1);
        auto iter = intervalMap.find(to);
        ConstantInt *constInt = dyn_cast<ConstantInt>(from);
        if (constInt == nullptr) {
            auto jter = intervalMap.find(from);
            ConstantInt *ic1 = dyn_cast<ConstantInt>(iter->second.first);
            ConstantInt *ic2 = dyn_cast<ConstantInt>(iter->second.second);
            ConstantInt *jc1 = dyn_cast<ConstantInt>(jter->second.first);
            ConstantInt *jc2 = dyn_cast<ConstantInt>(jter->second.second);

            if (jc1 != nullptr && jc2 != nullptr) {
                if (ic1 != nullptr && ic2 != nullptr) {
                    int minto = ic1->getSExtValue();
                    int maxto = ic2->getSExtValue();
                    int minfrom = jc1->getSExtValue();
                    int maxfrom = jc2->getSExtValue();
                    if (minfrom != minto && maxfrom != maxto && minfrom - minto == maxfrom - maxto) {
                        iter->second.second = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                    } else {
                        iter->second = jter->second;
                    }
                }
                if (ic1 == nullptr && ic2 == nullptr) {
                    iter->second = jter->second;
                }
            }

            if (jc1 == nullptr && jc2 == nullptr) {
                iter->second = jter->second;
            }
        }

        if (constInt != nullptr) {
            iter->second = make_pair(dyn_cast<Value>(constInt), dyn_cast<Value>(constInt));
        }
    }

    if (I.isBinaryOp()) {
        Value *op1 = I.getOperand(0);
        Value *op2 = I.getOperand(1);
        auto iter1 = intervalMap.find(op1);
        auto iter2 = intervalMap.find(op2);
        pair<Value*, Value*> interval1;
        pair<Value*, Value*> interval2;

        if (iter1 != intervalMap.end() && iter2 != intervalMap.end()) {
            interval1 = iter1->second;
            interval2 = iter2->second;
        }

        if (iter1 != intervalMap.end() && iter2 == intervalMap.end()) {
            interval1 = iter1->second;
            interval2 = make_pair(op2, op2);
        }

        if (iter1 == intervalMap.end() && iter2 != intervalMap.end()) {
            interval1 = make_pair(op1, op1);
            interval2 = iter2->second;
        }

        IntegerType *globalType = Type::getInt32Ty(getGlobalContext());
        ConstantFP *min1 = dyn_cast<ConstantFP>(interval1.first);
        ConstantFP *max1 = dyn_cast<ConstantFP>(interval1.second);
        ConstantFP *min2 = dyn_cast<ConstantFP>(interval2.first);
        ConstantFP *max2 = dyn_cast<ConstantFP>(interval2.second);




        Value *newMin;
        Value *newMax;



        switch(I.getOpcode()) {
            case Instruction::Add:
                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1 + minInt2));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1 + maxInt2));
                } else if ((min1 != nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) ||
                    (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 == nullptr) ||
                    (min1 != nullptr && min2 != nullptr && max1 == nullptr && max2 == nullptr)) {
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1 + maxInt2));
                } else if ((min1 == nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) ||
                    (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 != nullptr) ||
                    (min1 == nullptr && min2 == nullptr && max1 != nullptr && max2 != nullptr)) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1 + minInt2));
                    newMax = dyn_cast<Value>(max1);
                } else if (min1 != nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(max1);
                } else if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 != nullptr) {
                    newMin = dyn_cast<Value>(min2);
                    newMax = dyn_cast<Value>(max2);
                } else {
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(max1);
                }

                break;
            case Instruction::Sub:
                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1 - maxInt2));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1 - minInt2));
                }

                if (min1 != nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1 - minInt2));
                }

                if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1 - maxInt2));
                    newMax = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                }

                if (min1 == nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1 - maxInt2));
                    newMax = dyn_cast<Value>(max1);
                }

                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 != nullptr) {
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantFP::getInfinity(globalType, true));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1 - minInt2));
                }

                if (min1 != nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(max1);
                }

                if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 != nullptr) {
                    newMin = dyn_cast<Value>(min2);
                    newMax = dyn_cast<Value>(max2);
                }

                if (min1 != nullptr && min2 != nullptr && max1 == nullptr && max2 == nullptr) {
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max(maxInt1 - maxInt2, maxInt1 - maxInt2)));
                }

                if (min1 == nullptr && min2 == nullptr && max1 != nullptr && max2 != nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min(minInt1 - minInt2, minInt2 - minInt1)));
                    newMax = dyn_cast<Value>(max1);
                }

                if ((min1 != nullptr && min2 != nullptr) || (max1 != nullptr && max2 != nullptr) ||
                    (min1 == nullptr && min2 != nullptr) || (max1 != nullptr && max2 == nullptr)) {
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(max1);
                }

                break;
            case Instruction::Mul:
                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    int min = multiMin(minInt1 * maxInt1, minInt1 * maxInt2, minInt2 * maxInt1, minInt2 * maxInt2);
                    int max = multiMax(minInt1 * maxInt1, minInt1 * maxInt2, minInt2 * maxInt1, minInt2 * maxInt2);
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max));
                }

                if (min1 != nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    if (dyn_cast<ConstantInt>(interval2.first)->isNegative() && dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min(maxInt1 * minInt2, maxInt1 * maxInt2)));
                        newMax = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                    }

                    if (dyn_cast<ConstantInt>(interval2.first)->isNegative() && !dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(min1);
                        newMax = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                    }

                    if (!dyn_cast<ConstantInt>(interval2.first)->isNegative() && !dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(min1);
                        newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max(maxInt1 * minInt2, maxInt1 * maxInt2)));
                    }
                }

                if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    if (dyn_cast<ConstantInt>(interval1.first)->isNegative() && dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min(maxInt2 * minInt1, maxInt2 * maxInt1)));
                        newMax = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                    }

                    if (dyn_cast<ConstantInt>(interval1.first)->isNegative() && !dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(min2);
                        newMax = dyn_cast<Value>(ConstantFP::getInfinity(globalType));
                    }

                    if (!dyn_cast<ConstantInt>(interval1.first)->isNegative() && !dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(min2);
                        newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max(maxInt2 * minInt1, maxInt2 * maxInt1)));
                    }
                }

                if (min1 == nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    if (dyn_cast<ConstantInt>(interval2.first)->isNegative() && dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantFP::getInfinity(globalType, true));
                        newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max(minInt1 * maxInt2, minInt1 * minInt2)));
                    }

                    if (dyn_cast<ConstantInt>(interval2.first)->isNegative() && !dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantFP::getInfinity(globalType, true));
                        newMax = dyn_cast<Value>(max1);
                    }

                    if (!dyn_cast<ConstantInt>(interval2.first)->isNegative() && !dyn_cast<ConstantInt>(interval2.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min(minInt1 * minInt2, minInt1 * maxInt2)));
                        newMax = dyn_cast<Value>(max1);
                    }
                }

                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 != nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    if (dyn_cast<ConstantInt>(interval1.first)->isNegative() && dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantFP::getInfinity(globalType, true));
                        newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max(minInt1 * minInt2, maxInt1 * minInt2)));
                    }

                    if (dyn_cast<ConstantInt>(interval1.first)->isNegative() && !dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantFP::getInfinity(globalType, true));
                        newMax = dyn_cast<Value>(max2);
                    }

                    if (!dyn_cast<ConstantInt>(interval1.first)->isNegative() && !dyn_cast<ConstantInt>(interval1.second)->isNegative()) {
                        newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min(minInt1 * minInt2, maxInt1 * minInt2)));
                        newMax = dyn_cast<Value>(max2);
                    }
                }

                if (min1 != nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    newMin = dyn_cast<Value>(min1);
                    newMax = dyn_cast<Value>(max1);
                }

                if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 != nullptr) {
                    newMin = dyn_cast<Value>(min2);
                    newMax = dyn_cast<Value>(max2);
                }
                break;
            case Instruction::SRem:
                if (min1 == nullptr && min2 == nullptr && max1 == nullptr && max2 == nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int minInt2 = dyn_cast<ConstantInt>(interval2.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    if (minInt1 < minInt2 && minInt1 < maxInt2 && maxInt1 < minInt2 && maxInt1 < maxInt2) {
                        newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1));
                        newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1));
                        break;
                    }
                    int min;
                    int max;
                    int result;
                    bool firstIter = true;
                    for (int i = minInt1; i <= maxInt1; ++i) {
                        for (int j = minInt2; j <= maxInt2; ++j) {
                            result = i % j;
                            if (firstIter == true) {
                                min = result;
                                max = result;
                                firstIter = false;
                            }
                            if (result < min) {
                                min = result;
                            }
                            if (result > max) {
                                max = result;
                            }
                        }
                    }
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, min));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, max));
                }
                if (min1 != nullptr && min2 == nullptr && max1 != nullptr && max2 == nullptr) {
                    int maxInt2 = dyn_cast<ConstantInt>(interval2.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, -(maxInt2 - 1)));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt2 - 1));
                }

                if (min1 == nullptr && min2 != nullptr && max1 == nullptr && max2 != nullptr) {
                    int minInt1 = dyn_cast<ConstantInt>(interval1.first)->getSExtValue();
                    int maxInt1 = dyn_cast<ConstantInt>(interval1.second)->getSExtValue();
                    newMin = dyn_cast<Value>(ConstantInt::getSigned(globalType, minInt1));
                    newMax = dyn_cast<Value>(ConstantInt::getSigned(globalType, maxInt1));
                }


                if (min1 != nullptr && min2 != nullptr && max1 != nullptr && max2 != nullptr) {
                    cerr << "Undefined Action" << endl;
                    exit(EXIT_FAILURE);
                }
        }
        pair<Value*, Value*> newInterval = make_pair(newMin, newMax);
        intervalMap.insert(make_pair(dyn_cast<Value>(&I), newInterval));
    }
}

map<Value*, pair<Value*, Value*>> initVars(BasicBlock *BB)
{
    map<Value*, pair<Value*, Value*>> intervalMap;

    for (auto &I: *BB) {
        if (isa<AllocaInst>(I)) {
            IntegerType *globalType = Type::getInt32Ty(getGlobalContext());
            Constant *pInfinity = ConstantFP::getInfinity(globalType);
            Constant *nInfinity = ConstantFP::getInfinity(globalType, true);
            Value *pVal = dyn_cast<Value>(pInfinity);
            Value *nVal = dyn_cast<Value>(nInfinity);
            Value *allocVal = dyn_cast<Value>(&I);
            pair<Value*, Value*> intPair = make_pair(nVal, pVal);
            pair<Value*, pair<Value*, Value*>> valPair = make_pair(allocVal, intPair);
            intervalMap.insert(valPair);
        }
    }
    return intervalMap;
}

int sep(pair<Value*, Value*> p1, pair<Value*, Value*> p2)
{

    ConstantInt *minV1 = dyn_cast<ConstantInt>(p1.first);
    ConstantInt *maxV1 = dyn_cast<ConstantInt>(p1.second);
    ConstantInt *minV2 = dyn_cast<ConstantInt>(p2.first);
    ConstantInt *maxV2 = dyn_cast<ConstantInt>(p2.second);

    int result, last;
    bool firstCheck = true;

    for (int v1 = minV1->getSExtValue(); v1 <= maxV1->getSExtValue(); ++v1) {
        for (int v2 = minV2->getSExtValue(); v2 <= maxV2->getSExtValue(); ++v2) {

            if (firstCheck) {
                result = v1;
                firstCheck = false;
            }

            last = result;

            if (v1 >= v2) {
                result = v1 - v2;
            } else {
                result = v2 - v1;
            }

            result = max(last, result);
        }
    }
    return result;
}

void traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    int &reachedCount,
    map<Value*, pair<Value*, Value*>> &intervalMap)
{

    map<Value*, pair<Value*, Value*>> oldMap = intervalMap;

    for (auto &I: *BB) {
        updateVars(I, intervalMap);
    }

    cout << "Block " << blkCount << ":" << endl;
    printResult(intervalMap);

    ++blkCount;

    bool pointReached = reachFixedPoint(oldMap, intervalMap);

    if (pointReached) {
        ++reachedCount;
        cout << "<-------- Reached the fixed point " << reachedCount << " time(s) -------->" << endl;
    } else {
        cout << "<---------- Reset the counter of reaching the same fixed point ---------->" << endl;
        reachedCount = 0;
    }

    if (reachedCount == EXTRA_ITERATION) {
        return;
    }

    const TerminatorInst *TInst = BB->getTerminator();
    unsigned int NSucc = TInst->getNumSuccessors();
    for (unsigned i = 0; i < NSucc; ++i) {
        BasicBlock *Succ = TInst->getSuccessor(i);
        traverseCFG(Succ, blkCount, reachedCount, intervalMap);
    }
}

void printResult(map<Value*, pair<Value*, Value*>> map)
{
    for (auto it = map.begin(); it != map.end(); ++it) {
        for (auto jt = it; jt != map.end(); ++jt) {
            Value *var1 = (*it).first;
            pair<Value*, Value*> interval1 = (*it).second;
            Value *var2 = (*jt).first;
            pair<Value*, Value*> interval2 = (*jt).second;

            if (var1 == var2 || !var1->hasName() || !var2->hasName()) continue;
            cout << "sep(" << var1->getName().str().c_str() << ", ";
            cout << var2->getName().str().c_str() << ") = ";

            ConstantFP *minV1 = dyn_cast<ConstantFP>(interval1.first);
            ConstantFP *maxV1 = dyn_cast<ConstantFP>(interval1.second);
            ConstantFP *minV2 = dyn_cast<ConstantFP>(interval2.first);
            ConstantFP *maxV2 = dyn_cast<ConstantFP>(interval2.second);

            if (
                (minV1 != nullptr && minV1->getValueAPF().isInfinity()) ||
                (minV2 != nullptr && minV2->getValueAPF().isInfinity()) ||
                (maxV1 != nullptr && maxV1->getValueAPF().isInfinity()) ||
                (maxV2 != nullptr && maxV2->getValueAPF().isInfinity())) {
                    cout << "Infi";
            }

            if (
                minV1 == nullptr &&
                minV2 == nullptr &&
                maxV1 == nullptr &&
                maxV2 == nullptr) {
                    cout << sep(interval1, interval2);
            }
            cout << endl;
        }
    }
}

pair<Value*, Value*> compareIntervals(pair<Value*, Value*> p1, pair<Value*, Value*> p2)
{
    IntegerType *globalType = Type::getInt32Ty(getGlobalContext());
    ConstantFP *minV1 = dyn_cast<ConstantFP>(p1.first);
    ConstantFP *maxV1 = dyn_cast<ConstantFP>(p1.second);
    ConstantFP *minV2 = dyn_cast<ConstantFP>(p2.first);
    ConstantFP *maxV2 = dyn_cast<ConstantFP>(p2.second);
    ConstantInt *newMin;
    ConstantInt *newMax;

    if (minV1 == nullptr && minV2 != nullptr && minV2->getValueAPF().isInfinity()) {
        newMin = dyn_cast<ConstantInt>(p1.first);
    }

    if (minV2 == nullptr && minV1 != nullptr && minV1->getValueAPF().isInfinity()) {
        newMin = dyn_cast<ConstantInt>(p2.first);
    }

    if (minV1 == nullptr && minV2 == nullptr) {
        ConstantInt *const1 = dyn_cast<ConstantInt>(p1.first);
        ConstantInt *const2 = dyn_cast<ConstantInt>(p2.first);
        int v1 = const1->getSExtValue();
        int v2 = const2->getSExtValue();
        newMin = ConstantInt::getSigned(globalType, max(v1, v2));
    }

    if (maxV1 == nullptr && maxV2 != nullptr && maxV2->getValueAPF().isInfinity()) {
        newMax = dyn_cast<ConstantInt>(p1.second);
    }

    if (maxV2 == nullptr &&  maxV1 != nullptr && maxV1->getValueAPF().isInfinity()) {
        newMax = dyn_cast<ConstantInt>(p2.second);
    }

    if (maxV1 == nullptr && maxV2 == nullptr) {
        ConstantInt *const1 = dyn_cast<ConstantInt>(p1.second);
        ConstantInt *const2 = dyn_cast<ConstantInt>(p2.second);
        int v1 = const1->getSExtValue();
        int v2 = const2->getSExtValue();
        newMax = ConstantInt::getSigned(globalType, max(v1, v2));
    }

    return make_pair(dyn_cast<Value>(newMin), dyn_cast<Value>(newMax));
}


bool reachFixedPoint(
    map<Value*, pair<Value*, Value*>> oldMap,
    map<Value*, pair<Value*, Value*>> newMap) {
      if (oldMap.size() != newMap.size()) return false;

      for (auto it = newMap.begin(); it != newMap.end(); ++it) {
          for (auto jt = oldMap.begin(); jt != oldMap.end(); ++jt) {
              Value *var1 = it->first;
              Value *var2 = jt->first;
              if (oldMap.find(var1) == oldMap.end()) return false;
              if (var1 == var2) {
                  pair<Value*, Value*> interval1 = it->second;
                  pair<Value*, Value*> interval2 = jt->second;
                  ConstantFP *min1 = dyn_cast<ConstantFP>(interval1.first);
                  ConstantFP *max1 = dyn_cast<ConstantFP>(interval1.second);
                  ConstantFP *min2 = dyn_cast<ConstantFP>(interval2.first);
                  ConstantFP *max2 = dyn_cast<ConstantFP>(interval2.second);

                  if (
                      (min1 == nullptr && min2 != nullptr) ||
                      (min2 == nullptr && min1 != nullptr) ||
                      (max1 == nullptr && max2 != nullptr) ||
                      (max2 == nullptr && max1 != nullptr)) {
                      return false;
                  }

                  if (min1 == nullptr && min2 == nullptr) {
                      ConstantInt *const1 = dyn_cast<ConstantInt>(interval1.first);
                      ConstantInt *const2 = dyn_cast<ConstantInt>(interval2.first);
                      int v1 = const1->getSExtValue();
                      int v2 = const2->getSExtValue();
                      if (v1 != v2) return false;
                  }

                  if (max1 == nullptr && max2 == nullptr) {
                      ConstantInt *const1 = dyn_cast<ConstantInt>(interval1.second);
                      ConstantInt *const2 = dyn_cast<ConstantInt>(interval2.second);
                      int v1 = const1->getSExtValue();
                      int v2 = const2->getSExtValue();
                      if (v1 != v2) return false;
                  }
              }
          }
      }
      return true;
}
