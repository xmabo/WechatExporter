//
//  ITunesParser.h
//  WechatExporter
//
//  Created by Matthew on 2020/9/29.
//  Copyright © 2020 Matthew. All rights reserved.
//

#include <string>
#include <vector>
#include <map>

#include <sstream>
#include <iomanip>
#include <ctime>
#include "Utils.h"

#ifndef ITunesParser_h
#define ITunesParser_h

struct ITunesFile
{
    std::string fileId;
    std::string relativePath;
    unsigned int flags;
    unsigned int modifiedTime;
    std::vector<unsigned char> blob;
    
    ITunesFile()
    {
    }
    
    bool isDir() const
    {
        return flags == 2;
    }
};

using ITunesFileVector = std::vector<ITunesFile *>;
using ITunesFilesIterator = typename ITunesFileVector::iterator;
using ITunesFilesConstIterator = typename ITunesFileVector::const_iterator;
using ITunesFileRange = std::pair<ITunesFilesConstIterator, ITunesFilesConstIterator>;

class BackupManifest
{
protected:
    std::string m_path;
    std::string m_deviceName;
    std::string m_displayName;
    std::string m_backupTime;
    std::string m_iTunesVersion;
    std::string m_macOSVersion;
    std::string m_iOSVersion;
	bool m_encrypted;
    
public:
    BackupManifest() : m_encrypted(false)
    {
    }

	BackupManifest(const std::string& path, const std::string& deviceName, const std::string& displayName, const std::string& backupTime) : m_path(path), m_deviceName(deviceName), m_displayName(displayName), m_encrypted(false)
	{
	}
    
    bool operator==(const BackupManifest& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }
        
        return m_path == rhs.m_path;
    }

	void setPath(const std::string& path)
	{
		m_path = path;
	}

	void setDeviceName(const std::string& deviceName)
	{
		m_deviceName = deviceName;
	}

	void setDisplayName(const std::string& displayName)
	{
		m_displayName = displayName;
	}

	void setBackupTime(const std::string& backupTime)
	{
		m_backupTime = backupTime;
	}
    
    void setITunesVersion(const std::string& iTunesVersion)
    {
        m_iTunesVersion = iTunesVersion;
    }
    void setMacOSVersion(const std::string& macOSVersion)
    {
        m_macOSVersion = macOSVersion;
    }
    
    void setIOSVersion(const std::string& iOSVersion)
    {
        m_iOSVersion = iOSVersion;
    }
    
    std::string getIOSVersion() const
    {
        return m_iOSVersion;
    }
    
    bool isITunesVersionEmpty() const
    {
        return m_iTunesVersion.empty();
    }

	void setEncrypted(bool encrypted)
	{
		m_encrypted = encrypted;
	}
    
	bool isEncrypted() const
	{
		return m_encrypted;
	}

    bool isValid() const
    {
        return !m_displayName.empty() && !m_backupTime.empty() && !m_deviceName.empty();
    }
    
    std::string getITunesVersion() const
    {
        return m_iTunesVersion.empty() ? (m_macOSVersion.empty() ? "" : ("Embedded iTunes on MacOS " + m_macOSVersion)) : m_iTunesVersion;
    }

    std::string toString() const
    {
        return m_displayName + " [" + m_backupTime + "] (" + m_path + ")" + (m_iTunesVersion.empty() ? (" Embeded iTunes on MacOS:" + m_macOSVersion) : (" iTunes Version:" + m_iTunesVersion));
    }
    
    std::string getPath() const
    {
        return m_path;
    }
};

class ITunesDb
{
public:
    ITunesDb(const std::string& rootPath, const std::string& manifestFileName);
    ~ITunesDb();
    
    std::string getVersion() const
    {
        return m_version;
    }
    
    std::string getIOSVersion() const
    {
        return m_iOSVersion;
    }
    
    void setLoadingFilter(std::function<bool(const char *, int flags)> loadingFilter)
    {
        m_loadingFilter = std::move(loadingFilter);
    }
    
    bool load();
    bool load(const std::string& domain);
    bool load(const std::string& domain, bool onlyFile);
    // bool loadSessions();
    
    const ITunesFile* findITunesFile(const std::string& relativePath) const;
    std::string findFileId(const std::string& relativePath) const;
    std::string findRealPath(const std::string& relativePath) const;
    template<class TFilter>
    ITunesFileVector filter(TFilter f) const;
    template<class THandler>
    void enumFiles(THandler handler) const;
    
    std::string getRealPath(const ITunesFile& file) const;
    std::string getRealPath(const ITunesFile* file) const;
    
    static unsigned int parseModifiedTime(const std::vector<unsigned char>& data);
    bool copyFile(const std::string& vpath, const std::string& dest, bool overwrite = false) const;
    bool copyFile(const std::string& vpath, const std::string& destPath, const std::string& destFileName, bool overwrite = false) const;
    
protected:
    bool loadMbdb(const std::string& domain, bool onlyFile);
    std::string fileIdToRealPath(const std::string& fileId) const;
    
protected:
    bool m_isMbdb;
    mutable std::vector<ITunesFile *> m_files;
    std::string m_rootPath;
    std::string m_manifestFileName;
    std::string m_version;
    std::string m_iOSVersion;
    std::function<bool(const char *, int flags)> m_loadingFilter;
};

template<class TFilter>
ITunesFileVector ITunesDb::filter(TFilter f) const
{
    ITunesFileVector files;
    ITunesFileRange range = std::equal_range(m_files.cbegin(), m_files.cend(), f, f);
    if (range.first != range.second)
    {
        for (ITunesFilesConstIterator it = range.first; it != range.second; ++it)
        {
            if (f == *it)
            {
                files.push_back(*it);
            }
        }
    }
    
    return files;
}

template<class THandler>
void ITunesDb::enumFiles(THandler handler) const
{
    for (ITunesFilesConstIterator it = m_files.cbegin(); it != m_files.cend(); ++it)
    {
        if (!handler(*it))
        {
            break;
        }
    }
}

class ManifestParser
{
protected:
    std::string m_manifestPath;
	mutable std::string m_lastError;

public:
    ManifestParser(const std::string& manifestPath);
    bool parse(std::vector<BackupManifest>& manifets) const;
	std::string getLastError() const;

    friend ITunesDb;
    
protected:
    bool parseDirectory(const std::string& path, std::vector<BackupManifest>& manifests) const;
    bool parse(const std::string& path, BackupManifest& manifest) const;
	bool isValidBackupItem(const std::string& path) const;
    
    static bool parseInfoPlist(const std::string& backupIdPath, BackupManifest& manifest);
};

#endif /* ITunesParser_h */
