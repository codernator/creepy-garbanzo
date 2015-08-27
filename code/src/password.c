/** Encapsulate password encryption. */

#include "merc.h"
#include <string.h>
#include <stdio.h>

/** exports */
/*@shared@*/char *password_encrypt(const char *plain);
bool password_matches(char *existing, const char *plain);
int password_acceptance(const char *plain);
/*@shared@*/const char *password_accept_message(int code);


/* imports */
//merc.h
#ifdef POSIX_CRYPT
#include <crypt.h>
/*@shared@*/char *crypt(const char *key, const char *salt);
#endif


/** locals */
#ifndef POSIX_CRYPT
static /*@shared@*/char* crypt(const char *key, const char *salt);
#endif


#define MIN_PASSWORD_LENGTH 5
#define MAX_PASSWORD_LENGTH 120

#define ACCEPTANCE_ACCEPTABLE 0
#define ACCEPTANCE_TOOSHORT 1
#define ACCEPTANCE_TOOLONG 2
#define PWD_SALT "One~Ugly*Password,yo!"

static /*@shared@*/char *cryptwrapper(const char *plain);




char *password_encrypt(const char *plain)
{
    char *pwdnew;

    pwdnew = cryptwrapper(plain);
    return str_dup(pwdnew);
}

bool password_matches(char *existing, const char *plain) 
{
    char *pwdnew;
    size_t pwdlen;

    pwdlen = strlen(existing);
    pwdnew = cryptwrapper(plain);
    return strncmp(pwdnew, existing, pwdlen + 1) == 0;
}

int password_acceptance(const char *plain)
{
    size_t passlength;
    passlength = strlen(plain);

    if (passlength < MIN_PASSWORD_LENGTH) { return ACCEPTANCE_TOOSHORT; }
    if (passlength > MAX_PASSWORD_LENGTH) { return ACCEPTANCE_TOOLONG; }

    
    return ACCEPTANCE_ACCEPTABLE;
}

const char *password_accept_message(int code)
{
    #define MSG_LENGTH 256
    static char message[MSG_LENGTH];

    switch (code) {
    case ACCEPTANCE_ACCEPTABLE:
	message[0] = '\0';
	break;
    case ACCEPTANCE_TOOSHORT: 
	(void)snprintf(message, MSG_LENGTH, "Password must be at least %d characters.", MIN_PASSWORD_LENGTH);
	break;
    case ACCEPTANCE_TOOLONG:
	(void)snprintf(message, MSG_LENGTH, "Password must be at most %d characters.", MAX_PASSWORD_LENGTH);
	break;
    default: 
	(void)snprintf(message, MSG_LENGTH, "%s", "Password is not acceptable.");
	break;
    }

    return message;
}


char *cryptwrapper(const char *plain)
{
    char *pwdnew;
    char *p;

    /** No tilde allowed because of player file format. */
    pwdnew = crypt(plain, PWD_SALT);
    for (p = pwdnew; *p != '\0'; p++) {
	if (*p == '~') {
	    *p = '*';
	}
    }

    return pwdnew;
}


#ifndef POSIX_CRYPT
char* crypt(const char *key, /*@unused@*/const char *salt)
{
    /*@shared@*/static char buf[2+MAX_PASSWORD_LENGTH];
    (void)snprintf(buf, MAX_PASSWORD_LENGTH, "%s", key);
    return buf;
}
#endif
