# plush
Plush a simple UNIX shell written with C.


Plush Shell Features ->
1. The prompt and iterated input taking works flawlessly.
2. Shell can parse and execute all built in system commands.
3. Properly supports input and output redirections.
4. Supports multiple in-line commands separated by (;)
5. Supports multiple command in sequence (&&)
6. History is saved for each sessions. history command can be used to access it.
7. Signal handling works properly even though code shows incomplete struct.
8. Pipping is partially working. Issues occur with complex commands most likely problem while recursively iterating the BST and parsing.

Kindly use spaces after each command, pipes/semicolons/ampersands and arguements otherwise shell cannot detect them properly.

cd is not supported. 

Known issues includes ->
Trailing/leading spaces. 
Some | ; && combinations altogether used
executing only the end piped command etc.


Resources used -> 
Brennan: https://brennan.io/2015/01/16/write-a-shell-in-c/
CodeVault: https://www.youtube.com/c/CodeVault
HHp3: https://www.youtube.com/@hhp3
Cobb Coding: https://www.youtube.com/@cobbcoding
