//---------------------------------------------------------------------------
#include <vcl.h>

#include <memory>
#include <string>
#include <boost/regex.hpp>
#include <list>
#include <iterator>
#include <sstream>
#include <Shlobj.h>
//---------------------------------------------------------------------------
#pragma hdrstop

#include "UnitMain.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormFRenum *FormFRenum;

//---------------------------------------------------------------------------
#pragma comment(lib, "Shell32.lib")

#define MAX_DIGITS_LIMIT 18


//---------------------------------------------------------------------------
std::list<NamePair> FileList, FolderList;
int NumDirs = 0, NumDirsRenamed = 0;
int NumFiles = 0, NumFilesRenamed = 0;
int NumWarnings = 0, NumErrors = 0;
String Dir;
bool Recursive = false;
bool AlsoFolders = false;
int MaxDigits = 4;



//---------------------------------------------------------------------------
long long StrToLongLong(const wchar_t *str)
{
	long long num;
	std::wstring strnum(str);
	std::wistringstream is(strnum);
	is >> num;
	if (!is)
		throw Exception((String)L"StrToLongLong() conversion error of \"" + str + L"\"");

	return num;
}

//---------------------------------------------------------------------------
void BrowseToFile(wchar_t *filename)
{
	ITEMIDLIST *pidl = ILCreateFromPath(filename);
	if(pidl) {
		SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
		ILFree(pidl);
	}
}

//---------------------------------------------------------------------------
void TFormFRenum::showMsg(UnicodeString msg, void *data)
{
	TListItem *ListItem = ListView1->Items->Add();
	ListItem->Caption = msg;
	ListItem->Data = data;
}

//---------------------------------------------------------------------------
__fastcall TFormFRenum::TFormFRenum(TComponent* Owner)
	: TForm(Owner)
{
//	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (ParamCount() < 1)
	{
		ShowMessage((String)L"Pad/trim numbers in file/folder names with leading zeroes\n\n"
								"Usage:\nfrenum <folder> [N] [-r] [-d]\n\n"
								L"N   number of digits (default: " + MaxDigits +")\n"
								"-r   include subfolders\n"
								"-d   process folder names too");
		exit(0);
	}
	Dir = ParamStr(1);
	String p;
	for (int i = 2; i <= ParamCount(); i++)
	{
		p = ParamStr(i);
		if (p.CompareIC(L"-r") == 0)
		{
			Recursive = true;
		}
		else if (p.CompareIC(L"-d") == 0)
		{
			AlsoFolders = true;
		}
		else
		{
			try
			{
				MaxDigits = StrToInt(p);
				if (MaxDigits < 1 || MaxDigits > MAX_DIGITS_LIMIT)
				{
					ShowMessage((String)L"Number of digits cannot be less than 1 or more than " + MAX_DIGITS_LIMIT);
					exit(0);
				}
			} catch (EConvertError &e) { }
		}
	}
	return;
}

//---------------------------------------------------------------------------
void TFormFRenum::EnumFiles(const String& strFilePath, const bool& bRecursive, const int iMax)
{
	String strFoundFilePath;
	WIN32_FIND_DATA file;

	String msg;
	String ext, name;
	NamePair pair, *pPair;

	String strPathToSearch = strFilePath;
	if (!strPathToSearch.IsEmpty())
		strPathToSearch = IncludeTrailingPathDelimiter(strPathToSearch);

	HANDLE hFile = FindFirstFile((strPathToSearch + L"*").c_str(), &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		std::auto_ptr<TStringList> subDirs;

		do
		{
			String strTheNameOfTheFile = file.cFileName;

			// It could be a directory we are looking at, if so look into that dir
			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if ((strTheNameOfTheFile != L".") && (strTheNameOfTheFile != L".."))
				{
					// Folders
					NumDirs++;

					pair.Clear();
					FolderList.push_back(pair);
					pPair = &(*(--FolderList.end()));

					pPair->oldPath = strPathToSearch + strTheNameOfTheFile;
					pPair->newFile = strPadNum(strTheNameOfTheFile, strPathToSearch, L"", iMax, pPair);
					pPair->newPath = strPathToSearch + pPair->newFile;

					if (bRecursive)
					{
						if (subDirs.get() == NULL)
							subDirs.reset(new TStringList);

						subDirs->Add(strPathToSearch + strTheNameOfTheFile);
					}
				}
			}
			else
			{
				// Files
				NumFiles++;

				pair.Clear();
				FileList.push_back(pair);
				pPair = &(*(--FileList.end()));

				pPair->oldPath = strPathToSearch + strTheNameOfTheFile;
				ext = ExtractFileExt(strTheNameOfTheFile);
				name = strTheNameOfTheFile.SubString(1, strTheNameOfTheFile.Length() - ext.Length());
				pPair->newFile = strPadNum(name, strPathToSearch, ext, iMax, pPair) + ext;
				pPair->newPath = strPathToSearch + pPair->newFile;
			}
		}
		while (FindNextFile(hFile, &file));

		FindClose(hFile);

		if (subDirs.get() != NULL)
		{
			for (int i = 0; i < subDirs->Count; ++i)
			{
				EnumFiles(subDirs->Strings[i], bRecursive, iMax);
			}
		}
	}

	return;
}

