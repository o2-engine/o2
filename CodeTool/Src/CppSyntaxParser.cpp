#include "CppSyntaxParser.h"

#include <algorithm> 
#include <cctype>
#include <fstream>
#include <functional> 
#include <iosfwd>
#include <locale>
#include <iostream>
#include <sstream>

size_t str_len(const string &str) {
	return str.length();
}

string& TrimStart(const string &str, const string& chars /*= " "*/)
{
	string s = str;
	int l = (int)str.length();
	int i = 0;
	for (; i < l; i++)
	{
		if (chars.find(str[i]) == string::npos)
			break;
	}

	s.erase(0, i);

	return s;
}

string& TrimEnd(const string &str, const string& chars /*= " "*/)
{
	string s = str;
	int l = (int)str.length();
	int i = l - 1;
	for (; i >= 0; i--)
	{
		if (chars.find(str[i]) == string::npos)
			break;
	}

	s.erase(i + 1);

	return s;
}

string& Trim(const string &str, const string& chars /*= " "*/)
{
	string s = str;
	return TrimStart(TrimEnd(s, chars), chars);
}

bool StartsWith(const string& str, const string& starts)
{
	int l1 = (int)str.length(), l2 = (int)starts.length();

	if (l1 < l2)
		return false;

	for (int i = 0; i < l1 && i < l2; i++)
	{
		if (str[i] != starts[i])
			return false;
	}

	return true;
}

bool EndsWith(const string& str, const string& ends)
{
	int l1 = (int)str.length(), l2 = (int)ends.length();

	if (l1 < l2)
		return false;

	for (int i = 0; i < l1 && i < l2; i++)
	{
		if (str[l1 - i - 1] != ends[l2 - i - 1])
			return false;
	}

	return true;
}

void Split(const string &s, char delim, vector<string> &elems)
{
	stringstream ss;
	ss.str(s);
	string item;
	while (getline(ss, item, delim))
		elems.push_back(item);
}

vector<string> Split(const string &s, char delim)
{
	vector<string> elems;
	Split(s, delim, elems);
	return elems;
}

CppSyntaxParser::CppSyntaxParser()
{
	InitializeParsers();
}

CppSyntaxParser::~CppSyntaxParser()
{
	for (auto x : mParsers)
		delete x;
}

void CppSyntaxParser::InitializeParsers()
{
	mParsers.push_back(new ExpressionParser("namespace", &CppSyntaxParser::ParseNamespace, false, true));
	mParsers.push_back(new ExpressionParser("namespace", &CppSyntaxParser::ParseNamespace, true, true));
	mParsers.push_back(new ExpressionParser("//", &CppSyntaxParser::ParseComment, true, true));
	mParsers.push_back(new ExpressionParser("/*", &CppSyntaxParser::ParseMultilineComment, true, true));
	mParsers.push_back(new ExpressionParser("#pragma", &CppSyntaxParser::ParsePragma, false, true));
	mParsers.push_back(new ExpressionParser("#include", &CppSyntaxParser::ParseInclude, false, true));
	mParsers.push_back(new ExpressionParser("#define", &CppSyntaxParser::ParseDefine, true, true));
	mParsers.push_back(new ExpressionParser("#ifdef", &CppSyntaxParser::ParseIfdefMacros, true, true));
	mParsers.push_back(new ExpressionParser("#if", &CppSyntaxParser::ParseIfMacros, true, true));
	mParsers.push_back(new ExpressionParser("#endif", &CppSyntaxParser::ParseEndIfMacros, true, true));
	mParsers.push_back(new ExpressionParser("#else", &CppSyntaxParser::ParseElseMacros, true, true));
	mParsers.push_back(new ExpressionParser("#elif", &CppSyntaxParser::ParseElifMacros, true, true));
	mParsers.push_back(new ExpressionParser("meta class", &CppSyntaxParser::ParseMetaClass, true, true));
	mParsers.push_back(new ExpressionParser("class", &CppSyntaxParser::ParseClass, true, true));
	mParsers.push_back(new ExpressionParser("struct", &CppSyntaxParser::ParseStruct, true, true));
	mParsers.push_back(new ExpressionParser("template", &CppSyntaxParser::ParseTemplate, true, true));
	mParsers.push_back(new ExpressionParser("typedef", &CppSyntaxParser::ParseTypedef, true, true));
	mParsers.push_back(new ExpressionParser("enum", &CppSyntaxParser::ParseEnum, true, true));
	mParsers.push_back(new ExpressionParser("using", &CppSyntaxParser::ParseUsing, true, true));
	mParsers.push_back(new ExpressionParser("public:", &CppSyntaxParser::ParsePublicSection, true, false));
	mParsers.push_back(new ExpressionParser("private:", &CppSyntaxParser::ParsePrivateSection, true, false));
	mParsers.push_back(new ExpressionParser("protected:", &CppSyntaxParser::ParseProtectedSection, true, false));
	mParsers.push_back(new ExpressionParser("friend", &CppSyntaxParser::ParseFriend, true, false));
	mParsers.push_back(new ExpressionParser("ATTRIBUTE_COMMENT_DEFINITION", &CppSyntaxParser::ParseAttributeCommentDef, true, false));
	mParsers.push_back(new ExpressionParser("ATTRIBUTE_SHORT_DEFINITION", &CppSyntaxParser::ParseAttributeShortDef, true, false));
	mParsers.push_back(new ExpressionParser("ATTRIBUTES", &CppSyntaxParser::ParseAttributes, true, false));
	mParsers.push_back(new ExpressionParser("PROPERTIES", &CppSyntaxParser::ParseProperties, true, false));
	mParsers.push_back(new ExpressionParser("PROPERTY", &CppSyntaxParser::ParseProperty, true, false));
	mParsers.push_back(new ExpressionParser("GETTER", &CppSyntaxParser::ParseGetter, true, false));
	mParsers.push_back(new ExpressionParser("SETTER", &CppSyntaxParser::ParseSetter, true, false));
	mParsers.push_back(new ExpressionParser("ACCESSOR", &CppSyntaxParser::ParseAccessor, true, false));
}

