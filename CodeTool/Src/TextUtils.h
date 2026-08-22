#pragma once

#include <string>
#include <vector>

using namespace std;

bool EndsWith(const string& str, const string& ends);
bool StartsWith(const string& str, const string& starts);

string& Trim(string& str, const string& chars = " ");
string& TrimEnd(string& str, const string& chars = " ");
string& TrimStart(string& str, const string& chars = " ");

// Splits by delimiter, ignoring nesting
vector<string> Split(const string& s, char delim);

// Splits by delimiter at zero (), {}, [], <> nesting depth only
vector<string> SplitOutsideBraces(const string& data, char delim);

bool IsFileExist(const string& path);
string ReadFile(const string& path);
void WriteFile(const string& path, const string& data);

string GetFileName(const string& path);
