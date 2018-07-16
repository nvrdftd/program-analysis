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
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

class Interval {
    public:
        Interval() {
             nInfinity = true;
             pInfinity = true;
        }

        Interval(const bool inf, const bool sup): nInfinity(inf), pInfinity(sup) {}
        Interval(const int inf, const bool sup): pInfinity(sup), infimum(inf) {}
        Interval(const bool inf, const int sup): nInfinity(inf), supremum(sup) {}
        Interval(const int inf, const int sup): infimum(inf), supremum(sup) {}

        Interval operator+(const Interval &rhs)
        {
            const Interval &lhs = *this;
            int newInf = 0, newSup = 0;
            bool newNInfinity = false, newPInfinity = false;
            if (lhs.nInfinity || rhs.nInfinity) {
                newNInfinity = true;
            }

            if(!lhs.nInfinity && !rhs.nInfinity) {
                newNInfinity = false;
                newInf = lhs.infimum + rhs.infimum;
            }

            if (lhs.pInfinity || rhs.pInfinity) {
                newPInfinity = true;
            }

            if (!lhs.pInfinity && !rhs.pInfinity) {
                newPInfinity = false;
                newSup = lhs.supremum + rhs.supremum;
            }

            return Interval(newInf, newSup, newNInfinity, newPInfinity);
        }

