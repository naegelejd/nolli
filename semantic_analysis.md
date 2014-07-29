## Semantic Analysis

### Unit (A single file)

- Make or find package table for specified package name.

### Imports

- Find specified packages (files), parse them, obtain their AST.
- Build or add to existing package table for each AST

### Classes

- Make class table
- Add members to class members symbol table
- Add methods to class methods table

### Functions

- Add function name+signature (type) to scoped symbol table
- Make scoped symbol table for function body
- Add parameters to scoped symbol table
- If a method, add class members to scoped symbol table
- Analyze statements
- Check return value type

## If Statements

- Create scoped symbol table for true and false blocks
- Check that conditional expression type is boolean
- Check true statements
- Check false statements

## While Statements

- Create scoped symbol table
- Check that conditional expression type is boolean
- Check body

## For Statements

- Check that expression is iterable
- Create scoped symbol table
- Add identifier+type to scoped symbol table
- Check body

## Bind Statement

- Check that identifier is not declared in CURRENT scoped symbol table
- Check initializer expression
- Add identifer+type to CURRENT scoped symbol table

## Assignment Statement

- Ensure identifer is declared in scoped symbol table
- Check assignment expression
- Ensure type of assignment expression matches type of identifier

## Declarations

- Ensure idenitifier is not already declared in CURRENT scoped symbol table
- Add identifier+type to scoped symbol table

## Class Literals

- Check that identifier is a class
- For each initializer:
    - Ensure that identifier is valid class member
    - Analyze expression
    - Ensure expression's type matches class member's type

## List Literals

- For each element:
    - Check expression
    - Ensure expression type matches previous expression's type

## Map Literals

- For each element:
    - Check key expression
    - Check value expression
    - Ensure key's type matches previous key's type
    - Ensure value's type matches previous value's type

## Binary Expressions

- Check LHS
- Check RHS
- Check that type of LHS equals type of RHS
- Check that type of LHS and type RHS are valid for binary operator?

## Unary Expressions

- Check RHS
- Check that type of RHS is valid for unary operator

