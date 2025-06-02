#include "crypto.h"
#include "file_tree.h"
#include "tui.h"
#include <getopt.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

FileNode *root_node = NULL;

static char *current_tree_path = ".";
static int current_show_hidden = 0;

void cleanup();

void print_help(const char *prog_name) {
  printf("Usage: %s [options] [directory]\n\n", prog_name);
  printf("FileCryption: A tool to encrypt and decrypt files.\n\n");
  printf("Options:\n");
  printf("  -h, --help            Show this help message and exit.\n");
  printf(
      "  -a, --all             Show hidden files (files starting with '.').\n");
  printf("  -d, --directory DIR   Specify the starting directory for the file "
         "browser.\n");
  printf("                        If [directory] is also provided as a "
         "positional argument,\n");
  printf("                        the one from -d/--directory takes "
         "precedence.\n\n");
  printf("If no directory is specified via -d or as a positional argument, '.' "
         "(current directory) is used.\n");
}

int main(int argc, char **argv) {
  if (sodium_init() < 0) {
    fprintf(stderr, "Failed to initialise libsodium\n");
    return 1;
  }

  int show_hidden_arg = 0;
  char *path_arg = NULL;

  struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"all", no_argument, 0, 'a'},
      {"directory", required_argument, 0, 'd'},
      {0, 0, 0, 0} // terminator for options
  };

  int opt_char;
  int long_index = 0;
  while ((opt_char = getopt_long(argc, argv, "had:", long_options,
                                 &long_index)) != -1) {
    switch (opt_char) {
    case 'h':
      print_help(argv[0]);
      return 0;
    case 'a':
      show_hidden_arg = 1;
      break;
    case 'd':
      path_arg = optarg;
      break;
    default:
      print_help(argv[0]);
      return 1;
    }
  }

  if (path_arg) {
    current_tree_path = path_arg;
  } else if (optind < argc) {
    current_tree_path = argv[optind];
  } else {
    current_tree_path = ".";
  }
  current_show_hidden = show_hidden_arg;

  tui_init();
  root_node = file_tree_create(current_tree_path, current_show_hidden);

  if (!root_node) {
    if (argc > 1) {
      char error_msg[MAX_PATH_LENGTH + 100];
      snprintf(error_msg, sizeof(error_msg),
               "Failed to build file tree for '%s'. Path invalid or permission "
               "denied.",
               current_tree_path);
      tui_display_message(error_msg, TUI_MSG_ERROR);
      cleanup();
      return 1;
    } else {
      tui_display_message("Failed to build file tree", TUI_MSG_ERROR);
      cleanup();
      return 1;
    }
  }
  int running = 1;
  while (running) {
    int selection = tui_get_menu_selection();
    switch (selection) {
    case MENU_ENCRYPT: {
      FileNode *file = tui_get_file_browser_selection(root_node);
      if (!file) {
        break; // esc
      }
      if (file->is_dir) {
        tui_display_message("Please select a valid file", TUI_MSG_ERROR);
        tui_draw_layout();
        break;
      }

      const char *file_type = get_common_file_type(file);
      char confirm_prompt[MAX_PATH_LENGTH + 50];
      snprintf(confirm_prompt, sizeof(confirm_prompt),
               "Encrypt file \"%s\" (%s)?", file->name, file_type);

      if (tui_get_confirmation(confirm_prompt) != TUI_CONFIRM_YES) {
        tui_draw_layout(); // redraw UI after confirmation dialog
        break;
      }

      char *password =
          tui_get_password("Enter the password to encrypt the file:");
      if (!password || strlen(password) == 0) {
        tui_display_message("Password cannot be empty", TUI_MSG_WARNING);
        break;
      }
      char output_file[MAX_PATH_LENGTH];
      snprintf(output_file, MAX_PATH_LENGTH, "%s.enc", file->path);
      output_file[MAX_PATH_LENGTH - 1] = '\0';
      int result = crypto_encrypt_file(file->path, output_file, password);
      if (result == CRYPTO_SUCCESS) {
        char message[MAX_PATH_LENGTH + 30];
        sprintf(message, "File encrypted and saved to %s", output_file);
        tui_display_message(message, TUI_MSG_SUCCESS);
      } else {
        tui_display_message("Encryption failed", TUI_MSG_ERROR);
      }
      FileNode *old_root = root_node;
      root_node = file_tree_create(current_tree_path, current_show_hidden);
      if (root_node) {
        file_tree_destroy(old_root);
      }
      tui_draw_layout();
      tui_draw_file_browser(root_node, 0, 0);
      break;
    }
    case MENU_DECRYPT: {
      FileNode *file = tui_get_file_browser_selection(root_node);
      if (!file) {
        break; // esc
      }
      if (file->is_dir) {
        tui_display_message("Please select a valid file", TUI_MSG_ERROR);
        tui_draw_layout();
        break;
      }

      const char *file_type = get_common_file_type(file);
      char confirm_prompt[MAX_PATH_LENGTH + 50];
      snprintf(confirm_prompt, sizeof(confirm_prompt),
               "Decrypt file \"%s\" (%s)?", file->name, file_type);

      if (tui_get_confirmation(confirm_prompt) != TUI_CONFIRM_YES) {
        tui_draw_layout();
        break;
      }

      char *password =
          tui_get_password("Enter the password to decrypt the file:");
      if (!password || strlen(password) == 0) {
        tui_display_message("Password cannot be empty", TUI_MSG_WARNING);
        tui_draw_layout();
        break;
      }
      char output_file[MAX_PATH_LENGTH];
      char *ext = strrchr(file->path, '.');
      if (ext && strcmp(ext, ".enc") == 0) {
        strncpy(output_file, file->path, ext - file->path);
      } else {
        snprintf(output_file, MAX_PATH_LENGTH, "%s.dec", file->path);
      }
      output_file[ext - file->path] = '\0';
      int result = crypto_decrypt_file(file->path, output_file, password);
      if (result == CRYPTO_SUCCESS) {
        char message[MAX_PATH_LENGTH + 28];
        sprintf(message, "File decrypted and saved to %s", output_file);
        tui_display_message(message, TUI_MSG_SUCCESS);
      } else {
        tui_display_message("Decryption failed", TUI_MSG_ERROR);
      }
      tui_draw_layout();
      FileNode *old_root = root_node;
      root_node = file_tree_create(current_tree_path, current_show_hidden);
      if (root_node) {
        file_tree_destroy(old_root);
      }
      tui_draw_file_browser(root_node, 0, 0);
      break;
    }
    case MENU_EXIT:
      running = 0;
      break;
    }
  }
  cleanup();
  return 0;
}

void cleanup() {
  if (root_node) {
    file_tree_destroy(root_node);
    root_node = NULL;
  }
  tui_cleanup();
}