        Interval& operator+=(const Interval &rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        Interval operator-(const Interval &rhs)
        {
            const Interval &lhs = *this;
            int newInf = 0, newSup = 0;
            bool newNInfinity = false, newPInfinity = false;
            if (lhs.nInfinity || rhs.pInfinity) {
                newNInfinity = true;
            }

            if(!lhs.nInfinity && !rhs.pInfinity) {
                newNInfinity = false;
                newInf = lhs.infimum - rhs.supremum;
            }

            if (lhs.pInfinity || rhs.nInfinity) {
                newPInfinity = true;
            }

            if (!lhs.pInfinity && !rhs.nInfinity) {
                newPInfinity = false;
                newSup = lhs.supremum - rhs.infimum;
            }

            return Interval(newInf, newSup, newNInfinity, newPInfinity);
        }

        Interval& operator-=(const Interval &rhs)
        {
            *this = *this - rhs;
            return *this;
        }


        Interval operator*(const Interval &rhs)
        {
            const Interval &lhs = *this;
            int newInf = 0, newSup = 0;
            bool newNInfinity = false, newPInfinity = false;
            if (lhs.nInfinity || rhs.nInfinity) {
                newNInfinity = true;
            }

            if (lhs.pInfinity || rhs.pInfinity) {
                newPInfinity = true;
            }

            if (lhs.supremum < 0 && rhs.nInfinity) {
                newPInfinity = true;
            }

            if (lhs.infimum < 0 && rhs.nInfinity) {
                newPInfinity = true;
            }

            if (lhs.nInfinity && rhs.supremum < 0) {
                newPInfinity = true;
            }

            if (lhs.nInfinity && rhs.infimum < 0) {
                newPInfinity = true;
            }

            if (lhs.supremum < 0 && rhs.pInfinity) {
                newNInfinity = true;
            }

            if (lhs.infimum < 0 && rhs.pInfinity) {
                newNInfinity = true;
            }

            if (lhs.pInfinity && rhs.supremum < 0) {
                newNInfinity = true;
            }

            if (lhs.pInfinity && rhs.infimum < 0) {
                newNInfinity = true;
            }

            int ab = lhs.infimum * rhs.infimum;
            int bc = lhs.infimum * rhs.supremum;
            int cd = lhs.supremum * rhs.infimum;
            int da = lhs.supremum * rhs.supremum;
            newInf = this->min(ab, bc, cd, da);
            newSup = this->max(ab, bc, cd, da);

            return Interval(newInf, newSup, newNInfinity, newPInfinity);
        }

        Interval& operator*=(const Interval &rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        bool operator>(const int rhs)
        {
            const Interval &lhs = *this;

            if (lhs.pInfinity) {
                return true;
            }

            if (!lhs.nInfinity && lhs.infimum > rhs) {
                return true;
            }

            if (!lhs.pInfinity && lhs.supremum > rhs) {
                return true;
            }

            return false;
        }

        bool operator>=(const int rhs)
        {
            const Interval &lhs = *this;

            if (lhs.pInfinity) {
                return true;
            }

            if (!lhs.nInfinity && lhs.infimum >= rhs) {
                return true;
            }

            if (!lhs.pInfinity && lhs.supremum >= rhs) {
                return true;
            }

            return false;
        }

        bool operator<(const int rhs)
        {
            const Interval &lhs = *this;

            if (lhs.nInfinity) {
                return true;
            }

            if (!lhs.nInfinity && lhs.infimum < rhs) {
                return true;
            }

            if (!lhs.pInfinity && lhs.supremum < rhs) {
                return true;
            }

            return false;
        }

        bool operator<=(const int rhs)
        {
            const Interval &lhs = *this;

            if (lhs.nInfinity) {
                return true;
            }

            if (!lhs.nInfinity && lhs.infimum <= rhs) {
                return true;
            }

            if (!lhs.pInfinity && lhs.supremum <= rhs) {
                return true;
            }

            return false;
        }

        bool operator==(const int rhs)
        {
            const Interval &lhs = *this;

            if (lhs.nInfinity && lhs.pInfinity) {
                return true;
            }

            if (lhs.nInfinity && lhs.supremum >= rhs) {
                return true;
            }

            if (lhs.pInfinity && lhs.infimum <= rhs) {
                return true;
            }

            if (!lhs.nInfinity && lhs.pInfinity && lhs.infimum <= rhs) {
                return true;
            }

            if (!lhs.pInfinity && lhs.nInfinity && lhs.supremum >= rhs) {
                return true;
            }

            if (!lhs.pInfinity && !lhs.nInfinity) {
                if (lhs.infimum <= rhs && lhs.supremum >= rhs) {
                    return true;
                }
            }

            return false;
        }

        bool operator!=(const int rhs)
        {
            return !(*this == rhs);
        }

        bool operator==(const Interval rhs)
        {
            const Interval &lhs = *this;
            if (lhs.nInfinity != rhs.nInfinity) {
                return false;
            }

            if (lhs.pInfinity != rhs.pInfinity) {
                return false;
            }

            if (!lhs.nInfinity && !rhs.nInfinity) {
                if (lhs.infimum != rhs.infimum) {
                    return false;
                }
            }

            if (!lhs.pInfinity && !rhs.pInfinity) {
                if (lhs.supremum != rhs.supremum) {
                    return false;
                }
            }

            return true;
        }

        bool operator!=(const Interval rhs)
        {
            return !(*this == rhs);
        }


        void print() const
        {
            if (empty) {
                cout << "EMPTY INTERVAL" << endl;
            } else {
                cout << "[";
                if (nInfinity) {
                    cout << "-INFINITY";
                } else {
                    cout << infimum;
                }
                cout << ", ";
                if (pInfinity) {
                    cout << "INFINITY";
                } else {
                    cout << supremum;
                }
                    cout << "]" << endl;
            }
        }
        void widenWith(const Interval &rhs) {

            if (rhs.nInfinity) {
                this->nInfinity = true;
            }

            if (rhs.pInfinity) {
                this->pInfinity = true;
            }

            if (!rhs.nInfinity && !this->nInfinity && this->infimum > rhs.infimum) {
                this->nInfinity = true;
            }

            if (!rhs.pInfinity && !this->pInfinity && this->supremum < rhs.supremum) {
                this->pInfinity = true;
            }
        }

        void narrowWith(const Interval &rhs) {

            if (!rhs.nInfinity && this->nInfinity) {
                this->infimum = rhs.infimum;
                this->nInfinity = false;
            }

            if (!rhs.pInfinity && this->pInfinity) {
                this->supremum = rhs.supremum;
                this->pInfinity = false;
            }
        }

        void unionWith(const Interval &rhs) {
            if (rhs.nInfinity) {
                this->nInfinity = rhs.nInfinity;
            }

            if (rhs.pInfinity) {
                this->pInfinity = rhs.pInfinity;
            }

            if (!rhs.nInfinity && !this->nInfinity) {
                this->infimum = std::min(this->infimum, rhs.infimum);
            }

            if (!rhs.pInfinity && !this->pInfinity) {
                this->supremum = std::max(this->supremum, rhs.supremum);
            }
        }

        void intersectionWith(const Interval &rhs) {

            if (this->nInfinity && !rhs.nInfinity) {
                this->infimum = rhs.infimum;
                this->nInfinity = rhs.nInfinity;
            }

            if (this->pInfinity && !rhs.pInfinity) {
                this->supremum = rhs.supremum;
                this->pInfinity = rhs.pInfinity;
            }

            // if (!this->nInfinity && rhs.nInfinity) {
            //
            // }
            //
            // if (!this->pInfinity && rhs.pInfinity) {
            //
            // }

            if (!this->nInfinity && !this->pInfinity && !rhs.nInfinity && rhs.pInfinity && this->supremum >= rhs.infimum) {
                this->infimum = std::max(this->infimum, rhs.infimum);
            }

            if (!this->nInfinity && this->pInfinity && !rhs.nInfinity && !rhs.pInfinity && this->infimum <= rhs.supremum) {
                this->infimum = std::max(this->infimum, rhs.infimum);
                this->supremum = rhs.supremum;
            }

            if (!this->nInfinity && !this->pInfinity && rhs.nInfinity && !rhs.pInfinity && this->infimum <= rhs.supremum) {
                this->supremum = std::min(this->supremum, rhs.supremum);
            }

            if (this->nInfinity && !this->pInfinity && !rhs.nInfinity && !rhs.pInfinity && this->supremum >= rhs.infimum) {
                this->infimum = rhs.infimum;
                this->supremum = std::min(this->supremum, rhs.supremum);
            }

            // if (!(this->infimum <= rhs.supremum) || !(this->supremum >= rhs.infimum)) {
            //     this->empty = true;
            // }


        }

        bool justInitialized() {
            if (nInfinity && pInfinity) {
                return true;
            }
            return false;
        }

    private:
        Interval(const int inf, const int sup, const bool nInfi, const bool pInfi):
            nInfinity(nInfi), pInfinity(pInfi), infimum(inf), supremum(sup) {}
        bool nInfinity = false;
        bool pInfinity = false;
        bool empty = false;
        int infimum = 0;
        int supremum = 0;
        int min(int a, int b, int c, int d) {
            int array[4] = {a, b, c, d};
            sort(array, array + 4);
            return array[0];
        }

        int max(int a, int b, int c, int d) {
            int array[4] = {a, b, c, d};
            sort(array, array + 4);
            return array[3];
        }
};

map<Value*, Interval> traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    map<Value*, Interval> intervalMap,
    map<Value*, pair<bool, bool>> &boolMap,
    queue<BasicBlock*> &blockQueue,
    set<BasicBlock*> &masterTraversedBlocks,
    queue<BasicBlock*> &masterBlockQueue);
map<Value*, Interval> initInterval(BasicBlock *BB);
void printMap(const map<Value*, Interval> &intervalMap);
map<Value*, Interval> unionTwoMaps(map<Value*, Interval> newMap, map<Value*, Interval> oldMap);
map<Value*, Interval> widenMap(map<Value*, Interval> newMap, map<Value*, Interval> oldMap);
map<Value*, Interval> narrowMap(map<Value*, Interval> newMap, map<Value*, Interval> oldMap);
bool reachFixedPoint(map<Value*, Interval> map1, map<Value*, Interval> map2);

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

