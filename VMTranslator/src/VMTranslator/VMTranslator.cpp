/************************************************************************-
 *  VMTranslator.cpp, the implementation for VMTranslator.h.
 *  
 * 
 *  Started: January 15, 2018
 *  Finished: January 26, 2018
 *  Updates:
 *      - 
 *  ©2018 C. A. Acred all rights reserved.
 ----------------------------------------------------------*
*/
#include "VMTranslator.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <experimental/filesystem>


// Parser: 
Parser::Parser(){}

/**
 * Parses the vm code.
 * Stores the result in this->output.
 * 
 * @param input Unprocessed VM code as string pointer.
 */
void Parser::parseInput(string* input)
{
    *input = resolveExcess(input); // Find and remove all white space, excess newlines, and comments.
    
    output = parseVMString(*input); // Parse the VM commands into a 2D vector<string>.
    
    return;
}

/**
 * Gets the string* output.
 * 
 * @return string* output
 */
vector<vector<string>> Parser::getOutput()
{
    return this->output;
}

/**
 * Removes whitespace and comments from input.
 *
 * @param input The pointer to the string you wish to resolve.
 * @return The resolved version of input.
 */
string Parser::resolveExcess(string* input)
{
    int i = 0;
    char curChar = input->at(0);
    vector<string> temp;
    string output = "";
    string curLine = this->EMPTY_STR;
    
    while (curChar != '\0')// Iterate through input until it ends.
    {
        if (curChar == '/' && input->at(i+1) == '/') // If this line is a comment, skip it.
        {
            temp = VMTranslator::getLine(input, i);
            i += atoi(temp.at(1).c_str()); // Skip '\n' char
            if (curLine != this->EMPTY_STR) // If this is a comment after a command:
            {
                output.append(curLine);  // Add curLine to output plus '\n'.
                output.append(1, '\n');
                curLine = this->EMPTY_STR;
            }
        }
        else if (curChar == '\n') // If it's a new line char:
        {
            if (curLine != this->EMPTY_STR) // If we are not on a new line (If this is not just an extra new line).
            {
                output.append(curLine);  // Add curLine to output plus '\n'.
                output.append(1, curChar);
                curLine = this->EMPTY_STR;
            }
            i++;
        }
        else
        {
            if (curChar == ' ') // Skip white space if it is not in a command.
            {   // If we are on a command      && The next char is not a space,             a /,                   or a \.:
                if (curLine != this->EMPTY_STR && input->at(i+1) != ' ' && input->at(i+1) != '/' && input->at(i+1) != '\\') // If we are on a command and this is a space in between the elements:
                    curLine.append(1, curChar);
            }
            else 
                curLine.append(1, curChar);
            i++;
        }
        curChar = input->at(i);
        if (i == input->length()-1)
            break;
    }
    output.append(1, '\0');
    
    return output;
}

/**
 * Parses the VM commands, line by line. This logic is done here.
 * Command elements are separated by whitespace.
 * 
 * @param input Unprocessed VM code as string pointer.
 * @return A vector<vector<string>>, with each command in the first vector, and the elements of the commands in the second.
 */
vector<vector<string>> Parser::parseVMString(string input)
{
    char curChar = ' '; 
    int i = 0;
    string curComEle = this->EMPTY_STR;
    vector<vector<string>> result;
    vector<string> curCom;
    while (curChar != '\0')
    {
        curChar = input.at(i);
        if (curChar == ' ') // If we have ended a part of a command:
        {
            curCom.push_back(curComEle);
            curComEle = this->EMPTY_STR;
        }
        else if (curChar == '\n') // If we are at the end of a line:
        {
            if (curComEle != this->EMPTY_STR) // If there is a command element that we haven't added to curCom:
            {
                curCom.push_back(curComEle);
                curComEle = this->EMPTY_STR;
            }
            result.push_back(curCom);
            curCom.clear();
        }
        else
        {
            curComEle += curChar;
        }
        i++;
    }
    
    return result;
}

// Translator: 

/**
 * Initializes values for the Translator.
 */
