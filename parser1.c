/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1.c                                                          */
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
#include "./headers/global.h"
#include "./headers/scanner.h"
#include "./headers/line.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile; /*  CPL source comes from here.          */
PRIVATE FILE *ListFile; /*  For nicely-formatted syntax errors.  */

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
PRIVATE void ParseRestOfStatement(void);
PRIVATE void ParseProcCallList(void);
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
PRIVATE void ParseBooleanExpression(void);
PRIVATE void ParseAddOp(void);
PRIVATE void ParseMultOp(void);
PRIVATE void ParseRelOp(void);
PRIVATE void Accept(int code);
PRIVATE void ReadToEndOfFile(void);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PUBLIC int main(int argc, char *argv[]) {
    if (OpenFiles(argc, argv)) {
        InitCharProcessor(InputFile, ListFile);
        CurrentToken = GetToken();
        ParseProgram();
        fclose(InputFile);
        fclose(ListFile);
        printf("Valid\n");
        return EXIT_SUCCESS;
    } else
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

PRIVATE void ParseProgram(void) {
    Accept(PROGRAM);
    Accept(IDENTIFIER);
    Accept(SEMICOLON);

    if (CurrentToken.code == VAR) {
        ParseDeclarations();
    }

    while (CurrentToken.code == PROCEDURE) {
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


PRIVATE void ParseDeclarations(void) {
    Accept(VAR);
    Accept(IDENTIFIER);

    while (CurrentToken.code == COMMA) {
        Accept(COMMA);
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


PRIVATE void ParseProcDeclaration(void) {
    Accept(PROCEDURE);
    Accept(IDENTIFIER);

    if (CurrentToken.code == LEFTPARENTHESIS) {
        ParseParameterList();
    }

    Accept(SEMICOLON);

    if (CurrentToken.code == VAR) {
        ParseDeclarations();
    }

    while (CurrentToken.code == PROCEDURE) {
        ParseProcDeclaration();
    }

    ParseBlock();

    Accept(SEMICOLON);
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


PRIVATE void ParseParameterList(void) {
    Accept(LEFTPARENTHESIS);
    ParseFormalParameter();

    while (CurrentToken.code == COMMA) {
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

PRIVATE void ParseFormalParameter(void) {
    if (CurrentToken.code == REF) {
        Accept(REF);
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

PRIVATE void ParseBlock(void) {
    Accept(BEGIN);

    while (CurrentToken.code == WHILE || CurrentToken.code == IF || CurrentToken.code == READ || CurrentToken.code == WRITE || CurrentToken.code == IDENTIFIER) {
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

PRIVATE void ParseStatement(void) {

    /*Not sure how simple statement works, but this should call the rest of
      the statements correctly.*/

    if (CurrentToken.code == WHILE) {
        ParseWhileStatement();
    } else if (CurrentToken.code == IF) {
        ParseIfStatement();
    } else if (CurrentToken.code == READ) {
        ParseReadStatement();
    } else if (CurrentToken.code == WRITE) {
        ParseWriteStatement();
    } else {
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

PRIVATE void ParseSimpleStatement(void) {
    Accept(IDENTIFIER);
    ParseRestOfStatement();
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

PRIVATE void ParseRestOfStatement(void) {
    if (CurrentToken.code == LEFTPARENTHESIS)
        ParseProcCallList();
    else if (CurrentToken.code == ASSIGNMENT)
        ParseAssignment();
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

PRIVATE void ParseProcCallList(void) {
    Accept(LEFTPARENTHESIS);

    ParseActualParameter();

    while (CurrentToken.code == COMMA) {
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

PRIVATE void ParseAssignment(void) {
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

PRIVATE void ParseActualParameter(void) {
    Accept(IDENTIFIER);
    ParseExpression();
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

PRIVATE void ParseWhileStatement(void) {
    Accept(WHILE);
    ParseBooleanExpression();
    Accept(DO);
    ParseBlock();
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

PRIVATE void ParseIfStatement(void) {
    Accept(IF);
    ParseBooleanExpression();
    Accept(THEN);
    ParseBlock();

    if (CurrentToken.code == ELSE) {
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

PRIVATE void ParseReadStatement(void) {
    Accept(READ);
    Accept(LEFTPARENTHESIS);
    Accept(IDENTIFIER);
    while (CurrentToken.code == COMMA) {
        Accept(COMMA);
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

PRIVATE void ParseWriteStatement(void) {
    Accept(WRITE);
    Accept(LEFTPARENTHESIS);
    ParseExpression();
    while (CurrentToken.code == COMMA) {
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

PRIVATE void ParseExpression(void) {
    ParseCompoundTerm();
    while (CurrentToken.code == ADD || CurrentToken.code == SUBTRACT) {
        ParseAddOp();
        ParseCompoundTerm();
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

PRIVATE void ParseCompoundTerm(void) {
    ParseTerm();
    while (CurrentToken.code == MULTIPLY || CurrentToken.code == DIVIDE) {
        ParseMultOp();
        ParseTerm();
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

PRIVATE void ParseTerm(void) {
    if (CurrentToken.code == SUBTRACT)
        Accept(SUBTRACT);
    ParseSubTerm();
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

PRIVATE void ParseSubTerm(void) {
    if (CurrentToken.code == IDENTIFIER)
        Accept(IDENTIFIER);
    else if (CurrentToken.code == INTCONST)
        Accept(INTCONST);
    else if (CurrentToken.code == LEFTPARENTHESIS) {
        Accept(LEFTPARENTHESIS);
        ParseExpression();
        Accept(RIGHTPARENTHESIS);
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

PRIVATE void ParseBooleanExpression(void) {
    ParseExpression();
    ParseRelOp();
    ParseExpression();
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

PRIVATE void ParseAddOp(void) {
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

PRIVATE void ParseMultOp(void) {
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

PRIVATE void ParseRelOp(void) {
    if (CurrentToken.code == EQUALITY)
        Accept(EQUALITY);
    else if (CurrentToken.code == LESSEQUAL)
        Accept(LESSEQUAL);
    else if (CurrentToken.code == GREATEREQUAL)
        Accept(GREATEREQUAL);
    else if (CurrentToken.code == LESS)
        Accept(LESS);
    else if (CurrentToken.code == GREATER)
        Accept(GREATER);
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

PRIVATE void Accept(int ExpectedToken) {
    if (CurrentToken.code != ExpectedToken) {
        SyntaxError(ExpectedToken, CurrentToken);
        printf("Syntax Error\n");
        ReadToEndOfFile();
        fclose(InputFile);
        fclose(ListFile);
        exit(EXIT_FAILURE);
    } else CurrentToken = GetToken();
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

PRIVATE int OpenFiles(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "%s <inputfile> <listfile>\n", argv[0]);
        return 0;
    }

    if (NULL == (InputFile = fopen(argv[1], "r"))) {
        fprintf(stderr, "cannot open \"%s\" for input\n", argv[1]);
        return 0;
    }

    if (NULL == (ListFile = fopen(argv[2], "w"))) {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
        fclose(InputFile);
        return 0;
    }

    return 1;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadToEndOfFile:  Reads all remaining tokens from the input file.       */
/*              associated input and listing files.                         */
/*                                                                          */
/*    This is used to ensure that the listing file refects the entire       */
/*    input, even after a syntax error (because of crash & burn parsing,    */
/*    if a routine like this is not used, the listing file will not be      */
/*    complete.  Note that this routine also reports in the listing file    */
/*    exactly where the parsing stopped.  Note that this routine is         */
/*    superfluous in a parser that performs error-recovery.                 */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Reads all remaining tokens from the input.  There won't */
/*                  be any more available input after this routine returns. */
/*                                                                          */

/*--------------------------------------------------------------------------*/

PRIVATE void ReadToEndOfFile(void) {
    if (CurrentToken.code != ENDOFINPUT) {
        Error("Parsing ends here in this program\n", CurrentToken.pos);
        while (CurrentToken.code != ENDOFINPUT) CurrentToken = GetToken();
    }
}