void CppSyntaxParser::ParseFile(SyntaxFile& file, const string& filePath, const TimeStamp& fileEditDate)
{
	file.mPath = filePath;
	file.mLastEditedDate = fileEditDate;

	ifstream fin;
	fin.open(filePath.c_str());
	if (!fin.is_open())
		return;

	file.mData = string((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

	fin.close();

	if (file.mData.find("@CODETOOLIGNORE") != string::npos)
		return;

	ParseSyntaxSection(*file.mGlobalNamespace, file.mData, file, SyntaxProtectionSection::Public);
}

void CppSyntaxParser::ParseSyntaxSection(SyntaxSection& section, const string& source, SyntaxFile& file,
										 SyntaxProtectionSection protectionSection)
{
	section.mLength = (int)source.length();
	section.mData = source;
	section.mFile = &file;

	string skipSymbols = " \r\n\t;";

	int caret = 0;
	while (caret < section.mLength)
	{
		if (skipSymbols.find(source[caret]) != string::npos)
		{
			caret++;
			continue;
		}

		bool parsedByKeywork = false;
		for (auto parser : mParsers)
		{
			const char* keyWord = parser->keyWord;

			int i = 0;
			bool success = true;
			while (keyWord[i] != '\0')
			{
				if (source[i + caret] != keyWord[i])
				{
					success = false;
					break;
				}
				i++;
			}

			if (!success)
				continue;

			ParserDelegate pd = parser->parser;
			(this->*pd)(section, caret, protectionSection);
			parsedByKeywork = true;

			break;
		}

		if (parsedByKeywork)
			continue;

		int blockbegin = caret;
		string block = ReadBlock(section.mData, caret);
		Trim(block, " \r\t\n");
		if (block[0] == '{' && block[block.length() - 1] == '}')
		{
			block.erase(block.length() - 1, 1);
			block.erase(0, 1);
		}

		if (!block.empty())
			TryParseBlock(section, block, blockbegin, caret, protectionSection);
		else
			caret++;
	}
}

int CppSyntaxParser::GetLineNumber(const string& data, int caret)
{
	int res = 0;
	for (int i = 0; i < caret; i++)
		if (data[i] == '\n')
			res++;

	return res;
}

void CppSyntaxParser::TryParseBlock(SyntaxSection& section, const string& block, int blockBegin, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	if (IsFunction(block))
	{
		auto func = ParseFunction(block, protectionSection, blockBegin, caret);
		func->mFile = section.mFile;
		func->mLine = GetLineNumber(section.mData, caret);
		section.mFunctions.push_back(func);
	}
	else
	{
		auto var = ParseVariable(block, protectionSection, blockBegin, caret);
		var->mFile = section.mFile;
		var->mLine = GetLineNumber(section.mData, caret);
		section.mVariables.push_back(var);
	}
}

bool CppSyntaxParser::IsFunction(const string& data)
{
	int locCaret = 0;
	bool isFunction = false;

	string firstWord = ReadWord(data, locCaret, " \n\r(){}[]");

	if (firstWord.empty())
		return false;

	if (firstWord == "virtual")
		firstWord = ReadWord(data, locCaret, " \n\r(){}[]");

	if (firstWord == "static")
		firstWord = ReadWord(data, locCaret, " \n\r(){}[]");

	if (firstWord == "typename")
		firstWord = ReadWord(data, locCaret, " \n\r(){}[]");

	if (firstWord == "inline")
		firstWord = ReadWord(data, locCaret, " \n\r(){}[]");

	if (GetNextSymbol(data, locCaret, " \n\r\t") == '(')
	{
		string braces = ReadBraces(data, locCaret);
		Trim(braces, " \n\t\r()");

		int tmpCaret = 0;
		string word = ReadWord(braces, tmpCaret);

		isFunction = GetNextSymbol(braces, tmpCaret, " \n\r\t") != ':';

		if (!isFunction && StartsWith(braces, "std"))
			isFunction = true;
	}
	else
	{
		if (firstWord == "const")
			ReadWord(data, locCaret, " \n\r(){}[]");

		string thirdWord = ReadWord(data, locCaret, " \n\r(){}[]<>-");

		if (thirdWord == "operator")
			thirdWord = ReadWord(data, locCaret, " \n\r(){}", "\n\r", true, true, false);

		if (GetNextSymbol(data, locCaret, " \n\r\t") == '(')
			isFunction = true;
	}

	return isFunction;
}

SyntaxVariable* CppSyntaxParser::ParseVariable(const string& data, SyntaxProtectionSection& protectionSection,
											   int begin, int end)
{
	SyntaxVariable* res = new SyntaxVariable();
	res->mBegin = begin;
	res->mLength = end - begin;
	res->mData = data;

	int caret = 0;
	string typeWord = ReadWord(data, caret, " \n\r(){}[]");
	string typeDefinition = typeWord;

	if (typeWord == "static")
	{
		typeWord = ReadWord(data, caret, " \n\r(){}[]");
		typeDefinition += " " + typeWord;
		res->mIsStatic = true;
	}

	if (typeWord == "const")
	{
		typeWord = ReadWord(data, caret, " \n\r(){}[]");
		res->mType.mIsContant = true;

		typeDefinition += " " + typeWord;
	}

	if (typeWord == "mutable")
	{
		typeWord = ReadWord(data, caret, " \n\r(){}[]");
		res->mType.mIsMutable = true;

		typeDefinition += " " + typeWord;
	}

	if (!typeWord.empty() && typeWord[typeWord.length() - 1] == '&')
		res->mType.mIsReference = true;

	if (!typeWord.empty() && typeWord[typeWord.length() - 1] == '*')
		res->mType.mIsPointer = true;

	res->mType.mName = typeWord;

	res->mClassSection = protectionSection;

	if (GetNextSymbol(data, caret, " \n\r\t") == '(')
	{
		string braces = Trim(ReadBraces(data, caret), " \r\t\t()");
		string nextBraces = Trim(ReadBraces(data, caret), " \r\t\t()");

		int tmpCaret = 0;
		string bracesFirst = ReadWord(braces, tmpCaret);
		if (braces[tmpCaret] == '*')
		{
			tmpCaret += 3;
			res->mName = braces.substr(tmpCaret);

			res->mType.mName += " (" + bracesFirst + "*)(" + nextBraces + ")";
		}
	}
	else
	{
		string nextWord = ReadWord(data, caret, " (){}[]");
		if (nextWord == "const")
			res->mName = ReadWord(data, caret, " (){}[]");
		else
			res->mName = nextWord;

		nextWord = ReadWord(data, caret, " (){}[]");
		if (nextWord == "=")
			res->mDefaultValue = ReadWord(data, caret, ";");
	}

	return res;
}

SyntaxFunction* CppSyntaxParser::ParseFunction(const string& data, SyntaxProtectionSection& protectionSection,
											   int begin, int end)
{
	SyntaxFunction* res = new SyntaxFunction();
	res->mBegin = begin;
	res->mLength = end - begin;
	res->mData = data;

	res->mClassSection = protectionSection;

	int caret = 0;
	string typeWord = ReadWord(data, caret, " \n\r(){}[]");

	if (typeWord == "virtual")
	{
		res->mIsVirtual = true;
		typeWord = ReadWord(data, caret, " \n\r(){}[]");
	}

	if (typeWord == "static")
	{
		res->mIsStatic = true;
		typeWord = ReadWord(data, caret, " \n\r(){}[]");
	}

	if (typeWord == "inline")
		typeWord = ReadWord(data, caret, " \n\r(){}[]");

	if (typeWord == "typename")
		typeWord = ReadWord(data, caret, " \n\r(){}[]");

	if (typeWord == "explicit")
		typeWord = ReadWord(data, caret, " \n\r(){}[]");

	if (typeWord == "operator")
	{
		string nextWord = ReadWord(data, caret, " \n\r(){}[]");
		res->mName = typeWord + nextWord;
		res->mReturnType.mName = "void";
	}
	else
	{
		if (GetNextSymbol(data, caret, " \n\r\t") == '(')
		{
			res->mName = typeWord;
			res->mReturnType.mName = "void";
		}
		else
		{
			string typeDefinition = typeWord;

			if (typeWord == "const")
			{
				typeWord = ReadWord(data, caret, " \n\r(){}[]");
				res->mReturnType.mIsContant = true;

				typeDefinition += " " + typeWord;
			}

			if (!typeWord.empty() && typeWord[typeWord.length() - 1] == '&')
				res->mReturnType.mIsReference = true;

			if (!typeWord.empty() && typeWord[typeWord.length() - 1] == '*')
				res->mReturnType.mIsPointer = true;

			res->mReturnType.mName = typeWord;

			res->mName = ReadWord(data, caret, " \n\r(){}[]");

			if (res->mName == "operator")
				res->mName += " " + ReadWord(data, caret, " \n\r(){}");
		}
	}

	string paramsStr = Trim(ReadBraces(data, caret), " \n\r\t");
	string afterParamWord = ReadWord(data, caret);

	if (afterParamWord == "const")
		res->mIsContstant = true;

	if (!paramsStr.empty())
	{
		auto paramsArr = Split(paramsStr, ',');
		for (auto& prm : paramsArr)
		{
			Trim(prm, " \r\n\t");
			SyntaxProtectionSection tempProtectSection = SyntaxProtectionSection::Public;
			res->mParameters.push_back(ParseVariable(prm, tempProtectSection, begin, end));
		}
	}

	return res;
}

void CppSyntaxParser::ParseNamespace(SyntaxSection& section, int& caret,
									 SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("namespace");

	string namespaceName = ReadWord(section.mData, caret);
	int namespaceBegin = (int)section.mData.find("{", caret) + 1;
	string block = Trim(ReadBlock(section.mData, caret), " \r\t\n");

	SyntaxNamespace* newNamespace = new SyntaxNamespace();
	newNamespace->mBegin = begin;
	newNamespace->mLength = caret - begin;
	newNamespace->mLine = GetLineNumber(section.mData, caret);
	newNamespace->mData = Trim(block.substr(1, block.length() - 1), " \r\t\n");
	newNamespace->mName = namespaceName;
	newNamespace->mFullName = section.mFullName.empty() ? namespaceName : section.mFullName + "::" + namespaceName;
	newNamespace->mFile = section.mFile;
	newNamespace->mParentSection = &section;
	section.mSections.push_back(newNamespace);

	ParseSyntaxSection(*newNamespace, newNamespace->mData, *section.mFile, SyntaxProtectionSection::Public);
}

void CppSyntaxParser::ParseComment(SyntaxSection& section, int& caret,
								   SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("//");

	SyntaxComment* comment = new SyntaxComment();

	int dataLength = (int)section.mData.length();
	string skipSymbols = " \r\t;";
	int tmpCaret = caret;
	bool first = true;
	int line = 0;
	do
	{
		caret = tmpCaret;

		if (!first)
		{
			caret += 2;
			comment->mData += '\n';
		}

		string buff = Trim(ReadWord(section.mData, caret, "\n", ""), " \r");
		comment->mData += buff;

		line = GetLineNumber(section.mData, caret);

		tmpCaret = caret + 1;
		for (; tmpCaret < dataLength; tmpCaret++)
		{
			if (skipSymbols.find(section.mData[tmpCaret]) == string::npos)
				break;
		}

		if (first)
			first = false;
	}
	while (tmpCaret < dataLength - 2 && section.mData[tmpCaret] == '/' && section.mData[tmpCaret + 1] == '/');

	caret = tmpCaret;

	comment->mBegin  = begin;
	comment->mLength = caret - begin;
	comment->mFile   = section.mFile;
	comment->mLine   = line;

	section.mComments.push_back(comment);
}

void CppSyntaxParser::ParseMultilineComment(SyntaxSection& section, int& caret,
											SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("/*");
	int end = (int)section.mData.find("*/", caret);
	caret = end;

	SyntaxComment* comment = new SyntaxComment();
	comment->mData = Trim(section.mData.substr(begin + 2, end - 4 - begin), " \r\t\n");
	comment->mBegin = begin;
	comment->mLength = caret - begin;
	comment->mFile = section.mFile;

	section.mComments.push_back(comment);
}

void CppSyntaxParser::ParsePragma(SyntaxSection& section, int& caret,
								  SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#pragma");
	ReadWord(section.mData, caret);
}

void CppSyntaxParser::ParseInclude(SyntaxSection& section, int& caret,
								   SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#include");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseDefine(SyntaxSection& section, int& caret,
								  SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#define");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseIfdefMacros(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#ifdef");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseIfMacros(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#if");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseEndIfMacros(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#endif");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseElifMacros(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#elif");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseElseMacros(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("#else");
	ReadWord(section.mData, caret, "\n", "");
}

void CppSyntaxParser::ParseMetaClass(SyntaxSection& section, int& caret,
									 SyntaxProtectionSection& protectionSection)
{
	ParseClassOrStruct(section, caret, protectionSection, true, true, "");
}

void CppSyntaxParser::ParseClass(SyntaxSection& section, int& caret,
								 SyntaxProtectionSection& protectionSection)
{
	ParseClassOrStruct(section, caret, protectionSection, true, false, "");
}

void CppSyntaxParser::ParseStruct(SyntaxSection& section, int& caret,
								  SyntaxProtectionSection& protectionSection)
{
	ParseClassOrStruct(section, caret, protectionSection, false, false, "");
}

void CppSyntaxParser::ParseClassOrStruct(SyntaxSection& section, int& caret,
										 SyntaxProtectionSection& protectionSection,
										 bool isClass, bool isMeta, const string& templates)
{
	int begin = caret;

	if (isMeta)
		caret += (int)str_len("meta");

	if (isClass) caret += (int)str_len("class");
	else         caret += (int)str_len("struct");

	string className = ReadWord(section.mData, caret, " \n\t\r:;/");
	string afterName = Trim(ReadWord(section.mData, caret, ";{/"), " :\r\n\t");

	string shortClassName = className;
	className = section.mFullName.empty() ? className : section.mFullName + "::" + className;

	SyntaxClass* newClass = new SyntaxClass();

	newClass->mBegin              = begin;
	newClass->mLength             = caret - begin;
	newClass->mLine               = GetLineNumber(section.mData, caret);
	newClass->mData               = section.mData.substr(begin, caret - begin);
	newClass->mName               = shortClassName;
	newClass->mFullName           = className;
	newClass->mFile               = section.mFile;
	newClass->mParentSection      = &section;
	newClass->mClassSection       = protectionSection;
	newClass->mTemplateParameters = templates;

	if (!afterName.empty())
	{
		auto baseClasses = Split(afterName, ',');

		for (auto& baseClass : baseClasses)
		{
			Trim(baseClass);

			int spacePos = (int)baseClass.find(' ');
			if (spacePos == baseClass.npos)
				newClass->mBaseClasses.push_back(SyntaxClassInheritance(baseClass, SyntaxProtectionSection::Private));
			else
			{
				if (StartsWith(baseClass, "virtual"))
				{
					baseClass.erase(0, str_len("virtual") + 1);
					spacePos = (int)baseClass.find(' ');
				}

				string sectionTypeName = baseClass.substr(0, spacePos);
				string baseClassName = baseClass.substr(spacePos + 1);

				if (StartsWith(baseClassName, "virtual"))
					baseClassName.erase(0, str_len("virtual") + 1);

				SyntaxProtectionSection sectionType = SyntaxProtectionSection::Private;

				if (sectionTypeName == "public")
					sectionType = SyntaxProtectionSection::Public;
				else if (sectionTypeName == "protected")
					sectionType = SyntaxProtectionSection::Protected;

				newClass->mBaseClasses.push_back(SyntaxClassInheritance(baseClassName, sectionType));
			}
		}
	}

	int dataLength = (int)section.mData.length();

	if (caret < dataLength && section.mData[caret] == '/')
	{
		string comment = ReadWord(section.mData, caret, "\n");
		ReadWord(section.mData, caret, ";{/");
	}

	if (caret < dataLength && section.mData[caret] == '{')
	{
		int sectionBegin = caret;
		newClass->mData = Trim(ReadBlock(section.mData, caret), "{} \n\r\t");

		section.mSections.push_back(newClass);

		ParseSyntaxSection(*newClass, newClass->mData, *newClass->mFile, protectionSection);
	}
}

void CppSyntaxParser::ParseTemplate(SyntaxSection& section, int& caret,
									SyntaxProtectionSection& protectionSection)
{
	caret += (int)str_len("template");

	int dataLen = (int)section.mData.length();

	for (; caret < dataLen; caret++)
		if (section.mData[caret] == '<')
			break;

	caret++;
	int braces = 1;
	int begin = caret;

	for (; caret < dataLen; caret++)
	{
		if (section.mData[caret] == '<')
			braces++;

		if (section.mData[caret] == '>')
		{
			braces--;

			if (braces == 0)
				break;
		}
	}

	string tempInside = section.mData.substr(begin, caret - begin);

	int tmpCaret = caret + 1;
	string block = Trim(ReadBlock(section.mData, tmpCaret), " \n\r\t");

	if (StartsWith(block, "class"))
	{
		caret = (int)section.mData.find("class", caret + 1);
		ParseClassOrStruct(section, caret, protectionSection, true, false, tempInside);
	}
	else if (StartsWith(block, "struct"))
	{
		caret = (int)section.mData.find("struct", caret + 1);
		ParseClassOrStruct(section, caret, protectionSection, false, false, tempInside);
	}
	else if (StartsWith(block, "metaclass"))
	{
		caret = (int)section.mData.find("metaclass", caret + 1);
		ParseClassOrStruct(section, caret, protectionSection, true, true, tempInside);
	}
	else if (StartsWith(block, "friend"))
	{
		caret = (int)section.mData.find("friend", caret + 1);
		ParseFriend(section, caret, protectionSection);
	}
	else if (IsFunction(block))
	{
		auto func = ParseFunction(block, protectionSection, 0, (int)block.length());
		func->mTemplates = tempInside;
		func->mFile = section.mFile;
		func->mLine = GetLineNumber(section.mData, caret);
		section.mFunctions.push_back(func);

		caret = tmpCaret;
	}
	else caret = tmpCaret;
}

void CppSyntaxParser::ParseTypedef(SyntaxSection& section, int& caret,
								   SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("typedef");
	string temp = Trim(ReadWord(section.mData, caret, ";"), " \r\n\t");
	int lastSpace = (int)temp.rfind(' ');
	string value = Trim(temp.substr(0, lastSpace), " \r\t\n");
	string name = Trim(temp.substr(lastSpace + 1), " \r\t\n;");

	if (StartsWith(value, "typename"))
		value.erase(0, str_len("typename "));

	SyntaxTypedef* synTypedef = new SyntaxTypedef();
	synTypedef->mBegin      = begin;
	synTypedef->mLength     = caret - begin;
	synTypedef->mLine       = GetLineNumber(section.mData, caret);
	synTypedef->mData       = section.mData.substr(begin, caret - begin);
	synTypedef->mWhatName   = value;
	synTypedef->mNewDefName = name;

	section.mTypedefs.push_back(synTypedef);
}

void CppSyntaxParser::ParseEnum(SyntaxSection& section, int& caret,
								SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("enum");

	string name = ReadWord(section.mData, caret);

	if (name == "class")
		name = ReadWord(section.mData, caret);

	string block = Trim(ReadBlock(section.mData, caret), " {}\r\t\n");
	RemoveComments(block);
	auto content = Split(block, ',');

	SyntaxEnum* newEnum    = new SyntaxEnum();
	newEnum->mBegin        = begin;
	newEnum->mLength       = caret - begin;
	newEnum->mLine         = GetLineNumber(section.mData, caret);
	newEnum->mData         = section.mData.substr(begin, caret - begin);
	newEnum->mName         = name;
	newEnum->mFullName     = section.GetFullName().empty() ? name : section.GetFullName() + "::" + name;
	newEnum->mFile         = section.mFile;
	newEnum->mOwnerSection = &section;

	for (auto& x : content)
	{
		Trim(x, " \n\t\r");

		string name, value;
		int valuePos = (int)x.find('=');

		if (valuePos != x.npos)
		{
			name = Trim(x.substr(0, valuePos), " \n\t\r");
			value = Trim(x.substr(valuePos + 1), " \n\t\r");
		}
		else name = x;

		newEnum->mEntries[name] = value;
	}

	section.mEnums.push_back(newEnum);
}

void CppSyntaxParser::ParseUsing(SyntaxSection& section, int& caret,
								 SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("using");

	int tmpCaret = caret;
	string allWord = ReadWord(section.mData, tmpCaret, ";");
	if (allWord.find(':') != string::npos)
	{
		caret = tmpCaret + 1;
		return;
	}

	ReadWord(section.mData, caret);
	string name = Trim(ReadWord(section.mData, caret), " \r\n;");

	SyntaxUsingNamespace* newUsing = new SyntaxUsingNamespace();

	newUsing->mBegin              = begin;
	newUsing->mLength             = caret - begin;
	newUsing->mLine               = GetLineNumber(section.mData, caret);
	newUsing->mData               = section.mData.substr(begin, caret - caret);
	newUsing->mUsingNamespaceName = name;

	section.mUsingNamespaces.push_back(newUsing);
}

void CppSyntaxParser::ParsePublicSection(SyntaxSection& section, int& caret,
										 SyntaxProtectionSection& protectionSection)
{
	caret += (int)str_len("public:");
	protectionSection = SyntaxProtectionSection::Public;
}

void CppSyntaxParser::ParsePrivateSection(SyntaxSection& section, int& caret,
										  SyntaxProtectionSection& protectionSection)
{
	caret += (int)str_len("private:");
	protectionSection = SyntaxProtectionSection::Private;
}

void CppSyntaxParser::ParseProtectedSection(SyntaxSection& section, int& caret,
											SyntaxProtectionSection& protectionSection)
{
	caret += (int)str_len("protected:");
	protectionSection = SyntaxProtectionSection::Protected;
}

void CppSyntaxParser::ParseFriend(SyntaxSection& section, int& caret,
								  SyntaxProtectionSection& protectionSection)
{
	int begin = caret;
	caret += (int)str_len("friend");
	ReadWord(section.mData, caret, " \n\r\t");
	ReadWord(section.mData, caret, " \n\r\t");
}

void CppSyntaxParser::ParseAttributeCommentDef(SyntaxSection& section, int& caret,
											   SyntaxProtectionSection& protectionSection)
{
	SyntaxClass* classSection = dynamic_cast<SyntaxClass*>(&section);

	caret += (int)str_len("ATTRIBUTE_COMMENT_DEFINITION");
	caret = (int)section.mData.find('"', caret) + 1;
	int begin = caret;
	caret = (int)section.mData.find('"', caret);

	classSection->mAttributeCommentDef = section.mData.substr(begin, caret - begin);

	caret = (int)section.mData.find(';', caret) + 1;
}

void CppSyntaxParser::ParseAttributeShortDef(SyntaxSection& section, int& caret,
											 SyntaxProtectionSection& protectionSection)
{
	SyntaxClass* classSection = dynamic_cast<SyntaxClass*>(&section);

	caret += (int)str_len("ATTRIBUTE_SHORT_DEFINITION");
	caret = (int)section.mData.find('"', caret) + 1;
	int begin = caret;
	caret = (int)section.mData.find('"', caret);

	classSection->mAttributeShortDef = section.mData.substr(begin, caret - begin);

	caret = (int)section.mData.find(';', caret) + 1;
}

void CppSyntaxParser::ParseAttributes(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("ATTRIBUTES");
	caret = (int)section.mData.find('(', caret) + 1;
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);

	vector<string> separated = Split(braces, ',');
	for (auto& x : separated)
		Trim(x, " \n\r\t,");

	SyntaxAttributes* attributes = new SyntaxAttributes();
	attributes->mBegin  = begin;
	attributes->mLength = caret - begin;
	attributes->mLine   = GetLineNumber(section.mData, caret);
	attributes->mData   = section.mData.substr(begin, caret - caret);
	attributes->mAttributesList = separated;

	section.mAttributes.push_back(attributes);
}

void CppSyntaxParser::ParseProperties(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("PROPERTIES");
	caret = (int)section.mData.find('(', caret);
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);
}

