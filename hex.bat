@echo off
goto begin

*************
Hex by Arnaud
*************

syntax:
hex <board_size> <pie_rule> <monte_carlo> <processors> <player_color_X> <computer_color_O> <selection_color>

<board_size>		the size of the board					(default = 11, cannot be lower than 3)
<pie_rule>		YES/NO; is pie rule enforced				(default = YES)
<symmetry>		YES/NO; use symmetry for the pie rule			(default = YES)
<first_move>		X/O; who plays first, player [X] or computer [O]	(default = X)
<monte_carlo>		cumulated number of Monte Carlo paths for computer's AI	(default = 3000, cannot be lower than 100)
<processors>		number of core processors to use			(default = 2, cannot be lower than 1)
<player_color_X>	color for player [X]					(default = 15, white)
<computer_color_O>	color for computer [O]					(default = 12, red)
<selection_color>	color for active selection background			(default = 8, gray)
<display_modifs>	YES/NO; display parameters screen on startup		(default = YES)

black		0	gray	8
dark_blue	1	blue	9
dark_green	2	green	10
dark_aqua	3	aqua	11
dark_cyan	3	cyan	11
dark_red	4	red	12
dark_purple	5	purple	13
dark_pink	5	pink	13
dark_magenta	5	magenta	13
dark_yellow	6	yellow	14
dark_white	7	white	15

Update the command line below to change the defaults

*************

:begin

rem 	<board_size> <pie_rule> <symmetry> <first_move> <monte_carlo> <processors> <player_color_X> <computer_color_O> <selection_color> <display_modifs>
hex	          7        YES        YES            X          5000            6               15                 12                 8               NO