Translator::Translator()
{
    this->output = "";
    this->fileName = fileName;
    this->asmLineNum = 0;
    //this->curStaticNum = 0;
    
    initializePremadeASM();
    
    return;
}

/**
 * Creates a string that is a comment version of vm's command.
 *
 * @param vm The vector<string> that contains the vm command.
 * @return The comment as a string.
 */
string Translator::createVMComment(vector<string> vm)
{
    string output = "// ";
    
    for (int i = 0; i < vm.size(); i++)
    {
        output += vm.at(i) + " ";
    }
    
    return output + ":\n";
}

 /**
  * Translate a parsed vm command as a vector<string>.
  *
  * @param vm vector<string> of a vm command, being parsed by Parser.
  */
 void Translator::translateVMCom(vector<string> vm)
 {
    if (vm.at(0) == "push" || vm.at(0) == "pop") // If it's a push/pop command:
    {
        translatePopPush(vm);
    }
    else if (vm.at(0) == "label")
    {
        translateLabel(vm.at(1), false);
    }
    else if (vm.at(0).find("goto") != string::npos) // If the command is a goto or if-goto:
    {
        translateGoTo(vm, false);
    }
    else if (vm.at(0) == "function")
    {
        translateFuncCom(vm);
    }
    else if (vm.at(0) == "call")
    {
        translateCallCom(vm);
    }
    else if (vm.at(0) == "return")
    {
        translateReturnCom();
    }
    else if (vm.at(0) == "newfile")
    {
        this->fileName = vm.at(1);
    }
    else // If it's any other (arithmetic/logical) command:
    {
        translateAL(vm.at(0));
    }
    
    return;
 }

/**
 * Translates a push or pop command into asm.
 * Push takes the value at the address specified in vm.at(1-2) and puts it on the stack (*sp). sp is incremented by 1.
 * Pop decrements sp by 1, then takes the value at sp (*sp) and stores it in the address specified in vm.at(1-2).
 *
 * @param vm A vector<string> containing a pop/push vm command.
 */
 void Translator::translatePopPush(vector<string> vm)
 {
    string externalAddress = ""; // externalAddress is to hold the address that's not from sp; The address specified by vm.at(1)&.at(2).
    
    /* Push logic:
     *  Go to external address; Put M in D; Go to sp address; Store D in M; sp++.
     *
     * Pop logic:
     *  Store external address in R13; Go to top stack value; Store M in D; Go to externalAddress(*R13); Store D in M.
     */
    
    bool isConstant = false; // Needed because of the exact opposite needs of a constant value - Needed the address value rather than the value at the address.
    bool isPush = false;
    if (vm.at(0) == "push") 
        isPush = true;
    
    // Get externalAddress:
    if (vm.at(1) == "temp")
    { // temp registers start at reg 5, and there are 8 of them. 
        externalAddress = std::to_string(5 + std::stoi(vm.at(2)));
    }
    else if (vm.at(1) == "pointer")
    {
        if (vm.at(2) == "0")
            externalAddress = getPointer("this"); // May need to be recursive; Future lesson should tell.
        else
            externalAddress = getPointer("that");
    }
    else if (vm.at(1) == "static")
    {
        //externalAddress = this->fileName + "." + std::to_string(this->curStaticNum) + "." + vm.at(2);
        externalAddress = this->fileName + "." + vm.at(2);
        //this->curStaticNum++;
    }
    else if (vm.at(1) == "constant")
    {
        isConstant = true;
        externalAddress = vm.at(2);
    }
    else
    {
        externalAddress = getPointer(vm.at(1)) + "\nD=M\n@" + vm.at(2) + "\nD=D+A\nA=D"; // asm code to go to register[<pointer>+<index>].
    }
    
    if (isPush) 
    {
        addASMOutput("@" + externalAddress + "\n"); // Go to externalAddress.
        if (isConstant) // If it's a constant value wanted, we need to take the address value of externalAddress.
            addASMOutput("D=A\n"); // Get externalAddress A value in D.
        else
            addASMOutput("D=M\n"); // Get value at externalAddress into D.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "\n"); // Go to the register sp is pointing to. (@*sp).
        addASMOutput("M=D\n"); // Store D into *sp.
        addASMOutput("@" + getPointer("sp") + "\nM=M+1\n"); // sp++
    }
    else 
    {
        addASMOutput("@"+ externalAddress + "\n"); // Go to externalAddress.
        addASMOutput("D=A\n@R13\nM=D\n"); // Store externalAddress in R13.
        addASMOutput("@" + getPointer("sp") + "\nAM=M-1\n"); // Go to top value on the stack, and sp--.
        addASMOutput("D=M\n"); // Store value at sp in D.
        addASMOutput("@R13\n" + dereference + "\nM=D\n"); // Go to externalAddress and put D in it.
    }
    
    return;
 }
 
