// Principal ADL header include
#include "adl_global.h"
// List of embedded components
const char mos_headerSEList[] = "Developer Studio\0" "2.3.2.201310241753\0"
		"Open AT Framework package\0" "2.51.0.201206190958\0"
		"Open AT OS Package\0" "6.51.0.201206010944\0"
		"Firmware Package\0" "7.51.0.201205311751\0"
		"Internet Library Package\0" "5.54.0.201206011257\0"
		"\0";

#if __OAT_API_VERSION__ >= 636
// Application debug/release mode tag (only supported from Open AT OS 6.36)
#ifdef DS_DEBUG
const adl_CompilationMode_e adl_CompilationMode = ADL_COMPILATION_MODE_DEBUG;
#else
const adl_CompilationMode_e adl_CompilationMode = ADL_COMPILATION_MODE_RELEASE;
#endif
#endif

// Application name definition
const ascii adl_InitApplicationName[] = "Greenhouse MQTT";

// Company name definition
const ascii adl_InitCompanyName[] = "SierraWireless";

// Application version definition
const ascii adl_InitApplicationVersion[] = "1.0.1";
