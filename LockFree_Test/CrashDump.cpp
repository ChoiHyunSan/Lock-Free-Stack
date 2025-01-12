#pragma once

#include "CrashDump.h"

long CrashDump::_DumpCount = 0;

#ifdef DUMP

CrashDump g_crashDump;

#endif