/**
 * Translates an arithmetic/logic command into asm code.
 * By convention, AL commands will only contain one string.
 *
 * @param vm A vector<string> containing an A/L vm command.
 */
 void Translator::translateAL(string vm)
 {
    if (vm == "add")
    {
        addASMOutput(this->getLastTwoVal + "\nM=M+D\n");
    }
    else if (vm == "sub")
    {
        addASMOutput(this->getLastTwoVal + "\nM=M-D\n");
    }
    else if (vm == "neg")
    {
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=-M\n");
    }
    else if (vm == "eq") // Equal
    {
        translateAL("sub");
        addASMOutput("D=M\n");
        addASMOutput("@" + std::to_string(this->asmLineNum + 7) + "\n"); // Set code address to jump to if eq.
        addASMOutput("D;JEQ\n"); // If eq, jump to code for eq.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=0\n"); // Set top stack value to 0 (false) for !eq.
        addASMOutput("@" + std::to_string(this->asmLineNum + 5) + "\n"); // Set address to jump over the eq code.
        addASMOutput("0;JMP\n"); // Jump over the eq code.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=-1\n"); // Set top stack value to -1 (true) for eq. 
    }
    else if (vm == "get") // Greater than or equal to
    {
        translateAL("sub");
        addASMOutput("D=M\n");
        addASMOutput("@" + std::to_string(this->asmLineNum + 7) + "\n"); // Set code address to jump to if get.
        addASMOutput("D;JGE\n"); // If get, jump to code for get.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=0\n"); // Set top stack value to 0 (false) for !get.
        addASMOutput("@" + std::to_string(this->asmLineNum + 5) + "\n"); // Set address to jump over the get code.
        addASMOutput("0;JMP\n"); // Jump over the get code.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=-1\n"); // Set top stack value to -1 (true) for get.
    }
    else if (vm == "lt") // Less than
    {
        translateAL("sub");
        addASMOutput("D=M\n");
        addASMOutput("@" + std::to_string(this->asmLineNum + 7) + "\n"); // Set code address to jump to if lt.
        addASMOutput("D;JLT\n"); // If lt, jump to code for lt.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=0\n"); // Set top stack value to 0 (false) for !lt.
        addASMOutput("@" + std::to_string(this->asmLineNum + 5) + "\n"); // Set address to jump over the lt code.
        addASMOutput("0;JMP\n"); // Jump over the lt code.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=-1\n"); // Set top stack value to -1 (true) for lt.
    }
    else if (vm == "gt") // Greater than
    {
        translateAL("sub");
        addASMOutput("D=M\n");
        addASMOutput("@" + std::to_string(this->asmLineNum + 7) + "\n"); // Set code address to jump to if gt.
        addASMOutput("D;JGT\n"); // If gt, jump to code for gt.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=0\n"); // Set top stack value to 0 (false) for !gt.
        addASMOutput("@" + std::to_string(this->asmLineNum + 5) + "\n"); // Set address to jump over the gt code.
        addASMOutput("0;JMP\n"); // Jump over the gt code.
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\nM=-1\n"); // Set top stack value to -1 (true) for gt.
    }
    else if (vm == "and")
    {
        addASMOutput(this->getLastTwoVal + "\n"); // Get the top two values of the stack.
        addASMOutput("M=D&M\n"); // Set register[*sp--] to the top two values "and"ed. 
    }
    else if (vm == "or")
    {
        addASMOutput(this->getLastTwoVal + "\n"); // Get the top two values of the stack.
        addASMOutput("M=D|M\n"); // Set register[*sp--] to the top two values "or"ed. 
    }
    else if (vm == "not")
    {
        addASMOutput("@" + getPointer("sp") + "\n" + dereference + "-1\n"); // Go to the top value on the stack.
        addASMOutput("M=!M\n"); // Set register[*sp--] to not itself (!M).
    }
    return;
 }
 
