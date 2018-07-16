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

using namespace llvm;
using namespace std;

void printResult(map<Value*, Value*> varMap);
void traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    map<Value*, Value*> varMap);
map<Value*, Value*> initVars(BasicBlock *BB);

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

    map<Value*, Value*> varMap;

    int blkCount = 1;
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            varMap = initVars(BB);
            traverseCFG(BB, blkCount, varMap);
        }
    return 0;
}

void updateVars(Instruction &I, map<Value*, Value*> &varMap)
{
    if (isa<LoadInst>(&I)) {
        Value *inst = dyn_cast<Value>(&I);
        Value *op = I.getOperand(0);
        auto iter = varMap.find(op);
        pair<Value*, Value*> newPair = make_pair(inst, iter->second);
        varMap.insert(newPair);
    }

    if (isa<StoreInst>(&I)) {
        Value *from = I.getOperand(0);
        Value *to = I.getOperand(1);
        auto iter = varMap.find(to);
        ConstantInt *constInt = dyn_cast<ConstantInt>(from);
        if (constInt == nullptr) {
            auto jter = varMap.find(from);
            iter->second = jter->second;
        } else {
            iter->second = dyn_cast<Value>(constInt);
        }
    }

    if (I.isBinaryOp()) {
        Value *op1 = I.getOperand(0);
        Value *op2 = I.getOperand(1);
        auto iter1 = varMap.find(op1);
        auto iter2 = varMap.find(op2);
        ConstantInt *constInt1;
        ConstantInt *constInt2;
        ConstantInt *result;
        int const1;
        int const2;
        IntegerType *globalType = Type::getInt32Ty(getGlobalContext());

        if (iter1 != varMap.end() && iter2 != varMap.end()) {
            constInt1 = dyn_cast<ConstantInt>(iter1->second);
            constInt2 = dyn_cast<ConstantInt>(iter2->second);
            if (constInt1 == nullptr || constInt2 == nullptr) {
                cerr << "error: can't support unintialized variables " << endl;
                exit(EXIT_FAILURE);
            }
            const1 = constInt1->getSExtValue();
            const2 = constInt2->getSExtValue();
        }
        if (iter1 == varMap.end() && iter2 != varMap.end()) {
            constInt1 = dyn_cast<ConstantInt>(op1);
            constInt2 = dyn_cast<ConstantInt>(iter2->second);
            if (constInt2 == nullptr) {
                cerr << "error: can't support unintialized variables " << endl;
                exit(EXIT_FAILURE);
            }
            const1 = constInt1->getSExtValue();
            const2 = constInt2->getSExtValue();
        }
        if (iter1 != varMap.end() && iter2 == varMap.end()) {
            constInt1 = dyn_cast<ConstantInt>(iter1->second);
            constInt2 = dyn_cast<ConstantInt>(op2);
            if (constInt1 == nullptr) {
                cerr << "error: can't support unintialized variables " << endl;
                exit(EXIT_FAILURE);
            }
            const1 = constInt1->getSExtValue();
            const2 = constInt2->getSExtValue();
        }

        switch(I.getOpcode()) {
            case Instruction::Add:
                result = ConstantInt::getSigned(globalType, const1 + const2);
                break;
            case Instruction::Sub:
                result = ConstantInt::getSigned(globalType, const1 - const2);
                break;
            case Instruction::Mul:
                result = ConstantInt::getSigned(globalType, const1 * const2);
                break;
            case Instruction::SRem:
                result = ConstantInt::getSigned(globalType, const1 % const2);
                break;
        }
        varMap.insert(make_pair(dyn_cast<Value>(&I), dyn_cast<Value>(result)));
    }
}

map<Value*, Value*> initVars(BasicBlock *BB)
{
    map<Value*, Value*> varMap;

    for (auto &I: *BB) {
        if (isa<AllocaInst>(I)) {
            varMap.insert(make_pair(dyn_cast<Value>(&I), dyn_cast<Value>(&I)));
        }
    }
    return varMap;
}

int sep(Value* v1, Value* v2)
{
    int int1 = dyn_cast<ConstantInt>(v1)->getSExtValue();
    int int2 = dyn_cast<ConstantInt>(v2)->getSExtValue();
    if (int1 >= int2) {
        return int1 - int2;
    } else {
        return int2 - int1;
    }
}

void traverseCFG(
    BasicBlock* BB,
    int &blkCount,
    map<Value*, Value*> varMap)
{

    for (auto &I: *BB) {
        updateVars(I, varMap);
    }

    cout << "Block " << blkCount << ":" << endl;
    printResult(varMap);

    ++blkCount;

    const TerminatorInst *TInst = BB->getTerminator();
    unsigned int NSucc = TInst->getNumSuccessors();
    for (unsigned i = 0; i < NSucc; ++i) {
        BasicBlock *Succ = TInst->getSuccessor(i);
        traverseCFG(Succ, blkCount, varMap);
    }
}

void printResult(map<Value*, Value*> map)
{
    for (auto it = map.begin(); it != map.end(); ++it) {
        for (auto jt = it; jt != map.end(); ++jt) {
            Value *var1 = (*it).first;
            Value *int1 = (*it).second;
            Value *var2 = (*jt).first;
            Value *int2 = (*jt).second;

            if (var1 == var2) continue;

            if (int1 == var1 || int2 == var2) continue;

            if (var1->hasName() && var2->hasName()) {
                cout << "sep(" << var1->getName().str().c_str() << ", "
                << var2->getName().str().c_str() << ") = "
                << sep(int1, int2) << endl;
            }
        }
    }
}