void CppSyntaxParser::ParseProperty(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("PROPERTY");
	caret = (int)section.mData.find('(', caret);
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);

	vector<string> separated = Split(braces, ',');
	for (auto& x : separated)
		Trim(x, " \n\r\t,");

	SyntaxVariable* res = new SyntaxVariable();
	res->mBegin      = begin;
	res->mLength     = caret - begin;
	res->mData       = section.mData.substr(begin, caret - caret);
	res->mLine       = GetLineNumber(section.mData, caret);
	res->mName       = separated[1];
	res->mType.mName = separated[0];

	section.mVariables.push_back(res);
}

void CppSyntaxParser::ParseGetter(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("GETTER");
	caret = (int)section.mData.find('(', caret);
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);

	vector<string> separated = Split(braces, ',');
	for (auto& x : separated)
		Trim(x, " \n\r\t,");

	SyntaxVariable* res = new SyntaxVariable();
	res->mBegin      = begin;
	res->mLength     = caret - begin;
	res->mData       = section.mData.substr(begin, caret - caret);
	res->mLine       = GetLineNumber(section.mData, caret);
	res->mName       = separated[1];
	res->mType.mName = separated[0];

	section.mVariables.push_back(res);
}

void CppSyntaxParser::ParseSetter(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("SETTER");
	caret = (int)section.mData.find('(', caret);
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);

	vector<string> separated = Split(braces, ',');
	for (auto& x : separated)
		Trim(x, " \n\r\t,");

	SyntaxVariable* res = new SyntaxVariable();
	res->mBegin      = begin;
	res->mLength     = caret - begin;
	res->mData       = section.mData.substr(begin, caret - caret);
	res->mLine       = GetLineNumber(section.mData, caret);
	res->mName       = separated[1];
	res->mType.mName = separated[0];

	section.mVariables.push_back(res);
}

