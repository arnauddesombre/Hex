# Hex
Classical game of Hex

This repository includes the source code in C++11, a compiled version (Windows 64-bits), and a configuration launch hex.bat file, which allows the change of board size, pie rule usage, first player (player or computer), colors...

Compilation instructions are included in the hex.cpp source code, which is the only source file needed. I've used MinGW and the instructions are for a Geany build.

Hex's Artificial Intelligence is a Monte-Carlo, and the software uses parallel threading for maximum efficiency. Monte-Carlo is not so efficient for Hex, as the algorithm to determine who the winner is (Breadth-First Search, or BFS) is time expensive, and the time to make a computer move in an 11x11 board will be about 20 seconds at the beginning of the game when all positions are open. But the computer will play very well...

Hex is a board game described in Wikipedia:
https://en.wikipedia.org/wiki/Hex_(board_game)
