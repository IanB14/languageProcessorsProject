/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp1.c                                                          */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Ian Burke            13122525                                  */
/*           Lorcan Chinnock      14174103                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code.h"
#include "debug.h"
#include "global.h"
#include "line.h"
#include "scanner.h"
#include "sets.h"
#include "strtab.h"
#include "symbol.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile; /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;  /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;  /*  This is the output machine code file */

PRIVATE TOKEN CurrentToken; /*  Parser lookahead token.  Updated by  */
                            /*  routine Accept (below).  Must be     */
                            /*  initialised before parser starts.    */

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int OpenFiles(int argc, char *argv[]);
PRIVATE void ParseProgram(void);
PRIVATE void ParseDeclarations(void);
PRIVATE void ParseProcDeclaration(void);
PRIVATE void ParseParameterList(void);
PRIVATE void ParseFormalParameter(void);
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestOfStatement(SYMBOL *target);
PRIVATE void ParseProcCallList(SYMBOL *target);
PRIVATE void ParseAssignment(void);
PRIVATE void ParseActualParameter(void);
PRIVATE void ParseWhileStatement(void);
PRIVATE void ParseIfStatement(void);
PRIVATE void ParseReadStatement(void);
PRIVATE void ParseWriteStatement(void);
PRIVATE void ParseExpression(void);
PRIVATE void ParseCompoundTerm(void);
PRIVATE void ParseTerm(void);
PRIVATE void ParseSubTerm(void);
PRIVATE int ParseBooleanExpression(void);
PRIVATE void ParseAddOp(void);
PRIVATE void ParseMultOp(void);
PRIVATE int ParseRelOp(void);
PRIVATE void Accept(int code);
PRIVATE void MakeSymbolTableEntry(int symtype);
PRIVATE SYMBOL *LookupSymbol(void);
PRIVATE void ParseOpPrec(int minPrec);

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */

/*--------------------------------------------------------------------------*/
int errCount = 0; /*Int that counts amount of errors received by parser*/
int scope = 0;    //Global scope

PUBLIC int main(int argc, char *argv[])
{
    if (OpenFiles(argc, argv))
    {
        InitCharProcessor(InputFile, ListFile);
        InitCodeGenerator(CodeFile);
        CurrentToken = GetToken();
        ParseProgram();
        WriteCodeFile();
        fclose(InputFile);
        fclose(ListFile);
        if (errCount == 0)
        {
            printf("Valid\n");
        }
        return EXIT_SUCCESS;
    }
    else
        return EXIT_FAILURE;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*                                                                          */
/*  ParseProgram implements:                                                */
/*                                                                          */
/*       <Program>     :== "PROGRAM" <Identifier> ";" [ <Declarations> ]    */
/*		           { <ProcDeclaration> } <Block> "."                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram(void)
{
    Accept(PROGRAM);
    MakeSymbolTableEntry(STYPE_PROGRAM);
    Accept(IDENTIFIER);
    Accept(SEMICOLON);

    if (CurrentToken.code == VAR)
    {
        ParseDeclarations();
    }

    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclaration();
    }

    ParseBlock();

    Accept(ENDOFPROGRAM); /* Token "." has name ENDOFPROGRAM          */
}

/*--------------------------------------------------------------------------*/
/*	  ParseDeclarations implements:                                     */
/*                                                                          */
/*       <Declarations>     :== "VAR" <Variable> { "," <Variable> } ";"     */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations(void)
{
    Accept(VAR);
    MakeSymbolTableEntry(STYPE_VARIABLE);
    Accept(IDENTIFIER);

    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        MakeSymbolTableEntry(STYPE_VARIABLE);
        Accept(IDENTIFIER);
    }

    Accept(SEMICOLON);
}

