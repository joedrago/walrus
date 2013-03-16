#include "identity.h"
#include "dyn.h"

#include <stdlib.h>
#include <string.h>

Identity *IdentityCreate(const char *titleRegex, const char *classRegex)
{
    Identity *identity = calloc(1, sizeof(Identity));
    if(titleRegex)
    {
        dsCopy(&identity->titleRegex, titleRegex);
    }
    if(classRegex)
    {
        dsCopy(&identity->classRegex, classRegex);
    }
    return identity;
}

void IdentityDestroy(Identity *identity)
{
    IdentityClear(identity);
    free(identity);
}

void IdentityClear(Identity *identity)
{
    dsDestroy(&identity->titleRegex);
    dsDestroy(&identity->classRegex);
}

int IdentityMatches(Identity *identity, const char *windowTitle, const char *windowClass)
{
    int matches = 1;
    if(identity->titleRegex)
    {
        if(!strstr(windowTitle, identity->titleRegex))
        {
            matches = 0;
        }
    }
    if(identity->classRegex)
    {
        if(!strstr(windowClass, identity->classRegex))
        {
            matches = 0;
        }
    }
    return matches;
}
