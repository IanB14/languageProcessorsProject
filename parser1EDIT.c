/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           John Doe            12345678                                   */
/*           Jane Murphy         23456789                                   */
/*           Anthony N. Other    12345679                                   */
/*                                                                          */
/*                                                                          */
/*       Currently just a copy of "smallparser.c".  To create "parser1.c",  */
/*       modify this source to reflect the CPL grammar.                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       smallparser                                                        */
/*                                                                          */
/*       An illustration of the use of the character handler and scanner    */
/*       in a parser for the language                                       */
/*                                                                          */
/*       <Program>     :== "BEGIN" { <Statement> ";" } "END" "."            */
/*       <Statement>   :== <Identifier> ":=" <Expression>                   */
/*       <Expression>  :== <Identifier> | <IntConst>                        */
/*                                                                          */
/*                                                                          */
/*       Note - <Identifier> and <IntConst> are provided by the scanner     */
/*       as tokens IDENTIFIER and INTCONST respectively.                    */
/*                                                                          */
/*       Although the listing file generator has to be initialised in       */
/*       this program, full listing files cannot be generated in the        */
/*       presence of errors because of the "crash and burn" error-          */
/*       handling policy adopted. Only the first error is reported, the     */
/*       remainder of the input is simply copied to the output (using       */
/*       the routine "ReadToEndOfFile") without further comment.            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
								   /*  routine Accept (below).  Must be     */
								   /*  initialised before parser starts.    */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles(int argc, char *argv[]);
PRIVATE void ParseProgram(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseExpression(void);
PRIVATE void Accept(int code);
PRIVATE void ReadToEndOfFile(void);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main(int argc, char *argv[])
{
	if (OpenFiles(argc, argv)) {
		InitCharProcessor(InputFile, ListFile);
		CurrentToken = GetToken();
		ParseProgram();
		fclose(InputFile);
		fclose(ListFile);
		return  EXIT_SUCCESS;
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
/*							{ <ProcDeclaration> } <Block> "."               */
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
	Accept(IDENTIFIER);
	Accept(SEMICOLON);
	
	if (CurrentToken.code == VAR) {
		ParseDeclarations();
	}
	
	while CurrentToken.code == PROCEDURE){
	ParseProcDeclaration();
	}
	
	ParseBlock();

    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
}


/*--------------------------------------------------------------------------*/
/*	  ParseDeclarations implements:                                         */
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


PRIVATE void ParseDeclarations(void){
	Accept (VAR);
	Accept (IDENTIFIER);
	
	while(currentToken.code == COMMA){
		Accept(COMMA);
		Accept (IDENTIFIER);
	}
	
	Accept(SEMICOLON);
}

/*--------------------------------------------------------------------------*/
/*	  ParseProcDeclaration implements:                                      */
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


PRIVATE void ParseProcDeclaration(void){
	Accept(PROCEDURE);
	Accept(IDENTIFIER);
	
	if (currentToken.code == LEFTPARENTHESIS) {
		ParseParameterList();
	}

	Accept(SEMICOLON);

	if (currentToken.code == IDENTIFIER) {
		ParseDeclarations();
	}

	while (currentToken.code == PROCEDURE) {
		ParseProcDeclaration();
	}

	ParseBlock();

	Accept(SEMICOLON);
}


/*--------------------------------------------------------------------------*/
/*	  ParseParameterList implements:                                        */
/*                                                                          */
/*       <ParameterList>     :==  "(" <FormalParameter> { ","               */
/*								  <FormalParameter> } ")"                   */
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
	FormalParameter();

	while (currentToken.code == COLON) {
		FormalParameter();
	}

	Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*	  ParseFormalParameter implements:                                      */
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
	if (currentToken.code == REF) {
		/*Do nothing? REF is wrapped with [ ] in the rule, but it proceeds 
		  straight to <Variable> after*/
	}

	Accept(IDENTIFIER);
}

