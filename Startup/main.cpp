#ifdef _WIN64
#include <Windows.h>
#include <filesystem>

int main()
{
	char path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);

    std::filesystem::path fpath(path);

    fpath = fpath.remove_filename();
    std::string spath = "conhost.exe -- " + fpath.string() + "EMIGame.exe";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(NULL,   // the path
        (char*)spath.c_str(),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        DETACHED_PROCESS,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
#else
int main() {
    return 0;
}
#endif