void CppSyntaxParser::ParseAccessor(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection)
{
	int begin = caret;

	caret += (int)str_len("ACCESSOR");
	caret = (int)section.mData.find('(', caret);
	string braces = Trim(ReadBraces(section.mData, caret), " \n\r\t()");
	caret = (int)section.mData.find(';', caret);

	vector<string> separated = Split(braces, ',');
	for (auto& x : separated)
		Trim(x, " \n\r\t,");

	SyntaxVariable* res = new SyntaxVariable();
	res->mBegin      = begin;
	res->mLength     = caret - begin;
	res->mData       = section.mData.substr(begin, caret - caret);
	res->mLine       = GetLineNumber(section.mData, caret);
	res->mName       = separated[1];
	res->mType.mName = separated[0];

	section.mVariables.push_back(res);
}

string CppSyntaxParser::ReadWord(const string& data, int& caret,
								 const char* breakSymbols /*= " \n\r(){}.,;+-* /=@!|&*:~\\"*/,
								 const char* skipSymbols /*= " \n\r"*/,
								 bool checkBraces /*= true*/, bool checkFgBraces /*= true*/, bool checkTrBraces /*= true*/)
{
	string res = "";
	int braces = 0, sqBraces = 0, trBraces = 0, fgBraces = 0;
	int dataLen = (int)data.length();

	for (; caret < dataLen; caret++)
	{
		int i = 0;
		bool stop = true;
		char s = data[caret];

		while (skipSymbols[i] != '\0')
		{
			if (skipSymbols[i] == s)
			{
				stop = false;
				break;
			}

			i++;
		}

		if (stop)
			break;
	}

	for (; caret < dataLen; caret++)
	{
		int i = 0;
		bool stop = false;
		char s = data[caret];

		if ((fgBraces == 0 || !checkFgBraces) && (braces == 0 || !checkBraces) && (trBraces == 0 || !checkTrBraces))
		{
			while (breakSymbols[i] != '\0')
			{
				if (breakSymbols[i] == s)
				{
					stop = true;
					break;
				}

				i++;
			}

			if (stop)
				break;
		}

		switch (s)
		{
			case '{': fgBraces++; break;
			case '}': fgBraces--; if (fgBraces < 0) fgBraces = 0; break;

			case ')': braces--; if (braces < 0) braces = 0; break;
			case '(': braces++; break;

			case '[': sqBraces++; break;
			case ']': sqBraces--; if (sqBraces < 0) sqBraces = 0; break;

			case '<': trBraces++; break;
			case '>': trBraces--; if (trBraces < 0) trBraces = 0; break;
		}

		res += s;
	}

	return res;
}