//---------------------------------------------------------------------------
String TFormFRenum::strPadNum(const String &str, const String &path, const String &ext,
	const int max, NamePair *item)
{
	String tmp;
	int shift = 0;
	std::wstring strIn(str.c_str());
	std::wstring strResult(strIn);
	static const boost::wregex regex(L"\\d+");
	boost::wsregex_token_iterator iter(strIn.begin(), strIn.end(), regex, 0);
	boost::wsregex_token_iterator end;
	bool warn = false, warnLimit = false;

	for(; iter != end; ++iter)
	{
		if (iter->length() > MAX_DIGITS_LIMIT)
		{
			tmp = (*iter).str().c_str();
			warnLimit = true;
			NumWarnings++;
			showMsg((String)L"➦ WARNING: ignoring huge number exceeding " + MAX_DIGITS_LIMIT + L" digits");
		} else
		{
			tmp.sprintf((String(L"%0") + max + L"I64d").c_str(), StrToLongLong((*iter).str().c_str()));
		}
		strResult.replace(iter->first - strIn.begin() + shift, iter->length(), tmp.c_str());
		shift += tmp.Length() - iter->length();

		if (!warnLimit && tmp.Length() > max)
		{
			warn = true;
			NumWarnings++;
			showMsg((String)L"➦ WARNING: number exceeding set " + max + L" digits");
		}
	}

	if (warn || warnLimit)
		showMsg(path + strResult.c_str() + ext, item);

	return strResult.c_str();
}

//---------------------------------------------------------------------------
void __fastcall TFormFRenum::FormCreate(TObject *Sender)
{
	ListView1->Columns[0][0]->Width = ColumnHeaderWidth;
	ListView1->Columns[0][0]->Caption =
		L"Enter/Double click - jump to file/folder in Windows Explorer;  Esc - exit";

	EnumFiles(Dir, Recursive, MaxDigits);

	// process files
	std::list<NamePair>::iterator it = FileList.begin();
	while(it != FileList.end())
	{
		if (!(*it).Same())
		{
			if (!RenameFile((*it).oldPath, (*it).newPath)) {
				NumErrors++;
				(*it).deedDone = false;
				showMsg(L"➦ ERROR: unable to rename file");
				showMsg((*it).oldPath + L" ➡ " + (*it).newFile, &(*it));
			} else {
				NumFilesRenamed++;
			}
		}

		it++;
	}

	// process folders
	if (AlsoFolders)
	{
		it = FolderList.end();
		while(it != FolderList.begin())
		{
			it--;

			if (!(*it).Same())
			{
				if (!RenameFile((*it).oldPath, (*it).newPath)) {
					NumErrors++;
					(*it).deedDone = false;
					showMsg(L"➦ ERROR: unable to rename folder");
					showMsg((*it).oldPath + L" <DIR> ➡ " + (*it).newFile + L" <DIR>", &(*it));
				} else {
					NumDirsRenamed++;
				}
			}
		}
	}

	String tmp;
	showMsg(L"Done");
	tmp.sprintf(L"Files: %d, Renamed: %d; Folders: %d, Renamed: %d; Warnings: %d, Errors: %d",
			NumFiles, NumFilesRenamed, NumDirs, NumDirsRenamed, NumWarnings, NumErrors);
	showMsg(tmp);

	ListView1->Columns[0][0]->Width = ColumnTextWidth;
}

//---------------------------------------------------------------------------
void __fastcall TFormFRenum::FormKeyPress(TObject *Sender, wchar_t &Key)
{
	if (Key == 27) // Esc - exit
	{
		Close();
	}
	else if (Key == 13) // Enter - jump to file in Explorer
	{
		JumpToFile();
	}
}

//---------------------------------------------------------------------------
void __fastcall TFormFRenum::ListView1DblClick(TObject *Sender)
{
	JumpToFile();
}

//---------------------------------------------------------------------------
void TFormFRenum::JumpToFile()
{
	NamePair *pp = (ListView1->Selected)? (NamePair*)ListView1->Selected->Data : NULL;
	if (pp) {
		String path = pp->deedDone? pp->newPath : pp->oldPath;
		BrowseToFile(ExpandFileName(path).c_str());
	}
}

//---------------------------------------------------------------------------
void __fastcall TFormFRenum::FormShow(TObject *Sender)
{
	SNDMSG(ListView1->Handle, WM_VSCROLL, SB_BOTTOM, 0);
}

//---------------------------------------------------------------------------