/*--------------------------------------------------------------------------*/
/*	  ParseProcDeclaration implements:                                  */
/*                                                                          */
/*       <ProcDeclaration>     :== "PROCEDURE" <Identifier>                 */
/*                                 [<ParameterList>] ";" [<Declarations>]   */
/*                                 {<ProcDeclaration>} <Block> ";"          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration(void)
{
    Accept(PROCEDURE);
    MakeSymbolTableEntry(STYPE_PROCEDURE);
    Accept(IDENTIFIER);
    if (CurrentToken.code == LEFTPARENTHESIS)
    {
        ParseParameterList();
    }

    Accept(SEMICOLON);

    if (CurrentToken.code == VAR)
    {
        ParseDeclarations();
    }

    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclaration();
    }

    ParseBlock();

    Accept(SEMICOLON);
    RemoveSymbols(scope);
    scope--;
}

/*--------------------------------------------------------------------------*/
/*	  ParseParameterList implements:                                    */
/*                                                                          */
/*       <ParameterList>     :==  "(" <FormalParameter> { ","               */
/*				  <FormalParameter> } ")"                   */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseParameterList(void)
{
    Accept(LEFTPARENTHESIS);
    ParseFormalParameter();

    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseFormalParameter();
    }

    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*	  ParseFormalParameter implements:                                  */
/*                                                                          */
/*       <FormalParameter>     :==  [ "REF" ] <Variable>                    */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter(void)
{
    if (CurrentToken.code == REF)
    {
        Accept(REF);
        MakeSymbolTableEntry(STYPE_REFPAR);
    }
    else
    {
        MakeSymbolTableEntry(STYPE_VALUEPAR);
    }
    Accept(IDENTIFIER);
}

/*--------------------------------------------------------------------------*/
/*	  ParseBlock implements:		                            */
/*                                                                          */
/*       <Block>     :==  "BEGIN" { <Statement> ";" } "END"                 */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseBlock(void)
{
    Accept(BEGIN);

    while (CurrentToken.code == WHILE || CurrentToken.code == IF || CurrentToken.code == READ || CurrentToken.code == WRITE || CurrentToken.code == IDENTIFIER)
    {
        ParseStatement();
        Accept(SEMICOLON);
    }

    Accept(END);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*       <Statement>   :== <SimpleStatement> | <WhileStatement> |           */
/*			   <IfStatement> | <ReadStatement> |                */
/*			   <WriteStatement>                   		    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseStatement(void)
{

    /*Not sure how simple statement works, but this should call the rest of
	the statements correctly.*/

    if (CurrentToken.code == WHILE)
    {
        ParseWhileStatement();
    }
    else if (CurrentToken.code == IF)
    {
        ParseIfStatement();
    }
    else if (CurrentToken.code == READ)
    {
        ParseReadStatement();
    }
    else if (CurrentToken.code == WRITE)
    {
        ParseWriteStatement();
    }
    else
    {
        ParseSimpleStatement();
    }
}

/*--------------------------------------------------------------------------*/
/*	  ParseSimpleStatement implements:                                  */
/*                                                                          */
/*       <SimpleStatement>     :==  <VarOrProcName> <RestOfStatement>       */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement(void)
{

    SYMBOL *target;

    target = LookupSymbol();

    // Is this line unnecessary? Commenting out for now
    MakeSymbolTableEntry(STYPE_VALUEPAR);
    Accept(IDENTIFIER);
    ParseRestOfStatement(target);
}

/*--------------------------------------------------------------------------*/
/*	  ParseRestOfStatement implements:                                  */
/*                                                                          */
/*       <ParseRestOfStatement>     :==  <ProcCallList> | <Assignment> |    */
/*					 NULL     			    */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseRestOfStatement(SYMBOL *target)
{

    switch (CurrentToken.code)
    {
    case LEFTPARENTHESIS:
        ParseProcCallList(target);

    case SEMICOLON:
        if (target != NULL && target->type == STYPE_PROCEDURE)
            Emit(I_CALL, target->address);
        else
        {
            printf("Error - Not a procedure");
            KillCodeGeneration();
        }
        break;

    case ASSIGNMENT:
    default:
        ParseAssignment();
        if (target != NULL && target->type == STYPE_VARIABLE)
            Emit(I_STOREA, target->address);
        else
        {
            printf("Error - undeclared variable");
            KillCodeGeneration();
        }
    }
    /* Nothing needs to be parsed for epsilon */
}

