#include <Windows.h>
#include <DbgHelp.h>
#include <vector>
#include <fstream>
#include "ReplaceImport.h"


std::vector<const char *> sImportFunctionNames = 
{ 
"BinkAllocateFrameBuffers",
"BinkClose",
"BinkCloseTrack",
"BinkControlBackgroundIO",
"BinkCopyToBuffer",
"BinkCopyToBufferRect",
"BinkDoFrame",
"BinkDoFrameAsync",
"BinkDoFrameAsyncMulti",
"BinkDoFrameAsyncWait",
"BinkDoFramePlane",
"BinkFreeGlobals",
"BinkGetError",
"BinkGetFrameBuffersInfo",
"BinkGetGPUDataBuffersInfo",
"BinkGetKeyFrame",
"BinkGetPlatformInfo",
"BinkGetRealtime",
"BinkGetRects",
"BinkGetSummary",
"BinkGetTrackData",
"BinkGetTrackID",
"BinkGetTrackMaxSize",
"BinkGetTrackType",
"BinkGoto",
"BinkLogoAddress",
"BinkNextFrame",
"BinkOpen",
"BinkOpenDirectSound",
"BinkOpenMiles",
"BinkOpenTrack",
"BinkOpenWaveOut",
"BinkOpenWithOptions",
"BinkOpenXAudio2",
"BinkOpenXAudio27",
"BinkOpenXAudio28",
"BinkPause",
"BinkRegisterFrameBuffers",
"BinkRegisterGPUDataBuffers",
"BinkRequestStopAsyncThread",
"BinkRequestStopAsyncThreadsMulti",
"BinkService",
"BinkSetError",
"BinkSetFileOffset",
"BinkSetFrameRate",
"BinkSetIO",
"BinkSetIOSize",
"BinkSetMemory",
"BinkSetOSFileCallbacks",
"BinkSetPan",
"BinkSetSimulate",
"BinkSetSoundOnOff",
"BinkSetSoundSystem",
"BinkSetSoundSystem2",
"BinkSetSoundTrack",
"BinkSetSpeakerVolumes",
"BinkSetVideoOnOff",
"BinkSetVolume",
"BinkSetWillLoop",
"BinkShouldSkip",
"BinkStartAsyncThread",
"BinkUtilCPUs",
"BinkUtilFree",
"BinkUtilMalloc",
"BinkUtilMutexCreate",
"BinkUtilMutexDestroy",
"BinkUtilMutexLock",
"BinkUtilMutexLockTimeOut",
"BinkUtilMutexUnlock",
"BinkWait",
"BinkWaitStopAsyncThread",
"BinkWaitStopAsyncThreadsMulti",
"RADTimerRead"
};

extern "C" uintptr_t	iImportFunctions[73] = { 0 };

HINSTANCE				pOriginalHinst = nullptr;
HINSTANCE				pWarpperHinst = nullptr;
std::vector<HINSTANCE>  loadedPlugins;

PROC					Init_Original = nullptr;


std::string GetPluginsDirectory()
{
	return "NativeMods\\";
}

void Error(const char * msg)
{
	MessageBoxA(nullptr, msg, "NativeModLoader", 0);
}

int LoadDLLPlugin(const char * path)
{
	int state = -1;
	__try
	{
		HINSTANCE plugin = LoadLibraryA(path);
		if (!plugin)
			return 0;
		
		int ok = 1;
		FARPROC fnInit = GetProcAddress(plugin, "Init");
		if (fnInit != nullptr)
		{
			state = -2;
			((void(__cdecl *)())fnInit)();
			ok = 2;
		}

		loadedPlugins.push_back(plugin);
		return ok;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}

	return state;
}


void LoadLib()
{
	static bool isLoaded = false;
	if (!isLoaded)
	{
		isLoaded = true;

		std::ofstream fLog = std::ofstream("NativeModLoader.log");
		WIN32_FIND_DATA wfd;
		std::string dir = GetPluginsDirectory();
		std::string search_dir = dir + "*.dll";
		HANDLE hFind = FindFirstFileA(search_dir.c_str(), &wfd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			int i_error = 0;
			do
			{
				if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					continue;

				std::string name = wfd.cFileName;
				name = dir + name;

				if (fLog.good())
					fLog << "Checking \"" << name.c_str() << "\" ... ";

				int result = LoadDLLPlugin(name.c_str());
				switch (result)
				{
				case 2:
				{
					if (fLog.good())
						fLog << "OK - loaded and called Init().\n";
					break;
				}

				case 1:
				{
					if (fLog.good())
						fLog << "OK - loaded.\n";
					break;
				}

				case 0:
				{
					if (fLog.good())
						fLog << "LoadLibrary failed!\n";
					i_error = 1;
					std::string err = "LoadLibrary failed on ";
					err = err + name;
					Error(err.c_str());
					break;
				}

				case -1:
				{
					if (fLog.good())
						fLog << "LoadLibrary crashed! This means there's a problem in the plugin DLL file.\n";
					i_error = 1;
					std::string err = "LoadLibrary crashed on ";
					err = err + name;
					err = err + ". This means there's a problem in the plugin DLL file. Contact the author of that plugin.";
					Error(err.c_str());
					break;
				}

				case -2:
				{
					if (fLog.good())
						fLog << "Init() crashed! This means there's a problem in the plugin DLL file.\n";
					i_error = 1;
					std::string err = "Init() crashed on ";
					err = err + name;
					err = err + ". This means there's a problem in the plugin DLL file. Contact the author of that plugin.";
					Error(err.c_str());
					break;
				}
				}
			} while (i_error == 0 && FindNextFileA(hFind, &wfd));

			FindClose(hFind);
		}
		else
		{
			if (fLog.good())
				fLog << "Failed to get search handle to \"" << search_dir.c_str() << "\"!\n";
		}
	}
}

