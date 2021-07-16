#if     !defined(_TOKENS_H_)
#define _TOKENS_H_

#include <stdio.h>
#include <vector>
#include <string>

//  scrollcode, (v1.2)
//  Copyright (C) 2021-, Chris.McDonald@uwa.edu.au

typedef enum {
    T_BAD	= 0,
    T_EOLN,

    T_KEYWORD,
    T_IDENTIFIER,
    T_NUMBER,
    T_STRING,

    T_PREPROCESSOR,
    T_COMMENT,
    T_OTHER
} TOKENTYPE;

typedef	struct {
    TOKENTYPE	        token;
    int		        row, col;
    char	        *str;
    int                 x;
} TOKEN;

class Tokenize
{
public:
    Tokenize(void);
    std::vector<TOKEN>	tokenize(std::vector<std::string> lines);

private:
    TOKEN		next(void);
    static const char * const  keywords[];

    int			nextch;
    bool                in_block_comment;

    const char          *line;
    int			row, col;
};

#endif      // _TOKENS_H_

//  vim: ts=8 sw=4