/*--------------------------------------------------------------------------*/
/*	  ParseProcCallList implements:                                     */
/*                                                                          */
/*       <ProcCallList>     :==  "(" <ActualParameter> { ","                */
/*				 <ActualParameter> } ")"                    */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcCallList(SYMBOL *target)
{
    Accept(LEFTPARENTHESIS);

    ParseActualParameter();

    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseActualParameter();
    }

    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*	  ParseAssignment implements:                                       */
/*                                                                          */
/*       <Assignment>     :==  ":=" <Expression>                            */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseAssignment(void)
{
    Accept(ASSIGNMENT);
    ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*	  ParseActualParameter implements:                                  */
/*                                                                          */
/*       <ActualParameter>     :==  <Variable> | <Expression>               */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter(void)
{
    if (CurrentToken.code == SUBTRACT)
    {
        ParseExpression();
    }
    else if (CurrentToken.code == IDENTIFIER || CurrentToken.code == INTCONST || CurrentToken.code == LEFTPARENTHESIS)
    {
        ParseExpression();
    }
    else
    {
        MakeSymbolTableEntry(STYPE_LOCALVAR);
        Accept(IDENTIFIER);
    }
}

/*--------------------------------------------------------------------------*/
/*	  ParseWhileStatement implements:                                   */
/*                                                                          */
/*       <WhileStatement>     :==  "WHILE" <BooleanExpression> "DO" <Block> */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseWhileStatement(void)
{

    int Label1, Label2, L2BackPatchLoc;

    Accept(WHILE);
    Label1 = CurrentCodeAddress();
    L2BackPatchLoc = ParseBooleanExpression();
    Accept(DO);
    ParseBlock();
    Emit(I_BR, Label1);
    Label2 = CurrentCodeAddress();
    BackPatch(L2BackPatchLoc, Label2);
}

/*--------------------------------------------------------------------------*/
/*	  ParseIfStatement implements:                                      */
/*                                                                          */
/*       <IfStatement>     :==  "IF" <BooleanExpression> "THEN"             */
/*				<Block> ["ELSE" <Block> ]                   */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement(void)
{
    Accept(IF);
    ParseBooleanExpression();
    Accept(THEN);
    ParseBlock();

    if (CurrentToken.code == ELSE)
    {
        Accept(ELSE);
        ParseBlock();
    }
}

/*--------------------------------------------------------------------------*/
/*	  ParseReadStatement implements:                                    */
/*                                                                          */
/*       <ReadStatement>     :==  "READ" "(" <Variable> { "," <Variable>    */
/*                                 } ")"                                    */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement(void)
{
    Accept(READ);
    Accept(LEFTPARENTHESIS);
    MakeSymbolTableEntry(STYPE_VALUEPAR);
    Accept(IDENTIFIER);
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        MakeSymbolTableEntry(STYPE_VALUEPAR);
        Accept(IDENTIFIER);
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*	  ParseWriteStatement implements:                                   */
/*                                                                          */
/*       <WriteStatement>     :==  "WRITE" "(" <Expression> {","            */
/*				   <Expression>} ")"                        */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement(void)
{
    Accept(WRITE);
    Accept(LEFTPARENTHESIS);
    ParseExpression();
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseExpression();
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression>  :== <CompoundTerm> { <AddOp> <CompoundTerm> }        */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseExpression(void)
{

    int op;

    ParseCompoundTerm();
    ParseOpPrec(0);
    while ((CurrentToken.code == ADD) || (CurrentToken.code == SUBTRACT))
    {
        ParseAddOp();
        ParseCompoundTerm();

        if (op == ADD)
            _Emit(I_ADD);
        else
            _Emit(I_SUB);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                                           */
/*                                                                          */
/*       <CompoundTerm>  :== <Term> { <MultOp> <Term> }                       */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm(void)
{

    int op;

    ParseTerm();

    while ((CurrentToken.code == MULTIPLY) || (CurrentToken.code == DIVIDE))
    {
        ParseMultOp();
        ParseTerm();

        if (op == MULTIPLY)
            _Emit(I_MULT);
        else
            _Emit(I_DIV);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                                                   */
/*                                                                          */
/*       <Term>  :== [ "−" ] <SubTerm>                                      */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm(void)
{

    int negateflag = 0;

    if (CurrentToken.code == SUBTRACT)
        negateflag = 1;
    Accept(SUBTRACT);
    ParseSubTerm();

    if (negateflag)
        _Emit(I_NEG);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                                                */
/*                                                                          */
/*       <SubTerm>  :== <Variable> | <IntConst> | "(" <Expression> ")"      */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm(void)
{

    SYMBOL *var;

    switch (CurrentToken.code)
    {
    case IDENTIFIER:
    default:
        var = LookupSymbol();
        if (var != NULL && var->type == STYPE_VARIABLE)
        {
            Emit(I_LOADA, var->address);
        }
        else
        {
            printf("Error - Name undeclared or not a variable");
            Accept(IDENTIFIER);
            break;
        }
    case INTCONST:
        Emit(I_LOADI, CurrentToken.value);
        Accept(INTCONST);
        break;
    case LEFTPARENTHESIS:
        Accept(LEFTPARENTHESIS);
        ParseExpression();
        Accept(RIGHTPARENTHESIS);
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*       <BooleanExpression>  :== <Expression> <RelOp> <Expression>         */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE int ParseBooleanExpression(void)
{

    int BackPatchAddr, RelOpInstruction;
    ParseExpression();
    RelOpInstruction = ParseRelOp();
    ParseExpression();
    _Emit(I_SUB);
    BackPatchAddr = CurrentCodeAddress();
    Emit(RelOpInstruction, 0);
    return BackPatchAddr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                                  */
/*                                                                          */
/*       <AddOp>  :== "+" | "-"                                             */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseAddOp(void)
{
    if (CurrentToken.code == ADD)
        Accept(ADD);
    else if (CurrentToken.code == SUBTRACT)
        Accept(SUBTRACT);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                                  */
/*                                                                          */
/*       <MultOp>  :== "*" | "/"                                             */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ParseMultOp(void)
{
    if (CurrentToken.code == MULTIPLY)
        Accept(MULTIPLY);
    else if (CurrentToken.code == DIVIDE)
        Accept(DIVIDE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                                  */
/*                                                                          */
/*       <RelOp> :== "=" | "<=" | ">=" | "<" | ">"                          */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE int ParseRelOp(void)
{

    int RelOpInstruction;

    switch (CurrentToken.code)
    {
    case LESSEQUAL:
        RelOpInstruction = I_BG;
        Accept(LESSEQUAL);
        break;
    case GREATEREQUAL:
        RelOpInstruction = I_BL;
        Accept(GREATEREQUAL);
        break;
    case LESS:
        RelOpInstruction = I_BGZ;
        Accept(LESS);
        break;
    case GREATER:
        RelOpInstruction = I_BLZ;
        Accept(GREATER);
        break;
    case EQUALITY:
        RelOpInstruction = I_BR;
        Accept(EQUALITY);
        break;
    }

    return RelOpInstruction;
}

/*  No need to parse Variable                                               */

/*  No need to parse VarOrProcName                                          */

/*  No need to parse Identifier                                             */

/*  No need to parse IntConst                                               */

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void Accept(int ExpectedToken)
{
    static int recovering = 0;
    if (recovering)
    {
        while (CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT)
            CurrentToken = GetToken();
        recovering = 0;
    }
    if (CurrentToken.code != ExpectedToken)
    {
        printf("Syntax Error\n");
        SyntaxError(ExpectedToken, CurrentToken);
        errCount++;
        recovering = 1;
    }
    else
        CurrentToken = GetToken();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine mmodifies the globals "InputFile" and          */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE int OpenFiles(int argc, char *argv[])
{

    if (argc != 3)
    {
        fprintf(stderr, "%s <inputfile> <listfile>\n", argv[0]);
        return 0;
    }

    if (NULL == (InputFile = fopen(argv[1], "r")))
    {
        fprintf(stderr, "cannot open \"%s\" for input\n", argv[1]);
        return 0;
    }

    if (NULL == (ListFile = fopen(argv[2], "w")))
    {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
        fclose(InputFile);
        return 0;
    }

    return 1;
}

PRIVATE void MakeSymbolTableEntry(int symtype)
{
    //Variable declarations
    PRIVATE SYMBOL *oldsptr;
    PRIVATE SYMBOL *newsptr;
    char *cptr;
    int hashindex;
    int varaddress;
    if (CurrentToken.code == IDENTIFIER)
    {
        if (NULL == (oldsptr = Probe(CurrentToken.s, &hashindex)) || oldsptr->scope < scope)
        {
            if (oldsptr == NULL)
                cptr = CurrentToken.s;
            else
                cptr = oldsptr->s;
            if (NULL == (newsptr = EnterSymbol(cptr, hashindex)))
            {
                KillCodeGeneration();
            }
            else
            {
                if (oldsptr == NULL)
                    PreserveString();
                newsptr->scope = scope;
                newsptr->type = symtype;
                if (symtype == STYPE_VARIABLE)
                {
                    newsptr->address = varaddress;
                    varaddress++;
                }
                else
                    newsptr->address = -1;
            }
        }
        else
        {
            Error("Variable already declared", CurrentToken.pos);
        }
    }
}

PRIVATE SYMBOL *LookupSymbol(void)
{
    SYMBOL *sptr;
    if (CurrentToken.code == IDENTIFIER)
    {
        sptr = Probe(CurrentToken.s, NULL);
        if (sptr == NULL)
        {
            Error("Identifier not declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else
        sptr = NULL;
    return sptr;
}

//Need to be called in ParseExpression somewhere

// Declare the operator instruction array
int operatorInstruction[4];

PRIVATE void ParseOpPrec(int minPrec)
{

    // Declare 2 variables which will be used to determine precedence
    int op1, op2;

    // Declare the precedence array
    int prec[5];

    //This can be moved to a neater location later, but it's fine to leave it
    //here for now.
    prec[ADD] = 10;
    prec[SUBTRACT] = 10;
    prec[MULTIPLY] = 20;
    prec[DIVIDE] = 20;
    prec[LEFTPARENTHESIS] = -1;
    prec[SEMICOLON] = -1;

    // Again, this can be moved later
    operatorInstruction[ADD] = I_ADD;
    operatorInstruction[SUBTRACT] = I_SUB;
    operatorInstruction[MULTIPLY] = I_MULT;
    operatorInstruction[DIVIDE] = I_DIV;

    // Set op1 to whatever the current symbol is
    op1 = CurrentToken.code;

    // Begin a loop to check precedence - note that if the end of the expression is
    // reached, precedence will be less than one, which will end the loop
    while (prec[op1] >= minPrec)
    {
        CurrentToken = GetToken();

        // NOTE: This ParseTerm() was previously ParseInt(). This was replaced to handle
        // parentheses and unary minuses
        ParseTerm();
        // Get the second operator and write it to op2
        op2 = CurrentToken.code;

        // If op1 has a higher precedence than op2, op1 is run immediately
        // Otherwise, the precedence of op1 is incremented and the function is
        // called recursively
        if (prec[op2] > prec[op1])
            ParseOpPrec(prec[op1] + 1);

        // Emit whatever the operation is for op1 using the operatorInstruction
        // array above.
        _Emit(operatorInstruction[op1]);
        op1 = CurrentToken.code;
    }
}
