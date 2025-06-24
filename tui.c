
#include "tui.h"
#include "file_tree.h"
#include <magic.h>
#include <ncurses.h>
#include <string.h>

static WINDOW *header_win, *footer_win, *menu_win, *browser_win, *message_win,
    *input_win;
static int term_rows, term_cols;

void tui_init() {
  initscr();
  start_color();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  getmaxyx(stdscr, term_rows, term_cols);

  init_pair(1, COLOR_CYAN, COLOR_BLACK);   // used for header/footer
  init_pair(2, COLOR_WHITE, COLOR_BLUE);   // used for highlight
  init_pair(3, COLOR_YELLOW, COLOR_BLACK); // used for dirs and info msg
  init_pair(4, COLOR_GREEN, COLOR_BLACK);  // used for success
  init_pair(5, COLOR_RED, COLOR_BLACK);    // used for error

  tui_draw_layout(); // draw initial ui layout
}

void tui_cleanup() {
  if (header_win) {
    delwin(header_win);
  }
  if (footer_win) {
    delwin(footer_win);
  }
  if (menu_win) {
    delwin(menu_win);
  }
  if (browser_win) {
    delwin(browser_win);
  }
  if (message_win) {
    delwin(message_win);
  }
  if (input_win) {
    delwin(input_win);
  }

  endwin();
}
void tui_draw_layout() {
  refresh();
  if (header_win) {
    delwin(header_win);
    header_win = NULL;
  }
  if (footer_win) {
    delwin(footer_win);
    footer_win = NULL;
  }
  if (menu_win) {
    delwin(menu_win);
    menu_win = NULL;
  }
  if (browser_win) {
    delwin(browser_win);
    browser_win = NULL;
  }
  if (message_win) {
    delwin(message_win);
    message_win = NULL;
  }
  if (input_win) {
    delwin(input_win);
    input_win = NULL;
  }

  clear();

  // create header window (top 3 rows)
  header_win = newwin(3, term_cols, 0, 0);
  wbkgd(header_win, COLOR_PAIR(1));

  // create footer window (bottom 3 rows)
  footer_win = newwin(2, term_cols, term_rows - 2, 0);
  wbkgd(footer_win, COLOR_PAIR(1));

  // create menu window (left panel, quarter screen width)
  menu_win = newwin(term_rows - 5, term_cols / 4, 3, 0);
  box(menu_win, 0, 0);
  mvwprintw(menu_win, 0, 2, " MENU ");

  // create file browser window (right panel, three-quarters width)
  browser_win = newwin(term_rows - 5, (3 * term_cols) / 4, 3, term_cols / 4);
  box(browser_win, 0, 0);
  mvwprintw(browser_win, 0, 2, " FILES ");

  refresh();

  tui_draw_menu();
  tui_draw_header("FileCryption");
  tui_draw_footer("Arrow Keys: Navigate | Enter: Select Option | Esc: Exit");

  wrefresh(browser_win);
}

// handles resize events and redraws
void tui_resize_handler() {
  getmaxyx(stdscr, term_rows, term_cols);
  tui_draw_layout();
}

void tui_draw_header(const char *title) {
  wclear(header_win);
  mvwprintw(header_win, 1, (term_cols - strlen(title)) / 2, "%s", title);
  wrefresh(header_win);
}

void tui_draw_footer(const char *footer) {
  wclear(footer_win);
  mvwprintw(footer_win, 0, 2, "%s", footer);
  wrefresh(footer_win);
}

void tui_draw_menu() {
  wclear(menu_win);
  box(menu_win, 0, 0);
  mvwprintw(menu_win, 2, 2, "1. Encrypt File");
  mvwprintw(menu_win, 3, 2, "2. Decrypt File");
  mvwprintw(menu_win, 4, 2, "3. Exit");
  wrefresh(menu_win);
}

void tui_highlight_menu_item(int idx) {
  tui_draw_menu();
  wattron(menu_win, COLOR_PAIR(2));
  mvwprintw(menu_win, idx + 1, 2, "%d. %s", idx,
            idx == MENU_ENCRYPT   ? "Encrypt File"
            : idx == MENU_DECRYPT ? "Decrypt File"
                                  : "Exit");
  wattroff(menu_win, COLOR_PAIR(2));
  wrefresh(menu_win);
}

