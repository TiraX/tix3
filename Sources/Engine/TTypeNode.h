/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_NODE_TYPE
	{
		ENT_Node = TI_MAKE_IDENTIFIER('N', 'O', 'D', 'E'),
		ENT_SceneRoot = TI_MAKE_IDENTIFIER('R', 'O', 'O', 'T'),
		ENT_StaticMesh = TI_MAKE_IDENTIFIER('S', 'T', 'A', 'M'),
		ENT_SkeletalMesh = TI_MAKE_IDENTIFIER('S', 'K', 'M', 'M'),
		ENT_Camera = TI_MAKE_IDENTIFIER('C', 'A', 'M', 'A'),
		ENT_CameraNav = TI_MAKE_IDENTIFIER('C', 'A', 'M', 'N'),
		ENT_Light = TI_MAKE_IDENTIFIER('L', 'I', 'T', 'P'),
		ENT_Environment = TI_MAKE_IDENTIFIER('E', 'N', 'V', 'I'),
		ENT_Level = TI_MAKE_IDENTIFIER('L', 'V', 'L', '.'),
		ENT_SceneTile = TI_MAKE_IDENTIFIER('S', 'T', 'I', 'L'),
		//ENT_GEOMETRY		= TI_MAKE_IDENTIFIER('G', 'E', 'O', 'M'),
		//ENT_MESHGROUP		= TI_MAKE_IDENTIFIER('M', 'G', 'R', 'P'),
		//ENT_SKINMESH		= TI_MAKE_IDENTIFIER('S', 'K', 'M', 'S'),
		//ENT_MORPHMESH		= TI_MAKE_IDENTIFIER('M', 'O', 'R', 'P'),
		//ENT_JOINT			= TI_MAKE_IDENTIFIER('J', 'I', 'N', 'T'),
		//ENT_POINTLIGHT		= TI_MAKE_IDENTIFIER('P', 'L', 'I', 'T'),
		//ENT_SPOTLIGHT		= TI_MAKE_IDENTIFIER('S', 'L', 'I', 'T'),
		//ENT_LIGHTTARGET		= TI_MAKE_IDENTIFIER('L', 'T', 'G', 'T'),
		//ENT_DIRECTIONALLIGHT= TI_MAKE_IDENTIFIER('D', 'L', 'I', 'T'),
		//ENT_REFLECT_CAPTURE	= TI_MAKE_IDENTIFIER('R', 'F', 'L', 'E'),
		//ENT_LIGHTSUN		= TI_MAKE_IDENTIFIER('L', 'S', 'U', 'N'),
		//ENT_LIGHTSHADOW		= TI_MAKE_IDENTIFIER('L', 'S', 'D', 'W'),
		//ENT_TERRAIN			= TI_MAKE_IDENTIFIER('T', 'E', 'R', 'R'),
		//ENT_WATER           = TI_MAKE_IDENTIFIER('W', 'T', 'E', 'R'),
		//ENT_TILEOBJ			= TI_MAKE_IDENTIFIER('T', 'L', 'O', 'B'),
		//ENT_TRANSHELPER		= TI_MAKE_IDENTIFIER('H', 'E', 'L', 'P'),
		//ENT_UVATLAS			= TI_MAKE_IDENTIFIER('U', 'V', 'A', 'T'),
		//ENT_OBJECTUV		= TI_MAKE_IDENTIFIER('O', 'B', 'U', 'V'),
		//ENT_EFFECT			= TI_MAKE_IDENTIFIER('E', 'F', 'C', 'T'),
		//ENT_EMITTER			= TI_MAKE_IDENTIFIER('E', 'M', 'I', 'T'),
		//ENT_GRASS			= TI_MAKE_IDENTIFIER('G', 'R', 'A', 'S'),
		//ENT_SKYBOX			= TI_MAKE_IDENTIFIER('S', 'K', 'Y', 'B'),
		//ENT_PHYSIC			= TI_MAKE_IDENTIFIER('P', 'H', 'Y', 'S'),
		//ENT_BBOARD			= TI_MAKE_IDENTIFIER('B', 'B', 'R', 'D'),
		//ENT_DUMMYBOX		= TI_MAKE_IDENTIFIER('D', 'B', 'O', 'X'),
		//ENT_ACTOR			= TI_MAKE_IDENTIFIER('A', 'C', 'T', 'R'),
		//ENT_TRIGGER			= TI_MAKE_IDENTIFIER('T', 'R', 'I', 'G'),
		//ENT_UIHOLDER		= TI_MAKE_IDENTIFIER('U', 'I', 'H', 'D'),
		//ENT_INSG			= TI_MAKE_IDENTIFIER('I', 'N', 'S', 'G'),
		//ENT_INSG_SKIN		= TI_MAKE_IDENTIFIER('I', 'G', 'S', 'K'),
	};

	enum E_NODE_FLAG
	{
		ENF_VISIBLE = 1 << 0,
		ENF_DIRTY_POS = 1 << 1,
		ENF_DIRTY_ROT = 1 << 2,
		ENF_DIRTY_SCALE = 1 << 3,

		ENF_DIRTY_TRANSFORM = (ENF_DIRTY_POS | ENF_DIRTY_ROT | ENF_DIRTY_SCALE),

		ENF_ABSOLUTETRANSFORMATION_UPDATED = 1 << 4,
		ENF_HIDDEN = 1 << 5,
	};

	enum E_UINODE_COMMON_FLAG
	{
		EUNCF_VISIBLE			= 1 << 0,
		EUNCF_DIRTY_POS			= 1 << 1,
		EUNCF_DIRTY_ROT			= 1 << 2,
		EUNCF_DIRTY_SCALE		= 1 << 3,
		EUNCF_DIRTY_TRANS_MAT	= 1 << 4,

		EUNCF_DIRTY_TRANSFORM	= (EUNCF_DIRTY_POS | EUNCF_DIRTY_ROT | EUNCF_DIRTY_SCALE | EUNCF_DIRTY_TRANS_MAT),

		EUNCF_ABSOLUTETRANSFORMATION_UPDATED	= 1 << 5,

		EUNCF_FLIP_H			= 1 << 6,
		EUNCF_FLIP_V			= 1 << 7,

		EUNCF_DIRTY_OFFSET		= 1 << 8,
		EUNCF_ENABLE_CLIP		= 1 << 9,

		EUNCF_ANIMATION_PAUSE	= 1 << 10,
		EUNCF_ANIMATION_FINISHED = 1 << 11,

		EUNCF_DEFAULT_ANIMATION	= 1 << 12,
		EUNCF_HAS_DEFAULT_ANIM	= 1 << 13,
		EUNCF_FADING_OUT		= 1 << 14,	// after symbol fading out animation finished, mark this symbol as in-visible
		
		EUNCF_WITH_ROTATION		= 1 << 15,
		EUNCF_ERASE_ROTATION	= 1 << 16,
	};

	enum E_UINODE_EXTRA_FLAG
	{
		// buttons & text (because easybutton use both button flags and text flags)
		EUNEF_BUTTON_PRESSED	= 1 << 0,
		EUNEF_BUTTON_DISABLED	= 1 << 1,
		EUNEF_CHECKABLE			= 1 << 2,
		EUNEF_CHECKED			= 1 << 3,
		EUNEF_EVENT_PRESS		= 1 << 4,
		EUNEF_EVENT_RELEASE		= 1 << 5,
		EUNEF_EVENT_CLICK		= 1 << 6,
		EUNEF_EVENT_DRAG		= 1 << 7,

		EUNEF_USE_INTERNAL_CHARACTER	= 1 << 8,
		EUNEF_TEXT_WITH_BORDER	= 1 << 9,
		EUNEF_MULTI_LINES		= 1 << 10,
		EUNEF_TEXT_FLUSH		= 1 << 11,
		EUNEF_TEXT_LIMITLENGTH	= 1 << 12,
		EUNEF_TEXT_WITH_GRADIENT= 1 << 13,
		EUNEF_MULTI_AUTO_HEIGHT	= 1 << 14,
		EUNEF_SYSTEM_FONT		= 1 << 15,

		// scroll area
		EUNEF_SCROLLAREA_LOCKED	= 1 << 0,
		EUNEF_SCROLL_ALIGN_ITEM	= 1 << 1,
		EUNEF_SCROLL_ALIGN_CENTER = 1 << 2,
		EUNEF_SCROLL_AUTO_ITEM_HEIGHT = 1 << 3,

		// icon box
		EUNEF_ICONBOX_GRAY		= 1 << 0,
	};


	enum E_UINODE_ALIGNMENT
	{
		EUI_ALIGN_LEFT,
		EUI_ALIGN_HCENTER,
		EUI_ALIGN_RIGHT,
		EUI_ALIGN_RLEFT,
		EUI_ALIGN_RHCENTER,
		EUI_ALIGN_RRIGHT,

		EUI_ALIGN_TOP,
		EUI_ALIGN_VCENTER,
		EUI_ALIGN_BOTTOM,
		EUI_ALIGN_RTOP,
		EUI_ALIGN_RVCENTER,
		EUI_ALIGN_RBOTTOM,

		EUI_ALIGN_COUNT,
	};

	enum E_PHYSIC_TYPE
	{
		EPHYT_INVALID		= -1,

		EPHYT_MESH			= 0,
		EPHYT_SPHERE,
		EPHYT_CYLINDER,

		EPHYT_COUNT,
	};

	enum E_UI_TYPE
	{
		EUIT_INVALID	= -1,

		EUIT_UIROOT,
		EUIT_SYMBOL,	//			= TI_MAKE_IDENTIFIER('S', 'Y', 'M', 'B'),
		EUIT_BUTTON,	//			= TI_MAKE_IDENTIFIER('B', 'T', 'T', 'N'),
		EUIT_DIALOG,	//			= TI_MAKE_IDENTIFIER('D', 'L', 'O', 'G'),
		EUIT_IMAGE,		//			= TI_MAKE_IDENTIFIER('U', 'I', 'M', 'G'),
		EUIT_CHARACTER,
		EUIT_TEXTBOX,
		EUIT_ICONBOX,
		EUIT_SCROLLAREA,
		EUIT_FAN,
		EUIT_EVENTMASK,
		EUIT_DRAWBOX,
		EUIT_DIALOG_9,
		EUIT_EASYBUTTON,
		EUIT_SCROLLPAGE,

		EUIT_COUNT,
	};
}