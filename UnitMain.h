//---------------------------------------------------------------------------

#ifndef UnitMainH
#define UnitMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

//---------------------------------------------------------------------------
typedef struct NamePair
{
	String oldPath;
	String newPath;
	String newFile;
	bool deedDone;

	NamePair(): deedDone(true) {	}
	void Clear() {
		oldPath = L"";
		newPath = L"";
		newFile = L"";
		deedDone = true;
	}
	bool Same() {
		return (oldPath.CompareIC(newPath) == 0);
	}
} NamePair;

//---------------------------------------------------------------------------
class TFormFRenum : public TForm
{
__published:	// IDE-managed Components
	TListView *ListView1;
	void __fastcall FormKeyPress(TObject *Sender, wchar_t &Key);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ListView1DblClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
private:	// User declarations
	void EnumFiles(const String& strFilePath, const bool& bRecursive, const int iMax);
	void showMsg(UnicodeString msg, void *data = NULL);
	String strPadNum(const String &str, const String &path, const String &ext,
		const int max, NamePair *item);
  void JumpToFile();
public:		// User declarations
	__fastcall TFormFRenum(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormFRenum *FormFRenum;
//---------------------------------------------------------------------------
#endif