    map<Value*, Interval> oldMap;
    map<Value*, Interval> newMap;
    map<Value*, pair<bool, bool>> boolMap;
    queue<BasicBlock*> blockQueue;
    set<BasicBlock*> masterTraversedBlocks;

    int blkCount = 1;
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            blockQueue.push(BB);
            oldMap = initInterval(BB);
            while (!blockQueue.empty()) {
                BasicBlock *next = blockQueue.front();
                blockQueue.pop();
                if (masterTraversedBlocks.find(next) != masterTraversedBlocks.end()) {
                    cout << "It is a loop." << endl;
                } else {
                    masterTraversedBlocks.insert(next);
                }
                newMap = traverseCFG(next, blkCount, oldMap, boolMap, blockQueue, masterTraversedBlocks, blockQueue);
                if (blkCount >= 200) {
                    newMap = widenMap(newMap, oldMap);
                }
                oldMap = newMap;
            }
            cout << "=========== Final Result ===========" << endl;
            printMap(newMap);
        }
    return 0;
}

map<Value*, Interval> initInterval(BasicBlock *BB)
{
    map<Value*, Interval> intervalMap;

    for (auto &I: *BB) {
        if (isa<AllocaInst>(I)) {
            Value *allocVal = dyn_cast<Value>(&I);
            Interval newInterval;
            pair<Value*, Interval> pair = make_pair(allocVal, newInterval);
            intervalMap.insert(pair);
        }
    }
    return intervalMap;
}

