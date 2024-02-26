#include <cstdio>
#include <vector>
#include <windows.h>

#define RESET	  "\u001b[0m"
#define RED(s)	  "\u001b[1;31m" s RESET
#define GREEN(s)  "\u001b[1;32m" s RESET
#define YELLOW(s) "\u001b[1;33m" s RESET
#define CYAN(s)	  "\u001b[1;36m" s RESET
#define WHITE(s)  "\u001b[37;1m" s RESET

HANDLE __stdcall CreateFileCall(LPCSTR lpFileName, DWORD dwDesiredAccess,
								DWORD dwShareMode,
								LPSECURITY_ATTRIBUTES lpSecurityAttributes,
								DWORD dwCreationDisposition,
								DWORD dwFlagsAndAttributes,
								HANDLE hTemplateFile)
{
	return ::CreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
						 lpSecurityAttributes, dwCreationDisposition,
						 dwFlagsAndAttributes, hTemplateFile);
}

BOOL __stdcall CloseHandleCall(_In_ _Post_ptr_invalid_ HANDLE hObject)
{
	return ::CloseHandle(hObject);
}

BOOL __stdcall ReadFileCall(_In_ HANDLE hFile,
							_Out_writes_bytes_to_opt_(nNumberOfBytesToRead,
													  *lpNumberOfBytesRead)
								__out_data_source(FILE) LPVOID lpBuffer,
							_In_ DWORD nNumberOfBytesToRead,
							_Out_opt_ LPDWORD lpNumberOfBytesRead,
							_Inout_opt_ LPOVERLAPPED lpOverlapped)
{
	return ::ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
					  lpNumberOfBytesRead, lpOverlapped);
}

BOOL __stdcall WriteFileCall(_In_ HANDLE hFile,
							 _In_reads_bytes_opt_(nNumberOfBytesToWrite)
								 LPCVOID lpBuffer,
							 _In_ DWORD nNumberOfBytesToWrite,
							 _Out_opt_ LPDWORD lpNumberOfBytesWritten,
							 _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
	return ::WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
					   lpNumberOfBytesWritten, lpOverlapped);
}

BOOL __stdcall GetFileSizeExCall(_In_ HANDLE hFile,
								 _Out_ PLARGE_INTEGER lpFileSize)
{
	return ::GetFileSizeEx(hFile, lpFileSize);
}

struct Operations
{
	LARGE_INTEGER liOrigFileSize, liFinalFileSize, liBytesRemoved;
	HANDLE hSourceFile, hDestFile;
	DWORD dwBytesRead, dwBytesWritten;
	char cBuffer[16 * 1024];
} op;

struct Assert
{
	DWORD dwError;
	Assert() : dwError(GetLastError())
	{
	}
	bool check()
	{
		if (dwError != 0)
		{
			printf("[" RED("-") "]"
								" " RED("Error") " code: " WHITE("%lu") "\n ",
				   dwError);
			return false;
		}
		return true;
	}
} assert;

