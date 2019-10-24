/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "did.h"

static const char did_scheme[] = "did";
static const char did_method[] = "elastos";
static const char elastos_did_prefix[] = "did:elastos:";

// idstring has three informats:
// 1. did:elastos:xxxxxxx
// 2. did:elastos:xxxxxxx#xxxxx
// 3. #xxxxxxx
static int parse_id_string(char *id, char *fragment, const char *idstring, DID *ref)
{
    const char *s, *e;
    size_t len;

    assert(id);
    assert(idstring && *idstring);

    // Fragment only, need ref DID object
    if (*idstring == '#') {
        if (!ref || !fragment)
            return -1;

        len = strlen(++idstring);
        if (len == 0 || len >= MAX_FRAGMENT)
            return -1;

        strcpy(id, ref->idstring);
        strcpy(fragment, idstring);
        return 0;
    }

    if (strncmp(idstring, elastos_did_prefix, sizeof(elastos_did_prefix) - 1) != 0)
        return -1;

    s = idstring + sizeof(elastos_did_prefix) - 1;
    for (e = s; *e != '#' && *e != '?' && *e != '/' && *e != '\x0'; e++);
    len = e - s;
    if (len >= MAX_ID_SPECIFIC_STRING)
        return -1;

    strncpy(id, s, len);
    id[len] = 0;

    if (!fragment)
        return 0;

    for (; *e != '#' && *e != '\x0'; e++);
    if (*e != '#')
        return -1;

    len = strlen(++e);
    if (len == 0 || len >= MAX_FRAGMENT)
        return -1;

    strcpy(fragment, e);
    return 0;
}

int parse_did(DID *did, const char *idstring)
{
    return parse_id_string(did->idstring, NULL, idstring, NULL);
}

int parse_didurl(DIDURL *id, const char *idstring, DID *ref)
{
    return parse_id_string(id->did.idstring, id->fragment, idstring, ref);
}

DID *DID_FromString(const char *idstring)
{
    DID *did;

    if (!idstring || !*idstring)
        return NULL;

    did = (DID *)calloc(1, sizeof(DID));
    if (!did)
        return NULL;

    if (parse_did(did, idstring) < 0) {
        free(did);
        return NULL;
    }

    return did;
}

DID *DID_New(const char *method_specific_string)
{
    DID *did;

    if (!method_specific_string || !*method_specific_string ||
        strlen(method_specific_string) >= MAX_ID_SPECIFIC_STRING)
        return NULL;

    did = (DID *)calloc(1, sizeof(DID));
    if (!did)
        return NULL;

    strcpy(did->idstring, method_specific_string);

    return did;
}

const char *DID_GetMethod(DID *did)
{
    if (!did)
        return NULL;

    return did_method;
}

const char *DID_GetMethodSpecificString(DID *did)
{
    if (!did)
        return NULL;

    return (const char *)did->idstring;
}

char *DID_ToString(DID *did, char *idstring, size_t len)
{
    if (!did || !idstring)
        return NULL;

    if (strlen(did->idstring) + strlen(elastos_did_prefix) >= len)
        return NULL;

    strcpy(idstring, elastos_did_prefix);
    strcat(idstring, did->idstring);

    return idstring;
}

int DID_Copy(DID *dest, DID *src)
{
    if (!dest || !src)
        return -1;

    strcpy(dest->idstring, src->idstring);
    return 0;
}

bool DID_Equals(DID *did1, DID *did2)
{
    if (!did1 || !did2)
        return false;

    return strcmp(did1->idstring, did2->idstring) == 0;
}

void DID_Destroy(DID *did)
{
    if (!did)
        return;

    free(did);
}

DIDURL *DIDURL_FromString(const char *idstring)
{
    DIDURL *id;

    if (!idstring || !*idstring)
        return NULL;

    id = (DIDURL *)calloc(1, sizeof(DIDURL));
    if (!id)
        return NULL;

    if (parse_didurl(id, idstring, NULL) < 0) {
        free(id);
        return NULL;
    }

    return id;
}

DIDURL *DIDURL_New(const char *method_specific_string, const char *fragment)
{
    DIDURL *id;

    if (!method_specific_string || !*method_specific_string ||
        !fragment || !*fragment)
        return NULL;

    if (strlen(method_specific_string) >= MAX_ID_SPECIFIC_STRING ||
        strlen(fragment) >= MAX_FRAGMENT)
        return NULL;

    id = (DIDURL *)calloc(1, sizeof(DIDURL));
    if (!id)
        return NULL;

    strcpy(id->did.idstring, method_specific_string);
    strcpy(id->fragment, fragment);

    return id;
}

DID *DIDURL_GetDid(DIDURL *id)
{
    if (!id)
        return NULL;

    return &(id->did);
}

const char *DIDURL_GetFragment(DIDURL *id)
{
    if (!id)
        return NULL;

    return (const char*)id->fragment;
}

char *DIDURL_ToString(DIDURL *id, char *idstring, size_t len, bool compact)
{
    size_t expect_len = 0;

    if (!id || !idstring)
        return NULL;

    expect_len += strlen(id->fragment) + 1;         /* #xxxx */
    expect_len += compact ? 0 : strlen(elastos_did_prefix) + strlen(id->did.idstring);

    if (expect_len >= len)
        return NULL;

    if (compact)
        snprintf(idstring, len, "#%s", id->fragment);
    else
        snprintf(idstring, len, "%s%s#%s", elastos_did_prefix,
            id->did.idstring, id->fragment);

    return idstring;
}

bool DIDURL_Equals(DIDURL *id1, DIDURL *id2)
{
    if (!id1 || !id2)
        return false;

    return (strcmp(id1->did.idstring, id2->did.idstring) == 0 &&
            strcmp(id1->fragment, id2->fragment) == 0);
}

int DIDURL_Copy(DIDURL *dest, DIDURL *src)
{
    if (!dest || !src )
        return -1;

    strcpy(dest->did.idstring, src->did.idstring);
    strcpy(dest->fragment, src->fragment);

    return 0;
}

void DIDURL_Destroy(DIDURL *id)
{
    if (!id)
        return;

    free(id);
}