map<Value*, Interval>
backwardUpdate(
    BasicBlock *BB,
    map<Value*, Interval> intervalMap,
    map<Value*, pair<bool, bool>> &boolMap)
{
    bool updated = false;
    Value *cond;

    for (auto iter = (*BB).rbegin(); iter != (*BB).rend(); ++iter) {
        Instruction *I = &(*iter);

        if (isa<BranchInst>(I)) {
            const BranchInst *BInst = dyn_cast<BranchInst>(I);
            cond = BInst->getCondition();
            updated = true;
        }

        if (isa<ICmpInst>(I)) {
            Value *op1 = I->getOperand(0);
            Value *op2 = I->getOperand(1);
            auto iter1 = intervalMap.find(op1);
            auto iter2 = intervalMap.find(op2);
            Interval lhs;
            Interval rhs;

            auto boolIter = boolMap.find(cond);
            pair<bool, bool> brCond = boolIter->second;

            if (iter1 != intervalMap.end() && iter2 != intervalMap.end()) {
                lhs = iter1->second;
                rhs = iter2->second;
            }

            if (iter1 != intervalMap.end() && iter2 == intervalMap.end()) {
                lhs = iter1->second;
                int newVal = dyn_cast<ConstantInt>(op2)->getSExtValue();
                rhs = Interval(newVal, newVal);
            }

            if (iter1 == intervalMap.end() && iter2 != intervalMap.end()) {
                int newVal = dyn_cast<ConstantInt>(op1)->getSExtValue();
                lhs = Interval(newVal, newVal);
                rhs = iter2->second;
            }

            Interval originalInterval = intervalMap.find(dyn_cast<Value>(I))->second;

            switch(dyn_cast<CmpInst>(I)->getPredicate()) {
                case CmpInst::Predicate::ICMP_SGT:
                    if (brCond.first) {
                        originalInterval.intersectionWith(Interval(1, true));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.first = false;
                        break;
                    }
                    if (brCond.second) {
                        originalInterval.intersectionWith(Interval(true, 0));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.second = false;
                    }
                    break;
                case CmpInst::Predicate::ICMP_SGE:
                    if (brCond.first) {
                        originalInterval.intersectionWith(Interval(0, true));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.first = false;
                        break;
                    }
                    if (brCond.second) {
                        originalInterval.intersectionWith(Interval(true, -1));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.second = false;
                    }
                    break;
                case CmpInst::Predicate::ICMP_SLT:
                    if (brCond.first) {
                        originalInterval.intersectionWith(Interval(true, -1));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.first = false;
                        break;
                    }
                    if (brCond.second) {
                        originalInterval.intersectionWith(Interval(0, true));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.second = false;
                    }
                    break;
                case CmpInst::Predicate::ICMP_SLE:
                    if (brCond.first) {
                        originalInterval.intersectionWith(Interval(true, 0));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.first = false;
                        break;
                    }
                    if (brCond.second) {
                        originalInterval.intersectionWith(Interval(1, true));
                        if (iter1 != intervalMap.end()) {
                            Interval newInterval = originalInterval + rhs;
                            newInterval.intersectionWith(lhs);
                            iter1->second = newInterval;
                        }

                        if (iter2 != intervalMap.end()) {
                            Interval newInterval = lhs - originalInterval;
                            newInterval.intersectionWith(rhs);
                            iter2->second = newInterval;
                        }
                        boolIter->second.second = false;
                    }
                    break;
                // case CmpInst::Predicate::ICMP_EQ:
                //     if (operand == 0) {
                //     }
                //     if (operand != 0) {
                //     }
                //     break;
                // case CmpInst::Predicate::ICMP_NE:
                //     if (operand != 0) {
                //     }
                //     if (operand == 0) {
                //     }
                //     break;
                default:
                    cerr << "Undefined Comparison" << endl;
                    exit(EXIT_FAILURE);
            }

            updated = true;
        }

        if (isa<LoadInst>(I)) {
            Value *inst = dyn_cast<Value>(I);
            Value *op = I->getOperand(0);
            auto iter1 = intervalMap.find(inst);
            auto iter2 = intervalMap.find(op);
            iter2->second = iter1->second;
            updated = true;
        }

        if (I->isBinaryOp()) {
            Value *op1 = I->getOperand(0);
            Value *op2 = I->getOperand(1);
            auto iter1 = intervalMap.find(op1);
            auto iter2 = intervalMap.find(op2);
            Interval lhs;
            Interval rhs;

            if (iter1 != intervalMap.end() && iter2 != intervalMap.end()) {
                lhs = iter1->second;
                rhs = iter2->second;
            }

            if (iter1 != intervalMap.end() && iter2 == intervalMap.end()) {
                lhs = iter1->second;
                int newVal = dyn_cast<ConstantInt>(op2)->getSExtValue();
                rhs = Interval(newVal, newVal);
            }

            if (iter1 == intervalMap.end() && iter2 != intervalMap.end()) {
                int newVal = dyn_cast<ConstantInt>(op1)->getSExtValue();
                lhs = Interval(newVal, newVal);
                rhs = iter2->second;
            }

            Interval newInterval = intervalMap.find(dyn_cast<Value>(I))->second;
            switch(I->getOpcode()) {
                case Instruction::Add:
                    if (iter1 != intervalMap.end()) {
                        lhs.intersectionWith(newInterval - rhs);
                        iter1->second = lhs;
                    }
                    if (iter2 != intervalMap.end()) {
                        rhs.intersectionWith(newInterval - lhs);
                        iter2->second = rhs;
                    }
                    break;
                case Instruction::Sub:
                    if (iter1 != intervalMap.end()) {
                        lhs.intersectionWith(newInterval + rhs);
                        iter1->second = lhs;
                    }
                    if (iter2 != intervalMap.end()) {
                        rhs.intersectionWith(lhs - newInterval);
                        iter2->second = rhs;
                    }
                    break;
                // case Instruction::Mul:
                //     newInterval = lhs * rhs;
                //     break;
                default:
                    cerr << "Undefined Operation" << endl;
                    exit(EXIT_FAILURE);
            }
            updated = true;
        }

        if (!updated) {
            break;
        } else {
            updated = false;
        }
    }
    return intervalMap;
}