/*--------------------------------------------------------------------------*/
/*	  ParseBlock implements:		                                        */
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

	while (currentToken.code == WHILE || IF || READ || WRITE) {
		Accept(SEMICOLON);
	}

	Accept(END)
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*       <Statement>   :== <Identifier> ":=" <Expression>                   */
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

PRIVATE void ParseStatement( void )
{
	
	/*Not sure how simple statement works, but this should call the rest of
	  the statements correctly.*/
	
	if (currentToken.code == WHILE) {
		ParseWhileStatement();
	}
	else if (currentToken.code == IF) {
		ParseIfStatement();
	}
	else if (currentToken.code == READ) {
		ParseReadStatement();
	}
	else if (currentToken.code == WRITE) {
		ParseWriteStatement();
	}
/*
Maybe end this if-else statement with:

	else{
		SimpleStatement()
	}
*/

}

/*--------------------------------------------------------------------------*/
/*	  ParseSimpleStatement implements:                                      */
/*                                                                          */
/*       <SimpleStatement>     :==  <VarOrProcName> <RestOfStatement        */
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
	ParseVarOrProcName();
	ParseRestOfStatement();
}

/*--------------------------------------------------------------------------*/
/*	  ParseRestOfStatement implements:                                      */
/*                                                                          */
/*       <ParseRestOfStatement>     :==  <ProcCallList> | <Assignment>      */
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

PRIVATE void ParseRestOfStatement(void)
{
	ProcCallList(); || Assignment(); || /*e symbol? Member of?*/
}

/*--------------------------------------------------------------------------*/
/*	  ParseProcCallList implements:                                         */
/*                                                                          */
/*       <ProcCallList>     :==  "(" <ActualParameter> { ","                */
/*								 <ActualParameter> } ")"                    */
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

PRIVATE void ProcCallList(void)
{
	Accept(LEFTPARENTHESIS);

	ActualParameter();

	while (currentToken.code == colon) {
		ActualParameter();
	}

	Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*	  ParseAssignment implements:                                           */
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
	Expression();
}

/*--------------------------------------------------------------------------*/
/*	  ParseActualParameter implements:                                      */
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
	Accept(IDENTIFIER);
	Expression();
}

/*--------------------------------------------------------------------------*/
/*	  ParseWhileStatement implements:                                       */
/*                                                                          */
/*       <WhileStatement>     :==  "WHILE" <BooleanExpression> "DO" <Block  */
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
	Accept(WHILE);
	BooleanExpression();
	Accept(DO);
	ParseBlock();
}



/*--------------------------------------------------------------------------*/
/*	  ParseIfStatement implements:                                          */
/*                                                                          */
/*       <IfStatement>     :==  "IF" <BooleanExpression> "THEN"             */
/*								   <Block> ["ELSE" <Block> ]                */
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
	BooleanExpression();
	Accept(THEN);
	ParseBlock();

	if (currentToken.code == ELSE) {
		ParseBlock();
	}
}

/*--------------------------------------------------------------------------*/
/*	  ParseReadStatement implements:                                        */
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



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression>  :== <Identifier> | <IntConst>                        */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFIER and INTCONST respectively.    */
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
	if (CurrentToken.code == IDENTIFIER) {}
	Accept(IDENTIFIER);
}
    else  Accept( INTCONST );
}



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

PRIVATE void Accept( int ExpectedToken )
{
    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        ReadToEndOfFile();
        fclose( InputFile );
        fclose( ListFile );
        exit( EXIT_FAILURE );
    }
    else  CurrentToken = GetToken();
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

PRIVATE int  OpenFiles( int argc, char *argv[] )
{

    if ( argc != 3 )  {
        fprintf( stderr, "%s <inputfile> <listfile>\n", argv[0] );
        return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
        return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
        fclose( InputFile );
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

PRIVATE void ReadToEndOfFile( void )
{
    if ( CurrentToken.code != ENDOFINPUT )  {
        Error( "Parsing ends here in this program\n", CurrentToken.pos );
        while ( CurrentToken.code != ENDOFINPUT )  CurrentToken = GetToken();
    }
}
