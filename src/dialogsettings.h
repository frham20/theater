#pragma once

namespace Theater
{
	class DialogSettings : public Dialog
	{
	public:
		DialogSettings();

	protected:
		INT_PTR OnMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

	private:
	};

}
