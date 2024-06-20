#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j;
    bool found = false;

    readSector((byte *)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte *)&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Cari node yang cocok dengan nama dan parent_index yang diberikan
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
            strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    metadata->filesize = 0;
    for (j = 0; j < FS_MAX_SECTOR && data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j] != 0; j++) {
        readSector(metadata->buffer + j * SECTOR_SIZE, data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j]);
        metadata->filesize += SECTOR_SIZE;
    }

    *status = FS_SUCCESS;
}


// TODO: 3. Implement fsWrite function

void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j, k, node_index = -1, data_index = -1;

    readSector((byte *)&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector((byte *)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte *)&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Cek apakah nama node sudah ada
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
            strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
    }

    // Cek apakah ada node yang tersedia
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            node_index = i;
            break;
        }
    }

    if (node_index == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    // Cek apakah ada data item yang tersedia
    for (i = 0; i < FS_MAX_DATA; i++) {
        if (data_fs_buf.datas[i].sectors[0] == 0) {
            data_index = i;
            break;
        }
    }

    if (data_index == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    // Isi node baru
    node_fs_buf.nodes[node_index].parent_index = metadata->parent_index;
    node_fs_buf.nodes[node_index].data_index = data_index;
    strcpy(node_fs_buf.nodes[node_index].node_name, metadata->node_name);

    // Tulis data ke sektor-sektor
    for (j = 0, k = 0; j < FS_MAX_SECTOR && k * SECTOR_SIZE < metadata->filesize; j++) {
        int sector;
        for (sector = 0; sector < SECTOR_SIZE; sector++) {
            if (!map_fs_buf.is_used[sector]) {
                map_fs_buf.is_used[sector] = true;
                data_fs_buf.datas[data_index].sectors[j] = sector;
                writeSector(metadata->buffer + k * SECTOR_SIZE, sector);
                k++;
                break;
            }
        }
        if (sector == SECTOR_SIZE) {
            *status = FS_W_NOT_ENOUGH_SPACE;
            return;
        }
    }

    // Simpan perubahan ke sektor
    writeSector((byte *)&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector((byte *)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    writeSector((byte *)&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    *status = FS_SUCCESS;
}