int
main(int argc, char *argv[])
{
	SetConsoleTitleA("__stdcall byte copy :3c");
	if (argc != 4)
	{
		std::printf("[" YELLOW(
			"-") "] Input should be: < "
				 "./" YELLOW(
					 "program_file_name.exe") " > < "
											  "./" CYAN(
												  "source_file") " > "
																 "< "
																 "./" GREEN(
																	 "destinati"
																	 "on_file") " > "
																				"<"
																				" " WHITE(
																					"mode") " > "
																							"\n");

		std::printf("[" YELLOW("-") "]"
									"Mode: " WHITE("0") " for normal, " WHITE(
										"1") " for reverse. \n");
		return 1;
	}

	op.hSourceFile = CreateFileCall(argv[1], GENERIC_READ, 0, NULL,
									OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!GetFileSizeExCall(op.hSourceFile, &op.liOrigFileSize))
	{
		std::printf(
			"[" RED("-") "]"
						 " "
						 "Unable to get " CYAN("original") " file size. \n");
		return 1;
	}

	if (!assert.check() || op.hSourceFile == INVALID_HANDLE_VALUE)
	{
		printf("[" RED("-") "]"
							" " RED("Failed") " to open " CYAN(
								"source") " file: " CYAN("%s") " \n ",
			   argv[1]);
		return 1;
	}

	/* Do not write bytes if *Source* and *Destination* files are the same */
	if (strcmp(argv[1], argv[2]) == 0)
	{
		std::printf(
			"[" RED("-") "]"
						 " " RED("Error") ": " CYAN("Source") " and " GREEN(
							 "Destination") " files are the same.\n");
		return 1;
	}

	op.hDestFile = CreateFileCall(argv[2], GENERIC_WRITE, 0, NULL,
								  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!assert.check() || op.hDestFile == INVALID_HANDLE_VALUE)
	{
		std::printf(
			"[" RED(
				"-") "]"
					 " " RED(
						 "Failed") " to "
								   "open " GREEN("destination") " file: " GREEN(
									   "%s") " \n",
			argv[2]);
		CloseHandleCall(op.hSourceFile);
		return 1;
	}

	int iMode = atoi(argv[3]);
	switch (iMode)
	{
	case 0:
		do
		{
			if (!ReadFileCall(op.hSourceFile, op.cBuffer, sizeof(op.cBuffer),
							  &op.dwBytesRead, NULL))
			{
				if (!assert.check())
				{
					if (assert.dwError == ERROR_HANDLE_EOF)
					{
						std::printf(
							"[" RED("-") "]"
										 " "
										 "Reached the end of "
										 "the " CYAN("source") " file. \n");
					}
					else
					{
						std::printf("[" RED(
							"-") "]"
								 " " RED("Failed") " to read from " CYAN(
									 "source") " file. \n");
					}
					break;
				}
			}

			/* reached to the end */
			if (op.dwBytesRead == 0)
			{
				break;
			}

			if (!WriteFileCall(op.hDestFile, op.cBuffer, op.dwBytesRead,
							   &op.dwBytesWritten, NULL))
			{
				if (!assert.check())
				{
					std::printf(
						"[" RED("-") "]"
									 " " RED("Failed") " to write to " GREEN(
										 "destination") " file. \n");
					break;
				}
			}

			if (op.dwBytesRead != op.dwBytesWritten)
			{
				std::printf("[" RED("-") "]"
										 " "
										 " Mismatch in bytes read and"
										 "written. \n");
				break;
			}

			if (!GetFileSizeExCall(op.hDestFile, &op.liFinalFileSize))
			{
				std::printf("[" RED("-") "]"
										 " "
										 "Unable to get " GREEN(
											 "final") " file size. \n");
				return 1;
			}
		} while (op.dwBytesRead > 0);
		CloseHandleCall(op.hSourceFile);
		CloseHandleCall(op.hDestFile);

		op.liBytesRemoved.QuadPart =
			op.liOrigFileSize.QuadPart - op.liFinalFileSize.QuadPart;

		if (op.liBytesRemoved.QuadPart > 0)
		{
			std::printf(
				"[" GREEN("-") "]"
							   " " GREEN("Successfully") " removed" WHITE(
								   " %lu") " bytes from the original file. \n",
				static_cast<unsigned long>(op.liBytesRemoved.QuadPart));
		}
		else
		{
			std::printf("[" GREEN(
				"-") "]"
					 " "
					 "No bytes were removed from the original file. \n");
		}

		std::printf("[" GREEN("-") "]"
								   " " GREEN("Successfully") " copied " WHITE(
									   "%lu") " bytes to"
											  ": " GREEN("%s") ". \n ",
					static_cast<unsigned long>(op.dwBytesWritten), argv[2]);
		break;
	case 1:
		do
		{
			if (!ReadFileCall(op.hSourceFile, op.cBuffer, sizeof(op.cBuffer),
							  &op.dwBytesRead, NULL))
			{
				if (!assert.check())
				{
					if (assert.dwError == ERROR_HANDLE_EOF)
					{
						std::printf(
							"[" RED("-") "]"
										 " "
										 "Reached the end of "
										 "the " CYAN("source") " file. \n");
					}
					else
					{
						std::printf("[" RED(
							"-") "]"
								 " " RED("Failed") " to read from " CYAN(
									 "source") " file. \n");
					}
					break;
				}
			}

			/* reached to the end */
			if (op.dwBytesRead == 0)
			{
				break;
			}

			/* reversing bytes */
			for (DWORD i = 0; i < op.dwBytesRead / 2; ++i)
			{
				std::swap(op.cBuffer[i], op.cBuffer[op.dwBytesRead - 1 - i]);
			}

			if (!WriteFileCall(op.hDestFile, op.cBuffer, op.dwBytesRead,
							   &op.dwBytesWritten, NULL))
			{
				if (!assert.check())
				{
					std::printf(
						"[" RED("-") "]"
									 " " RED("Failed") " to write to " GREEN(
										 "destination") " file. \n");
					break;
				}
			}

			if (op.dwBytesRead != op.dwBytesWritten)
			{
				std::printf("[" RED("-") "]"
										 " "
										 " Mismatch in bytes read and"
										 "written. \n");
				break;
			}

			if (!GetFileSizeExCall(op.hDestFile, &op.liFinalFileSize))
			{
				std::printf("[" RED("-") "]"
										 " "
										 "Unable to get " GREEN(
											 "final") " file size. \n");
				return 1;
			}

		} while (op.dwBytesRead > 0);
		CloseHandleCall(op.hSourceFile);
		CloseHandleCall(op.hDestFile);

		op.liBytesRemoved.QuadPart =
			op.liOrigFileSize.QuadPart - op.liFinalFileSize.QuadPart;

		if (op.liBytesRemoved.QuadPart > 0)
		{
			std::printf(
				"[" GREEN("-") "]"
							   " " GREEN("Successfully") " removed" WHITE(
								   " %lu") " bytes from the original file. \n",
				static_cast<unsigned long>(op.liBytesRemoved.QuadPart));
		}
		else
		{
			std::printf("[" GREEN(
				"-") "]"
					 " "
					 "No bytes were removed from the original file. \n");
		}

		std::printf(
			"[" GREEN("-") "]"
						   " " GREEN("Successfully") " copied " WHITE(
							   "%lu") " bytes to"
									  ": " GREEN("%s") " in reverse order. "
													   " \n ",
			static_cast<unsigned long>(op.dwBytesWritten), argv[2]);
		break;
	default:
		std::printf(
			"[" RED("-") "]"
						 " " RED("Invalid") " mode specified. Use " WHITE(
							 "0") " for normal "
								  "or " WHITE("1") " for reverse. \n");
		return 1;
	}
	return 0;
}