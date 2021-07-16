#include "tokens.h"
#include <cstring>

//  scrollcode, (v1.2)
//  Copyright (C) 2021-, Chris.McDonald@uwa.edu.au

#define get()		nextch = line[++col]
#define unget()		nextch = line[--col]
#define nextnextch	line[col]

Tokenize::Tokenize(void)
{
}

std::vector<TOKEN> Tokenize::tokenize(std::vector<std::string> lines)
{
    std::vector<TOKEN> tokens;

    tokens.clear();
    row                 = 0;
    in_block_comment    = false;

    for(const auto &it: lines) {
        line    = it.c_str();
        col     = 0;

        while(true) {
            TOKEN token = next();

//fprintf(stderr, "r=%i c=%i\t'%s'\n", token.row+1, token.col+1, token.str);
            if(token.token == T_EOLN) {
                break;
            }
            else if(token.token == T_BAD) {
                fprintf(stderr, "T_BAD r=%i c=%i\n", token.row, token.col);
                exit(EXIT_FAILURE);
            }
            tokens.push_back(token);
        }
        ++row;
    }
    return tokens;
}

TOKEN Tokenize::next(void)
{
    TOKEN	token;
    char	chararray[1024], *cp = chararray;
    memset(chararray, '\0', 8);

    token.row	= row;
    token.col	= col;
    nextch      = line[col];

//  ALREADY PROCESSING A BLOCK COMMENT?
    if(in_block_comment) {
        while(nextch != '\0') {
            if(nextch == '*' && nextnextch == '/') {
                *cp++   = '*';
                get();
                *cp++   = '/';
                get();

                in_block_comment    = false;
                break;
            }
            else {
                *cp++   = nextch;
            }
        }
        *cp             = '\0';
        token.token     = T_COMMENT;
        token.str       = strdup(chararray);
        return token;
    }

//  SKIP SPACES (TABS HAVE ALREADY BEEN EXPANDED)
    while(nextch == ' ') {
	get();
    }
    token.col	= col;

//  REACHED END OF CURRENT LINE
    if(nextch == '\0') {
	token.token	= T_EOLN;
	token.str	= strdup("<eoln>");
	return token;
    }

//  PREPROCESSOR LINE
    if(nextch == '#') {
        while(nextch != '\0') {
            *cp++   = nextch;
            get();
        }
        *cp             = '\0';
	token.token	= T_PREPROCESSOR;
	token.str	= strdup(chararray);
	return token;
    }

//  LOOK FOR KEYWORD OR IDENTIFIER
    if(isalpha(nextch) || nextch == '_') {
	while(isalnum(nextch) || nextch == '_') {
	    *cp++ = nextch;
	    get();
	}
	*cp             = '\0';
        token.str       = strdup(chararray);

	for(int kw=0 ; keywords[kw] != NULL ; ++kw) {
	    if(strcmp(chararray, keywords[kw]) == 0) {  // yes, should be sorted
		token.token	= T_KEYWORD;
		return token;
	    }
	}
        token.token	= T_IDENTIFIER;
        return token;
    }
//  LOOK FOR NUMERIC CONSTANT
    else if(isdigit(nextch) || nextch == '.') {
	int	ndpt	= 0;

	while(isdigit(nextch) || (nextch == '.' && ndpt < 2)) {
	    if(nextch == '.') {
		++ndpt;
            }
	    *cp++	= nextch;
	    get();
	}
	*cp		= '\0';

        token.token	= T_NUMBER;
        token.str	= strdup(chararray);
        return token;
    }
//  LOOK FOR STRING (OR CHARACTER CONSTANT)
    else if(nextch == '"' || nextch == '\'') {
	char	quote = nextch;

        *cp++   = quote;
	get();
	while(nextch != '\0' && nextch != quote) {
            if(nextch == '\\') {
                *cp++	= nextch;
                get();
            }
	    *cp++	= nextch;
	    get();
	}
	*cp		= '\0';
	if(nextch == quote) {
            *cp++   = quote;
            *cp     = '\0';
            get();
            token.token	= T_STRING;
            token.str	= strdup(chararray);
            return token;
	}
        token.token	= T_BAD;
	token.str	= strdup("<bad>");
        return token;
    }

//  COMMENT TO END-OF-LINE
    else if(nextch == '/' && nextnextch == '/') {
        while(nextch != '\0') {
            *cp++   = nextch;
            get();
        }
        *cp             = '\0';
        token.token	= T_COMMENT;
        token.str	= strdup(chararray);
        return token;
    }
//  BLOCK COMMENT
    else if(nextch == '/' && nextnextch == '*') {
        in_block_comment    = true;
        *cp++           = nextch;
        get();
        *cp++           = nextch;
        get();

        while(nextch != '\0') {
            if(nextch == '*' && nextnextch == '/') {
                *cp++   = nextch;
                get();
                *cp++   = nextch;
                get();

                in_block_comment    = false;
                break;
            }
            else {
                *cp++   = nextch;
            }
        }
        *cp             = '\0';
        token.token     = T_COMMENT;
        token.str       = strdup(chararray);
        return token;
    }

//  LOOK FOR MULTI-CHARACTER OPERATORS

//  =   ==
    else if(nextch == '=') {
        *cp++	= nextch;
	get();
	if(nextch == '=') {
            *cp++	= nextch;
	    get();
	}
    }

//  +   -   &   |   ++  --  &&  ||  +=  -=  &=  |=  ->
    else if(strchr("+-&|", nextch)) {
        char savech = nextch;

        *cp++	= nextch;
	get();
	if(savech == '-' && nextch == '>') {
            *cp++	= nextch;
	    get();
	}
	else if(nextch == savech) {
            *cp++	= nextch;
	    get();
	}
	else if(nextch == '=') {
            *cp++	= nextch;
	    get();
	}
    }

//  !   *   /   %   ^   ~   !=   *=   /=   %=   ^=   ~=
    else if(strchr("!*/%^~", nextch)) {
        *cp++	= nextch;
	get();
	if(nextch == '=') {
            *cp++	= nextch;
	    get();
	}
    }

//  <   >   <<  >>  <=  >=  <<= >>=
    else if(nextch == '<' || nextch == '>') {
        char savech = nextch;

        *cp++	= nextch;
	get();
	if(nextch == savech) {
            *cp++	= nextch;
	    get();
            if(nextch == '=') {
                *cp++	= nextch;
                get();
            }
	}
	else if(nextch == '=') {
            *cp++	= nextch;
	    get();
	}
    }

//  FINALLY, SINGLE-CHARACTER SYMBOLS
    else if(strchr("()[]{},;:.?", nextch)) {
	*cp++   = nextch;
        get();
    }
    else {
        token.token     = T_BAD;
	token.str	= strdup("<bad>");
        return token;
    }

    *cp         = '\0';
    token.token = T_OTHER;
    token.str   = strdup(chararray);
    return token;
}

// -------------------------------------------------------------------------

//  https://en.cppreference.com/w/c/keyword

const char * const Tokenize::keywords[] = {
//  32 keywords in ANSI and C90
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
//  added by C99
    "_Bool",
    "_Complex",
    "_Imaginary",
    "restrict",
//  added by C11
    "_Alignas",
    "_Alignof",
    "_Atomic",
    "_Generic",
    "_Noreturn",
    "_Static_assert",
    "_Thread_local",
//  added by C23
    "_Decimal128",
    "_Decimal32",
    "_Decimal64",

    NULL
};

//  vim: ts=8 sw=4
