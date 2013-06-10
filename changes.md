20130609b:
Finished the Makefile. Now G++ 4.5.3 with --std=c++0x compiles the program.
The test programs are built AND executed with "make test". The makefile isn't
super smart about headers or cleaning.

20130609:
Reorg the test code into separate programs for each test grammar. The interesting
equations grammar remains in peg.cpp for now.

The aexp = [exp|qual+] production has been added to aexp. This seemed nicer after
moving aexp. No testing beyond the fact that the existing peghs test still works.

20130608: 
Mostly non-semantic change moving the main classes to their own headers.

The parser templates moved from peg.cpp to peg.h, except for the hs-specific
templates.

There have been some terms added to the Haskell expression. Adding them does
not seem to have broken the single test case. There does seem to be one more 
term left. In aexp the [exp|qual,..] is not addressed.

The Haskell grammar doesn't address layout. That would have to be done 
separately. I have layout code elsewhere.
