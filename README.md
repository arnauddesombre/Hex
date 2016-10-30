# Hex
Classical game of Hex
(Hex is a board game for two players. It was independently invented by Piet Hein in 1942 and John Nash in 1948 and became popular after 1950 under the name Hex).

This repository includes the source code in C++11, a compiled version (for Windows 64-bits), and a well documented configuration launch hex.bat file, which allows the change of board size, pie rule usage, first player (player or computer), colors...

Compilation instructions are included in the hex.cpp stand-alone source code - it is the only source file needed. I've used MinGW and compiled using:

    g++ -Wall -O3 -o "hex" "hex.cpp" -s -std=c++11 -static-libgcc -static-libstdc++ -static -lwinpthread
If you have a C++11 version installed, you can generate a smaller .exe file using:

    g++ -Wall -O3 -o "hex" "hex.cpp" -s -std=c++11

Hex's Artificial Intelligence is a Monte-Carlo, and the software uses parallel threading for maximum efficiency. Monte-Carlo is not so efficient for Hex, as the algorithm to determine who the winner is (Breadth-First Search, or BFS) is time expensive, and the time to make a computer move in an 11x11 board will be about 20 seconds at the beginning of the game when all positions are open. But the computer will play very well...

Hex is a board game described in [Wikipedia](https://en.wikipedia.org/wiki/Hex_%28board_game%29). The rules are simple:

Hex is most commonly played on a board with 11x11 cells, but it can also be played on a board of another size. The red player tries to connect the two red borders with a chain of red cells by coloring empty cells red, while the blue player tries the same with the blue borders.<br>
The game can never end with a draw and when all cells have been colored, there must exists either a red chain or a blue chain.<br>a
The player who moves first has a big advantage. In order to compensate for this, there is often used the so-called 'swap rule' or 'pie rule': After the first move, the second player is allowed to swap the sides.
