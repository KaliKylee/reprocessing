/*
 * xash_filesystem.h
 *
 * Minimal IFileSystem implementation used when the mod runs on Xash3D FWGS
 * (Android / Linux) where the GoldSource "filesystem_stdio" module does not
 * exist. Only what the Reprocessing code really uses is implemented properly
 * (Open/Close/Read/Write/Seek/Size/FPrintf, CreateDirHierarchy, FindFirst/Next,
 * FileExists, GetFileTime). The rest are harmless stubs.
 *
 * Relative paths are resolved against the mod directory ("<gamedir>/<path>"),
 * exactly like hlsdk-portable does with GET_GAME_DIR + fopen. If a file cannot
 * be opened for reading from disk, the engine loader is tried (so files inside
 * PAKs still work).
 */
#pragma once

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <map>

#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "FileSystem.h"

// Provided by filesystem_utils.cpp (client/server specific)
const char* FileSystem_XashGameDir();
bool FileSystem_XashEngineLoad(const char* fileName, std::vector<unsigned char>& out);

class CXashFileSystem : public IFileSystem
{
private:
	struct Handle
	{
		FILE* fp = nullptr;
		std::vector<unsigned char> mem; // data loaded through the engine
		size_t pos = 0;
		bool isMem = false;
	};

	struct FindState
	{
		std::vector<std::string> names;
		size_t next = 0;
		std::string current;
	};

	std::map<int, FindState> m_Finds;
	int m_NextFind = 1;

	static std::string Join(const char* gameDir, const char* rel)
	{
		std::string out;
		if (gameDir && *gameDir)
		{
			out = gameDir;
			out += '/';
		}
		out += rel ? rel : "";
		for (auto& c : out)
		{
			if (c == '\\')
				c = '/';
		}
		return out;
	}

	std::string Resolve(const char* rel) const { return Join(FileSystem_XashGameDir(), rel); }

	static void MakeDirs(const std::string& path)
	{
		std::string cur;
		for (size_t i = 0; i < path.size(); ++i)
		{
			cur += path[i];
			if (path[i] == '/' && cur.size() > 1)
				mkdir(cur.c_str(), 0777);
		}
		mkdir(path.c_str(), 0777);
	}

public:
	void Mount(void) override {}
	void Unmount(void) override {}
	void RemoveAllSearchPaths(void) override {}
	void AddSearchPath(const char*, const char*) override {}
	bool RemoveSearchPath(const char*) override { return true; }

	void RemoveFile(const char* pRelativePath, const char*) override
	{
		if (pRelativePath)
			unlink(Resolve(pRelativePath).c_str());
	}

	void CreateDirHierarchy(const char* path, const char*) override
	{
		if (path)
			MakeDirs(Resolve(path));
	}

	bool FileExists(const char* pFileName) override
	{
		struct stat st;
		return pFileName && stat(Resolve(pFileName).c_str(), &st) == 0;
	}

	bool IsDirectory(const char* pFileName) override
	{
		struct stat st;
		return pFileName && stat(Resolve(pFileName).c_str(), &st) == 0 && S_ISDIR(st.st_mode);
	}

	FileHandle_t Open(const char* pFileName, const char* pOptions, const char* = nullptr) override
	{
		if (!pFileName || !pOptions)
			return FILESYSTEM_INVALID_HANDLE;

		std::string full = Resolve(pFileName);
		const bool writing = std::strpbrk(pOptions, "wa+") != nullptr;

		if (writing)
		{
			// make sure the directory exists
			const size_t slash = full.find_last_of('/');
			if (slash != std::string::npos)
				MakeDirs(full.substr(0, slash));
		}

		std::string mode = pOptions;
		if (mode.find('b') == std::string::npos && mode.find('t') == std::string::npos)
			mode += 'b'; // never do text translation

		auto* h = new Handle();
		h->fp = std::fopen(full.c_str(), mode.c_str());
		if (h->fp)
			return h;

		if (!writing && FileSystem_XashEngineLoad(pFileName, h->mem))
		{
			h->isMem = true;
			return h;
		}

		delete h;
		return FILESYSTEM_INVALID_HANDLE;
	}