/**
 * Translates vm label commands.
 * A label is a marker in the code that can be returned to. It is used to reuse code.
 * If the label is the start of a function, the function name must be in the format: <VMFileName>.<FunctionName>.
 *
 * @param labelName A string containing a label name.
 * @param isFunc A bool that says whether the label is intended to be a function or not.
 */
 void Translator::translateLabel(string labelName, bool isFunc)
 {
    transform(labelName.begin(), labelName.end(), labelName.begin(),::toupper);
    if (isFunc)
        addASMOutput("(" + labelName + ")\n");
    else
        addASMOutput("(" + this->fileName + "." + this->curFuncName + "$" + labelName + ")\n");
    return;
 }
 
 /**
 * Translates vm go-to commands. It can be conditional (if-goto) or unconditional (goto).
 * The condition is based off the value on the top of the stack (*sp--).
 * The goto refers to a label, which represents a point in the program. 
 * If the command is conditional, the sp is decremented to overwrite the condition value.
 * If the label is the start of a function, the function name must be in the format: <VMFileName>.<FunctionName>.
 *
 * @param goToCom A vector<string> containing a go-to vm command.
 * @param isFunc A bool that says whether the label that we intend to go to is a function label or not.
 */
 void Translator::translateGoTo(vector<string> goToCom, bool isFunc)
 {
 // If the go to is a label, you need to add the prefix. If it is a function, you only need to add the fileName as a prefix.
    transform(goToCom.at(1).begin(), goToCom.at(1).end(), goToCom.at(1).begin(),::toupper); // Make label name uppercase.
    if (goToCom.at(0) == "goto")
    {
        if (!isFunc)
            addASMOutput("@" + this->fileName + "." + this->curFuncName + "$" + goToCom.at(1) + "\n"); // Load the address for the label in question.
        else 
            addASMOutput("@" + goToCom.at(1) + "\n"); // Load the address for the label in question.
        addASMOutput("0;JMP\n"); // Jump the code to the label.
    }
    else // If the command is if-goto:
    {
        addASMOutput("@" + getPointer("sp") + "\nM=M-1\n" + dereference + "\n"); // Go to *sp-- and decrement sp.
        addASMOutput("D=M\n"); // Save the value of the top of the stack in D (it should be either true (-1) or false (0)).
        if (!isFunc)
            addASMOutput("@" + this->fileName + "." + this->curFuncName + "$" + goToCom.at(1) + "\n"); // Load the address for the label in question.
        else
            addASMOutput("@" + this->fileName + "." + goToCom.at(1) + "\n"); // Load the address for the label in question.
        addASMOutput("D;JNE\n"); // Jump if D is true (-1; Jump if D != 0).
    }
    
    return;
 }
 
/**
 * Translates the vm function command: function <functionName> <nVars>.
 * This command defines a function in the code to be reused. 
 * A function can take some arguments, have some local variables, and always returns one value.
 * <functionName> is the desired name of the function. This will be accessed via a label.
 * <nVars> specifies how many local variables this function has. They are initialized to 0.
 *
 * @param funcCom A vector<string> containing a function vm command.
 */
 void Translator::translateFuncCom(vector<string> funcCom)
 {
    this->curFuncName = funcCom.at(1);
    output += createVMComment({"label", funcCom.at(1)}); // Add a comment for this command.
    translateLabel(funcCom.at(1), true);
    
    for (int i = 0; i < std::stoi(funcCom.at(2)); i++)
        translatePopPush({"push", "constant", "0"});
    
    return;
 }
 