string CppSyntaxParser::ReadBlock(const string& data, int& caret)
{
	int begin = caret;
	int braces = 0, fgBraces = 0, sqBraces = 0, trBraces = 0;
	bool isInstring = false;
	int dataLen = (int)data.length();

	for (; caret < dataLen; caret++)
	{
		if (data[caret] == '{')
			break;

		if (data[caret] == ';')
			return data.substr(begin, caret - begin);
	}

	caret++;
	fgBraces++;

	bool complete = false;
	for (; caret < dataLen && !complete; caret++)
	{
		if (isInstring)
		{
			if (data[caret] == '"' && data[caret - 1] != '\\')
				isInstring = false;

			continue;
		}

		switch (data[caret])
		{
			case '{': fgBraces++; break;
			case '}':
			fgBraces--;
			if (fgBraces == 0 && braces == 0 && sqBraces == 0)
				complete = true;
			break;

			case ')': braces--; break;
			case '(': braces++; break;

			case '[': sqBraces++; break;
			case ']': sqBraces--; break;

			case '<': trBraces++; break;
			case '>': trBraces--; break;

			case '"':
			isInstring = true;
			break;
		}
	}

	return data.substr(begin, min(caret, dataLen - 1) - begin);
}

string CppSyntaxParser::ReadBraces(const string& data, int& caret)
{
	int begin = caret;
	int braces = 0, fgBraces = 0, sqBraces = 0, trBraces = 0;
	bool isInstring = false;
	int dataLen = (int)data.length();

	for (; caret < dataLen; caret++)
	{
		if (data[caret] == '(')
			break;

		if (data[caret] == ';')
			return data.substr(begin, caret - begin);
	}

	caret++;
	braces++;

	bool complete = false;
	for (; caret < dataLen && !complete; caret++)
	{
		if (isInstring)
		{
			if (data[caret] == '"' && data[caret - 1] != '\\')
				isInstring = false;

			continue;
		}

		switch (data[caret])
		{
			case '}': fgBraces--; break;
			case '{': fgBraces++; break;

			case '[': sqBraces++; break;
			case ']': sqBraces--; break;

			case '<': trBraces++; break;
			case '>': trBraces--; break;

			case '(': braces++; break;
			case ')':
			braces--;
			if (fgBraces == 0 && braces == 0 && sqBraces == 0 && trBraces == 0)
				complete = true;
			break;

			case '"':
			isInstring = true;
			break;
		}
	}

	string res = data.substr(begin, min(caret, dataLen) - begin);
	int resLen = (int)res.length();

	if (!res.empty() && res[resLen - 1] == ')')
		res.erase(resLen - 1, 1);

	if (!res.empty() && res[0] == '(')
		res.erase(0, 1);

	return res;
}

