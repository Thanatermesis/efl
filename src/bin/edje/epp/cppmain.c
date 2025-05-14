/**
 * @file cppmain.c
 * @brief CPP main program, using CPP Library.
 *
 * This program serves as a command-line interface to the C Preprocessor (CPP)
 * library. It handles parsing command-line options, setting up the input
 * and output files, and then invoking the CPP library to process the input.
 *
 * Copyright (C) 1995 Free Software Foundation, Inc.
 * Written by Per Bothner, 1994-95.
 * Copyright (C) 2003-2011 Kim Woelders
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Help stamp out software-hoarding!  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cpplib.h"

#define EPP_DEBUG 0

/** @brief The main CPP reader structure, holding the state of the preprocessor. */
cpp_reader          parse_in;
/** @brief Structure holding the command-line options for the preprocessor. */
cpp_options         options;

/**
 * @brief Main entry point for the CPP program.
 *
 * Initializes the CPP library, parses command-line arguments,
 * sets up input and output, and processes the input file token by token.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of strings representing the command-line arguments.
 *             For a command like `cppmain -o output.c input.c`, `argc` would be 4 and
 *             `argv` would be: `{"cppmain", "-o", "output.c", "input.c"}`.
 * @return int Returns `SUCCESS_EXIT_CODE` (0) on successful completion,
 *             `FATAL_EXIT_CODE` (typically non-zero) on error.
 */
int
main(int argc, char **argv)
{
   char               *p;
   int                 i;
   int                 argi = 1;	/* Next argument to handle. */
   struct cpp_options *opts = &options;
   enum cpp_token      kind;
#if EPP_DEBUG
   int                 got_text = 0;
#endif

   /**
    * @brief Extracts program name from argv[0].
    *
    * This part of the code isolates the program's name from its full path
    * in `argv[0]`. It does so by scanning backwards from the end of the string
    * for a directory separator ('/' or '\\' on EMX). The result is stored in
    * the global `progname` variable.
    */
   p = argv[0] + strlen(argv[0]);
#ifndef __EMX__
   while (p != argv[0] && p[-1] != '/')
#else
   while (p != argv[0] && p[-1] != '/' && p[-1] != '\\')
#endif
      --p;
   progname = p;

   init_parse_file(&parse_in);
   parse_in.data = opts;

   init_parse_options(opts);

   argi += cpp_handle_options(&parse_in, argc - argi, argv + argi);
   if (argi < argc)
      cpp_fatal("Invalid option `%s'", argv[argi]);
   parse_in.show_column = 1;

   i = push_parse_file(&parse_in, opts->in_fname);
   if (i != SUCCESS_EXIT_CODE)
      return i;

   /* Now that we know the input file is valid, open the output.  */
   /* Default to stdout if no output file is specified. */
   if (!opts->out_fname || !strcmp(opts->out_fname, ""))
      opts->out_fname = "stdout";
   else if (!freopen(opts->out_fname, "wb", stdout))
      cpp_pfatal_with_name(&parse_in, opts->out_fname);

   /**
    * Main processing loop.
    * Reads tokens from the input stream one by one using cpp_get_token()
    * and processes them based on their kind.
    * The loop continues until an CPP_EOF token is encountered or an error occurs.
    */
   for (i = 0;; i++)
     {
	kind = cpp_get_token(&parse_in);
#if EPP_DEBUG
	fprintf(stderr, "%03d: kind=%d len=%d out=%d text=%d\n", i,
		kind, CPP_WRITTEN(&parse_in), !opts->no_output, got_text);
#endif
	switch (kind)
	  {
	  case CPP_EOF: /* End Of File token. */
	     goto done;

	  case CPP_HSPACE: /* Horizontal whitespace. Skip and continue. */
	     continue;

	  case CPP_VSPACE: /* Vertical whitespace. Output and continue. */
	     break;

	  default: /* For most tokens, just continue to the output stage. */
	  case CPP_OTHER:
	  case CPP_NAME:
	  case CPP_NUMBER:
	  case CPP_CHAR:
	  case CPP_STRING:
	  case CPP_LPAREN:
	  case CPP_RPAREN:
	  case CPP_LBRACE:
	  case CPP_RBRACE:
	  case CPP_COMMA:
	  case CPP_SEMICOLON:
	  case CPP_3DOTS:
#if EPP_DEBUG
             got_text = 1;
#endif
	     continue;

	  case CPP_COMMENT:   /* Comments are handled by the library; skip output here. */
	  case CPP_DIRECTIVE: /* Directives are handled by the library; skip output here. */
	  case CPP_POP:       /* Pop file/macro context; skip output here. */
	     continue;
	  }
#if EPP_DEBUG
	fprintf(stderr, "'");
	fwrite(parse_in.token_buffer, 1, CPP_WRITTEN(&parse_in), stderr);
	fprintf(stderr, "'\n");
#endif
	if (!opts->no_output)
	  {
	     size_t n;

	     n = CPP_WRITTEN(&parse_in);
	     if (fwrite(parse_in.token_buffer, 1, n, stdout) != n)
		exit(FATAL_EXIT_CODE);
	  }
	parse_in.limit = parse_in.token_buffer;
#if EPP_DEBUG
        got_text = 0;
#endif
     }

 done:
   /** Final cleanup of the CPP library state. */
   cpp_finish(&parse_in);

   if (parse_in.errors)
      exit(FATAL_EXIT_CODE);
   exit(SUCCESS_EXIT_CODE);
}