/**
 * Translates the vm function command: function <functionName> <nArgs>.
 * This command runs the pre-defined code in the function <functionName>.
 * <nArgs> specifies how many arguments <functionName> takes from the stack.
 *
 * @param callCom A vector<string> containing a call vm command.
 */
 void Translator::translateCallCom(vector<string> callCom)
 {
    // Save return address, local, argument, this, that:
    
    // Push return address:
    string returnAdd = "";
    if (callCom.size() > 2) // Need to skip all the following call code for the return address.
        returnAdd = std::to_string(this->asmLineNum + 47);
    else
        returnAdd = std::to_string(this->asmLineNum + 40); // Doesn't have to skip the conditional code for arguments.
    output += createVMComment({"push", "constant", returnAdd}); // Add a comment for this command.
    translatePopPush({"push", "constant", returnAdd});
    
    // Push local, argument, this, and that pointers:
    for (int i = 0; i < 4; i++)
    {
        addASMOutput("@" + getPointer(this->STACK_BASE_NUMS.at(i+1).at(0)) + "\nD=M\n"); // Go to  pointer. Put M in D.
        addASMOutput("@" + getPointer("sp") + "\nA=M\nM=D\n"); // Save  pointer value at *sp.
        addASMOutput("@" + getPointer("sp") + "\nM=M+1\n"); // sp++.
    }
    
    // Adjust argument segment pointer using callCom.at(3): 
    if (callCom.size() > 2) // If the function has arguments.
    {
        addASMOutput("@" + std::to_string(stoi(callCom.at(2)) + 5) + "\nD=A\n");
        addASMOutput("@" + getPointer("sp") + "\nD=M-D\n");
        addASMOutput("@" + getPointer("argument") + "\nM=D\n");
        
        // Adjust local segment to point to the upcoming local vars:
        addASMOutput("@" + getPointer("sp") + "\n");
    }
    // count. 
    addASMOutput("D=M\n"); // Don't need to get sp again since it's already the current address.
    addASMOutput("@" + getPointer("local") + "\nM=D\n");
    
    // Go to function:
    
    output += createVMComment({"goto", callCom.at(1)}); // Add a comment for this command.
    translateGoTo({"goto", callCom.at(1)}, true);
    
    return;
 }
 
/**
 * Translates the vm return command.
 * This command resets the stack, so that the function we are currently in returns a value to the stack instead of 
 * All it's processing that was previously on the stack. The return value will be located where the original first argument was.
 *
 * Previous states of the local, argument, this, and that pointers are saved on the stack relative to the current local pointer.
 * The saved that is at local - 1.
 * this is at local - 2.
 * argument is at local - 3.
 * local is at local - 4.
 * The return address is in local - 5.
 *
 * The return value will be located at *sp--.
 */
 void Translator::translateReturnCom()
 {
    // Save return value at R13:
    addASMOutput("@" + getPointer("sp") + "\nA=M-1\nD=M\n"); // Put the return value in D.
    addASMOutput("@R13\nM=D\n"); // Save D at R13 for now.
    
    // Save argument at R14; sp should be set here once return is finished.
    addASMOutput("@" + getPointer("argument") + "\nD=M\n@R14\nM=D\n");
    
    // Set sp to current local:
    addASMOutput("@" + getPointer("local") + "\nD=M\n@" + getPointer("sp") + "\nM=D\n"); // Set sp to local.
    
    //  Reset local, argument, this, and that:
    for (int i = 0; i < 4; i++)
    {
        addASMOutput("@" + getPointer("sp") + "\nAM=M-1\n"); // Decrement and go to *sp.
        addASMOutput("D=M\n"); // Get value of M into D. This is a saved value for resetting the pointers.
        addASMOutput("@" + getPointer(this->STACK_BASE_NUMS.at(4-i).at(0)) + "\nM=D\n"); // Set the that pointer to this saved value.
    }
    
    // Save the return address at R15:
    addASMOutput("@" + getPointer("sp") + "\nAM=M-1\n"); // Decrement and go to *sp.
    addASMOutput("D=M\n"); // Put return address in D.
    addASMOutput("@R15\nM=D\n"); // Store return address in R15 temporarily.
    
    // Set sp to the value at R14:
    addASMOutput("@R14\nD=M\n@" + getPointer("sp") + "\nM=D\n");
    
    // Push return value (R13):
    addASMOutput("@R13\nD=M\n@" + getPointer("sp") + "\n" + dereference + "\nM=D\n");
    addASMOutput("@" + getPointer("sp") + "\n" + "M=M+1\n"); // sp++
    
    
    // Jump to return address at R15:
    addASMOutput("@R15\nA=M\n"); // Go to *R15.
    addASMOutput("0;JMP\n"); // Jump.
    return;
 }
 