void transfer(
    Instruction &I,
    map<Value*, Interval> &intervalMap,
    map<Value*, pair<bool, bool>> &boolMap)
{
    if (isa<ICmpInst>(&I)) {
        Value *op1 = I.getOperand(0);
        Value *op2 = I.getOperand(1);
        auto iter1 = intervalMap.find(op1);
        auto iter2 = intervalMap.find(op2);
        Interval lhs;
        Interval rhs;

        if (iter1 != intervalMap.end() && iter2 != intervalMap.end()) {
            lhs = iter1->second;
            rhs = iter2->second;
        }

        if (iter1 != intervalMap.end() && iter2 == intervalMap.end()) {
            lhs = iter1->second;
            int newVal = dyn_cast<ConstantInt>(op2)->getSExtValue();
            rhs = Interval(newVal, newVal);
        }

        if (iter1 == intervalMap.end() && iter2 != intervalMap.end()) {
            int newVal = dyn_cast<ConstantInt>(op1)->getSExtValue();
            lhs = Interval(newVal, newVal);
            rhs = iter2->second;
        }

        Interval operand = lhs - rhs;
        auto existing = intervalMap.find(dyn_cast<Value>(&I));
        if (existing == intervalMap.end()) {
            intervalMap.insert(make_pair(dyn_cast<Value>(&I), operand));
        } else {
            existing->second = operand;
        }
        bool br1 = false;
        bool br2 = false;

        switch(dyn_cast<CmpInst>(&I)->getPredicate()) {
            case CmpInst::Predicate::ICMP_SGT:
                if (operand > 0) {
                    br1 = true;
                }
                if (operand <= 0) {
                    br2 = true;
                }
                break;
            case CmpInst::Predicate::ICMP_SGE:
                if (operand >= 0) {
                    br1 = true;
                }
                if (operand < 0) {
                    br2 = true;
                }
                break;
            case CmpInst::Predicate::ICMP_SLT:
                if (operand < 0) {
                    br1 = true;
                }
                if (operand >= 0) {
                    br2 = true;
                }
                break;
            case CmpInst::Predicate::ICMP_SLE:
                if (operand <= 0) {
                    br1 = true;
                }
                if (operand > 0) {
                    br2 = true;
                }
                break;
            case CmpInst::Predicate::ICMP_EQ:
                if (operand == 0) {
                    br1 = true;
                }
                if (operand != 0) {
                    br2 = true;
                }
                break;
            case CmpInst::Predicate::ICMP_NE:
                if (operand != 0) {
                    br1 = true;
                }
                if (operand == 0) {
                    br2 = true;
                }
                break;
            default:
                cerr << "Undefined Comparison" << endl;
                exit(EXIT_FAILURE);
        }
        auto found = boolMap.find(dyn_cast<Value>(&I));
        pair<bool, bool> pair = make_pair(br1, br2);
        if (found == boolMap.end()) {
            boolMap.insert(make_pair(dyn_cast<Value>(&I), pair));
        } else {
            found->second = pair;
        }
    }

    if (isa<LoadInst>(&I)) {
        Value *inst = dyn_cast<Value>(&I);
        Value *op = I.getOperand(0);
        auto iter = intervalMap.find(op);
        auto found = intervalMap.find(inst);
        pair<Value*, Interval> newPair = make_pair(inst, iter->second);
        if (found == intervalMap.end()) {
            intervalMap.insert(newPair);
        } else {
            found->second = iter->second;
        }
    }

    if (isa<StoreInst>(&I)) {
        Value *from = I.getOperand(0);
        Value *to = I.getOperand(1);
        auto iter = intervalMap.find(to);
        ConstantInt *constInt = dyn_cast<ConstantInt>(from);

        if (constInt == nullptr) {
            auto jter = intervalMap.find(from);
            iter->second = jter->second;
        }

        if (constInt != nullptr) {
            int newConst = constInt->getSExtValue();
            iter->second = Interval(newConst, newConst);
        }
    }

    if (I.isBinaryOp()) {
        Value *op1 = I.getOperand(0);
        Value *op2 = I.getOperand(1);
        auto iter1 = intervalMap.find(op1);
        auto iter2 = intervalMap.find(op2);
        Interval lhs;
        Interval rhs;

        if (iter1 != intervalMap.end() && iter2 != intervalMap.end()) {
            lhs = iter1->second;
            rhs = iter2->second;
        }

        if (iter1 != intervalMap.end() && iter2 == intervalMap.end()) {
            lhs = iter1->second;
            int newVal = dyn_cast<ConstantInt>(op2)->getSExtValue();
            rhs = Interval(newVal, newVal);
        }

        if (iter1 == intervalMap.end() && iter2 != intervalMap.end()) {
            int newVal = dyn_cast<ConstantInt>(op1)->getSExtValue();
            lhs = Interval(newVal, newVal);
            rhs = iter2->second;
        }

        Interval newInterval;

        switch(I.getOpcode()) {
            case Instruction::Add:
                newInterval = lhs + rhs;
                break;
            case Instruction::Sub:
                newInterval = lhs - rhs;
                break;
            case Instruction::Mul:
                newInterval = lhs * rhs;
                break;
            default:
                cerr << "Undefined Operation" << endl;
                exit(EXIT_FAILURE);
        }
        auto found = intervalMap.find(dyn_cast<Value>(&I));
        if (found == intervalMap.end()) {
            intervalMap.insert(make_pair(dyn_cast<Value>(&I), newInterval));
        } else {
            found->second = newInterval;
        }
    }
}