void tui_draw_file_tree_recursive(FileNode *node, int selected_idx,
                                  int scroll_offset, int *current_idx, int *y,
                                  int max_y, WINDOW *win) {
  if (!node || *y >= max_y)
    return;

  if (*current_idx >= scroll_offset) {
    int depth = 0;
    for (FileNode *parent = node->parent; parent; parent = parent->parent)
      depth++;

    char display[MAX_NAME_LENGTH + 4];
    snprintf(display, sizeof(display), "%s %s", node->is_dir ? "->" : "  ",
             node->name);
    display[sizeof(display) - 1] = '\0';

    int display_attributes = 0;
    if (node->is_dir) {
      display_attributes = COLOR_PAIR(3);
    }
    if (*current_idx == selected_idx) {
      display_attributes |= A_REVERSE;
    }

    if (display_attributes != 0) {
      wattron(win, display_attributes);
    }

    mvwprintw(win, *y, depth * 2 + 1, "%s", display);
    wattroff(win, A_REVERSE | COLOR_PAIR(3));

    (*y)++;
  }

  (*current_idx)++;

  for (int i = 0; i < node->num_children; i++) {
    tui_draw_file_tree_recursive(node->children[i], selected_idx, scroll_offset,
                                 current_idx, y, max_y, win);
    if (*y > max_y) {
      break;
    }
  }
}

void tui_draw_file_browser(FileNode *root, int sel_idx, int scr_offset) {
  werase(browser_win); // clear previous browser
  box(browser_win, 0, 0);
  mvwprintw(browser_win, 0, 2, " FILES ");

  if (!root) {
    mvwprintw(browser_win, 1, 2, "No files or directory loaded");
    wrefresh(browser_win);
    return;
  }

  int max_y = getmaxy(browser_win);

  int current_idx = 0;
  int y_pos = 1;
  tui_draw_file_tree_recursive(root, sel_idx, scr_offset, &current_idx, &y_pos,
                               max_y - 1, browser_win);

  wrefresh(browser_win);
}

void tui_display_message(const char *message, int message_type) {
  int width = (int)strlen(message) + 6;
  int height = 5;

  if (width < 30) {
    width = 30;
  }
  if (width > term_cols - 4) {
    width = term_cols - 4;
  }

  int startx = (term_cols - width) / 2;
  int starty = (term_rows - height) / 2;

  if (message_win) {
    delwin(message_win);
  }
  message_win = newwin(height, width, starty, startx);
  int colorpair = 0;
  switch (message_type) {
  case TUI_MSG_SUCCESS:
    colorpair = 4;
    break;
  case TUI_MSG_WARNING:
  case TUI_MSG_ERROR:
    colorpair = 5;
    break;
  case TUI_MSG_INFO:
    colorpair = 3;
    break;
  }
  box(message_win, 0, 0);
  if (colorpair > 0) {
    wattron(message_win, COLOR_PAIR(colorpair));
  }
  mvwprintw(message_win, 2, 3, "%s", message);
  if (colorpair > 0) {
    wattroff(message_win, COLOR_PAIR(colorpair));
  }
  mvwprintw(message_win, height - 2, (width - 25) / 2,
            "Press any key to continue");

  wrefresh(message_win);
  getch(); // wait for key-press

  delwin(message_win);
  message_win = NULL;

  touchwin(stdscr); // mark all windows to be refreshed
  wrefresh(stdscr);
}

int tui_get_menu_selection() {
  int current_selection = MENU_ENCRYPT; // default to first item

  tui_highlight_menu_item(current_selection);

  while (1) {
    int ch = getch();
    switch (ch) {
    case KEY_RESIZE:
      tui_resize_handler();
      tui_highlight_menu_item(current_selection);
      break;
    case '1':
    case '2':
    case '3':
      current_selection = ch - '0';
      break;
    case KEY_UP:
      if (current_selection > MENU_ENCRYPT) {
        current_selection--;
      } else {
        current_selection = MENU_EXIT;
      }
      break;
    case KEY_DOWN:
      if (current_selection < MENU_EXIT) {
        current_selection++;
      } else {
        current_selection = MENU_ENCRYPT;
      }
      break;
    case 10:
    case KEY_ENTER:
      return current_selection;
    case 27: // esc key
      return MENU_EXIT;
    }
    tui_highlight_menu_item(current_selection);
  }
}

char *tui_get_password(const char *prompt) {
  static char password[128];
  memset(password, 0, sizeof(password));

  int width = term_cols / 2;
  if (width < (int)strlen(prompt) + 5) {
    width = (int)strlen(prompt) + 10; // ensure prompt fits
  }
  if (width > term_cols - 4) {
    width = term_cols - 4; // cap box width
  }
  int height = 5;
  int startx = (term_cols - width) / 2;
  int starty = (term_rows - height) / 2;

  if (input_win) {
    delwin(input_win);
  }

  input_win = newwin(height, width, starty, startx);
  box(input_win, 0, 0);

  mvwprintw(input_win, 0, 2, " Password Entry ");
  mvwprintw(input_win, 2, 2, "%s", prompt);
  wrefresh(input_win);

  WINDOW *field_win = derwin(input_win, 1, width - 6, 3, 3);
  wbkgd(field_win, COLOR_PAIR(0));
  wrefresh(field_win);

  curs_set(1);
  wmove(field_win, 0, 0);
  // echo();
  wgetnstr(field_win, password, sizeof(password) - 1);
  // noecho();
  curs_set(0);

  delwin(field_win);
  delwin(input_win);
  input_win = NULL;

  touchwin(stdscr);
  wrefresh(stdscr);

  return password;
}

