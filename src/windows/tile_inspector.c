/*****************************************************************************
* Copyright (c) 2014 Ted John
* OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
*
* This file is part of OpenRCT2.
*
* OpenRCT2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "../addresses.h"
#include "../localisation/localisation.h"
#include "../interface/widget.h"
#include "../interface/window.h"
#include "../interface/viewport.h"
#include "../world/scenery.h"
#include "../world/map.h"
#include "../world/footpath.h"

enum WINDOW_TILE_INSPECTOR_WIDGET_IDX {
	WIDX_BACKGROUND,
	WIDX_TITLE,
	WIDX_CLOSE,
	WIDX_CONTENT_PANEL,
	WIDX_SCROLL
};

#define WW 500
#define WH 400
#define MIN_WH 150
#define MAX_WH 800

rct_widget window_tile_inspector_widgets[] = {
	{ WWT_FRAME,			0,	0,			WW - 1,	0,		WH - 1,		0x0FFFFFFFF,						STR_NONE },				// panel / background
	{ WWT_CAPTION,			0,	1,			WW - 2,	1,		14,			STR_TILE_INSPECTOR_TITLE,			STR_WINDOW_TITLE_TIP },	// title bar
	{ WWT_CLOSEBOX,			0,	WW - 13,	WW - 3,	2,		13,			STR_CLOSE_X,						STR_CLOSE_WINDOW_TIP },	// close x button
	{ WWT_RESIZE,			1,	0,			WW - 1,	43,		WH - 1,		0x0FFFFFFFF,						STR_NONE },				// content panel
	{ WWT_SCROLL,			1,	3,			WW - 3,	65,		WH - 30,	3,									STR_NONE },				// scroll area
	{ WIDGETS_END },
};

static int window_tile_inspector_tile_x;
static int window_tile_inspector_tile_y;
static int window_tile_inspector_item_count;

static void window_tile_inspector_emptysub() { }
static void window_tile_inspector_close();
static void window_tile_inspector_tool_update();
static void window_tile_inspector_tool_down();
static void window_tile_inspector_tool_abort();
static void window_tile_inspector_scrollgetsize();
static void window_tile_inspector_scrollmouseover();
static void window_tile_inspector_mouseup();
static void window_tile_inspector_resize();
static void window_tile_inspector_invalidate();
static void window_tile_inspector_paint();
static void window_tile_inspector_scrollpaint();

static void* window_tile_inspector_events[] = {
	window_tile_inspector_close,
	window_tile_inspector_mouseup,
	window_tile_inspector_resize,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_tool_update,
	window_tile_inspector_tool_down,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_tool_abort,
	window_tile_inspector_emptysub,
	window_tile_inspector_scrollgetsize,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_scrollmouseover,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_emptysub,
	window_tile_inspector_invalidate,
	window_tile_inspector_paint,
	window_tile_inspector_scrollpaint
};

void window_tile_inspector_open()
{
	rct_window* window;

	// Check if window is already open
	window = window_bring_to_front_by_class(WC_TILE_INSPECTOR);
	if (window != NULL)
		return;

	window = window_create(
		0,
		29,
		WW,
		WH,
		(uint32*)window_tile_inspector_events,
		WC_TILE_INSPECTOR,
		0x100
	);
	window->widgets = window_tile_inspector_widgets;
	window->enabled_widgets = (1 << WIDX_CLOSE);

	window_init_scroll_widgets(window);
	window->colours[0] = 7;
	window->colours[1] = 7;
	window->colours[2] = 7;
	window->flags |= WF_RESIZABLE;
	window->min_width = WW;
	window->min_height = MIN_WH;
	window->max_width = WW;
	window->max_height = MAX_WH;

	window_tile_inspector_tile_x = -1;
	window_tile_inspector_tile_y = -1;

	tool_set(window, WIDX_BACKGROUND, 62);
}

static void window_tile_inspector_close()
{
	tool_cancel();
}

static void window_tile_inspector_mouseup()
{
	short widgetIndex;
	rct_window *w;

	window_widget_get_registers(w, widgetIndex);

	switch (widgetIndex) {
	case WIDX_CLOSE:
		window_close(w);
		break;
	}
}

static void window_tile_inspector_resize()
{
	rct_window *w;

	window_get_register(w);

	w->min_width = WW;
	w->min_height = MIN_WH;
	if (w->width < w->min_width) {
		window_invalidate(w);
		w->width = w->min_width;
	}
	if (w->height < w->min_height) {
		window_invalidate(w);
		w->height = w->min_height;
	}
}

static void window_tile_inspector_tool_update()
{
	short widgetIndex;
	rct_window *w;
	short x, y;
	int direction;

	window_tool_get_registers(w, widgetIndex, x, y);
	map_invalidate_selection_rect();
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) &= ~(1 << 0);
	screen_pos_to_map_pos(&x, &y, &direction);

	if (x == (short)0x8000) {
		return;
	}

	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) |= (1 << 0);
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_A_X, sint16) = x;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_A_Y, sint16) = y;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_B_X, sint16) = x;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_B_Y, sint16) = y;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_TYPE, uint16) = 4;

	map_invalidate_selection_rect();

}

static void window_tile_inspector_tool_down()
{
	short widgetIndex;
	rct_window* w;
	short x, y;
	int direction;

	window_tool_get_registers(w, widgetIndex, x, y);
	screen_pos_to_map_pos(&x, &y, &direction);

	if (x == (short)0x8000) {
		return;
	}

	window_tile_inspector_tile_x = x >> 5;
	window_tile_inspector_tile_y = y >> 5;

	rct_map_element *element = map_get_first_element_at(window_tile_inspector_tile_x, window_tile_inspector_tile_y);
	int numItems = 0;
	do {
		numItems++;
	} while (!map_element_is_last_for_tile(element++));

	window_tile_inspector_item_count = numItems;

	w->scrolls[0].v_top = 0;
	window_invalidate(w);

}

static void window_tile_inspector_tool_abort()
{
	rct_window *w;
	short widgetIndex, x, y;
	window_tool_get_registers(w, widgetIndex, x, y);
	window_close(w);
}

static void window_tile_inspector_scrollgetsize()
{

	rct_window *w;
	int width, height;
	window_get_register(w);

	height = window_tile_inspector_item_count * 11;
	width = WW - 30;

	window_scrollsize_set_registers(width, height);

}

static void window_tile_inspector_scrollmouseover()
{
	short x, y, scrollIndex;
	rct_window *w;

	window_scrollmouse_get_registers(w, scrollIndex, x, y);
	window_invalidate(w);
}

static void window_tile_inspector_invalidate()
{
	rct_window *w;

	window_get_register(w);

	window_tile_inspector_widgets[WIDX_BACKGROUND].right = w->width - 1;
	window_tile_inspector_widgets[WIDX_BACKGROUND].bottom = w->height - 1;
	window_tile_inspector_widgets[WIDX_CLOSE].left = w->width - 13;
	window_tile_inspector_widgets[WIDX_CLOSE].right = w->width - 3;
	window_tile_inspector_widgets[WIDX_CONTENT_PANEL].right = w->width - 1;
	window_tile_inspector_widgets[WIDX_CONTENT_PANEL].bottom = w->height - 1;
	window_tile_inspector_widgets[WIDX_SCROLL].bottom = w->height - 30;
}

static void window_tile_inspector_paint()
{
	int x, y;
	rct_window *w;
	rct_drawpixelinfo *dpi;
	char buffer[256];

	window_paint_get_registers(w, dpi);

	window_draw_widgets(w, dpi);

	x = w->x + 20;
	y = w->y + 25;

	if (window_tile_inspector_tile_x == -1) {

		// No tile selected
		gfx_draw_string_left(dpi, STR_TILE_INSPECTOR_CHOOSE_MSG, NULL, 12, x, y);

	} else {

		sprintf(
			buffer,
			"X: %d, Y: %d",
			window_tile_inspector_tile_x,
			window_tile_inspector_tile_y
		);

		gfx_draw_string(dpi, buffer, 12, x, y);

	}

	y += 25;

	draw_string_left_underline(dpi, STR_TILE_INSPECTOR_ELEMENT_TYPE, NULL, 12, x, y);
	draw_string_left_underline(dpi, STR_TILE_INSPECTOR_BASE_HEIGHT, NULL, 12, x + 200, y);
	draw_string_left_underline(dpi, STR_TILE_INSPECTOR_CLEARANGE_HEIGHT, NULL, 12, x + 280, y);
	draw_string_left_underline(dpi, STR_TILE_INSPECTOR_FLAGS, NULL, 12, x + 390, y);
	
}

static void window_tile_inspector_scrollpaint()
{
	int x = 15, y = 11 * (window_tile_inspector_item_count - 1), i = 0;
	rct_window *w;
	rct_drawpixelinfo *dpi;
	char buffer[256];

	window_paint_get_registers(w, dpi);

	if (window_tile_inspector_tile_x == -1)
		return;

	rct_map_element *element = map_get_first_element_at(window_tile_inspector_tile_x, window_tile_inspector_tile_y);

	do {

		int type = map_element_get_type(element);
		char *type_name;
		int base_height = element->base_height;
		int clearance_height = element->clearance_height;

		if ((i & 1) != 0)
			gfx_fill_rect(dpi, x - 15, y, x + WW - 20, y + 11, RCT2_GLOBAL(0x0141FC4A + (w->colours[1] * 8), uint8) | 0x1000000);

		switch (type) {
			case MAP_ELEMENT_TYPE_SURFACE:
				sprintf(
					buffer,
					"Surface (%s, %s)",
					language_get_string(STR_TILE_INSPECTOR_TERRAIN_START + map_element_get_terrain(element)),
					language_get_string(STR_TILE_INSPECTOR_TERRAIN_EDGE_START + map_element_get_terrain_edge(element))
				);
				type_name = buffer;
				break;
			case MAP_ELEMENT_TYPE_PATH:
			{
				// TODO: use these
				uint8 pathType, pathDirection;
				pathType = element->properties.path.type >> 2;
				pathDirection = element->properties.path.type & 3;
			}
			sprintf(
				buffer,
				"Path (%s)",
				"" // TODO: queue? has bins? has benches? e.t.c.
			);
			type_name = buffer;
			break;
			case MAP_ELEMENT_TYPE_TRACK:
				type_name = "Track"; // TODO: show type?
				break;
			case MAP_ELEMENT_TYPE_SCENERY:
				sprintf(
					buffer,
					"Scenery (%s)",
					language_get_string(g_smallSceneryEntries[element->properties.scenery.type]->name)
				);
				type_name = buffer;
				break;
			case MAP_ELEMENT_TYPE_ENTRANCE:
				sprintf(
					buffer,
					"Entrance (%s)",
					language_get_string(STR_TILE_INSPECTOR_ENTRANCE_START + element->properties.entrance.type)
					);
				type_name = buffer;
				break;
			case MAP_ELEMENT_TYPE_FENCE:
				sprintf(
					buffer,
					"Fence (%s)",
					language_get_string(g_wallSceneryEntries[element->properties.scenery.type]->name)
					);
				type_name = buffer;
				break;
			case MAP_ELEMENT_TYPE_SCENERY_MULTIPLE:
				type_name = "Scenery multiple";
				break;
			case MAP_ELEMENT_TYPE_BANNER:
				sprintf(
					buffer,
					"Banner (%d)",
					element->properties.banner.index
				);
				type_name = buffer;
				break;
		}

		gfx_draw_string(dpi, type_name, 12, x, y);
		gfx_draw_string_left(dpi, 5182, &base_height, 12, x + 200, y);
		gfx_draw_string_left(dpi, 5182, &clearance_height, 12, x + 280, y);

		uint8 flags = element->flags;
		char j;

		buffer[8] = '\0';

		for (j = 7; j >= 0; j--, flags >>= 1) {
			buffer[j] = flags & 1 ? '1' : '0';
		}

		gfx_draw_string(dpi, buffer, 12, x + 390, y);

		y -= 11;
		i++;

	} while (!map_element_is_last_for_tile(element++));

}
