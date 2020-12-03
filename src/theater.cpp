#include "theater.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	if (!App_Init())
	{
		App_Close();
		return -1;
	}

	const auto result = App_Run();
	App_Close();
	return result;
}