/**
 * Fetches the appropriate register address that maps to *input. These values are stored in STACK_BASE_NUMS.
 * The first value of a sub vector is the name for the pointer, the second is it's corresponding register address.
 * These register locations are part of the hack vm architecture. They serve as stack pointers.
 * 
 * @param input A string pointer to the register pointer name.
 * @return A string of the register address.
 */
 string Translator::getPointer(string input)
 {
    for (int i = 0; i < this->STACK_BASE_NUMS.size(); i++)
    {
        if (this->STACK_BASE_NUMS.at(i).at(0) == input)
            return this->STACK_BASE_NUMS.at(i).at(1);
    }
 }
 
/**
 * Initializes premade asm code, such as the code to get the top two values off the stack.
 * *Not* terminated with a \n, more asm code may be added on to last command.
 */
 void Translator::initializePremadeASM()
 {
    this->dereference = "A=M"; 
    this->getLastTwoVal = "@" + getPointer("sp") + "\nM=M-1\n" + dereference + "\nD=M\nA=A-1"; // Gets top two values from the stack. The top into D, the second into M. sp--.
 }
 
/**
 * Adds asm code to this->output.
 * Keeps track of current asm line number in this->asmLineNum.
 * 
 * @param input A string of the asm code you wish to add to this->output.
 */
 void Translator::addASMOutput(string input)
 {
    if (input[0] != '(') // Label declarations are not included in the final machine code. Thus, they should not add to the line number.
        this->asmLineNum += std::count(input.begin(), input.end(), '\n'); // Keep track of what the current asm line is.
    
    output += input;
    return;
 }

/**
 * Adds asm initialization code, required for every hack program.
 */
 void Translator::addInitCode()
 {    
    // Set sp to 256:
    addASMOutput("@256\nD=A\n@" + getPointer("sp") + "\nM=D\n");
    
    // Call Sys.init:
    this->fileName = "Sys.vm";
    vector<string> temp = {"call", "Sys.init"};
    output += createVMComment(temp); // Add a comment for this command.
    translateVMCom(temp);
 }
 
 /**
 * Translates the vm program, parsed by the Parser, into this->output (a string) as an asm program,
 * Complete with comments.
 * Requires input to be parsed by Parser.
 *
 * @param input The pointer to the 2D string vector that is the parsed input. It MUST have been parsed with Parser.
 * @param fileName The name of the vm file we are translating.
 */
void Translator::translateInput(vector<vector<string>> input)
{
    vector<string> curCom;

    for (int i = 0; i < input.size(); i++)
    {
        curCom = input.at(i);
        
        // Add a comment in output preceding the asm translation that says the vm code to be translated.
        output += createVMComment(curCom);
        
        // Translate the current VM Command.
        translateVMCom(curCom);
    }
    
    return;
}
 
/**
 * Gets the output of the interpretation.
 *
 * @return The interpreted output as a NULL terminated string.
 */
 string Translator::getOutput()
 {
    return output;
 }
 
// VMTranslator:

/**
 * Initializes members of VMTranslator.
 */
VMTranslator::VMTranslator()
{
    translator = new Translator();
    parser = new Parser();
    this->input = "";
    return;
}

