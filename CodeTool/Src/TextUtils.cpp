#include "TextUtils.h"

#include <filesystem>
#include <fstream>
#include <sstream>

bool StartsWith(const string& str, const string& starts)
{
    return str.compare(0, starts.length(), starts) == 0;
}

bool EndsWith(const string& str, const string& ends)
{
    if (str.length() < ends.length())
        return false;

    return str.compare(str.length() - ends.length(), ends.length(), ends) == 0;
}

string& TrimStart(string& str, const string& chars /*= " "*/)
{
    size_t i = str.find_first_not_of(chars);
    str.erase(0, i == string::npos ? str.length() : i);
    return str;
}

string& TrimEnd(string& str, const string& chars /*= " "*/)
{
    size_t i = str.find_last_not_of(chars);
    str.erase(i == string::npos ? 0 : i + 1);
    return str;
}

string& Trim(string& str, const string& chars /*= " "*/)
{
    return TrimStart(TrimEnd(str, chars), chars);
}

vector<string> Split(const string& s, char delim)
{
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
        elems.push_back(item);

    return elems;
}

vector<string> SplitOutsideBraces(const string& data, char delim)
{
    vector<string> res;
    int braces = 0, sqBraces = 0, trBraces = 0, fgBraces = 0;
    int dataLen = (int)data.length();

    int lastSplit = 0;
    for (int i = 0; i < dataLen; i++)
    {
        switch (data[i])
        {
        case '{': fgBraces++; break;
        case '}': fgBraces--; break;
        case '(': braces++; break;
        case ')': braces--; break;
        case '<': trBraces++; break;
        case '>': trBraces--; break;
        case '[': sqBraces++; break;
        case ']': sqBraces--; break;
        }

        if (braces == 0 && sqBraces == 0 && trBraces == 0 && fgBraces == 0 && data[i] == delim)
        {
            res.push_back(data.substr(lastSplit, i - lastSplit));
            lastSplit = i + 1;
        }
    }

    res.push_back(data.substr(lastSplit));

    return res;
}

bool IsFileExist(const string& path)
{
    filesystem::directory_entry entry{ path };
    return entry.exists() && !entry.is_directory();
}

string ReadFile(const string& path)
{
    ifstream fin(path.c_str());
    if (!fin.is_open())
        return string();

    return string((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
}

void WriteFile(const string& path, const string& data)
{
    ofstream fout(path.c_str());
    if (!fout.is_open())
        return;

    fout.write(data.c_str(), data.length());
}

string GetFileName(const string& path)
{
    return filesystem::path(path).filename().string();
}
