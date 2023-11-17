# LR(0) grammar matcher

## Context-free grammar syntax

### Backus-Naur form

General form:
```
<Variable0> ::= Rule00 | Rule01 | ... | Rule0m
<Variable1> ::= Rule10 | Rule11 | ... | Rule1m
...
<Variablen> ::= Rulen0 | Rulen1 | ... | Rulenm
```

Where `Rulenm` is a sequence of variables (`<Variable>`) or strings (`"string"`). For example:

```
<Expr> ::= <Expr> "+" <Expr> | <Expr> "*" <Expr>
<Expr> ::= "0" | "1"
```

Double quote (`"`) can be escaped inside of a string with dash (`\`), like `"\"Hello, World!\""`

### Custom form

General form:
```
Variable0: Rule00 | Rule01 | ... | Rule0m;
Variable1: Rule10 | Rule11 | ... | Rule1m;
Variablen: Rulen0 | Rulen1 | ... | Rulenm;
```

`Variablen` is a string that starts with upper case letter and can contain upper/lower case letters, numbers, dash (`-`) or underscore (`_`). Example: `Expr-n_0`.

`Rulenm` is any sequence of characters or variables, possibly separated by spaces, like `Expr: Expr + Expr` or `Expr: Expr+Expr`.

Characters `:`, `;`, `|`, ` `, `\` and upper case letters can be escaped with backslash (`\`). For example: `A: A\|A | a`.

## Command line options

| Option               | Argument       | Description |
| :------------------: | :------------: | ----------- |
| `-f`                 | `bnf`/`custom` | Intepret string in Backus-Naur form or custom form |
| `--generate-steps`   | None           | Generaete JSON containing steps needed to simulate pushdown automaton |

## Examples of grammars:

* `S: (S)S | ()`
* `EmptyString: `
* `A: a A a | b B b | ; B: c`

## Compilation and execution

```
g++ -O3 -o a.out src/main.cpp
./a.out "S: (S)S | ()"
```
