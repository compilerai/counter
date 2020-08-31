Sections-

1. Parser logic
2. Changes 
3. TO-DO

1. PARSER LOGIC

a) Organized as a tree with nodes having different significance:-
   i) Function nodes
   ii) Conditional(if then else) nodes
   iii) Boolean nodes(representing a boolean expression)
   iv) Value nodes( nodes representing simple value)

b) Functions for constructors, solvers, and freeing of memory
c) Unresolved references to functions are saved in a map in the first pass
d) In the second pass these references are resolved.
e) The result of a node is defined by the struct "result"
f) A function map containing results of global and local functions is created
g) A function for converting "result" to "expr_ref" added
h) map of function name(only globals) and the corresponding expr_ref returned 

2. MAJOR CHANGES

Changes made over the commit "6096f9a1b450c10e3ec9df930d35085ffcaa21a3" (July 31st)

a) Added a folder lib/parser

-> z3_model.l,z3_model.ypp (lexer and parser)
-> parse_tree.h (separate file for isolating parser logic from the grammer)
-> test.cpp (executable for testing correctness)

b) smt_helper_process

-> read entire model and write to file.
-> send name of file via pipe.

c) z3_solver

-> reads the file name and calls parsing function.

3. TO-DO

Important ->
	a) Add utility functions to expr.h for printing and generalize constructors and functions for array(currently used aseem's version of the same)
	b) Verify expr generated using parser
	c) Create parser for yices as smt_helper_process may use either for the model -> 
		Ideally we would want the parse_tree file to work for both and only the lexer and parser file be added.

Code Quality
	a) Since now smt_helper_process is a cpp file change copy_to_buffer function and use stl_functions
	b) Converting to expr_ref maybe done directly instead of first converting to "result"
