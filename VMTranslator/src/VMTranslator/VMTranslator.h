/************************************************************************-
 *  VMTranslator.h, contains the classes needed to create the VMTranslator.
 *
 *  Started: January 15, 2018
 *  Finished: January 26, 2018
 *  Updates:
 *      - 
 *  ©2018 C. A. Acred all rights reserved.
 ----------------------------------------------------------*
*/

#ifndef VMTRANSLATOR_H
#define VMTRANSLATOR_H

#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

class Parser;
class Translator;
class VMTranslator;

/**
 * Parses the VM commands after removing excess whitespace and comments.
 * Stores parsed vm code in a 2D vector<string>, with each element of a command being separated.
 * I.E. output{lineOne{push, this, 10}, etc...}
 */
class Parser
{
private:
    const string EMPTY_STR = "";
    
    vector<vector<string>> output;
    vector<vector<string>> parseVMString(string input); 
    
public:
    Parser();
    ~Parser();
    
    void parseInput(string* input);
    vector<vector<string>> getOutput();
    string resolveExcess(string* input);
};


/**
 * Translates vm code into hack asm code. 
 * Precedes asm translation with a comment of the command in vm.
 */
class Translator
{
private:
    // Register numbers for pointers according to the hack vm specification. sp is included for consistency.
    const vector<vector<string>> STACK_BASE_NUMS = {{"sp", "0"}, {"local", "1"}, {"argument", "2"}, {"this", "3"}, {"that", "4"}};
    string output;
    string fileName; // Current VM file name.
    int asmLineNum; // Current .asm line number.
    string curFuncName = ""; // Used to create labels within a function, so they are not mixed up with other labels.
    int curStaticNum; // The count of static variables.
    
    // ASM code corresponding to vm commands(not \n terminated):
    string getLastTwoVal;
    string dereference;
    
    string createVMComment(vector<string> vm);
    void translateVMCom(vector<string> vm);
    void translatePopPush(vector<string> vm);
    void translateAL(string vm);
    void translateLabel(string labelName, bool isFunc);
    void translateGoTo(vector<string> goToCom, bool isFunc);
    void translateFuncCom(vector<string> funcCom);
    void translateCallCom(vector<string> funcCom);
    void translateReturnCom();
    string getPointer(string input);
    void initializePremadeASM();
    void addASMOutput(string input);
    
public:
    Translator();
    ~Translator();
    
    void addInitCode();
    void translateInput(vector<vector<string>> input);
    string getOutput();
};


/**
 * Translates vm code at path into HACK asm code.
 * Creates a new .asm file of the same name as path, in the same directory.
 */
 class VMTranslator 
 {
private: 
    string input;
    Parser* parser;
    Translator* translator;
    
    int loadInput(string* path);
    
public:
    VMTranslator();
    ~VMTranslator();
    
    int translate(char* path);
    static vector<string> getLine(string* input, int start);
    static vector<string> getLine(string* input, int start, char endChar);
    bool isDirectory(string* input);
    string getFileExtention(string input);
 };

#endif