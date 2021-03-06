/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 * 
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
** File:          plgetopt.c
** Description:   utilities to parse argc/argv
*/

#include "prmem.h"
#include "prlog.h"
#include "prerror.h"
#include "plstr.h"
#include "plgetopt.h"

#include <string.h>

static char static_Nul = 0;

struct PLOptionInternal
{
    const char *options;        /* client options list specification */
    PRIntn argc;                /* original number of arguments */
    char **argv;                /* vector of pointers to arguments */
    PRIntn xargc;               /* which one we're processing now */
    const char *xargv;          /* where within *argv[xargc] */
    PRBool minus;               /* do we already have the '-'? */
};

/*
** Create the state in which to parse the tokens.
**
** argc        the sum of the number of options and their values
** argv        the options and their values
** options    vector of single character options w/ | w/o ':
*/
PR_IMPLEMENT(PLOptState*) PL_CreateOptState(
    PRIntn argc, char **argv, const char *options)
{
    PLOptState *opt = NULL;
    if (NULL == options)
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    else
    {
        opt = PR_NEWZAP(PLOptState);
        if (NULL == opt)
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        else
        {
            PLOptionInternal *internal = PR_NEW(PLOptionInternal);
            if (NULL == internal)
            {
                PR_DELETE(opt);
                PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            }
            else
            {
				opt->option = 0;
				opt->value = NULL;
				opt->internal = internal;

                internal->argc = argc;
                internal->argv = argv;
                internal->xargc = 0;
                internal->xargv = &static_Nul;
                internal->minus = PR_FALSE;
                internal->options = options;
            }
        }
    }
    return opt;
}  /* PL_CreateOptState */

/*
** Destroy object created by CreateOptState()
*/
PR_IMPLEMENT(void) PL_DestroyOptState(PLOptState *opt)
{
    PR_DELETE(opt->internal);
    PR_DELETE(opt);
}  /* PL_DestroyOptState */

PR_IMPLEMENT(PLOptStatus) PL_GetNextOpt(PLOptState *opt)
{
    PLOptionInternal *internal = opt->internal;
    PRIntn cop, eoo = PL_strlen(internal->options);

    /*
    ** If the current xarg points to nul, advance to the next
    ** element of the argv vector. If the vector index is equal
    ** to argc, we're out of arguments, so return an EOL.
	** Note whether the first character of the new argument is
	** a '-' and skip by it if it is.
    */
    while (0 == *internal->xargv)
    {
        internal->xargc += 1;
        if (internal->xargc >= internal->argc)
		{
			opt->option = 0;
			opt->value = NULL;
			return PL_OPT_EOL;
		}
        internal->xargv = internal->argv[internal->xargc];
		internal->minus = ('-' == *internal->xargv ? PR_TRUE : PR_FALSE);  /* not it */
		if (internal->minus) internal->xargv += 1;  /* and consume */
    }

    /*
    ** If we already have a '-' in hand, xargv points to the next
    ** option. See if we can find a match in the list of possible
    ** options supplied.
    */

    if (internal->minus)
    {
        for (cop = 0; cop < eoo; ++cop)
        {
            if (internal->options[cop] == *internal->xargv)
            {
                opt->option = *internal->xargv;
                internal->xargv += 1;
                /*
                ** if options indicates that there's an associated
				** value, this argv is finished and the next is the
				** option's value.
                */
                if (':' == internal->options[cop + 1])
                {
					PR_ASSERT(0 == *internal->xargv);
                    opt->value = internal->argv[++(internal->xargc)];
                    internal->xargv = &static_Nul;
                    internal->minus = PR_FALSE;
                }
				else opt->value = NULL;
                return PL_OPT_OK;
            }
        }
        internal->xargv += 1;  /* consume that option */
        return PL_OPT_BAD;
    }
    /*
    ** No '-', so it must be a standalone value. The option is nul.
    */
    opt->value = internal->argv[(internal->xargc)++];
    internal->xargv = &static_Nul;
    opt->option = 0;
    return PL_OPT_OK;
}  /* PL_GetNextOpt */

/* plgetopt.c */
