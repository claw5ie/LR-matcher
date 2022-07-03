# LR(0) item sets

## Grammar syntax

`<Var>: <Seq> | <Seq> | ... | <Seq>; <Var>: <Seq> | <Seq> | ... | <Seq>; ...`

`<Var>`: string that starts with upper case letter and can contain upper/lower case letters, numbers, dash (`-`) or underscore (`_`).

`<Seq>`: any sequence of characters or variables, possibly separated by spaces.

Special characters (`:`, `;`, `|`, ` `, `\` and upper case letters) can be escaped with backslash (`\`). For example: `A: A\|A | a`.

## Examples of grammars:

* `S: (S)S | ()`
* `EmptyString: `
* `A: a A a | b B b | ; B: c`

## Compilation and execution

Using GNU make
```
make
./a.out "S: (S)S | ()"
```
or GCC
```
g++ -O3 src/main.cpp src/Grammar.cpp src/ItemSets.cpp
./a.out "S: (S)S | ()"
```