map<Value*, Interval> traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    map<Value*, Interval> intervalMap,
    map<Value*, pair<bool, bool>> &boolMap,
    queue<BasicBlock*> &blockQueue,
    set<BasicBlock*> &masterTraversedBlocks,
    queue<BasicBlock*> &masterBlockQueue)
{
    map<Value*, Interval> oldMap = intervalMap;
    for (auto &I: *BB) {
        transfer(I, intervalMap, boolMap);
    }

    cout << "Block " << blkCount << endl;
    cout << "=========== Old Interval Map ===========" << endl;
    printMap(oldMap);
    intervalMap = unionTwoMaps(intervalMap, oldMap);
    cout << "=========== New Interval Map ===========" << endl;
    printMap(intervalMap);

    ++blkCount;

    const TerminatorInst *TInst = BB->getTerminator();
    // unsigned int NSucc = TInst->getNumSuccessors();
    if (isa<BranchInst>(TInst)) {
        const BranchInst *BInst = dyn_cast<BranchInst>(TInst);
        if (BInst->isConditional()) {
            Value *cond = BInst->getCondition();
            auto iter = boolMap.find(cond);
            queue<BasicBlock*> trueBrQueue;
            queue<BasicBlock*> falseBrQueue;

            if (iter->second.first) {
                trueBrQueue.push(TInst->getSuccessor(0));
            }
            if (iter->second.second) {
                falseBrQueue.push(TInst->getSuccessor(1));
            }

            map<Value*, Interval> newIntervalMap1 = backwardUpdate(BB, intervalMap, boolMap);
            map<Value*, Interval> newIntervalMap2 = backwardUpdate(BB, intervalMap, boolMap);

            while (!trueBrQueue.empty()) {
                BasicBlock *next = trueBrQueue.front();
                trueBrQueue.pop();
                newIntervalMap1 = unionTwoMaps(traverseCFG(next, blkCount, newIntervalMap1, boolMap, trueBrQueue, masterTraversedBlocks, masterBlockQueue), newIntervalMap1);
            }

            if (masterTraversedBlocks.find(BB) != masterTraversedBlocks.end() && reachFixedPoint(newIntervalMap1, oldMap)) {
                masterBlockQueue.pop();
            }

            if (!masterBlockQueue.empty() && masterTraversedBlocks.find(BB) != masterTraversedBlocks.end()) {
                intervalMap = unionTwoMaps(newIntervalMap1, intervalMap);
                return intervalMap;
            }

            while (!falseBrQueue.empty()) {
                BasicBlock *next = falseBrQueue.front();
                falseBrQueue.pop();
                newIntervalMap2 = unionTwoMaps(traverseCFG(next, blkCount, newIntervalMap2, boolMap, falseBrQueue, masterTraversedBlocks, masterBlockQueue), newIntervalMap2);
            }
            intervalMap = unionTwoMaps(newIntervalMap2, intervalMap);
            intervalMap = unionTwoMaps(newIntervalMap1, intervalMap);
        } else {
            if (masterTraversedBlocks.find(TInst->getSuccessor(0)) == masterTraversedBlocks.end()) {
                blockQueue.push(TInst->getSuccessor(0));
            } else if (masterBlockQueue.front() != TInst->getSuccessor(0)) {
                masterBlockQueue.push(TInst->getSuccessor(0));
            }
        }
    }
    return intervalMap;
}

