g++ -g main.cpp VMTranslator/VMTranslator.cpp -o vmtranslator -std=c++11 "-lstdc++fs" -static-libgcc -static-libstdc++
gdb --args vmtranslator.exe C:\Users\Night_Blader\Desktop\nand2tetris\projects\08\ProgramFlow\FibonacciSeries\FibonacciSeries.vm