const char *get_common_file_type(FileNode *node) {
  if (!node) {
    return "Unknown";
  }
  if (node->is_dir) {
    return "Directory";
  }
  if (strcmp(strrchr(node->name, '.'), ".enc") == 0) {
    return "Encrypted File";
  }
  if (strcmp(strrchr(node->name, '.'), ".dec") == 0) {
    return "Decrypted File";
  }
#if defined(__unix__) || defined(__APPLE__)
  static char type_buffer[256];
  const char *default_type_on_error = "File";

  magic_t magic_cookie = magic_open(MAGIC_SYMLINK);
  if (magic_cookie == NULL) {
    // failed to initialize libmagic
    return default_type_on_error;
  }

  if (magic_load(magic_cookie, NULL) != 0) {
    // failed to load magic database
    magic_close(magic_cookie);
    return default_type_on_error;
  }

  const char *full_type = magic_file(magic_cookie, node->path);

  if (full_type != NULL) {
    strncpy(type_buffer, full_type, sizeof(type_buffer) - 1);
    type_buffer[sizeof(type_buffer) - 1] = '\0'; // ensure null-termination

    char *comma = strchr(type_buffer, ',');
    if (comma != NULL) {
      *comma = '\0'; // truncate the string at the comma
    }
    magic_close(magic_cookie);
    return type_buffer;
  } else {
    magic_close(magic_cookie);
    return default_type_on_error;
  }
#endif
  const char *dot = strrchr(node->name, '.');

  if (!dot || dot == node->name) {
    return "File";
  }

  if (strcmp(dot, ".txt") == 0) {
    return "Text File";
  }
  if (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0) {
    return "C Source";
  }
  if (strcmp(dot, ".zip") == 0 || strcmp(dot, ".tar") == 0 ||
      strcmp(dot, ".gz") == 0 || strcmp(dot, ".bz2") == 0 ||
      strcmp(dot, ".7z") == 0) {
    return "Archive";
  }
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0 ||
      strcmp(dot, ".jpe") == 0 || strcmp(dot, ".jfif") == 0) {
    return "JPEG Image";
  }
  if (strcmp(dot, ".png") == 0) {
    return "PNG Image";
  }
  if (strcmp(dot, ".gif") == 0) {
    return "GIF Image";
  }
  if (strcmp(dot, ".bmp") == 0) {
    return "BMP Image";
  }
  if (strcmp(dot, ".pdf") == 0) {
    return "PDF Document";
  }
  if (strcmp(dot, ".mp3") == 0) {
    return "MP3 Audio";
  }
  if (strcmp(dot, ".wav") == 0) {
    return "WAV Audio";
  }
  if (strcmp(dot, ".mp4") == 0) {
    return "MP4 Video";
  }
  if (strcmp(dot, ".avi") == 0) {
    return "AVI Video";
  }
  if (strcmp(dot, ".doc") == 0 || strcmp(dot, ".docx") == 0) {
    return "Word Document";
  }
  if (strcmp(dot, ".xls") == 0 || strcmp(dot, ".xlsx") == 0) {
    return "Excel Spreadsheet";
  }
  if (strcmp(dot, ".ppt") == 0 || strcmp(dot, ".pptx") == 0) {
    return "PowerPoint Presentation";
  }
  if (strcmp(dot, ".odt") == 0) {
    return "OpenDocument Text";
  }
  if (strcmp(dot, ".ods") == 0) {
    return "OpenDocument Spreadsheet";
  }
  if (strcmp(dot, ".odp") == 0) {
    return "OpenDocument Presentation";
  }
  if (strcmp(dot, ".rtf") == 0) {
    return "Rich Text Format";
  }
  if (strcmp(dot, ".csv") == 0) {
    return "CSV Data";
  }
  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
    return "HTML Document";
  }
  if (strcmp(dot, ".xml") == 0) {
    return "XML Data";
  }
  if (strcmp(dot, ".json") == 0) {
    return "JSON Data";
  }

  return "File";
}

