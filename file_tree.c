#include "file_tree.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

FileNode *file_tree_create(const char *path, int show_hidden) {
  FileNode *node = (FileNode *)malloc(sizeof(FileNode));

  if (!node) {
    return NULL;
  }

  memset(node, 0, sizeof(FileNode));
  strncpy(node->path, path, MAX_PATH_LENGTH - 1);
  node->path[MAX_PATH_LENGTH - 1] = '\0';

  // extract the name from the path (the part after the last '/')
  const char *basename = strrchr(path, '/');
  if (basename) {
    strncpy(node->name, basename + 1, MAX_NAME_LENGTH - 1);
  } else {
    // no '/' in path, so the whole path is the name
    strncpy(node->name, path, MAX_NAME_LENGTH - 1);
  }
  node->name[MAX_NAME_LENGTH - 1] = '\0';

  if (node->name[0] == '\0' && path[0] == '/' && path[1] == '\0') {
    strcpy(node->name, "/");
  }

  struct stat st;
  // get file status to determine if it's a directory.
  node->is_dir = stat(path, &st) == 0 ? S_ISDIR(st.st_mode) : 0;

  if (node->is_dir) {
    DIR *dir = opendir(path);
    if (dir != NULL) {
      struct dirent *entry;
      // iterate over directory entries
      while ((entry = readdir(dir)) != NULL) {
        // skip current and parent directories
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
          continue;
        }
        // skip hidden files/dirs if 'show_hidden' is false
        if (!show_hidden && entry->d_name[0] == '.') {
          continue;
        }

        char child_path[MAX_PATH_LENGTH];
        // construct the child path
        if (strcmp(path, "/") == 0) {
          snprintf(child_path, MAX_PATH_LENGTH, "/%s", entry->d_name);
        } else {
          int path_len = strlen(path);
          if (path[path_len - 1] == '/') { // prevents double addition of '/'
            snprintf(child_path, MAX_PATH_LENGTH, "%s%s", path, entry->d_name);
          } else {
            snprintf(child_path, MAX_PATH_LENGTH, "%s/%s", path, entry->d_name);
          }
        }
        child_path[MAX_PATH_LENGTH - 1] = '\0';

        FileNode *child = file_tree_create(child_path, show_hidden);
        if (child != NULL) {
          child->parent = node;
          if (node->num_children < MAX_CHILDREN) {
            node->children[node->num_children++] = child;
          } else {
            free(child);
            break;
          }
        }
      }
      closedir(dir);
    }
  }
  return node;
}

void file_tree_destroy(FileNode *node_to_destroy) {
  if (!node_to_destroy) {
    return;
  }
  // recursively destroy all child nodes
  for (int i = 0; i < node_to_destroy->num_children; i++) {
    file_tree_destroy(node_to_destroy->children[i]);
    node_to_destroy->children[i] = NULL;
  }
  free(node_to_destroy);
  node_to_destroy = NULL;
}

FileNode *file_tree_get_by_path(FileNode *root, const char *path) {
  if (!root) {
    return NULL;
  }
  if (strcmp(root->path, path) == 0) {
    return root;
  }
  for (int i = 0; i < root->num_children; i++) {
    FileNode *found = file_tree_get_by_path(root->children[i], path);
    if (found) {
      return found;
    }
  }
  return NULL;
}

FileNode *file_tree_get_by_index(FileNode *root, int index, int *num_visited) {
  if (!root) {
    return NULL;
  }

  if (*num_visited == index) {
    return root;
  }

  // add current node to number fo visited nodes
  (*num_visited)++;

  for (int i = 0; i < root->num_children; i++) {
    FileNode *found =
        file_tree_get_by_index(root->children[i], index, num_visited);
    if (found) {
      return found;
    }
  }
  return NULL;
}

int file_tree_count_nodes(FileNode *root) {
  if (!root) {
    return 0;
  }
  int count = 1;
  for (int i = 0; i < root->num_children; i++) {
    count += file_tree_count_nodes(root->children[i]);
  }
  return count;
}
