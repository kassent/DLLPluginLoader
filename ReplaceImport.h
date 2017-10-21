#pragma once

struct ReplaceImport
{
	static int Replace(const char * dllName, const char * funcName, PROC newFunc, PROC* oldFunc)
	{
		HMODULE mainModule = GetModuleHandle(NULL);
		if (mainModule == NULL)
			return 1;

		PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;
		{
			ULONG Size = 0;
			pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(mainModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size);
		}

		if (pImportDesc == NULL)
			return 2;

		for (; pImportDesc->Name; pImportDesc++)
		{
			if (_stricmp((PSTR)((ULONGLONG)mainModule + pImportDesc->Name), dllName))
				continue;

			PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((ULONGLONG)mainModule + pImportDesc->OriginalFirstThunk);
			PROC * pIat = (PROC*)((ULONGLONG)mainModule + pImportDesc->FirstThunk);
			for (; pThunk->u1.Function; pThunk++, pIat++)
			{
				if ((pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG))
				{

				}
				else
				{
					PIMAGE_IMPORT_BY_NAME pImport = (PIMAGE_IMPORT_BY_NAME)((ULONGLONG)mainModule + pThunk->u1.AddressOfData);
					if (strcmp(pImport->Name, funcName) == 0)
					{
						*oldFunc = *pIat;
						DWORD oldPt = 0;
						if (!VirtualProtect(pIat, sizeof(void*), PAGE_READWRITE, &oldPt))
							return 3;
						*pIat = newFunc;
						VirtualProtect(pIat, sizeof(void*), oldPt, &oldPt);
						return 0;
					}
				}
			}

			return 4;
		}
		return 5;
	}
};

