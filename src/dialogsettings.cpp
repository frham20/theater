#include "theater.h"
#include "resource.h"

namespace Theater
{
	DialogSettings::DialogSettings()
	    : Dialog(IDD_SETTINGS)
	{
	}

	INT_PTR DialogSettings::OnMessage( UINT message, WPARAM wParam, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER( wParam );
		UNREFERENCED_PARAMETER( lParam );

		switch (message)
		{
		case WM_INITDIALOG: {


			return TRUE;
		}
		}

		return 0;
	}

}