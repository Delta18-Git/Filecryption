#ifndef FILE_TREE_H
#define FILE_TREE_H

#define MAX_PATH_LENGTH 512
#define MAX_NAME_LENGTH 256
#define MAX_CHILDREN 100

typedef struct FileNode {
  char name[MAX_NAME_LENGTH];
  char path[MAX_PATH_LENGTH];
  unsigned int is_dir : 1;
  struct FileNode *parent;
  struct FileNode *children[MAX_CHILDREN];
  int num_children;
} FileNode;

FileNode *file_tree_create(const char *path, int show_hidden);
void file_tree_destroy(FileNode *root);
FileNode *file_tree_get_by_path(FileNode *root, const char *path);
FileNode *file_tree_get_by_index(FileNode *root, int index, int *current_index);
int file_tree_count_nodes(FileNode *root);

#endif
