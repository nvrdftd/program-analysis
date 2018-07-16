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

void generateCFG(BasicBlock* BB, int &counter, set<Value*> &sourceVars, set<BasicBlock *> &traversalBlocks);
bool compareSets(set<Value*> a, set<Value*> b);

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
    set<BasicBlock*> traversalBlocks;
    int counter = 1;
    for (auto &F: *M)
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            cout << "Start collecting the blocks to traverse over..." << endl;
            generateCFG(BB, counter, sourceVars, traversalBlocks);
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




void generateCFG(BasicBlock* BB, int &counter, set<Value*> &sourceVars, set<BasicBlock*> &traversalBlocks)
{

    set<Value*> sinkVars = checkTainted(BB, sourceVars);

    if (traversalBlocks.find(BB) != traversalBlocks.end() && compareSets(sinkVars, sourceVars)) {
        return;
    }

    sourceVars = sinkVars; // Update the tainted varaiable set
    traversalBlocks.insert(BB);

    // Print out the tainted variables
    cout << "Block " << counter << ": {";
    for (auto i = sourceVars.begin(); i != sourceVars.end(); ++i) {

        if((*i)->hasName())
            cout << (*i)->getName().str().c_str() << ", ";
    }
    cout << "}" << endl;
    ++counter;

    const TerminatorInst *TInst = BB->getTerminator();
    unsigned int NSucc = TInst->getNumSuccessors();
    for (unsigned i = 0; i < NSucc; ++i) {
        BasicBlock *Succ = TInst->getSuccessor(i);
        generateCFG(Succ, counter, sourceVars, traversalBlocks);
    }

    if (NSucc == 0) {
        cout << "Tainted Variables: {";
        for (auto i = sourceVars.begin(); i != sourceVars.end(); ++i) {

            if((*i)->hasName())
                cout << (*i)->getName().str().c_str() << ", ";
        }
        cout << "}" << endl;
    }

}

bool compareSets(set<Value*> a, set<Value*> b)
{
    for (auto e: a) {
        if (b.find(e) == b.end()) return false;
    }
    if (a.size() == b.size())
        return true;
    else
        return false;
}