	void Close(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h)
			return;
		if (h->fp)
			std::fclose(h->fp);
		delete h;
	}

	void Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h)
			return;
		if (h->isMem)
		{
			long base = seekType == FILESYSTEM_SEEK_HEAD ? 0 : seekType == FILESYSTEM_SEEK_CURRENT ? (long)h->pos : (long)h->mem.size();
			long np = base + pos;
			if (np < 0)
				np = 0;
			if ((size_t)np > h->mem.size())
				np = (long)h->mem.size();
			h->pos = (size_t)np;
			return;
		}
		std::fseek(h->fp, pos, seekType == FILESYSTEM_SEEK_HEAD ? SEEK_SET : seekType == FILESYSTEM_SEEK_CURRENT ? SEEK_CUR : SEEK_END);
	}

	unsigned int Tell(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h)
			return 0;
		return h->isMem ? (unsigned int)h->pos : (unsigned int)std::ftell(h->fp);
	}

	unsigned int Size(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h)
			return 0;
		if (h->isMem)
			return (unsigned int)h->mem.size();
		const long cur = std::ftell(h->fp);
		std::fseek(h->fp, 0, SEEK_END);
		const long end = std::ftell(h->fp);
		std::fseek(h->fp, cur, SEEK_SET);
		return end < 0 ? 0 : (unsigned int)end;
	}

	unsigned int Size(const char* pFileName) override
	{
		struct stat st;
		if (pFileName && stat(Resolve(pFileName).c_str(), &st) == 0)
			return (unsigned int)st.st_size;
		std::vector<unsigned char> tmp;
		if (pFileName && FileSystem_XashEngineLoad(pFileName, tmp))
			return (unsigned int)tmp.size();
		return 0;
	}

	long GetFileTime(const char* pFileName) override
	{
		struct stat st;
		if (pFileName && stat(Resolve(pFileName).c_str(), &st) == 0)
			return (long)st.st_mtime;
		return 0;
	}

	void FileTimeToString(char* pStrip, int maxChars, long fileTime) override
	{
		if (pStrip && maxChars > 0)
			std::snprintf(pStrip, maxChars, "%ld", fileTime);
	}

	bool IsOk(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		return h && (h->isMem || (h->fp && !std::ferror(h->fp)));
	}

	void Flush(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (h && h->fp)
			std::fflush(h->fp);
	}

	bool EndOfFile(FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h)
			return true;
		return h->isMem ? h->pos >= h->mem.size() : std::feof(h->fp) != 0;
	}

	int Read(void* pOutput, int size, FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h || !pOutput || size <= 0)
			return 0;
		if (h->isMem)
		{
			size_t n = h->mem.size() - h->pos;
			if ((size_t)size < n)
				n = (size_t)size;
			std::memcpy(pOutput, h->mem.data() + h->pos, n);
			h->pos += n;
			return (int)n;
		}
		return (int)std::fread(pOutput, 1, (size_t)size, h->fp);
	}

	int Write(const void* pInput, int size, FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h || !h->fp || !pInput || size <= 0)
			return 0;
		return (int)std::fwrite(pInput, 1, (size_t)size, h->fp);
	}

	char* ReadLine(char* pOutput, int maxChars, FileHandle_t file) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h || !pOutput || maxChars <= 0)
			return nullptr;
		if (h->isMem)
		{
			if (h->pos >= h->mem.size())
				return nullptr;
			int i = 0;
			while (i < maxChars - 1 && h->pos < h->mem.size())
			{
				char c = (char)h->mem[h->pos++];
				pOutput[i++] = c;
				if (c == '\n')
					break;
			}
			pOutput[i] = '\0';
			return pOutput;
		}
		return std::fgets(pOutput, maxChars, h->fp);
	}

	int FPrintf(FileHandle_t file, const char* pFormat, ...) override
	{
		auto* h = static_cast<Handle*>(file);
		if (!h || !h->fp)
			return 0;
		va_list args;
		va_start(args, pFormat);
		const int r = std::vfprintf(h->fp, pFormat, args);
		va_end(args);
		return r;
	}

	void* GetReadBuffer(FileHandle_t, int* outBufferSize, bool) override
	{
		if (outBufferSize)
			*outBufferSize = 0;
		return nullptr;
	}

	void ReleaseReadBuffer(FileHandle_t, void*) override {}

	const char* FindFirst(const char* pWildCard, FileFindHandle_t* pHandle, const char* = nullptr) override
	{
		if (pHandle)
			*pHandle = FILESYSTEM_INVALID_FIND_HANDLE;
		if (!pWildCard)
			return nullptr;

		std::string wild = pWildCard;
		for (auto& c : wild)
		{
			if (c == '\\')
				c = '/';
		}
		const size_t slash = wild.find_last_of('/');
		const std::string dir = slash == std::string::npos ? "" : wild.substr(0, slash);
		const std::string pattern = slash == std::string::npos ? wild : wild.substr(slash + 1);

		FindState state;
		const std::string fullDir = Resolve(dir.c_str());
		if (DIR* d = opendir(fullDir.empty() ? "." : fullDir.c_str()))
		{
			while (dirent* e = readdir(d))
			{
				if (e->d_name[0] == '.' && (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0')))
					continue;
				if (fnmatch(pattern.c_str(), e->d_name, FNM_CASEFOLD) == 0)
					state.names.emplace_back(e->d_name);
			}
			closedir(d);
		}

		if (state.names.empty())
			return nullptr;

		const int id = m_NextFind++;
		FindState& st = m_Finds[id];
		st = std::move(state);
		st.current = st.names[st.next++];
		if (pHandle)
			*pHandle = id;
		return st.current.c_str();
	}

	const char* FindNext(FileFindHandle_t handle) override
	{
		auto it = m_Finds.find(handle);
		if (it == m_Finds.end() || it->second.next >= it->second.names.size())
			return nullptr;
		it->second.current = it->second.names[it->second.next++];
		return it->second.current.c_str();
	}

	bool FindIsDirectory(FileFindHandle_t) override { return false; }

	void FindClose(FileFindHandle_t handle) override { m_Finds.erase(handle); }

	void GetLocalCopy(const char*) override {}

	const char* GetLocalPath(const char* pFileName, char* pLocalPath, int size) override
	{
		if (pLocalPath && size > 0)
			std::snprintf(pLocalPath, size, "%s", Resolve(pFileName).c_str());
		return pLocalPath;
	}

	char* ParseFile(char* pFileBytes, char*, bool*) override { return pFileBytes; }

	bool FullPathToRelativePath(const char* pFullpath, char* pRelative) override
	{
		if (!pFullpath || !pRelative)
			return false;
		std::strcpy(pRelative, pFullpath);
		return true;
	}

	bool GetCurrentDirectory(char* pDirectory, int maxlen) override { return getcwd(pDirectory, (size_t)maxlen) != nullptr; }

	void PrintOpenedFiles(void) override {}
	void SetWarningFunc(void (*)(const char*, ...)) override {}
	void SetWarningLevel(FileWarningLevel_t) override {}
	void LogLevelLoadStarted(const char*) override {}
	void LogLevelLoadFinished(const char*) override {}
	int HintResourceNeed(const char*, int) override { return 0; }
	int PauseResourcePreloading(void) override { return 0; }
	int ResumeResourcePreloading(void) override { return 0; }
	int SetVBuf(FileHandle_t, char*, int, long) override { return 0; }

	void GetInterfaceVersion(char* p, int maxlen) override
	{
		if (p && maxlen > 0)
			std::snprintf(p, maxlen, "%s", FILESYSTEM_INTERFACE_VERSION);
	}

	bool IsFileImmediatelyAvailable(const char*) override { return true; }
	WaitForResourcesHandle_t WaitForResources(const char*) override { return 0; }
	bool GetWaitForResourcesProgress(WaitForResourcesHandle_t, float* progress, bool* complete) override
	{
		if (progress)
			*progress = 1.0f;
		if (complete)
			*complete = true;
		return true;
	}
	void CancelWaitForResources(WaitForResourcesHandle_t) override {}
	bool IsAppReadyForOfflinePlay(int) override { return true; }
	bool AddPackFile(const char*, const char*) override { return false; }
	FileHandle_t OpenFromCacheForRead(const char* pFileName, const char* pOptions, const char* pathID = nullptr) override { return Open(pFileName, pOptions, pathID); }
	void AddSearchPathNoWrite(const char*, const char*) override {}
};
