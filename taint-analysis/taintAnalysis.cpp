#include <cstdio>
#include <iostream>
#include <set>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Value.h"

using namespace llvm;
using namespace std;

void generateCFG(BasicBlock* BB, int &counter, set<Value*> sourceVars, set<Value*> &finalVars);

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

    // tainted variable set
    set<Value*> sourceVars;
    set<Value*> finalVars;

    int counter = 1;
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            generateCFG(BB, counter, sourceVars, finalVars);
            cout << "Tainted Variables: {";
            for (auto i = finalVars.begin(); i != finalVars.end(); ++i) {
                if((*i)->hasName())
                    cout << (*i)->getName().str().c_str() << ", ";
            }
            cout << "}" << endl;
        }
    return 0;
}


set<Value*> checkTainted(BasicBlock* BB, set<Value*> sinkVars)
{
    for (auto &I: *BB) {
        if (strncmp(I.getName().str().c_str(), "source", 6) == 0)
            sinkVars.insert(dyn_cast<Value>(&I));




        if (isa<StoreInst>(I)) {
            // Check store instructions
            Value* storeFrom = I.getOperand(0);
            Value* storeTo = I.getOperand(1);
            if (sinkVars.find(storeFrom) != sinkVars.end())
                sinkVars.insert(storeTo);
            else if ((strncmp(storeTo->getName().str().c_str(), "source", 6) != 0) &&
                     (sinkVars.find(storeTo) != sinkVars.end()))
                sinkVars.erase(storeTo);

        } else {
            // Check all other instructions
            for (unsigned x = 0; x < I.getNumOperands(); ++x) {
                Value *v = I.getOperand(x);
                if (sinkVars.find(v) != sinkVars.end())
         	          sinkVars.insert(dyn_cast<Value>(&I));
            }
        }
    }
    return sinkVars;
}




void generateCFG(BasicBlock* BB, int &counter, set<Value*> sourceVars, set<Value*> &finalVars)
{

    cout << "Block " << counter << ": {";
    set<Value*> sinkVars = checkTainted(BB, sourceVars);
    // Print out the tainted variables
    for (auto i = sinkVars.begin(); i != sinkVars.end(); ++i) {
        if((*i)->hasName())
            cout << (*i)->getName().str().c_str() << ", ";
    }
    cout << "}" << endl;
    ++counter;
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned int NSucc = TInst->getNumSuccessors();
    for (unsigned i = 0; i < NSucc; ++i) {
        BasicBlock *Succ = TInst->getSuccessor(i);
        generateCFG(Succ, counter, sinkVars, finalVars);
    }

    if (NSucc == 0) {
        finalVars.insert(sinkVars.begin(), sinkVars.end());
    }
}
