#ifndef BETTER_AIMING_H_
#define BETTER_AIMING_H_

#include "foxhollow_mod_api.h"

FH_MOD_EXPORT int fh_mod_initialize(FhMod* mod, const FhModHost* host);
FH_MOD_EXPORT void fh_mod_config_changed(FhMod* mod);
FH_MOD_EXPORT void fh_mod_shutdown(FhMod* mod);

#endif