char CppSyntaxParser::GetNextSymbol(const string& data, int begin, const char* skipSymbols /*= " \n\r\t()[]{}"*/)
{
	int dataLen = (int)data.length();
	for (; begin < dataLen; begin++)
	{
		int i = 0;
		bool stop = true;
		char s = data[begin];

		while (skipSymbols[i] != '\0')
		{
			if (skipSymbols[i] == s)
			{
				stop = false;
				break;
			}

			i++;
		}

		if (stop)
			return s;
	}

	return '\0';
}

vector<string> CppSyntaxParser::Split(const string& data, char splitSymbol)
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

		if (braces == 0 && sqBraces == 0 && trBraces == 0 && fgBraces == 0 && data[i] == splitSymbol)
		{
			res.push_back(data.substr(lastSplit, i - lastSplit));
			lastSplit = i + 1;
		}
	}

	res.push_back(data.substr(lastSplit));

	return res;
}

void CppSyntaxParser::RemoveComments(string& input)
{
	size_t p;
	do
	{
		p = input.find("/*");

		if (p == string::npos)
			break;

		size_t end = input.find("*/", p);

		if (end == string::npos)
			end = input.length();
		else
			end += 2;

		input.erase(p, end - p);
	}
	while (p != string::npos);

	do
	{
		p = input.find("//");

		if (p == string::npos)
			break;

		size_t end = input.find('\n', p);

		if (end == string::npos)
			end = input.length();
		else
			end++;

		input.erase(p, end - p);
	}
	while (p != string::npos);
}

CppSyntaxParser::ExpressionParser::ExpressionParser(const char* keyWord, ParserDelegate parser,
													bool isPossibleInClass /*= true*/,
													bool isPossibleInNamespace /*= true*/):
	keyWord(keyWord), parser(parser), isPossibleInClass(isPossibleInClass), isPossibleInNamespace(isPossibleInNamespace)
{}
