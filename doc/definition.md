Base Types
==========
chr     - character
int     - integral number
real    - real number
lst     - homogeneous list of base type instances
map     - homogeneous list of key-value pairs
tup     - heterogeneous collection of base type instances
func    - function
strm    - IO stream (reading/writing)

tup and func types are actually defined by their signature, i.e.

1. every possible grouping combination of base types is an individual `tup` type
1. every possible combination of return types and parameter types is an individual `func` type


tuple type signatures:

