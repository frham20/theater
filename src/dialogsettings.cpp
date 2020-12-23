#include "theater.h"
#include "resource.h"

namespace Theater
{
	DialogSettings::DialogSettings()
	    : Dialog(IDD_SETTINGS)
	{
	}

	BOOL DialogSettings::OnInitDialog()
	{
		return __super::OnInitDialog();
	}

	INT_PTR DialogSettings::OnMessage( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return __super::OnMessage(message, wParam, lParam);
	}

}