/************************************************************************-
 *  VMTranslator translates .vm files to .asm files according to the HACK computer and VM language specifications.
 *  Usage: vmtranslator <path to .vm file or directory containing .vm files> 
 *  Output: A .asm file in the same directory as path with the same name.
 *
 *  Hack VM specifications:
 *      
 *      A frame on the stack is essentially a save point. When a function is called, the pointers
 *      (sp, local, argument, this, that) 
 *      Are saved on the stack, along with the return address so that you can return to the previous code's
 *      State once you return from the current function.
 *      The frames are stored as follows:
 *          <...Potential Arguments...>
 *          <return address>
 *          <local pointer value>
 *          <argument pointer value>
 *          <this pointer value>
 *          <that pointer value>
 *          <...Local Variables...>
 *
 *      The hack architecture separates the RAM into several memory segments:
 *          - "The stack" : This is the primary stack where values are pushed, popped, and have operations done to them. The pointer value is at RAM[0].
 *          - local : For local variables. There is one local segment for each function on the stack. The pointer value is at RAM[1].
 *          - argument : For arguments in functions. There is one local segment for each function on the stack. The pointer value is at RAM[2].
 *          - this : <Not yet explained in the course! Assumed to be for 'this' references.> The pointer value is at RAM[3].
 *          - that : <Not yet explained in the course!> The pointer value is at RAM[4].
 *          - temp : Used for storing temporary values. The range is from RAM[5] to RAM [12].
 *          - static : For static variables. Each .vm file has it's own set of static variables.
 *          Note: The keyword "constant" can be used, but is not actually a memory segment and instead allows the pushing of specific numbers into the stack.
 *      The base addresses of these segments are found in pointers, which are the first several registers in RAM. 
 *      The most used is sp, meaning stack pointer. This points to the current, available location on the stack. Therefore,
 *      The top-most current value on the stack at any given time is at *sp--, unless the stack is empty.
 *      These base addressed may be combined with an index, if the memory segment is more than one register.
 *          I.E. local[5] = Value stored in the local pointer + 5, used as an address(*local+5).
 *
 *      The commands in hack's vm language are as follows:
 *          - push <memseg> <index> : Takes a value from the address specified with <memseg> <index> and puts it in the current stack register.
 *          - pop <memseg> <index> : Takes the top-most value off the stack and stores it in the address specified by <memseg> <index>.
 *      
 *          Logical/Arithmetic commands:
 *          Note: The ones that take two inputs (add, sub, etc) use the top two stack values. The ones that use one (not, etc) 
 *                use the top-most value. All commands replace their factors on the stack with the result (I.E. stack = 3,4. After add, stack = 7).
 *                The first factor in every operation is the second top-most value. We will use x (*sp - 2) and y(*sp--) below.
 *
 *              - add : x + y.
 *              - sub : x - y.
 *              - neg : y = -y.
 *              - eq : Outputs true or false depending on if x == y.
 *              - get : Ouputs true or false depending on if x >= y.
 *              - lt : Ouputs true or false depending on if x < y.
 *              - gt : Ouputs true or false depending on if x > y.
 *              - and : Performs the operation x & y.
 *              - or : Performs the operation x | y.
 *              - not : y = !y.
 *
 *
 *          Function commands:
 *          Note: All function names are expected to be proceeded by "<file name>."
 *                  I.E. Main.myFunction.
 *              - function <name> <number of local variables> : Declares a label for easy jumping.
 *                Initializes local variables by pushing to the stack as many zeros as are local variables.
 *              - call <function name> <number of arguments> : Pushes a frame for the current (caller) function.
 *                Insures that the argument and local pointers are set correctly for the new function.
 *              - return : Resets all pointers with the latest frame. Essentially replaces the latest frame and
 *                Any code after it with the return value, which is the top stack value before return is called.
 *
 *
 *          Program Flow commands:
 *
 *              - label <label name> : Inserts a label into the asm code. The label should only be a name;
 *                The file name and function name will automatically be included.
 *              - goto <label name> : Jumps the code to the specified label. If the label is for a function, the
 *                Function name should proceed it. I.E. Main.myLabel. If it is for a normal label, all the proceeding/preceding 
 *                Requirements are met automatically.
 *              - if-goto <label name> : The exact same as goto, however it will only execute if the top stack value is a -1.
 *
 *
 *      Notes:
 *          This VMTranslator assumes no errors in vm code, as per class instructions.
 *          -1 means true logically, and 0 means false.
 *          The sp pointer always points to the register after the top stack value.
 *          R13-15 are reserved for use as temp registers for the VMTranslator.
 *          The only way to hard-code numbers into hack asm (besides 1,0-1) is to address the number you wish to hard-code and use the A register.
 *
 *  Started: January 15, 2018
 *  Finished: January 26, 2018
 *  Updates:
 *      - Added function commands. Finished March 18th, 2018.
 * 
 *  ©2018 C. A. Acred all rights reserved.
 ----------------------------------------------------------*
*/

// Compile: g++ main.cpp VMTranslator/VMTranslator.cpp -o vmtranslator -std=c++11 -static-libgcc -static-libstdc++
// Debug:   g++ -g main.cpp VMTranslator/VMTranslator.cpp -o vmtranslator -std=c++11 -static-libgcc -static-libstdc++

#include "VMTranslator/VMTranslator.h"
#include <iostream>

main(int argc, char** argv)
{
    if (argc != 2) // Make sure you got a path, and only one path.
    {
        cout << "Invalid usage; Usage: vmtranslator (path to .vm file or dir of .vm files)\n";
        return 1;
    }
    
    VMTranslator* vmTranslator = new VMTranslator();
    int error = vmTranslator->translate(argv[1]);
    if (error == 1)
    {
        cout << "Error has occurred!\n";
        return error;
    }
    return 0;
}