void printMap(const map<Value*, Interval> &intervalMap)
{
    for (auto iter = intervalMap.begin(); iter != intervalMap.end(); ++iter) {
        Value* var = iter->first;
        if (var->hasName()) {
            cout << var->getName().str().c_str() << ": ";
            iter->second.print();
        }
    }
}

map<Value*, Interval> unionTwoMaps(map<Value*, Interval> newMap, map<Value*, Interval> oldMap) {

        for (auto oldIter = oldMap.begin(); oldIter != oldMap.end(); ++oldIter) {
            auto newIter = newMap.find(oldIter->first);
            if (newIter == newMap.end()) {
                newMap.insert(make_pair(oldIter->first, oldIter->second));
            } else {
                if (!oldIter->second.justInitialized()) {
                    newIter->second.unionWith(oldIter->second);
                }
            }
        }
    return newMap;
}

map<Value*, Interval> widenMap(map<Value*, Interval> newMap, map<Value*, Interval> oldMap) {

        for (auto newIter = newMap.begin(); newIter != newMap.end(); ++newIter) {
            auto oldIter = oldMap.find(newIter->first);
            oldIter->second.widenWith(newIter->second);
        }
    return oldMap;
}

map<Value*, Interval> narrowMap(map<Value*, Interval> newMap, map<Value*, Interval> oldMap) {

        for (auto newIter = newMap.begin(); newIter != newMap.end(); ++newIter) {
            auto oldIter = oldMap.find(newIter->first);
            oldIter->second.narrowWith(newIter->second);
        }
    return oldMap;
}

bool reachFixedPoint(map<Value*, Interval> map1, map<Value*, Interval> map2) {
    if (map1.size() != map2.size()) {
        return false;
    }

    for (auto iter2 = map2.begin(); iter2 != map2.end(); ++iter2) {
        auto iter1 = map1.find(iter2->first);
        if (iter1 != map1.end()) {
            if (iter1->second != iter2->second) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}
