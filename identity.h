#ifndef IDENTITY_H
#define IDENTITY_H

typedef struct Identity
{
    char *titleRegex;
    char *classRegex;
} Identity;

Identity *IdentityCreate(const char *titleRegex, const char *classRegex);
void IdentityDestroy(Identity *identity);
void IdentityClear(Identity *identity);

int IdentityMatches(Identity *identity, const char *windowTitle, const char *windowClass);

#endif
