// ======================================
// written by BUzer for HL: Paranoia modification
// ======================================

#ifndef _VGUIMSG_H
#define _VGUIMSG_H
using namespace vgui;

#include "vgui_shadowtext.h"
#include "getfont.h"


class CScreenMessage : public ShadowTextPanel
{
public:
	CScreenMessage();

	void Initialize();

	void SetMessage(client_textmessage_t *msg);

protected:
	virtual void paint()
	{
		int mr, mg, mb, ma;
		getFgColor(mr, mg, mb, ma);

		float curtime = gEngfuncs.GetClientTime() - m_starttime;
		if (curtime < 0)
			return;
		
		if (curtime > (m_hold + m_fadein + m_fadeout))
		{
			setVisible(false);
			return;
		}

		if (curtime < m_fadein)
		{
			float alpha = curtime / m_fadein;
			setFgColor( mr, mg, mb, (1 - alpha) * 255 );
		}
		else if (curtime > (m_fadein + m_hold))
		{
			float alpha = (curtime - m_fadein - m_hold) / m_fadeout;
			setFgColor( mr, mg, mb, (alpha) * 255 );
		}
		else
		{
			setFgColor( mr, mg, mb, 0 );
		}

		ShadowTextPanel::paint();
	}

	float m_starttime;
	float m_hold;
	float m_fadein;
	float m_fadeout;
};

#endif // _VGUIMSG_H
