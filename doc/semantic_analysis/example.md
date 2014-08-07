Enter `nl_analyze(...)` with 2 ASTs, 1 for each "unit" (file).

Make symbol tables:
- Make type, symbol tables for package "demo"
- Make type, symbol tables for package "shape"

Collect class names:
- Add "BoxedString" to "demo" type table with an empty `class` type
- Add "Circle" to "shape" type table with an empty `class` type

Collect aliases:
- Add "CircleType" to "demo" type table with type `Circle` from "shape" type table
- Add "impure" to "demo" type table with type `func ()`

Collect class types:
- Update "BoxedString" in "demo" type table with template,member,method names+types
- Update "Circle" in "shape" type table with template,member,method names+types

Collect function signatures:
- Add "main" to "demo" symbol table with type `func int (list<str> args)`
- Add "isUnitCircle" to "shape" symbol table with type `func bool (Circle c)`

Collect global declarations:
- Add "C" to "demo" symbol table with type `Circle` from "shape" type table
- Add "bs" to "demo" symbol table with type `BoxedString`
- Add "pi" to "shape" symbol table with type `real`
- Add "UnitCircle" to "shape" symbol table with type `Circle`

Analyze global initializations:
- Type-check the initialization of "C" in "demo"
- Type-check the initialization of "pi" in "shape"
- Type-check the initialization of "UnitCircle" in "shape"

Analyze class methods and function bodies:
- Analyze each statement of "area" in "Circle" in "shape"
- Analyze each statement of "main" in "demo"
- Analyze each statement of "isUnitCircle" in "shape"
