20130608: 
Mostly non-semantic change moving the main classes to their own headers.

The parser templates moved from peg.cpp to peg.h, except for the hs-specific
templates.

There have been some terms added to the Haskell expression. Adding them does
not seem to have broken the single test case. There does seem to be one more 
term left. In aexp the [exp|qual,..] is not addressed.

The Haskell grammar doesn't address layout. That would have to be done 
separately. I have layout code elsewhere.
