#include <assert.h>

#include "../ipasir.hh"
#include "stdio.h"
// #include "picosat.h"

#define LGL_SIMP_DELAY 10000
#define LGL_MIN_BLIMIT 50000
#define LGL_MAX_BLIMIT 200000

extern "C" void* lglinit();
extern "C" void* lglclone(void* solver);
extern "C" int lglunclone(void* solver, void* clone);
extern "C" void lglfixate(void* solver);
extern "C" void lglmeltall(void* solver);
extern "C" void lglsetprefix(void* res, char* prefix);
extern "C" void lglsetopt(void* res, char* opt, int v);
extern "C" void lglsetout(void* res, FILE* out);
extern "C" void lglstats(void* solver);
extern "C" void lglrelease(void* solver);
extern "C" void lgladd(void* solver, int lit);
extern "C" void lglfreeze(void* solver, int lit);
extern "C" bool lglfrozen(void* solver, int lit);
extern "C" void lglassume(void* solver, int lit);
extern "C" int lglsat(void* solver);
extern "C" int lglfailed(void* solver, int lit);
extern "C" int lglderef(void* solver, int var);

static const char* sig = "lingeling";

unsigned int lgl_limit = LGL_MIN_BLIMIT;
unsigned int lgl_nforked = 0;

const char* ipasir_signature() { return sig; }

void* ipasir_init() {
    char prefix[80];
    void* res = lglinit();
    sprintf(prefix, "c [%s] ", sig);
    lglsetprefix(res, prefix);
    lglsetopt(res, "verbose", 0);
    lglsetout(res, stdout);
    return res;
}

void ipasir_release(void* solver) {
    lglstats(solver);
    lglrelease(solver);
}

void ipasir_add(void* solver, int lit) {
    lgladd(solver, lit);
    if (lit != 0 && !lglfrozen(solver, lit)) lglfreeze(solver, lit);
}

void ipasir_assume(void* solver, int lit) { lglassume(solver, lit); }

int ipasir_solve(void* lgl) {
    assert(lgl_limit);
    void* clone;

    int res = 0;
    int bfres = 0;
    const char* str;
    char name[80];

    lglsetopt(lgl, "simplify", 1);
    lglsetopt(lgl, "simpdelay", LGL_SIMP_DELAY);

    lglsetopt(lgl, "flipping", 0);
    lglsetopt(lgl, "locs", 0);

    lglsetopt(lgl, "clim", lgl_limit);
    if (!(res = lglsat(lgl))) {
        lgl_limit *= 2;
        if (lgl_limit > LGL_MAX_BLIMIT) lgl_limit = LGL_MAX_BLIMIT;

        lgl_nforked++;
        clone = lglclone(lgl);
        lglfixate(clone);
        lglmeltall(clone);
        str = "clone";
        lglsetopt(clone, "clim", -1);

        sprintf(name, "[%s lgl%s%d] ", sig, str, lgl_nforked);
        printf("Forked Lingeling: %s\n", name);

        lglsetprefix(clone, name);
        lglsetout(clone, stdout);

        res = lglsat(clone);
        bfres = lglunclone(lgl, clone);
        lglrelease(clone);
        assert(!res || bfres == res);
        res = bfres;
    } else {
        lgl_limit = 9 * (lgl_limit / 10);
        if (lgl_limit < LGL_MIN_BLIMIT) lgl_limit = LGL_MIN_BLIMIT;
    }

    return res;
}

int ipasir_failed(void* solver, int lit) { return lglfailed(solver, lit); }

int ipasir_val(void* solver, int var) {
    int val = lglderef(solver, var);
    if (!val) return -var;
    return val < 0 ? -var : var;
}

void ipasir_bump(void* solver, int lit) { (void)0; }