PVOID Init_Hook(PVOID arg1, PVOID arg2)
{
	LoadLib();

	return ((PVOID(*)(PVOID, PVOID))Init_Original)(arg1, arg2);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	pWarpperHinst = hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		pOriginalHinst = LoadLibraryA("bink2w64_original.dll");
		if (!pOriginalHinst)
		{
			Error("Failed to load bink2w64_.dll!");
			return FALSE;
		}

		for (size_t i = 0; i < sImportFunctionNames.size(); i++)
			iImportFunctions[i] = reinterpret_cast<uintptr_t>(GetProcAddress(pOriginalHinst, sImportFunctionNames[i]));

		int result = ReplaceImport::Replace("api-ms-win-crt-runtime-l1-1-0.dll", "_initterm_e", (PROC)Init_Hook, &Init_Original);
		switch (result)
		{
		case 0: break;
		case 1: Error("Failed to get handle to main module!"); break;
		case 2: Error("Failed to find import table in executable!"); break;
		case 3: Error("Failed to change protection flags on memory page!"); break;
		case 4: Error("Failed to find API function in module!"); break;
		case 5: Error("Failed to find module!"); break;
		default: Error("Unknown error occurred!"); break;
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		FreeLibrary(pOriginalHinst);

		if (!loadedPlugins.empty())
		{
			for(auto plugin : loadedPlugins)
				FreeLibrary(plugin);
			loadedPlugins.clear();
		}
	}
	return TRUE;
}

extern "C" void BinkAllocateFrameBuffers();
extern "C" void BinkClose();
extern "C" void BinkCloseTrack();
extern "C" void BinkControlBackgroundIO();
extern "C" void BinkCopyToBuffer();
extern "C" void BinkCopyToBufferRect();
extern "C" void BinkDoFrame();
extern "C" void BinkDoFrameAsync();
extern "C" void BinkDoFrameAsyncMulti();
extern "C" void BinkDoFrameAsyncWait();
extern "C" void BinkDoFramePlane();
extern "C" void BinkFreeGlobals();
extern "C" void BinkGetError();
extern "C" void BinkGetFrameBuffersInfo();
extern "C" void BinkGetGPUDataBuffersInfo();
extern "C" void BinkGetKeyFrame();
extern "C" void BinkGetPlatformInfo();
extern "C" void BinkGetRealtime();
extern "C" void BinkGetRects();
extern "C" void BinkGetSummary();
extern "C" void BinkGetTrackData();
extern "C" void BinkGetTrackID();
extern "C" void BinkGetTrackMaxSize();
extern "C" void BinkGetTrackType();
extern "C" void BinkGoto();
extern "C" void BinkLogoAddress();
extern "C" void BinkNextFrame();
extern "C" void BinkOpen();
extern "C" void BinkOpenDirectSound();
extern "C" void BinkOpenMiles();
extern "C" void BinkOpenTrack();
extern "C" void BinkOpenWaveOut();
extern "C" void BinkOpenWithOptions();
extern "C" void BinkOpenXAudio2();
extern "C" void BinkOpenXAudio27();
extern "C" void BinkOpenXAudio28();
extern "C" void BinkPause();
extern "C" void BinkRegisterFrameBuffers();
extern "C" void BinkRegisterGPUDataBuffers();
extern "C" void BinkRequestStopAsyncThread();
extern "C" void BinkRequestStopAsyncThreadsMulti();
extern "C" void BinkService();
extern "C" void BinkSetError();
extern "C" void BinkSetFileOffset();
extern "C" void BinkSetFrameRate();
extern "C" void BinkSetIO();
extern "C" void BinkSetIOSize();
extern "C" void BinkSetMemory();
extern "C" void BinkSetOSFileCallbacks();
extern "C" void BinkSetPan();
extern "C" void BinkSetSimulate();
extern "C" void BinkSetSoundOnOff();
extern "C" void BinkSetSoundSystem();
extern "C" void BinkSetSoundSystem2();
extern "C" void BinkSetSoundTrack();
extern "C" void BinkSetSpeakerVolumes();
extern "C" void BinkSetVideoOnOff();
extern "C" void BinkSetVolume();
extern "C" void BinkSetWillLoop();
extern "C" void BinkShouldSkip();
extern "C" void BinkStartAsyncThread();
extern "C" void BinkUtilCPUs();
extern "C" void BinkUtilFree();
extern "C" void BinkUtilMalloc();
extern "C" void BinkUtilMutexCreate();
extern "C" void BinkUtilMutexDestroy();
extern "C" void BinkUtilMutexLock();
extern "C" void BinkUtilMutexLockTimeOut();
extern "C" void BinkUtilMutexUnlock();
extern "C" void BinkWait();
extern "C" void BinkWaitStopAsyncThread();
extern "C" void BinkWaitStopAsyncThreadsMulti();
extern "C" void RADTimerRead();