int tui_get_confirmation(const char *prompt) {
  int width = (int)strlen(prompt) + 10;
  int height = 7;

  if (width < 40) {
    width = 40;
  }
  if (width > term_cols - 4) {
    width = term_cols - 4;
  }

  int startx = (term_cols - width) / 2;
  int starty = (term_rows - height) / 2;

  if (input_win) {
    delwin(input_win);
  }

  input_win = newwin(height, width, starty, startx);
  box(input_win, 0, 0);

  mvwprintw(input_win, 0, 2, " Confirmation ");
  mvwprintw(input_win, 2, 2, "%s", prompt);

  // Draw options
  mvwprintw(input_win, 4, (width / 2) - 6, "Yes");
  mvwprintw(input_win, 4, (width / 2) + 2, "No");

  wrefresh(input_win);

  curs_set(0);

  int highlight = TUI_CONFIRM_YES;

  wattron(input_win, A_REVERSE);
  mvwprintw(input_win, 4, (width / 2) - 6, "Yes");
  wattroff(input_win, A_REVERSE);
  wrefresh(input_win);

  int ch;
  while ((ch = getch()) != 27) { // 27 is ESC
    switch (ch) {
    case KEY_LEFT:
    case KEY_RIGHT:
      highlight = 1 - highlight;
      mvwprintw(input_win, 4, (width / 2) - 6, "Yes");
      mvwprintw(input_win, 4, (width / 2) + 2, "No");

      if (highlight == TUI_CONFIRM_YES) {
        wattron(input_win, A_REVERSE);
        mvwprintw(input_win, 4, (width / 2) - 6, "Yes");
        wattroff(input_win, A_REVERSE);
      } else {
        wattron(input_win, A_REVERSE);
        mvwprintw(input_win, 4, (width / 2) + 2, "No");
        wattroff(input_win, A_REVERSE);
      }
      wrefresh(input_win);
      break;
    case 10: // Enter key
    case KEY_ENTER:
      delwin(input_win);
      input_win = NULL;
      touchwin(stdscr);
      wrefresh(stdscr);
      return highlight;
    case 'y':
    case 'Y':
      delwin(input_win);
      input_win = NULL;
      touchwin(stdscr);
      wrefresh(stdscr);
      return TUI_CONFIRM_YES;
    case 'n':
    case 'N':
      delwin(input_win);
      input_win = NULL;
      touchwin(stdscr);
      wrefresh(stdscr);
      return TUI_CONFIRM_NO;
    case KEY_RESIZE:
      tui_resize_handler();
      getmaxyx(stdscr, term_rows, term_cols);
      return tui_get_confirmation(prompt);
    }
  }

  delwin(input_win);
  input_win = NULL;
  touchwin(stdscr);
  wrefresh(stdscr);
  return TUI_CONFIRM_NO;
}
FileNode *tui_get_file_browser_selection(FileNode *root) {
  tui_draw_footer("Arrow Keys: Navigate | Enter: Select File | Esc: Exit Menu");
  if (!root) {
    tui_display_message("File tree is not loaded or is empty.", TUI_MSG_ERROR);
    tui_draw_layout();
    return NULL;
  }

  int total_nodes = file_tree_count_nodes(root);
  int selected_idx = 0;
  int scroll_offset = 0;

  int max_y = getmaxy(browser_win);

  const int visible_rows = max_y - 2;
  while (1) {
    if (selected_idx < scroll_offset) {
      scroll_offset = selected_idx;
    } else if (selected_idx >= scroll_offset + visible_rows) {
      scroll_offset = selected_idx - visible_rows + 1;
    }

    tui_draw_file_browser(root, selected_idx, scroll_offset);

    switch (getch()) {
    case KEY_UP:
      selected_idx = (selected_idx > 0) ? selected_idx - 1 : 0;
      break;
    case KEY_DOWN:
      selected_idx =
          (selected_idx < total_nodes - 1) ? selected_idx + 1 : total_nodes - 1;
      break;
    case KEY_PPAGE:
      selected_idx = (selected_idx - visible_rows > 0)
                         ? selected_idx - visible_rows - 1
                         : 0;
      break;
    case KEY_NPAGE:
      selected_idx = (selected_idx + visible_rows < total_nodes - 1)
                         ? selected_idx + visible_rows + 1
                         : total_nodes - 1;
      break;
    case KEY_HOME:
      selected_idx = 0;
      break;
    case KEY_END:
      selected_idx = total_nodes - 1;
      break;
    case 10:
    case KEY_ENTER: {
      int current = 0;
      FileNode *selected = file_tree_get_by_index(root, selected_idx, &current);
      tui_draw_layout();
      if (selected) {
        return selected;
      }
      break;
    }
    case 27:
      tui_draw_layout();
      return NULL;
    case KEY_RESIZE: // terminal resized
      tui_resize_handler();
      max_y = getmaxy(browser_win);
      break;
    }
  }
}
