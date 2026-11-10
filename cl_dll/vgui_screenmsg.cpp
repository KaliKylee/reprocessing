// ====================================
// written by BUzer for HL: Paranoia modification
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_subtitles.h"
#include "VGUI_TextImage.h"
#include "vgui_screenmsg.h"

CScreenMessage::CScreenMessage() : ShadowTextPanel("", 0, 0, ScreenWidth, ScreenHeight)
{
	setVisible(false);
	setPaintBackgroundEnabled(false);
}

void CScreenMessage::Initialize()
{
	setVisible(false);
}

void CScreenMessage::SetMessage(client_textmessage_t *msg)
{
	const char *text = msg->pMessage;
	Font *pFont = FontFromMessage(text);
	setFont(pFont);
	setText(text);
	setFgColor(msg->r1, msg->g1, msg->b1, msg->a1);

	m_starttime = gEngfuncs.GetClientTime();
	m_hold = msg->holdtime;
	m_fadein = msg->fadein;
	m_fadeout = msg->fadeout;

	// Wargon: Если координаты заданы неправильно, то текст просто центрируется.
	if (msg->x < 0 || msg->x > 1 || msg->y < 0 || msg->y > 1)
	{
		// gEngfuncs.Con_Printf("Error: invalid message coordinates!\n");
		// return;
		int tw, th;
		getTextImage()->getTextSizeWrapped(tw, th);
		setSize(tw, th);
		setPos((ScreenWidth - tw) * 0.5, (ScreenHeight - th) * 0.5);
	}
	else
	{
		int x = msg->x * ScreenWidth;
		int y = msg->y * ScreenHeight;
		setPos(x, y);
	}

	setVisible(true);
}

void VGuiAddScreenMessage( client_textmessage_t *msg )
{
	if (gViewPort && gViewPort->m_pScreenMsg)
	{
		gViewPort->m_pScreenMsg->SetMessage( msg );
	}
	else
		gEngfuncs.Con_Printf("Screenmessage error: ViewPort is not constructed!\n");
}