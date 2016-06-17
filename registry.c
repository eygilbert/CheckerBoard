// registry.c
//
// part of checkerboard
//
// this file implements load/save settings from/to registry
// it distinguishes between 32- and 64-bit versions in the registry to enable these two versions
// to live in parallel on a 64-bit system

#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#pragma warning(disable:4091)
#include <shlobj.h>
#pragma warning(default:4091)
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "registry.h"
#include "utility.h"
#include "CheckerBoard.h"
#include "crc.h"


#ifdef _WIN64
#define CB_REGISTRY_NAME "Software\\Martin Fierz\\CheckerBoard64\\"
#else
#define CB_REGISTRY_NAME "Software\\Martin Fierz\\CheckerBoard\\"
#endif
// VERSION will be appended to this name


void savesettings(struct CBoptions *options)
	{
	// save settings in the registry 	
	HKEY hKey;
	unsigned long result;
	char subkey[256];

	// create key name from CB version
	sprintf(subkey,"%s%s", CB_REGISTRY_NAME, VERSION); 
	// open registry key 
	RegCreateKeyEx(HKEY_CURRENT_USER, subkey, 0, "CB_Key", 0, KEY_WRITE, NULL, &hKey, &result);

	// save options struct
	options->crc = sizeof(struct CBoptions);
	options->crc = crc_calc((char *)options, sizeof(struct CBoptions));
	RegSetValueEx(hKey, "options structure", 0, REG_BINARY, (LPBYTE)options, sizeof(struct CBoptions));

	// close registry
	RegCloseKey(hKey);
}

void loadsettings(struct CBoptions *options, char CBdirectory[256])
{
	// load settings from the registry 
	char lstr[MAX_PATH];
	HKEY hKey;
	unsigned long result;
	unsigned int reg_crc;
	DWORD datatype, datasize;
	int defaultvalues;
	int use_registry_install_path;
	char subkey[256];

	// create key name from CB version
	sprintf(subkey,"%s%s", CB_REGISTRY_NAME, VERSION); 
	
	// open registry key for checkerboard,
	// if it doesnt exist, create it 
	RegCreateKeyEx(HKEY_CURRENT_USER, subkey, 0, "CB_Key", 0, KEY_READ, NULL, &hKey, &result);

	/* Initialize the CBdirectory with the location of the executable.
	 * If that fails, then initialize it with the current directory and 
	 * if we find the InstallPath in the registry, we will overwrite CBdirectory with that.
	 */
	use_registry_install_path = 0;
	GetModuleFileName(NULL, lstr, sizeof(lstr));
	if (extract_path(lstr, CBdirectory)) {

		/* Must be a problem with getting the path from the module filename.
		 * Set to current directory for now, and read InstallPath from registry.
		 */
		GetCurrentDirectory(sizeof(lstr) - 1, CBdirectory);
		use_registry_install_path = 1;
	}

	/* Create the standard set of CheckerBoard directories under My Documents. */
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, lstr))) {

		/* Create the directories under My Documents. */
		PathAppend(lstr, "Martin Fierz");
		CreateDirectory(lstr, NULL);

		PathAppend(lstr, "CheckerBoard");
		CreateDirectory(lstr, NULL);
		strcpy(CBdocuments, lstr);

		PathAppend(lstr, "games");
		CreateDirectory(lstr, NULL);

		PathAppend(lstr, "matches");
		CreateDirectory(lstr, NULL);

		strcpy(lstr, CBdocuments);
		PathAppend(lstr, "analysis");
		CreateDirectory(lstr, NULL);
	}

	// if no info is in the registry, we set the values to default values 
	defaultvalues = 0;
	if (result == REG_CREATED_NEW_KEY)
		defaultvalues = 1;
	
	else if (result == REG_OPENED_EXISTING_KEY) {
		// read values from keys 
		if (use_registry_install_path) {
			datasize = 255;
			result = RegQueryValueEx(hKey, "InstallPath", NULL, &datatype, (LPBYTE)CBdirectory, &datasize);
		}
		
		// get CB options struct
		datasize = sizeof(struct CBoptions);
		result = RegQueryValueEx(hKey, "options structure", NULL, &datatype, (LPBYTE)options, &datasize);
		if (result != ERROR_SUCCESS)
			defaultvalues = 1;		// could not read options - use defaults again.
		else {

			/* Verify the crc.
			 * CRC is calculated on the whole struct using its size in the crc field.
 			 */
			reg_crc = options->crc;
			options->crc = sizeof(struct CBoptions);
			options->crc = crc_calc((char *)options, sizeof(struct CBoptions));
			if (options->crc != reg_crc)
				defaultvalues = 1;
		}
	}

	// if we could not load settings from registry, use default settings.
	if (defaultvalues) {
		strcpy(options->userdirectory, CBdocuments);
		PathAppend(options->userdirectory, "games");

		strcpy(options->matchdirectory, options->userdirectory);
		PathAppend(options->matchdirectory, "matches");

		options->colors[0]=PALETTERGB(255,255,255);	
		options->colors[1]=PALETTERGB(255,255,255);	
		options->colors[2]=PALETTERGB(120,208,216);	
		options->colors[3]=PALETTERGB(0,128,192);	
		options->colors[4]=PALETTERGB(255,0,0);	
		sprintf(options->EGTBdirectory,"%s\\db",CBdirectory);
		options->exact=0;	
		options->highlight=0;
		options->invert=0;
		options->level=2;
		options->mirror=0;
		options->numbers=1;
		options->op_barred=0;
		options->op_crossboard=1;
		options->op_mailplay=0;
#ifdef _WIN64
#pragma message("_WIN64 is defined.")
		sprintf(options->primaryenginestring,"cake64.dll");
		sprintf(options->secondaryenginestring,"simplech64.dll");
#else
#pragma message("_WIN64 is not defined.")
		sprintf(options->primaryenginestring,"cake.dll");
		sprintf(options->secondaryenginestring,"simplech.dll");
#endif
		options->priority=0;
		options->sound=0;
		options->userbook=0;
		options->window_x=0;
		options->addoffset = 0;
		options->language = ENGLISH;
		options->piecesetindex = 0;
		RegSetValueEx(hKey, "options structure", 0, REG_BINARY, (LPBYTE)options, sizeof(struct CBoptions));
	}
	else {
		// if window size is too small, enlarge it
		options->window_height = max(options->window_height, 480);
		options->window_width = max(options->window_width, 400);
		if (options->window_x < 0 || options->window_x > 500)
			options->window_x = 1;
		if (options->window_y < 0 || options->window_y > 500)
			options->window_y = 1;	
	}

	SetCurrentDirectory(CBdirectory);
	
	sprintf(lstr, "language is %i", options->language);
	CBlog(lstr);

	RegCloseKey(hKey);
}
