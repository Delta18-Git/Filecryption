#ifndef TUI_H
#define TUI_H

#include "file_tree.h"
#include <ncurses.h>

void tui_init();
void tui_cleanup();
void tui_draw_layout();
void tui_resize_handler();

void tui_draw_header(const char *title);
void tui_draw_footer(const char *footer);
void tui_draw_menu();
void tui_highlight_menu_item(int idx);
void tui_draw_file_browser(FileNode *root, int sel_idx, int scr_offset);
void tui_display_message(const char *message, int message_type);

int tui_get_menu_selection();
char *tui_get_password(const char *prompt);
FileNode *tui_get_file_browser_selection(FileNode *root);

int tui_get_confirmation(const char *prompt);
const char *get_common_file_type(FileNode *node);

#define TUI_MSG_INFO 1
#define TUI_MSG_SUCCESS 2
#define TUI_MSG_ERROR 3
#define TUI_MSG_WARNING 4

#define MENU_ENCRYPT 1
#define MENU_DECRYPT 2
#define MENU_EXIT 3

#define TUI_CONFIRM_YES 1
#define TUI_CONFIRM_NO 0

#endif
