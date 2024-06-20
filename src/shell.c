#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    byte current = cwd;
    char path[256];
    int path_len = 0;  // Starting with the root
    int i;

    // Clear the path buffer
    clear(path, 256);

    // Read node sector
    readSector((byte *)&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    // Traverse back to root
    while (current != FS_NODE_P_ROOT) {
        struct node_item node = node_fs_buf.nodes[current];
        int node_name_len = strlen(node.node_name);

        // Shift the existing path to the right
        for (i = path_len; i >= 0; i--) {
            path[i + node_name_len + 1] = path[i];
        }

        // Insert the '/' and node name at the beginning of the path
        path[0] = '/';
        for (i = 0; i < node_name_len; i++) {
            path[i + 1] = node.node_name[i];
        }

        // Update the path length
        path_len += node_name_len + 1;

        // Move to the parent
        current = node.parent_index;
    }

    // If cwd is root, just print "/"
    if (path_len == 0) {
        path[0] = '/';
        path[1] = '\0';
    }

    // Print the path
    printString(path);
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0, k = 0;
    bool isArg = false;

    // Clear cmd and arg buffers
    clear(cmd, 64);
    clear(arg[0], 64);
    clear(arg[1], 64);

    // Skip leading spaces
    while (buf[i] == ' ') {
        i++;
    }

    // Parse command
    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[j++] = buf[i++];
    }
    cmd[j] = '\0';

    // Skip spaces before first argument
    while (buf[i] == ' ') {
        i++;
    }

    // Parse arguments
    for (k = 0; k < 2; k++) {
        j = 0;
        while (buf[i] != ' ' && buf[i] != '\0') {
            arg[k][j++] = buf[i++];
        }
        arg[k][j] = '\0';

        // Skip spaces before the next argument
        while (buf[i] == ' ') {
            i++;
        }
    }
}


// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    byte target_dir_index = cwd;
    bool found = false;

    // Read the current node sector
    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    // If a directory name is provided, find its index
    if (dirname[0] != '\0') {
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
                target_dir_index = i;
                found = true;
                break;
            }
        }

        if (!found) {
            printString("Directory not found\n");
            return;
        }
    }

    // List the contents of the directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == target_dir_index) {
            printString(node_fs_buf.nodes[i].node_name);
            printString("\n");
        }
    }
}


// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    struct node_item new_node;
    int i;
    int free_node_index = -1;
    int free_data_index = -1;

    // Read the current filesystem state
    readSector((byte*)&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte*)&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Find a free node slot
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == 0x00 && node_fs_buf.nodes[i].data_index == 0x00) {
            free_node_index = i;
            break;
        }
    }

    if (free_node_index == -1) {
        printString("Error: No free node available\n");
        return;
    }

    // Find a free data slot
    for (i = 0; i < FS_MAX_DATA; i++) {
        if (!map_fs_buf.is_used[i]) {
            free_data_index = i;
            break;
        }
    }

    if (free_data_index == -1) {
        printString("Error: No free data slot available\n");
        return;
    }

    // Mark the data slot as used
    map_fs_buf.is_used[free_data_index] = true;

    // Initialize the new directory node
    new_node.parent_index = cwd;
    new_node.data_index = free_data_index;
    strcpy(new_node.node_name, dirname);

    // Save the new directory node in the filesystem
    node_fs_buf.nodes[free_node_index] = new_node;

    // Write the updated filesystem state back to disk
    writeSector((byte*)&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    writeSector((byte*)&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    printString("Directory created successfully\n");
}