/**
 * Opens path, translates the .vm file into .asm.
 * Outputs this new .asm file with the same name into the same directory.
 * If path is a directory, then all .vm files will be translated into a .asm file with the directory's name.
 *
 * @return 0 if file at path was loaded successfully, 1 if not.
 */
 int VMTranslator::translate(char* path)
 {
    // Go through dir, find a vm file, and then add it to this->input:
    string pathS = string(path); // Get input path.
    string outputFileName = pathS.substr(pathS.find_last_of("\\") + 1); // Set the name for the output .asm file.

    bool isDir = isDirectory(&pathS);
    
    if (isDir)
    {
        namespace fs = std::experimental::filesystem;
        fs::path fileSysPath(path); 
        
        if(!exists(fileSysPath) || !is_directory(fileSysPath)) 
        {
            cout << fileSysPath << " is not a proper path!\n";
            return 1;
        }
        fs::recursive_directory_iterator begin(fileSysPath), end;
        vector<fs::directory_entry> files(begin, end);
        
        // Iterate through the files and add the .vm files to this->input:
        string temp;
        for (int i = 0; i < files.size(); i++)
        {
            temp = files.at(i).path().string();
            if (getFileExtention(temp) == "vm")
                loadInput(&temp);
        }
        
    }
    else
    {
        loadInput(&pathS);
        
        outputFileName = outputFileName.substr(0, outputFileName.length() - 3);
        
        // Remove the file name from pathS:
        pathS = pathS.substr(0, pathS.find_last_of("\\"));
    }
    this->input.append(1, '\0'); // Add NULL terminating char.
    
    
    if (isDir)
    {
        // Add init code:
        translator->addInitCode();
    }

    
    // Logic:
    parser->parseInput(&input); // Parse commands.
    
    vector<vector<string>> parsedOutput = parser->getOutput();
    
    translator->translateInput(parsedOutput); // Translate.
    
    string transOutput = translator->getOutput();
    
    
    //Output:
    pathS = pathS + "\\" + outputFileName + ".asm"; // Change file extension to asm.
    ofstream outputFile;
    outputFile.open(pathS, ios::out);
    outputFile << transOutput;
    outputFile.close();
    
    return 0;
 }
 
 /**
 * Load file at path into this->input.
 *
 * @param input The path as a char*.
 * @return 0 if the file at path loaded successfully, 1 if not.
 */
 int VMTranslator::loadInput(string* path)
 {
    input.append("newfile " + path->substr(path->find_last_of("\\") + 1, path->length() - 3) + "\n"); // Tell the translator what file this is.
    
    ifstream vmFile;
    vmFile.open(path->c_str(), ios::in);
    if (!vmFile.is_open()) // If path does not open properly.
    {
        cout << "Path invalid; Usage: vmtranslator (path to .vm file/directory)\n";
        return 1;
    }
    
    string temp; // Get the contents of the vm file into input.
    while (std::getline(vmFile, temp))
    {
        this->input.append(temp);
        this->input.append("\n");
    }
    vmFile.close();
    
    return 0;
 }
 
/**
 * Takes input, starts at start, and returns a string with the value to offset the received string and \n char.
 * The string is a substring of *input. It is the characters from *input[start] till a \n char.
 * 
 * @param input the string* of which you want the line.
 * @param start
 * @return The line as output.at(0), the value needed to offset the received string + \n at output.at(1) as a string.
 */
vector<string> VMTranslator::getLine(string* input, int start)
{
    char curChar = input->at(start);
    vector<string> output = {"", ""};
    int i = start;
    while (curChar != '\n')
    {
        output.at(0).append(1, curChar);
        i++;
        curChar = input->at(i);
    }
    i++;
    output.at(1) = std::to_string(i - start);
    
    return output;
}

/**
 * Tests to see is input is a directory path or not.
 * Checks to see if there is a file extension (implying a file and not a dir).ont
 * 
 * @param input the string that is the path you are testing.
 * @return True if input is a dir.
 */
bool VMTranslator::isDirectory(string* input)
{
    if ((*input).find_last_of(".") != string::npos)
        return false;
    return true;
}

/**
 * Returns the file extention of a given path.
 * 
 * @param input the string of the path you want the extension from.
 * @return The file extension as a string.
 */
string VMTranslator::getFileExtention(string input)
{
    return input.substr(input.find_last_of(".") + 1);
}
