NAME		= main

HEADERS         = frame.h wxstuff.h tokens.h
OBJS		= main.o inpanel.o outpanel.o codepanel.o frame.o tokens.o
DEPENDENCIES	= $(OBJS) $(HEADERS)

include		Makefile.wx
